#include <nczero/chess/color.h>
#include <nczero/chess/move.h>
#include <nczero/worker.h>

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace neocortex;

worker::worker(int bsize) {
    set_batch_size(bsize);
}

void worker::start(shared_ptr<node>& root, chess::position& rootpos) {
    pos = rootpos;
    running = true;

    // Placeholder worker
    worker_thread = thread(&worker::job, this, std::ref(root));
}

void worker::job(shared_ptr<node>& root) {
    status_mutex.lock();
    current_status = status();
    status_mutex.unlock();

    while (running) {
        // Clear batch nodes, new children
        new_children.clear();
        batch_nodes.clear();

        // Prepare next batch
        current_batch_size = 0;

        status_mutex.lock();
        current_status.code = "building";
        status_mutex.unlock();

        make_batch(root, max_batch_size);

        if (current_batch_size > 0) {
            // Execute batch
            status_mutex.lock();
            current_status.code = "execute ";
            status_mutex.unlock();

            vector<nn::output> results = nn::evaluate(&board_input[0], &lmm_input[0], current_batch_size);

            // Apply results
            for (int i = 0; i < results.size(); ++i) {
                node* dst = batch_nodes[i];

                // Apply policy to new children
                for (auto& child: new_children[i]) {
                    child->apply_policy(results[i].policy);
                }

                if (!dst->set_children(new_children[i])) continue;
                dst->backprop(results[i].value);

                status_mutex.lock();
                ++current_status.node_count;
                status_mutex.unlock();
            }

            // Update status
            status_mutex.lock();
            ++current_status.batch_count;
            status_mutex.unlock();
        }
    }
}

void worker::stop() {
    running = false;
}

void worker::join() {
    worker_thread.join();
}

void worker::set_batch_size(int bsize) {
    max_batch_size = bsize;
    board_input.resize(bsize * 8 * 8 * 85, 0.0f);
    lmm_input.resize(bsize * 4096, 0.0f);
    new_children.resize(bsize);
}

int worker::make_batch(shared_ptr<node>& root, int allocated) {
    if (current_batch_size >= max_batch_size) {
        return 0;
    }

    if (root->has_children()) {
        // Continue selecting

        // Order children
        vector<pair<shared_ptr<node>, float>> uct_pairs;
        float uct_total = 0.0f;

        for (auto& child : root->get_children()) {
            float uct = child->get_uct();
            uct_total += uct;
            uct_pairs.push_back(make_pair(child, uct));       
        }

        sort(uct_pairs.begin(), uct_pairs.end(), [](auto& a, auto& b) { return a.second > b.second; });

        // Distribute batches to children
		int total_batches = 0;

		for (int i = 0; i < uct_pairs.size(); ++i) {
			if (allocated <= 0) {
				break;
			}

			shared_ptr<node>& child = uct_pairs[i].first;

			int child_alloc = ceil(uct_pairs[i].second * float(allocated) / uct_total);
			uct_total -= uct_pairs[i].second;

			pos.make_move(child->get_action());

			int new_batches = make_batch(child, child_alloc);

			pos.unmake_move();

			allocated -= new_batches;
			total_batches += new_batches;

			if (allocated == 0) {
				return total_batches;
			}
		}

		return total_batches;
    }

    // No children, check if cached terminal
    if (root->backprop_terminal()) {
        // Update node count
        status_mutex.lock();
        ++current_status.node_count;
        status_mutex.unlock();

        return 0;
    }

    // Not cached terminal, check if HRM
    if (pos.is_draw_by_hrm()) {
        root->backprop_terminal(0);

        // Update node count
        status_mutex.lock();
        ++current_status.node_count;
        status_mutex.unlock();

        return 0;
    }

    // Generate moves
	int moves[chess::MAX_PL_MOVES];
	int num_pl_moves = pos.pseudolegal_moves(moves);
	int num_moves = 0;

    std::vector<shared_ptr<node>> tmp_new_children;

	for (int i = 0; i < num_pl_moves; ++i) {
		if (pos.make_move(moves[i])) {
			++num_moves;

            tmp_new_children.push_back(
                make_shared<node>(root, moves[i])
            );
		}

		pos.unmake_move();

        // Write legal move mask
        int src = chess::move::src(pos.last_move());
        int dst = chess::move::dst(pos.last_move());

        for (int j = 0; j < 4096; ++j) {
            lmm_input[current_batch_size * 4096 + j] = 0.0f;
        }

        if (pos.get_color_to_move() == chess::color::WHITE) {
            lmm_input[current_batch_size * 4096 + src * 64 + dst] = 1.0f;
        } else {
            lmm_input[current_batch_size * 4096 + (63 - src) * 64 + (63 - dst)] = 1.0f;
        }
    }

    if (!num_moves) {
        // No moves, set terminal cache and backprop
        root->backprop_terminal(pos.check() ? -1 : 0);

        // Update node count
        status_mutex.lock();
        ++current_status.node_count;
        status_mutex.unlock();

        return 0;
    }

    // Write board input
    memcpy(&board_input[current_batch_size * 8 * 8 * nn::SQUARE_BITS], &pos.get_input(), sizeof(float) * 8 * 8 * nn::SQUARE_BITS);

    // Store new children
    new_children.push_back(tmp_new_children);

    // Write batch node
    batch_nodes.push_back(root.get());

    // Finally, increment batch counter
    ++current_batch_size;

    return 1;
}

worker::status worker::get_status() {
    status output;

    status_mutex.lock();
    output = current_status;
    status_mutex.unlock();

    return output;
}

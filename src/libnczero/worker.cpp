#include <nczero/chess/color.h>
#include <nczero/chess/move.h>
#include <nczero/worker.h>

using namespace neocortex;

worker::worker(int bsize) {
    set_batch_size(bsize);
}

void worker::start(shared_ptr<node>& root, chess::position& rootpos) {
    pos = rootpos;
    running = true;

    // Placeholder worker
    worker_thread = thread([&](worker* self) {
        while (running) {
            this_thread::sleep_for(chrono::duration<int, std::milli>(50));
        }
    }, this);
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

void worker::make_batch(shared_ptr<node>& root, int allocated) {
    if (root->has_children()) {
        // Continue selecting
    }

    // No children, check if cached terminal
    if (root->backprop_terminal()) {
        // Update node count
        status_mutex.lock();
        ++current_status.node_count;
        status_mutex.unlock();

        return;
    }

    // Not cached terminal, check if HRM
    if (pos.is_draw_by_hrm()) {
        root->backprop_terminal(0);

        // Update node count
        status_mutex.lock();
        ++current_status.node_count;
        status_mutex.unlock();

        return;
    }

    // Generate moves
	int moves[chess::MAX_PL_MOVES];
	int num_pl_moves = pos.pseudolegal_moves(moves);
	int num_moves = 0;

    std::vector<shared_ptr<node>> tmp_new_children;

	for (int i = 0; i < num_pl_moves; ++i) {
		if (pos.make_move(moves[i])) {
			++num_moves;

            new_children[current_batch_size].push_back(
                make_shared<node>(root, moves[i])
            );
		}

		pos.unmake_move();

        // Write legal move mask
        int src = chess::move::src(pos.last_move());
        int dst = chess::move::dst(pos.last_move());

        for (int i = 0; i < 4096; ++i) {
            lmm_input[current_batch_size * 4096 + i] = 0.0f;
        }

        if (pos.get_color_to_move() == chess::color::WHITE) {
            lmm_input[current_batch_size * 4096 + src * 64 + dst] = 1.0f;
        } else {
            lmm_input[current_batch_size * 4096 + 4095 - src * 64 - dst] = 1.0f;
        }
    }

    if (!num_moves) {
        // No moves, set terminal cache and backprop
        root->backprop_terminal(pos.check() ? -1 : 0);

        // Update node count
        status_mutex.lock();
        ++current_status.node_count;
        status_mutex.unlock();

        return;
    }

    // Write board input
    memcpy(&board_input[current_batch_size * 8 * 8 * 85], &pos.get_input(), sizeof(float) * 8 * 8 * 85);

    // Finally, increment batch counter
    ++current_batch_size;
}

worker::status worker::get_status() {
    status output;

    status_mutex.lock();
    output = current_status;
    status_mutex.unlock();

    return output;
}
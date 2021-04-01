#include <nczero/log.h>
#include <nczero/pool.h>
#include <nczero/util.h>
#include <nczero/worker.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <iostream>
#include <random>
#include <vector>

#define POOL_INFO_DELAY 500

using namespace neocortex;
using namespace std;

static int batch_size = DEFAULT_BATCH_SIZE;
static vector<shared_ptr<worker>> workers;

static void _init_workers();

void pool::init(int num_threads) {
    set_num_threads(num_threads);
}

int pool::search(shared_ptr<node>& root, int maxtime, chess::position& p, bool uci) {
    util::time_point starttime = util::time_now();

    // Start workers.
    for (auto& w : workers) {
        w->start(root, p);
    }

    while (1) {
        int elapsed = util::time_elapsed_ms(starttime);
        int node_count = 0, batch_count = 0, batch_avg = 0, exec_avg = 0;

        if (elapsed >= maxtime) {
            break;
        }

        for (auto& w : workers) {
            worker::status cstatus = w->get_status();

            node_count += cstatus.node_count;
            batch_count += cstatus.batch_count;
            batch_avg += cstatus.batch_avg;
            exec_avg += cstatus.exec_avg;
        }

        int nps = (long) node_count * 1000 / ((long) elapsed + 1);

        // Print search status
        if (uci) {
            // UCI info on stdout
            cout << "info time " << elapsed << " nodes " << node_count << " nps " << nps << "\n";
        } else {
            // Non-uci pretty status on stderr
        }

        this_thread::sleep_for(chrono::duration<int, std::milli>(POOL_INFO_DELAY));
    }

    // Stop workers.
    for (auto& w : workers) {
        w->stop();
    }

    for (auto& w : workers) {
        w->join();
    }

    // Choose move ND
    std::vector<int> n_dist;

	std::random_device device;
	std::mt19937 rng(device());
	std::discrete_distribution<> dist(n_dist.begin(), n_dist.end());

	shared_ptr<node>& chosen = root->get_children[dist(rng)];

	// Print debug info
	neocortex_info("Picking from %d children:\n", root.get_children().size());

	std::vector<std::pair<shared_ptr<node>, int>> n_pairs;

	int n_total = 0;

    for (auto& c : root->get_children()) {
        
    }

    for (int i = 0; i < root->get_children().size(); ++i) {
        n_total += root->get_children()[i]->get_n();
		n_pairs.push_back({ root->get_children()[i], root->get_children()[i]->get_n() });
        n_dist.push_back(root->get_children()[i]->get_n());
    }

	std::sort(n_pairs.begin(), n_pairs.end(), [&](auto& a, auto& b) { return a.second > b.second; });

	for (int i = 0; i < n_pairs.size(); ++i) {
		auto& child = n_pairs[i].first;
		std::string movestr = move::to_uci(child->get_action());

		neocortex_info(
			"%s> %5s  %03.1f%% | N=%4d | Q=%+04.2f | W=%+04.2f | P=%05.3f%%\n",
			(child.get() == chosen.get()) ? "##" : "  ",
			movestr.c_str(),
			100.0f * (float)child.n / (float)n_total,
			child.n,
			child.q,
			child.w,
			100.0f * (float)child.p
		);
	}

	std::string movestr = move::to_uci(chosen);
	neocortex_info("Selecting move %s\n", movestr.c_str());

    return chess::move::null();
}

void pool::set_batch_size(int bsize) {
    batch_size = bsize;

    for (auto& i : workers) {
        i->set_batch_size(bsize);
    }
}

void pool::set_num_threads(int num_threads) {
    workers.clear();

    for (int i = 0; i < num_threads; ++i) {
        workers.emplace_back(make_shared<worker>(batch_size));
    }
}

int pool::get_num_threads() {
    return workers.size();
}

int pool::get_batch_size() {
    return batch_size;
}
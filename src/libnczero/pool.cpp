#include <nczero/pool.h>
#include <nczero/util.h>
#include <nczero/worker.h>

#include <chrono>
#include <memory>
#include <iostream>
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
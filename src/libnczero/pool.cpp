#include <nczero/log.h>
#include <nczero/pool.h>
#include <nczero/timer.h>
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

void pool::init(int num_threads) {
    set_num_threads(num_threads);
}

int pool::search(shared_ptr<node>& root, int maxtime, chess::position& p, bool uci) {
    timer::time_point starttime = timer::time_now();

    // Start workers.
    for (auto& w : workers) {
        w->start(root, p);
    }

    bool first = true;

    while (1) {
        int elapsed = timer::time_elapsed_ms(starttime);
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

            // Rewind console if update
            if (!first) {
                for (size_t i = 0; i < workers.size() + 6; ++i) {
                    fprintf(stderr, "\033[F");
                }
            } else {
                first = false;
            }

            // Table top border
            int width = 54;
            cout << "+" << string(width - 2, '-') << "+\n";

            // Table headers
            cout << "| ID  | batches | nodes |    nps |    bavg |    eavg |\n";

            // Separating border
            cout << "+" << string(width - 2, '-') << "+\n";

            int btotal = 0;
            int ndtotal = 0;
            int npstotal = 0;
            int bavg = 0;
            int eavg = 0;

            // Compute totals so we can display them first
            for (size_t i = 0; i < workers.size(); ++i) {
                worker::status st = workers[i]->get_status();

                btotal += st.batch_count;
                ndtotal += st.node_count;
                npstotal += st.node_count * 1000 / (elapsed + 1);
                bavg += st.batch_avg;
                eavg += st.exec_avg;
            }

            // Total row
            cout << "| ALL";
            cout << " | " << setw(7) << btotal;
            cout << " | " << setw(5) << ndtotal;
            cout << " | " << setw(6) << npstotal;
            cout << " | " << setw(5) << (bavg / workers.size()) << "ms";
            cout << " | " << setw(5) << (eavg / workers.size()) << "ms";
            cout << " |\n";

            // Separating border
            cout << "+" << string(width - 2, '-') << "+\n";

            // Rows
            for (size_t i = 0; i < workers.size(); ++i) {
                worker::status st = workers[i]->get_status();

                cout << "| " << setw(3) << i;
                cout << " | " << setw(7) << st.batch_count;
                cout << " | " << setw(5) << st.node_count;
                cout << " | " << setw(6) << st.node_count * 1000 / (elapsed + 1);
                cout << " | " << setw(5) << st.batch_avg << "ms";
                cout << " | " << setw(5) << st.exec_avg << "ms";
                cout << " |\n";
            }
            
            // Bottom border
            cout << "+" << string(width - 2, '-') << "+\n";
        }

        this_thread::sleep_for(chrono::duration<int, std::milli>(POOL_INFO_DELAY));

        // Reset timer if no nodes have been searched yet
        if (node_count == 0) {
            starttime = timer::time_now();
        }
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

    for (size_t i = 0; i < root->get_children().size(); ++i) {
        n_dist.push_back(root->get_children()[i]->get_value().n);
    }

	return root->get_children()[dist(rng)]->get_action();
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

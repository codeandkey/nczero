/* vim: set ts=4 sw=4 noet: */

/*
 * This file is subject to the terms and conditions defined in
 * LICENSE.txt, included in this source code distribution.
 */

#pragma once
#define DEFAULT_BATCH_SIZE 16

#include <nczero/chess/position.h>
#include <nczero/node.h>


#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;

namespace neocortex {
    class worker {
        public:
            worker(int bsize = DEFAULT_BATCH_SIZE);
            
            void start(shared_ptr<node>& root, chess::position& rootpos);
            void stop();
            void join();
            void job(shared_ptr<node>& root);
            void set_batch_size(int bsize);

            struct status {
                std::string code = "uninitialized";
                int batch_count = 0, node_count = 0, batch_avg = 0, exec_avg = 0;
            };

            status get_status();
        private:
            void make_batch(shared_ptr<node>& root, int allocated);

            atomic<bool> running;

            status current_status;
            mutex status_mutex;

            chess::position pos;
            thread worker_thread;

            int current_batch_size, max_batch_size;
            vector<float> board_input, lmm_input;

            vector<vector<shared_ptr<node>>> new_children;
            vector<node*> batch_nodes;
    };
}
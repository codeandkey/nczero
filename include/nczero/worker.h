/* vim: set ts=4 sw=4 noet: */

/*
 * This file is subject to the terms and conditions defined in
 * LICENSE.txt, included in this source code distribution.
 */

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
            
            void start(node* root, chess::position& rootpos);
            void stop();
            void set_batch_size(int bsize);

            struct Status {
                std::string code;
                int batch_count, node_count, batch_avg, exec_avg;
            };
        private:
            void make_batch(shared_ptr<node>& root, int allocated);

            atomic<bool> running;

            Status status;
            mutex status_mutex;

            chess::position pos;
            thread worker_thread;

            int current_batch_size, max_batch_size;
            vector<float> board_input, lmm_input;
            vector<vector<shared_ptr<node>>> new_children;
    };
}
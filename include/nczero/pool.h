/* vim: set ts=4 sw=4 noet: */

/*
 * This file is subject to the terms and conditions defined in
 * LICENSE.txt, included in this source code distribution.
 */

#include <nczero/chess/position.h>
#include <nczero/net.h>
#include <nczero/worker.h>

#include <memory>

namespace neocortex {
    namespace pool {
        void init(int num_threads);
        void search(int maxtime, std::shared_ptr<nn::Network> net, chess::position& p);
        
        void set_batch_size(int bsize);
        int get_batch_size();

        void set_num_threads(int num_threads);
        int get_num_threads();
    }
}
#include <nczero/pool.h>
#include <nczero/worker.h>

#include <atomic>
#include <memory>
#include <vector>

using namespace neocortex;
using namespace std;

static atomic<int> batch_size;
static vector<shared_ptr<worker>> workers;

static void _init_workers();

void pool::init(int num_threads) {
    set_num_threads(num_threads);
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
        workers.emplace_back(batch_size);
    }
}
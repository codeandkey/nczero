#include <nczero/node.h>

using namespace neocortex;

node::node(shared_ptr<node> parent, int action) {
    this->pov = !parent->pov;
    this->parent = parent;
    this->action = action;

    n = 0;
    terminal = 1;
    w = p = 0.0f;
}

float node::get_uct() {
    return 0.0f; // TODO
}

bool node::backprop_terminal(float tv) {
    if (tv < 1.0f) {
        terminal = tv;
    }

    if (terminal < 1.0f) {
        backprop(terminal);
        return true;
    } else {
        return false;
    }
}

bool node::has_children() {
    return flag_has_children;
}

void node::backprop(float value) {
    value_lock.lock();
    ++n;
    w = w + value;
    value_lock.unlock();

    if (parent != nullptr) {
        parent->backprop(-value);
    }
}
#include <nczero/node.h>

using namespace neocortex;

node::node(shared_ptr<node> parent, int action, int pov) {
    if (parent != nullptr) {
        this->pov = !parent->pov;
    } else {
        this->pov = pov;
    }
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

bool node::set_children(std::vector<shared_ptr<node>> new_children) {
    if (flag_has_children.exchange(true)) {
        return false;
    }

    children = new_children;
    return true;
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

void node::apply_policy(float* pbuf) {
    if (pov == chess::color::BLACK) {
        // This node is a decision for WHITE, use normal index
        p = pbuf[chess::move::src(action) * 64 + chess::move::dst(action)];
    } else {
        // This node is a decision for BLACK, use reversed index
        p = pbuf[(63 - chess::move::src(action)) * 64 + (63 - chess::move::dst(action))];
    }
}
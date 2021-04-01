#include <nczero/node.h>

#include <cmath>

using namespace neocortex;

node::node(shared_ptr<node> parent, int action, int pov) {
    this->parent = parent;
    this->action = action;

    if (parent != nullptr) {
        this->pov = !parent->pov;
    } else {
        this->pov = pov;
    }

    n = 0;
    terminal = 1;
    w = p = total_p = 0.0f;
    flag_has_children = false;
}

float node::get_uct() {
    value_lock.lock();
    float uct = (w / (n + 1)) + POLICY_WEIGHT * (p / parent->total_p) + EXPLORATION * sqrtf(log(parent->n) / (n + 1));
    value_lock.unlock();
    return uct;
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

    parent->total_p = parent->total_p + p;
}

shared_ptr<node> node::move_child(int action) {
    // Find child
    for (auto& child : children) {
        if (child->action == action) {
            child->parent = nullptr;
            return child;
        }
    }

    throw runtime_error("No such child for action" + chess::move::to_uci(action));
}

vector<shared_ptr<node>>& node::get_children() {
    return children;
}

int node::get_action() {
    return action;
}

int node::get_n() {
    lock_guard<mutex> lock(value_lock);
    return n;
}

float node::get_w() {
    lock_guard<mutex> lock(value_lock);
    return w;
}
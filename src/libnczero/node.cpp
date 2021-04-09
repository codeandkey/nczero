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

    terminal = 1;
    p = total_p = 0.0f;
    flag_has_children = false;
    claimed = false;
}

float node::get_uct() {
    value_lock.lock();
    float uct = (our_value.w / (our_value.n + 1)) + POLICY_WEIGHT * (p / parent->total_p) + EXPLORATION * sqrtf(log(parent->our_value.n) / (our_value.n + 1));
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

void node::set_children(std::vector<shared_ptr<node>> new_children) {
    children = new_children;
}

bool node::has_children() {
    return flag_has_children;
}

bool node::try_claim() {
    return !claimed.exchange(true);
}

bool node::is_claimed() {
    return claimed;
}

void node::unclaim() {
    claimed = false;
}

void node::backprop(float value) {
    value_lock.lock();
    ++our_value.n;
    our_value.w += value;
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

node::value node::get_value() {
    lock_guard<mutex> lock(value_lock);
    return our_value;
}

void node::set_value(node::value v) {
    lock_guard<mutex> lock(value_lock);
    our_value = v;
}

float node::get_p_pct() {
    return p / parent->total_p;
}

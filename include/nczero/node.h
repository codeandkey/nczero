/* vim: set ts=4 sw=4 noet: */

/*
 * This file is subject to the terms and conditions defined in
 * LICENSE.txt, included in this source code distribution.
 */

/*
    The parallel MCTS algorithm is described as follows.
*/

#pragma once

#include <nczero/chess/color.h>
#include <nczero/chess/move.h>
#include <nczero/net.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

using namespace std;

namespace neocortex {
    class node {
        public:
            /**
             * Controls the tendency of UCT to favor nodes with lower n.
             */
            static constexpr float EXPLORATION = 1.414l; // sqrt(2)

            /**
             * Adds additional weight to policy values in UCT calculation.
             */
            static constexpr float POLICY_WEIGHT = 5.0f;

            /**
             * Constructs a new child node.
             * @param parent parent node
             * @param action node decision
             * @param pov POV if parent not provided
             */
            node(shared_ptr<node> parent = nullptr, int action = chess::move::null(), int pov = chess::color::WHITE);

            /**
             * Applies a policy value to this node.
             * @param pbuf Policy result from evaluation of parent node
             */
            void apply_policy(float* pbuf);

            /**
             * Applies a evaluated value to this node.
             * @param val Value result from evaluation of this node
             */
            void apply_value(float val);

            /**
             * Sets the cached terminal value in the node.
             * @param result Terminal result, from white POV.
             */
            void set_terminal(int result);

            bool set_children(std::vector<shared_ptr<node>> new_children);

            /**
             * Finds a child with action <action>, sets its parent to nullptr
             * and returns it.
             *
             * @param action Action to find
             * @return shared_ptr to child
             */
            shared_ptr<node> move_child(int action);

            /**
             * Gets the UCT of this node.
             */
            float get_uct();

            /**
             * Backpropagates a cached terminal value if there is one,
             * or sets a terminal and backprops if provided.
             *
             * @param tv New terminal value, should be 0 (draw) or -1 (loss) if provided
             * @return true if a terminal value was backpropped, false otherwise
             */
            bool backprop_terminal(float tv = 1.0);

            /**
             * Tests if this node has children.
             * @return true if children are ready, false otherwise
             */
            bool has_children();

            /**
             * Backpropagates a value up through the tree.
             * @param value Value to propagate, from node's POV.
             */
            void backprop(float value);

            /**
             * Gets a reference to the children of this node.
             * @return Reference to children vec.
             */
            vector<shared_ptr<node>>& get_children();

            /**
             * Returns the action at this node, or move::null() if none exists
             * @return Reference to node action.
             */
            int get_action();

            int get_n();
            float get_w();

        private:
            int pov, action, terminal, n;
            float w, p, total_p;
            bool is_terminal;
			atomic<bool> flag_has_children;

            shared_ptr<node> parent;

            std::vector<shared_ptr<node>> children;
            mutex value_lock;
    };
}

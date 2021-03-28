/* vim: set ts=4 sw=4 noet: */

/*
 * This file is subject to the terms and conditions defined in
 * LICENSE.txt, included in this source code distribution.
 */

/*
    The parallel MCTS algorithm is described as follows.
*/

#pragma once

#include <nczero/chess/move.h>
#include <nczero/net.h>

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
             */
            node(shared_ptr<node> parent = nullptr, int action = chess::move::null());

            /**
             * Checks if the node is claimed.
             * @return true if node is claimed, false otherwise
             */
            bool is_claimed();

            /**
             * Tries to claim the node.
             * @return true if node is claimed, false otherwise
             */
            bool try_claim();

            /**
             * Unclaims the node.
             */
            void unclaim();

            /**
             * Applies a network result to this node.
             * @param result Result from network to backprop.
             */
            void apply_result(nn::Network::Output& result);

            /**
             * Expands the node and creates placeholder children.
             * @param moves Legal move array pointer.
             * @param num_moves Number of legal moves.
             */
            void expand(int* moves, int num_moves);

            /**
             * Sets the cached terminal value in the node.
             * @param result Terminal result, from white POV.
             */
            void set_terminal(int result);

            void set_children(std::vector<shared_ptr<node>> new_children);

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

        protected:
            /**
             * Backpropagates a value up through the tree.
             * @param value Value to propagate, from node's POV.
             */
            void backprop(float value);

        private:
            int pov, action;

            atomic<int> n, terminal;
            atomic<float> w, p;
            atomic<bool> is_terminal, flag_has_children;

            shared_ptr<node> parent;

            std::vector<shared_ptr<node>> children;
            mutex value_lock;
    };
}
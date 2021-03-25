/* vim: set ts=4 sw=4 noet: */

/*
 * This file is subject to the terms and conditions defined in
 * LICENSE.txt, included in this source code distribution.
 */

#pragma once

#include <string>
#include <vector>
#include <initializer_list>

#include <torch/script.h>

namespace neocortex {
	namespace nn {
		class Network {
		public:
			struct Output {
				float policy[4096], value;
			};

			Network(std::string path);

			std::vector<Output> evaluate(std::vector<float>& inp_board, std::vector<float>& inp_lmm, int num_batches);

		private:
			torch::jit::script::Module module;
		};
	}
}
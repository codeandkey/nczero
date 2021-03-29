/* vim: set ts=4 sw=4 noet: */

/*
 * This file is subject to the terms and conditions defined in
 * LICENSE.txt, included in this source code distribution.
 */

#pragma once

#include <filesystem>
#include <initializer_list>
#include <string>
#include <vector>

namespace neocortex {
	namespace nn {
		/**
		 * Bits per square in the input layer.
		 * Input shape is (bsize, 8, 8, SQUARE_BITS)
		 */
		constexpr size_t SQUARE_BITS = 85;

		/**
		 * Path to model directory.
		 */
		constexpr const char* MODEL_DIR_PATH = "models";

		/**
		 * Name for latest model.
		 */
		constexpr const char* MODEL_LATEST_NAME = "latest";

		/**
		 * Torchscript model filename.
		 */
		constexpr const char* MODEL_FILENAME = "network.pt";

		/**
		 * Network output structure.
		 */
		struct output {
			float policy[4096], value;
		};

		void init(bool allow_gen = true);
		void generate();

		std::vector<output> evaluate(float* inp_board, float* inp_lmm, int batchsize);
	}
}
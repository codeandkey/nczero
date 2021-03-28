/* vim: set ts=4 sw=4 noet: */

/*
 * This file is subject to the terms and conditions defined in
 * LICENSE.txt, included in this source code distribution.
 */

#include <nczero/chess/attacks.h>
#include <nczero/chess/color.h>
#include <nczero/chess/position.h>
#include <nczero/chess/zobrist.h>
#include <nczero/log.h>
#include <nczero/net.h>
#include <nczero/pool.h>
#include <nczero/platform.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <cerrno>

#define MAX_BATCH_SIZE 256

using namespace neocortex;

int train();

int main(int argc, char** argv) {
#ifdef NEOCORTEX_DEBUG
	log::set_level(log::DEBUG);
#endif

	srand(time(NULL));

	neocortex_info(NEOCORTEX_NAME " " NEOCORTEX_VERSION " " NEOCORTEX_BUILDTIME " " NEOCORTEX_DEBUG_STR "\n");

	auto model_dir = std::filesystem::path("models");
	auto games_dir = std::filesystem::path("games");

	std::filesystem::create_directories(model_dir);
	std::filesystem::create_directories(games_dir);

	chess::bb::init();
	chess::zobrist::init();
	chess::attacks::init();

	// start uci
	std::string mode;
	std::getline(std::cin, mode);

	if (mode == "train") {
		 return train();
	}

	if (mode != "uci") {
		std::cout << "Unknown mode '" << mode << "'.";
		return 1;
	}

	std::cout << "id name neocortex 2.0\n";
	std::cout << "id author Justin Stanley\n";

	std::cout << "option name Threads type spin default " << pool::get_num_threads() << " min 1 max " << pool::get_num_threads() << "\n";
	std::cout << "option name Batch type spin default " << pool::get_batch_size() << " min 1 max " << MAX_BATCH_SIZE << "\n";
	std::cout << "option name NDPly type spin default -1 min -1 max 1024\n";
	std::cout << "uciok\n";

	std::string line;
	while (std::getline(std::cin, line)) {

		std::vector<std::string> args;
		std::istringstream f(line);
		std::string arg;

		while (std::getline(f, arg, ' ')) {
			if (arg.size()) {
				args.push_back(arg);
			}
		}

		if (args.empty()) {
			continue;
		}

		if (args[0] == "setoption") {
			if (args.size() < 5) {
				neocortex_error("setoption: expected 4 arguments, read %d\n", args.size() - 1);
			}

			if (args[1] != "Name") {
				neocortex_error("setoption: expected Name, read '%s'\n", args[1].c_str());
				continue;
			}

			if (args[3] != "Value") {
				neocortex_error("setoption: expected Value, read '%s'\n", args[3].c_str());
				continue;
			}

			int value;

			try {
				value = std::stoi(args[4]);
			} catch (std::exception& e) {
				neocortex_error("setoption: %s", e.what());
			}

			if (args[2] == "Threads") {
				if (value < 1 || value > std::thread::hardware_concurrency()) {
					neocortex_error("Invalid number of threads (min %d, max %d).\n", 1, std::thread::hardware_concurrency());
				}

				pool::set_num_threads(value);
			} else if (args[2] == "Batch") {
				if (value < 1 || value > MAX_BATCH_SIZE) {
					neocortex_error("Invalid batch size (min %d, max %d).\n", 1, MAX_BATCH_SIZE);
				}
			} else {
				neocortex_warn("Unknown option %s", args[2].c_str());
			}
		}

		if (args[0] == "go") {
		}

		if (args[0] == "train") {
			neocortex_info("Switching to training mode.\n");
			return train();
		}
	}

	return 0;
}

int usage(char* a0) {
	std::cout << "usage: " << a0 << " [-n num] [-b maxbatchsize] [-t time] [-j threads] [modelA [modelB]]\n";

	return 1;
}
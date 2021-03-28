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
#define MAX_ND_PLY 1024

using namespace neocortex;
using namespace std;

int train();

int main(int argc, char** argv) {
#ifdef NEOCORTEX_DEBUG
	log::set_level(log::DEBUG);
#endif

	srand(time(NULL));

	neocortex_info(NEOCORTEX_NAME " " NEOCORTEX_VERSION " " NEOCORTEX_BUILDTIME " " NEOCORTEX_DEBUG_STR "\n");

	auto model_dir = filesystem::path("models");
	auto games_dir = filesystem::path("games");

	filesystem::create_directories(model_dir);
	filesystem::create_directories(games_dir);

	chess::bb::init();
	chess::zobrist::init();
	chess::attacks::init();

	const size_t max_threads = thread::hardware_concurrency();

	pool::init(max_threads);

	// start uci
	string mode;
	getline(cin, mode);

	if (mode == "train") {
		 return train();
	}

	if (mode != "uci") {
		cout << "Unknown mode '" << mode << "'.";
		return 1;
	}

	cout << "id name neocortex 2.0\n";
	cout << "id author Justin Stanley\n";

	cout << "option name Threads type spin default " << pool::get_num_threads() << " min 1 max " << pool::get_num_threads() << "\n";
	cout << "option name Batch type spin default " << pool::get_batch_size() << " min 1 max " << MAX_BATCH_SIZE << "\n";
	cout << "option name NDPly type spin default -1 min -1 max " << MAX_ND_PLY << "\n";
	cout << "uciok\n";

	string line;
	while (getline(cin, line)) {

		vector<string> args;
		istringstream f(line);
		string arg;

		while (getline(f, arg, ' ')) {
			if (arg.size()) {
				args.push_back(arg);
			}
		}

		if (args.empty()) {
			continue;
		}

		if (args[0] == "isready") {
			cout << "readyok\n";
			continue;
		}

		if (args[0] == "setoption") {
			if (args.size() < 5) {
				neocortex_error("setoption: expected 4 arguments, read %d\n", args.size() - 1);
			}

			if (args[1] != "name") {
				neocortex_error("setoption: expected name, read '%s'\n", args[1].c_str());
				continue;
			}

			if (args[3] != "value") {
				neocortex_error("setoption: expected value, read '%s'\n", args[3].c_str());
				continue;
			}

			int value;

			try {
				value = stoi(args[4]);
			} catch (exception& e) {
				neocortex_error("setoption: %s", e.what());
			}

			if (args[2] == "Threads") {
				if (value < 1 || value > max_threads) {
					neocortex_error("Invalid number of threads (min %d, max %d).\n", 1, max_threads);
				}

				pool::set_num_threads(value);
			} else if (args[2] == "Batch") {
				if (value < 1 || value > MAX_BATCH_SIZE) {
					neocortex_error("Invalid batch size (min %d, max %d).\n", 1, MAX_BATCH_SIZE);
				}
			} else {
				neocortex_warn("Unknown option %s", args[2].c_str());
			}

			continue;
		}

		if (args[0] == "go") {
			continue;
		}

		if (args[0] == "train") {
			neocortex_info("Switching to training mode.\n");
			return train();
		}
	}

	return 0;
}

int train() {
	neocortex_info("Starting training.\n");

	return 0;
}

int usage(char* a0) {
	cout << "usage: " << a0 << " [-n num] [-b maxbatchsize] [-t time] [-j threads] [modelA [modelB]]\n";

	return 1;
}
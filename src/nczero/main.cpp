/* vim: set ts=4 sw=4 noet: */

/*
 * This file is subject to the terms and conditions defined in
 * LICENSE.txt, included in this source code distribution.
 */

#include <nczero/chess/attacks.h>
#include <nczero/chess/color.h>
#include <nczero/chess/move.h>
#include <nczero/chess/position.h>
#include <nczero/chess/zobrist.h>
#include <nczero/log.h>
#include <nczero/net.h>
#include <nczero/pool.h>
#include <nczero/platform.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <fstream>

#define MAX_BATCH_SIZE 256
#define MAX_ND_PLY 1024
#define DEFAULT_MOVE_FRAC 10
#define DEFAULT_MOVE_TIME 5000
#define NUM_GAMES 16

using namespace neocortex;
using namespace std;

int train();
int uci();

static size_t max_threads = thread::hardware_concurrency();

int main(int argc, char** argv) {
#ifdef NEOCORTEX_DEBUG
	log::set_level(log::DEBUG);
#endif

	srand(time(NULL));

	neocortex_info(NEOCORTEX_NAME " " NEOCORTEX_VERSION " " NEOCORTEX_BUILDTIME " " NEOCORTEX_DEBUG_STR "\n");

	chess::bb::init();
	chess::zobrist::init();
	chess::attacks::init();

	pool::init(max_threads);

	util::time_point start_point = util::time_now();

	try {
		nn::init();
	} catch(std::exception& e) { 
		neocortex_error("Failed to load model: %s\n", e.what());
		return 1;
	}

	neocortex_info("Loaded model in %d ms\n", util::time_elapsed_ms(start_point));

	if (argc > 1 && std::string(argv[1]) == "uci") {
		return uci();
	}

	return train();
}

int train() {
	neocortex_info("Starting training.\n");

	for (int i = 0; i < NUM_GAMES; ++i) {
		// Search for existing game path
		filesystem::path game_path = nn::MODEL_DIR_PATH;
		
		game_path /= nn::MODEL_LATEST_NAME;
		game_path /= std::to_string(i);

		if (filesystem::exists(game_path)) continue;
		neocortex_info("Starting game %d/%d\n", i + 1, NUM_GAMES);

		chess::position pos;
		std::ofstream output(game_path);

		if (!output) {
			neocortex_error("Failed to open %s for writing: %s", game_path.string().c_str(), strerror(errno));
			continue;
		}

		shared_ptr<node> search_tree = make_shared<node>();

		// Play game
		while (1) {
			// Search the position.
			int action = pool::search(search_tree, DEFAULT_MOVE_TIME, pos);

			// Write the decision.
			output << chess::move::to_uci(action);

			// Write the input layer.
			for (auto& el : pos.get_input()) {
				output << " " << el;
			}

			// Write the LMM layer.
			float lmm[4096] = {0.0f};

			for (auto& m : pos.legal_moves()) {
				int src = chess::move::src(m);
				int dst = chess::move::dst(m);

				if (pos.get_color_to_move() == chess::color::WHITE) {
					lmm[src * 64 + dst] = 1.0f;
				} else {
					lmm[(63 - src) * 64 + (63 - dst)] = 1.0f;
				}
			}

			for (int i = 0; i < 4096; ++i) {
				output << " " << lmm[i];
			}

			// Write normalized MCTS counts for POV
			// First get total n

			int total_n = 0;

			for (auto& c : search_tree->get_children()) {
				total_n += c->get_value().n;
			}

			std::vector<float> mcts_counts(4096, 0.0f);

			for (auto& c : search_tree->get_children()) {
				int src = (pos.get_color_to_move() == chess::color::WHITE) ? chess::move::src(c->get_action()) : (63 - chess::move::src(c->get_action()));
				int dst = (pos.get_color_to_move() == chess::color::WHITE) ? chess::move::dst(c->get_action()) : (63 - chess::move::dst(c->get_action()));

				mcts_counts[src * 64 + dst] = (float) c->get_value().n / (float) total_n;
			}

			for (size_t i = 0; i < mcts_counts.size(); ++i) {
				output << " " << mcts_counts[i];
			}

			output << "\n";

			// Write debug info on decision.
			std::vector<std::pair<int, shared_ptr<node>>> node_pairs;

			for (auto& c : search_tree->get_children()) {
				node_pairs.push_back(make_pair(c->get_value().n, c));
			}

			std::sort(node_pairs.begin(), node_pairs.end(), [&](auto& a, auto& b) { return a.first > b.first; });

			neocortex_debug("==> Selecting from %d children\n", node_pairs.size());
			for (size_t i = 0; i < node_pairs.size(); ++i) {
				node::value val = node_pairs[i].second->get_value();
				neocortex_debug(
					"=> %s %3.1f%% | N = %4d | Q = %3.2f | P = %3.1f%%\n",
					chess::move::to_uci(node_pairs[i].second->get_action()).c_str(),
					100.0f * (float) val.n / (float) total_n,
					val.n,
					(float) val.w / (float) val.n,
					100.0f * node_pairs[i].second->get_p_pct()
				);
			}

			neocortex_info("Making move %s\n", chess::move::to_uci(action).c_str());

			// Make the move.
			if (!pos.make_move(action)) {
				throw runtime_error("Search returned illegal move");
			}

			// Check if the game is over.
			optional<int> result = pos.is_game_over();

			if (result.has_value()) {
				// Write the result and stop this game.
				output << *result << "\n";
				break;
			}

			// Advance the search tree for next move.
			search_tree = search_tree->move_child(action);
		}
	}

	return 0;
}

int uci() {
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
	cout << "uciok\n";


	chess::position pos(chess::STARTING_FEN, true);
	shared_ptr<node> search_tree = make_shared<node>();

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
				if (value < 1 || value > (int) max_threads) {
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
			// Parse arguments

			int move_time = -1;
			int wtime = -1, btime = -1;
			int ourtime = (pos.get_color_to_move() == chess::color::WHITE) ? wtime : btime;

			for (size_t i = 1; i < args.size(); ++i) {
				auto parse_arg = [&](string a, int* dst) {
					if (i + 1 >= args.size()) {
						neocortex_error("go(%s): expected argument", a.c_str());
						return;
					}

					try {
						*dst = stoi(args[i + 1]);
					} catch (exception& e) {
						neocortex_error("go(%s): %s", a.c_str(), e.what());
					}
				};

				if (args[i] == "movetime") {
					parse_arg("movetime", &move_time);
					continue;
				}

				if (args[i] == "wtime") {
					parse_arg("wtime", &wtime);
					continue;
				}

				if (args[i] == "btime") {
					parse_arg("btime", &btime);
					continue;
				}

				neocortex_warn("go: unsupported option %s", args[i].c_str());
			}

			if (move_time == -1 && ourtime != -1) {
				move_time = ourtime / DEFAULT_MOVE_FRAC;
			}

			if (move_time == -1) {
				move_time = DEFAULT_MOVE_TIME;
			}

			int action = pool::search(search_tree, move_time, pos, true);

			cout << "bestmove " << chess::move::to_uci(action) << "\n";

			continue;
		}

		if (args[0] == "train") {
			neocortex_info("Switching to training mode.\n");
			return train();
		}
	}

	return 0;
}

int usage(char* a0) {
	cout << "usage: " << a0 << " [-n GAMES] [-m MOVEMS] [-t THREADS] [-b BATCH] [uci]\n";

	return 1;
}

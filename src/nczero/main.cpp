/* vim: set ts=4 sw=4 noet: */

/*
 * This file is subject to the terms and conditions defined in
 * LICENSE.txt, included in this source code distribution.
 */

#define _CRT_SECURE_NO_WARNINGS

#include "attacks.h"
#include "color.h"
#include "log.h"
#include "platform.h"
#include "zobrist.h"
#include "net.h"
#include "position.h"
#include "input.h"
#include "search.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <cerrno>

#define MODE_TRAIN 0

using namespace neocortex;

void train() {}

int usage(char* a0);

int main(int argc, char** argv) {
#ifdef NEOCORTEX_DEBUG
	log::set_level(log::DEBUG);
#endif

	srand(time(NULL));

	neocortex_info(NEOCORTEX_NAME " " NEOCORTEX_VERSION " " NEOCORTEX_BUILDTIME " " NEOCORTEX_DEBUG_STR "\n");

	auto model_dir = std::filesystem::path("models");
	auto games_dir = std::filesystem::path("games");

	if (std::filesystem::create_directories(model_dir)) {
		std::cout << "Created new models directory at " << model_dir << "\n";
	}

	if (std::filesystem::create_directories(games_dir)) {
		std::cout << "Created new games directory at " << games_dir << "\n";
	}

	bb::init();
	zobrist::init();
	attacks::init();

	int num_games = 32;
	int max_batchsize_per_thread = 16;
	int num_search_threads = 1; // std::thread::hardware_concurrency();
	int search_time = 2500;

	std::vector<std::string> model_paths;

	for (int i = 1; i < argc; ++i) {
		if (std::string(argv[i]) == "-n") {
			if (i == argc - 1) {
				neocortex_error("'-n': expected argument\n");
				return usage(*argv);
			}

			try {
				num_games = std::stoi(argv[i + 1], NULL, 10);
				++i;
			}
			catch (std::exception& e) {
				neocortex_error("Exception parsing '-n' argument: %s\n", e.what());
				return usage(*argv);
			}
		}
		else if (std::string(argv[i]) == "-b") {
			if (i == argc - 1) {
				neocortex_error("'-b': expected argument\n");
				return usage(*argv);
			}

			try {
				max_batchsize_per_thread = std::stoi(argv[i + 1], NULL, 10);
				++i;
			}
			catch (std::exception& e) {
				neocortex_error("Exception parsing '-b' argument: %s\n", e.what());
				return usage(*argv);
			}
		}
		else if (std::string(argv[i]) == "-t") {
			if (i == argc - 1) {
				neocortex_error("'-t': expected argument\n");
				return usage(*argv);
			}

			try {
				search_time = (int) (std::stof(argv[i + 1]) * 1000.0f);
				++i;
			}
			catch (std::exception& e) {
				neocortex_error("Exception parsing '-t' argument: %s\n", e.what());
				return usage(*argv);
			}
		}
		else if (std::string(argv[i]) == "-j") {
			if (i == argc - 1) {
				neocortex_error("'-j': expected argument\n");
				return usage(*argv);
			}

			try {
				num_search_threads = std::stoi(argv[i + 1], NULL, 10);
				++i;
			}
			catch (std::exception& e) {
				neocortex_error("Exception parsing '-j' argument: %s\n", e.what());
				return usage(*argv);
			}
		}

		else {
			model_paths.push_back(argv[i]);
		}
	}

	// No model provided, locate the newest generation
	if (!model_paths.size()) {
		int cgen = 0;

		while (1) {
			std::filesystem::path p = model_dir;
			p /= (std::string("gen") + std::to_string(cgen));

			if (std::filesystem::is_directory(p)) {
				++cgen;
			}
			else {
				if (!cgen) {
					neocortex_error("Failed to find first generation, cannot continue\n");
					return 1;
				}
				
				--cgen;
				break;
			}
		}

		neocortex_info("Defaulting to latest generation: %d\n", cgen);
		model_paths.push_back(std::string("gen") + std::to_string(cgen));
	}

	if (model_paths.size() > 2) {
		std::cerr << "Too many models!\n";
		return usage(*argv);
	}

	std::sort(model_paths.begin(), model_paths.end());

	// Start playing games!
	neocortex_info("Generating up to %d games between %s and %s\n", num_games, model_paths.front().c_str(), model_paths.back().c_str());

	std::filesystem::path target_games_dir = games_dir;
	target_games_dir /= model_paths.front() + "-vs-" + model_paths.back();

	if (std::filesystem::create_directories(target_games_dir)) {
		std::cout << "Created new game directory at " << target_games_dir << "\n";
	}

	neocortex_info("Saving games to %s\n", target_games_dir.string().c_str());
	neocortex_info("Initializing searchers.\n");
	
	try {
		std::list<Search*> searchers;

		for (int i = 0; i < model_paths.size(); ++i) {
			neocortex_info("Initializing search %d of %d\n", i + 1, model_paths.size());

			std::filesystem::path model_path = model_dir;
			model_path /= model_paths[i];

			searchers.push_back(new Search(model_path.string(), max_batchsize_per_thread, num_search_threads));
		}

		for (int i = 0; i < num_games; ++i) {
			std::filesystem::path game_path = target_games_dir;
			game_path /= std::to_string(i);

			// Skip games already generated
			if (std::filesystem::exists(game_path)) continue;

			neocortex_info("Generating game %d (%s)..\n", i, game_path.string().c_str());

			// Select white and black networks
			int w = rand() % 2;

			Search* players[2] = {
				w ? searchers.front() : searchers.back(),
				w ? searchers.back() : searchers.front(),
			};

			neocortex_info("White: %s | Black: %s\n",
				players[color::WHITE]->get_name().c_str(),
				players[color::BLACK]->get_name().c_str()
			);

			std::vector<int> game_moves;

			// Start making moves until game is over.
			int ctm = color::WHITE;

			std::vector<std::vector<float>> mcts_frames;
			std::vector<std::vector<float>> lmm_frames;
			std::vector<std::vector<float>> input_frames;
			std::vector<int> moves;

			Position p;
			int result;

			while (!p.is_game_over(&result)) {
				std::vector<float> mcts_frame, input_frame;

				int action = players[ctm]->search(search_time, &mcts_frame);

				lmm_frames.push_back(players[ctm]->get_lmm_frame());
				input_frames.push_back(players[ctm]->get_input_frame());

				players[ctm]->do_action(action);

				// Update the other searcher if there is one
				if (players[ctm] != players[!ctm]) {
					players[!ctm]->do_action(action);
				}

				mcts_frames.push_back(mcts_frame);
				moves.push_back(action);
				p.make_move(action);
				ctm = p.get_color_to_move();
			}

			neocortex_info("Game over, result %d\n", result);

			// Write decisions to output
			std::ofstream outfile(game_path.string().c_str());

			if (!outfile) {
				throw std::runtime_error("Failed to write game to " + game_path.string() + " : " + strerror(errno));
			}

			if (result != 0) {
				outfile << players[(1 - result) / 2]->get_name() << "\n";
			}
			else {
				outfile << "draw\n";
			}

			ctm = color::WHITE;

			for (int j = 0; j < moves.size(); ++j) {
				outfile << move::to_uci(moves[j]); // write decision
				outfile << " ";
				outfile << ((ctm == color::WHITE) ? 1 : -1) * result; // write outcome

				for (auto c : input_frames[j]) { // write input frame
					outfile << " ";
					outfile << c;
				}

				for (auto c : mcts_frames[j]) { // write mcts frame
					outfile << " ";
					outfile << c;
				}

				for (auto c : lmm_frames[j]) { // write lmm frame
					outfile << " ";
					outfile << c;
				}

				outfile << "\n";

				ctm = !ctm;
			}

			neocortex_info("Wrote gamedata to %s\n", game_path.string().c_str());

			for (auto s : searchers) {
				s->reset();
			}
		}

		while (searchers.size()) {
			delete searchers.back();
			searchers.pop_back();
		}
	}
	catch (std::exception& e) {
		neocortex_error("Exception raised during generation: %s\n", e.what());
		neocortex_error("Exiting!\n");
		return 1;
	}

	return 0;
}

int usage(char* a0) {
	std::cout << "usage: " << a0 << " [-n num] [-b maxbatchsize] [-t time] [-j threads] [modelA [modelB]]\n";

	return 1;
}

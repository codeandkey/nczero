/* vim: set ts=4 sw=4 noet: */

/*
 * This file is subject to the terms and conditions defined in
 * LICENSE.txt, included in this source code distribution.
 */

#include <nczero/chess/perft.h>
#include <nczero/log.h>
#include <nczero/timer.h>

#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace neocortex::chess;
using namespace std;

static perft::results current_results;
static void perft_movegen(position& p, int depth);

string perft::results::header() {
	return "| depth |     nodes |   captures |   checks | castles |   time |       nps |\n";
}

string perft::results::to_row(int depth) {
	ostringstream out;

	out << "|";
	out << " "	<< setw(5) << depth << " |";
	out << " "	<< setw(9) << nodes << " |";
	out << " "	<< setw(10) << captures << " |";
	out << " "	<< setw(8) << checks << " |";
	out << " "	<< setw(7) << castles << " |";
	out << " "	<< setw(6) << setprecision(2) << totaltime << " |";
	out << " "	<< setw(9) << nps << " |";

	return out.str();
}

perft::results perft::run(position& p, int depth) {
	if (depth < 0) {
		throw runtime_error("Invalid perft depth");
	}

	current_results = perft::results();

	timer::time_point now = timer::time_now();

	perft_movegen(p, depth);

	current_results.totaltime = timer::time_elapsed(now);
	current_results.nps = (unsigned long) (current_results.nodes / current_results.totaltime);

	return current_results;
}

void perft::start(position& p, int depth, ostream& out) {
	cout << "| depth |     nodes |   captures |   checks | castles |   time |       nps |\n";

	for (int i = 1; i <= depth; ++i) {
		cout << perft::run(p, i).to_row(i);
	}
}

void perft_movegen(position& p, int depth) {
	if (!depth) {
		current_results.nodes++;

		if (p.capture()) current_results.captures++;
		if (p.check()) current_results.checks++;
		if (p.castle()) current_results.castles++;
		if (p.en_passant()) current_results.en_passant++;
		if (p.promotion()) current_results.promotions++;

		return;
	}

	int pl_moves[MAX_PL_MOVES];
	int num_pl_moves = p.pseudolegal_moves(pl_moves);

	for (int i = 0; i < num_pl_moves; ++i) {
		if (p.make_move(pl_moves[i])) {
			perft_movegen(p, depth - 1);
		}

		p.unmake_move();
	}
}

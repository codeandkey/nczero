/* vim: set ts=4 sw=4 noet: */

/*
 * This file is subject to the terms and conditions defined in
 * LICENSE.txt, included in this source code distribution.
 */

#include <nczero/chess/board.h>
#include <nczero/chess/color.h>
#include <nczero/chess/type.h>
#include <nczero/log.h>

#include <cassert>
#include <cctype>
#include <vector>
#include <sstream>

using namespace neocortex::chess;

board::board() {
	for (int i = 0; i < 64; ++i) {
		state[i] = -1;
	}

	for (int c = 0; c < 2; ++c) {
		color_occ[c] = 0;
	}

	for (int p = 0; p < 6; ++p) {
		piece_occ[p] = 0;
	}

	key = 0;
	global_occ = 0;
}

board::board(std::string uci) : board() {
	std::stringstream ss(uci);
	std::string rank;

	int r = 8;

	while (std::getline(ss, rank, '/')) {
		--r;
		int f = 0;

		if (r < 0) {
			throw std::runtime_error("Invalid UCI: invalid rank count");
		}

		for (auto c : rank) {
			if (isdigit(c)) {
				for (int j = 0; j < (c - '0'); ++j) {
					if (f >= 8) {
						throw std::runtime_error("Invalid UCI: overflow in rank");
					}

					state[square::at(r, f++)] = piece::null();
				}
			} else {
				if (f >= 8) {
					throw std::runtime_error("Invalid UCI: overflow in rank");
				}

				place(square::at(r, f++), piece::from_uci(c));
			}
		}

		if (f != 8) {
			throw std::runtime_error("Invalid UCI: not enough pieces");
		}
	}

	if (r != 0) {
		throw std::runtime_error("Invalid UCI: invalid rank count");
	}
}

board board::standard() {
	return board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
}

void board::place(int sq, int p) {
	assert(!square::is_null(sq));
	assert(!piece::is_null(p));
	assert(piece::is_null(state[sq]));

	state[sq] = p;

	bitboard mask = bb::mask(sq);

	global_occ ^= mask;
	piece_occ[piece::type(p)] ^= mask;
	color_occ[piece::color(p)] ^= mask;
	key ^= zobrist::piece(sq, p);
}

int board::remove(int sq) {
	assert(!square::is_null(sq));
	assert(!piece::is_null(state[sq]));

	int res = state[sq];

	bitboard mask = bb::mask(sq);

	global_occ ^= mask;
	piece_occ[piece::type(res)] ^= mask;
	color_occ[piece::color(res)] ^= mask;
	key ^= zobrist::piece(sq, res);

	state[sq] = piece::null();

	return res;
}

int board::replace(int sq, int p) {
	assert(!square::is_null(sq));
	assert(!piece::is_null(state[sq]));

	int res = state[sq];

	bitboard mask = bb::mask(sq);

	piece_occ[piece::type(res)] ^= mask;
	color_occ[piece::color(res)] ^= mask;
	piece_occ[piece::type(p)] ^= mask;
	color_occ[piece::color(p)] ^= mask;

	key ^= zobrist::piece(sq, res);
	key ^= zobrist::piece(sq, p);

	state[sq] = p;

	return res;
}

std::string board::to_uci() {
	std::string output;

	int null_count = 0;

	for (int r = 7; r >= 0; --r) {
		for (int f = 0; f < 8; ++f) {
			int sq = square::at(r, f);

			if (piece::is_null(state[sq])) {
				null_count++;
			} else {
				if (null_count > 0) {
					output += '0' + null_count;
					null_count = 0;
				}

				output += piece::get_uci(state[sq]);
			}
		}

		if (null_count > 0) {
			output += '0' + null_count;
			null_count = 0;
		}

		if (r) output += '/';
	}

	return output;
}

std::string board::to_pretty() {
	std::string output;

	for (int r = 7; r >= 0; --r) {
		for (int f = 0; f < 8; ++f) {
			int sq = square::at(r, f);

			if (piece::is_null(state[sq])) {
				output += '.';
			} else {
				output += piece::get_uci(state[sq]);
			}
		}

		output += '\n';
	}

	return output;
}

bitboard board::attacks_on(int sq) {
	assert(!square::is_null(sq));

	bitboard white_pawns = piece_occ[type::PAWN] & color_occ[color::WHITE];
	bitboard black_pawns = piece_occ[type::PAWN] & color_occ[color::BLACK];
	bitboard rooks_queens = piece_occ[type::ROOK] | piece_occ[type::QUEEN];
	bitboard bishops_queens = piece_occ[type::BISHOP] | piece_occ[type::QUEEN];

	return (attacks::pawn(color::WHITE, sq) & black_pawns) |
		(attacks::pawn(color::BLACK, sq) & white_pawns) |
		(attacks::knight(sq) & piece_occ[type::KNIGHT]) |
		(attacks::bishop(sq, global_occ) & bishops_queens) |
		(attacks::rook(sq, global_occ) & rooks_queens) |
		(attacks::king(sq) & piece_occ[type::KING]);
}

bool board::mask_is_attacked(bitboard mask, int col) {
	bitboard opp = color_occ[col];

	while (mask) {
		if (attacks_on(bb::poplsb(mask)) & opp) return true;
	}

	return false;
}

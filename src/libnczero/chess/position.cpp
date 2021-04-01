/* vim: set ts=4 sw=4 noet: */

/*
 * This file is subject to the terms and conditions defined in
 * LICENSE.txt, included in this source code distribution.
 */

#include <nczero/chess/attacks.h>
#include <nczero/chess/color.h>
#include <nczero/chess/position.h>
#include <nczero/log.h>
#include <nczero/net.h>
#include <nczero/util.h>

#include <cassert>
#include <climits>
#include <cstring>

using namespace neocortex::chess;

position::position() {
	b = board::standard();

	State first_state;

	first_state.castle_rights = 0xF;
	first_state.en_passant_square = square::null();
	first_state.fullmove_number = 1;
	first_state.halfmove_clock = 0;
	first_state.captured_piece = piece::null();
	first_state.captured_square = square::null();
	first_state.last_move = move::null();
	first_state.key = 0;
	first_state.key ^= b.get_tt_key();
	first_state.key ^= zobrist::castle(first_state.castle_rights);
	first_state.in_check = 0;

	ply.push_back(first_state);
	color_to_move = color::WHITE;

	input[color::WHITE].resize(nn::SQUARE_BITS * 8 * 8, 0.0f);
	input[color::BLACK].resize(nn::SQUARE_BITS * 8 * 8, 0.0f);

	_write_frame();
}

position::position(std::string fen) {
	std::vector<std::string> fields = util::split(fen, ' ');

	if (fields.size() != 6) {
		throw util::fmterr("Invalid FEN: expected 6 fields, parsed %d", fields.size());
	}

	b = board(fields[0]);
	color_to_move = color::from_uci(fields[1][0]);
	
	State first_state;

	first_state.castle_rights = 0;

	for (auto c : fields[2]) {
		switch (c) {
		case 'K':
			first_state.castle_rights |= CASTLE_WHITE_K;
			break;
		case 'Q':
			first_state.castle_rights |= CASTLE_WHITE_Q;
			break;
		case 'k':
			first_state.castle_rights |= CASTLE_BLACK_K;
			break;
		case 'q':
			first_state.castle_rights |= CASTLE_BLACK_Q;
			break;
		}
	}

	first_state.en_passant_square = square::from_uci(fields[3]);
	first_state.halfmove_clock = std::stoi(fields[4]);
	first_state.fullmove_number = std::stoi(fields[5]);

	first_state.captured_piece = piece::null();
	first_state.captured_square = piece::null();
	first_state.last_move = move::null();

	first_state.key = 0;
	first_state.key ^= b.get_tt_key();
	first_state.key ^= zobrist::en_passant(first_state.en_passant_square);
	first_state.key ^= zobrist::castle(first_state.castle_rights);

	first_state.in_check = test_check(color_to_move);

	if (color_to_move == color::BLACK) {
		first_state.key ^= zobrist::black_to_move();
	}

	ply.push_back(first_state);

	input[color::WHITE].resize(nn::SQUARE_BITS * 8 * 8, 0.0f);
	input[color::BLACK].resize(nn::SQUARE_BITS * 8 * 8, 0.0f);

	_write_frame();
}

std::string position::to_fen() {
	assert(ply.size());

	std::string output;

	output += b.to_uci() + ' ';
	output += color::to_uci(color_to_move);
	output += ' ';

	if (!ply.back().castle_rights) {
		output += '-';
	} else {
		if (ply.back().castle_rights & CASTLE_WHITE_K) output += 'K';
		if (ply.back().castle_rights & CASTLE_WHITE_Q) output += 'Q';
		if (ply.back().castle_rights & CASTLE_BLACK_K) output += 'k';
		if (ply.back().castle_rights & CASTLE_BLACK_Q) output += 'q';
	}

	output += ' ';

	output += square::to_uci(ply.back().en_passant_square) + ' ';
	output += std::to_string(ply.back().halfmove_clock) + ' ';
	output += std::to_string(ply.back().fullmove_number);

	return output;
}

board& position::get_board() {
	return b;
}

bool position::make_move(int m) {
	assert(!test_check(!color_to_move));

	State last_state = ply.back();
	ply.push_back(last_state);

	State& next_state = ply.back();

	next_state.last_move = m;
	
	if (color_to_move == color::BLACK) {
		next_state.fullmove_number++;
	}

	next_state.halfmove_clock++;

	next_state.en_passant_square = square::null();
	next_state.captured_piece = piece::null();
	next_state.captured_square = square::null();

	int src = move::src(m);
	int dst = move::dst(m);
	int ctm = color_to_move;

	int src_piece = b.remove(src);

	if (m & move::CAPTURE) {
		// Move is standard capture
		next_state.captured_piece = b.remove(dst);
		next_state.captured_square = dst;
		next_state.halfmove_clock = 0;
	}
	else if (m & move::CAPTURE_EP) {
		// Move is en-passant capture
		static int adv_dir[2] = { NORTH, SOUTH };
		int capture_square = dst - adv_dir[ctm];

		next_state.captured_piece = b.remove(capture_square);
		next_state.captured_square = capture_square;
		next_state.halfmove_clock = 0;
	}
	else if (m & move::CASTLE_KS) {
		// Move is kingside castle
		static int ks_rook_src[2] = { square::H1, square::H8 };
		static int ks_rook_dst[2] = { square::F1, square::F8 };

		b.place(ks_rook_dst[ctm], b.remove(ks_rook_src[ctm]));
	}
	else if (m & move::CASTLE_QS) {
		// Move is queenside castle
		static int qs_rook_src[2] = { square::A1, square::A8 };
		static int qs_rook_dst[2] = { square::D1, square::D8 };

		b.place(qs_rook_dst[ctm], b.remove(qs_rook_src[ctm]));
	}

	// Test if pawn move to reset hmc
	if (b.get_piece_occ(type::PAWN) & bb::mask(src)) {
		next_state.halfmove_clock = 0;
	}

	if (m & move::PROMOTION) {
		b.place(dst, piece::make(ctm, move::ptype(m)));
	} else {
		b.place(dst, src_piece);
	}

	bitboard modmask = bb::mask(src) | bb::mask(dst);

	static bitboard ks_revoke[2] = { bb::mask(square::E1) | bb::mask(square::H1), bb::mask(square::E8) | bb::mask(square::H8) };
	static bitboard qs_revoke[2] = { bb::mask(square::E1) | bb::mask(square::A1), bb::mask(square::E8) | bb::mask(square::A8) };

	// Reset castling rights if either piece is moved or captured

	if (modmask & ks_revoke[color::WHITE]) {
		next_state.castle_rights &= ~CASTLE_WHITE_K;
	}

	if (modmask & qs_revoke[color::WHITE]) {
		next_state.castle_rights &= ~CASTLE_WHITE_Q;
	}

	if (modmask & ks_revoke[color::BLACK]) {
		next_state.castle_rights &= ~CASTLE_BLACK_K;
	}

	if (modmask & qs_revoke[color::BLACK]) {
		next_state.castle_rights &= ~CASTLE_BLACK_Q;
	}

	/* Update en passant */
	if (m & move::PAWN_JUMP) {
		static int adv_dir[2] = { NORTH, SOUTH };
		next_state.en_passant_square = dst - adv_dir[ctm];
	}

	/* Flip color to move */
	color_to_move = !color_to_move;

	/* Update zobrist key */
	next_state.key = b.get_tt_key();
	next_state.key ^= zobrist::en_passant(next_state.en_passant_square);
	next_state.key ^= zobrist::castle(next_state.castle_rights);

	next_state.in_check = test_check(color_to_move);

	if (color_to_move == color::BLACK) {
		next_state.key ^= zobrist::black_to_move();
	}

	_push_frame();
	_write_frame();

	/* Check that king is not in attack */
	return !test_check(!color_to_move);
}

bool position::make_matched_move(int m, int* matched_out) {
	int moves[MAX_PL_MOVES];
	int count = pseudolegal_moves(moves);

	for (int i = 0; i < count; ++i) {
		if (move::match(m, moves[i])) {
			if (matched_out) *matched_out = moves[i];
			return make_move(moves[i]);
		}
	}

	return false;
}

int position::unmake_move() {
	assert(ply.size() > 1);

	int m = ply.back().last_move;

	State last_state = ply.back();
	ply.pop_back();

	/* Flip CTM early for readability */
	color_to_move = !color_to_move;

	int src = move::src(m);
	int dst = move::dst(m);
	int ctm = color_to_move;

	int moved_piece = b.remove(dst);

	if (m & (move::CAPTURE | move::CAPTURE_EP)) {
		// Move was standard capture or EP

		b.place(last_state.captured_square, last_state.captured_piece);
	}
	else if (m & move::CASTLE_KS) {
		// Move was kingside castle
		static int ks_rook_src[2] = { square::H1, square::H8 };
		static int ks_rook_dst[2] = { square::F1, square::F8 };

		b.place(ks_rook_src[ctm], b.remove(ks_rook_dst[ctm]));
	}
	else if (m & move::CASTLE_QS) {
		// Move was queenside castle
		static int qs_rook_src[2] = { square::A1, square::A8 };
		static int qs_rook_dst[2] = { square::D1, square::D8 };

		b.place(qs_rook_src[ctm], b.remove(qs_rook_dst[ctm]));
	}

	if (m & move::PROMOTION) {
		// Move was promotion

		b.place(src, piece::make(ctm, type::PAWN));
	} else {
		b.place(src, moved_piece);
	}

	_pop_frame();
	_write_frame();

	return m;
}

bitboard position::en_passant_mask() {
	int sq = ply.back().en_passant_square;
	return square::is_null(sq) ? 0ULL : bb::mask(sq);
}

int position::pseudolegal_moves(int* out) {
	if (check()) {
		return pseudolegal_moves_evasions(out);
	}

	int count = 0;

	bitboard ctm = b.get_color_occ(color_to_move);
	bitboard opp = b.get_color_occ(!color_to_move);
	
	bitboard ep_mask = 0;
	int ep_square = ply.back().en_passant_square;

	if (!square::is_null(ep_square)) {
		ep_mask = bb::mask(ep_square);
	}

	/* Pawn moves */
	bitboard pawns = ctm & b.get_piece_occ(type::PAWN);

	bitboard promoting_rank = (color_to_move == color::WHITE) ? RANK_7 : RANK_2;
	bitboard starting_rank = (color_to_move == color::WHITE) ? RANK_2 : RANK_7;

	int adv_dir = (color_to_move == color::WHITE) ? NORTH : SOUTH;
	int left_dir = (color_to_move == color::WHITE) ? NORTHWEST : SOUTHWEST;
	int right_dir = (color_to_move == color::WHITE) ? NORTHEAST : SOUTHEAST;

	bitboard promoting_pawns = pawns & promoting_rank;

	/* Promoting left captures */
	bitboard promoting_left_cap = bb::shift(promoting_pawns & ~FILE_A, left_dir) & opp;

	while (promoting_left_cap) {
		int dst = bb::poplsb(promoting_left_cap);
		out[count++] = move::make(dst - left_dir, dst, type::QUEEN, move::PROMOTION | move::CAPTURE);
		out[count++] = move::make(dst - left_dir, dst, type::KNIGHT, move::PROMOTION | move::CAPTURE);
		out[count++] = move::make(dst - left_dir, dst, type::ROOK, move::PROMOTION | move::CAPTURE);
		out[count++] = move::make(dst - left_dir, dst, type::BISHOP, move::PROMOTION | move::CAPTURE);
	}

	/* Promoting right captures */
	bitboard promoting_right_cap = bb::shift(promoting_pawns & ~FILE_H, right_dir) & opp;

	while (promoting_right_cap) {
		int dst = bb::poplsb(promoting_right_cap);
		out[count++] = move::make(dst - right_dir, dst, type::QUEEN, move::PROMOTION | move::CAPTURE);
		out[count++] = move::make(dst - right_dir, dst, type::KNIGHT, move::PROMOTION | move::CAPTURE);
		out[count++] = move::make(dst - right_dir, dst, type::ROOK, move::PROMOTION | move::CAPTURE);
		out[count++] = move::make(dst - right_dir, dst, type::BISHOP, move::PROMOTION | move::CAPTURE);
	}

	/* Promoting advances */
	bitboard promoting_advances = bb::shift(promoting_pawns, adv_dir) & ~b.get_global_occ();

	while (promoting_advances) {
		int dst = bb::poplsb(promoting_advances);
		out[count++] = move::make(dst - adv_dir, dst, type::QUEEN, move::PROMOTION);
		out[count++] = move::make(dst - adv_dir, dst, type::KNIGHT, move::PROMOTION);
		out[count++] = move::make(dst - adv_dir, dst, type::ROOK, move::PROMOTION);
		out[count++] = move::make(dst - adv_dir, dst, type::BISHOP, move::PROMOTION);
	}

	/* Nonpromoting pawn moves */
	bitboard npm_pawns = pawns & ~promoting_pawns;

	/* Advances */
	bitboard npm_advances = bb::shift(npm_pawns, adv_dir) & ~b.get_global_occ();

	while (npm_advances) {
		int dst = bb::poplsb(npm_advances);
		out[count++] = move::make(dst - adv_dir, dst);
	}

	bitboard npm_left_atk = bb::shift(npm_pawns & ~FILE_A, left_dir);
	bitboard npm_right_atk = bb::shift(npm_pawns & ~FILE_H, right_dir);

	/* Left captures */
	bitboard npm_left_cap = npm_left_atk & opp;

	while (npm_left_cap) {
		int dst = bb::poplsb(npm_left_cap);
		out[count++] = move::make(dst - left_dir, dst, 0, move::CAPTURE);
	}

	/* Right captures */
	bitboard npm_right_cap = npm_right_atk & opp;

	while (npm_right_cap) {
		int dst = bb::poplsb(npm_right_cap);
		out[count++] = move::make(dst - right_dir, dst, 0, move::CAPTURE);
	}

	if (ep_mask) {
		/* Left EP captures */
		bitboard npm_left_ep = npm_left_atk & ep_mask;

		while (npm_left_ep) {
			int dst = bb::poplsb(npm_left_ep);
			out[count++] = move::make(dst - left_dir, dst, 0, move::CAPTURE_EP);
		}

		/* Right EP captures */
		bitboard npm_right_ep = npm_right_atk & ep_mask;

		while (npm_right_ep) {
			int dst = bb::poplsb(npm_right_ep);
			out[count++] = move::make(dst - right_dir, dst, 0, move::CAPTURE_EP);
		}
	}

	/* Jumps */
	bitboard npm_jumps = bb::shift(pawns & starting_rank, adv_dir) & ~b.get_global_occ();
	npm_jumps = bb::shift(npm_jumps, adv_dir) & ~b.get_global_occ();

	while (npm_jumps) {
		int dst = bb::poplsb(npm_jumps);
		out[count++] = move::make(dst - 2 * adv_dir, dst, 0, move::PAWN_JUMP);
	}

	/* Queen moves */
	bitboard queens = ctm & b.get_piece_occ(type::QUEEN);
	
	while (queens) {
		int src = bb::poplsb(queens);
		bitboard atk = attacks::queen(src, b.get_global_occ());

		bitboard quiets = atk & ~b.get_global_occ();
		bitboard captures = atk & opp;

		while (quiets) {
			int dst = bb::poplsb(quiets);
			out[count++] = move::make(src, dst);
		}

		while (captures) {
			int dst = bb::poplsb(captures);
			out[count++] = move::make(src, dst, 0, move::CAPTURE);
		}
	}

	/* Rook moves */
	bitboard rooks = ctm & b.get_piece_occ(type::ROOK);

	while (rooks) {
		int src = bb::poplsb(rooks);
		bitboard atk = attacks::rook(src, b.get_global_occ());

		bitboard quiets = atk & ~b.get_global_occ();
		bitboard captures = atk & opp;

		while (quiets) {
			int dst = bb::poplsb(quiets);
			out[count++] = move::make(src, dst);
		}

		while (captures) {
			int dst = bb::poplsb(captures);
			out[count++] = move::make(src, dst, 0, move::CAPTURE);
		}
	}

	/* Knight moves */
	bitboard knights = ctm & b.get_piece_occ(type::KNIGHT);

	while (knights) {
		int src = bb::poplsb(knights);
		bitboard atk = attacks::knight(src);

		bitboard quiets = atk & ~b.get_global_occ();
		bitboard captures = atk & opp;

		while (quiets) {
			int dst = bb::poplsb(quiets);
			out[count++] = move::make(src, dst);
		}

		while (captures) {
			int dst = bb::poplsb(captures);
			out[count++] = move::make(src, dst, 0, move::CAPTURE);
		}
	}

	/* Bishop moves */
	bitboard bishops = ctm & b.get_piece_occ(type::BISHOP);

	while (bishops) {
		int src = bb::poplsb(bishops);
		bitboard atk = attacks::bishop(src, b.get_global_occ());

		bitboard quiets = atk & ~b.get_global_occ();
		bitboard captures = atk & opp;

		while (quiets) {
			int dst = bb::poplsb(quiets);
			out[count++] = move::make(src, dst);
		}

		while (captures) {
			int dst = bb::poplsb(captures);
			out[count++] = move::make(src, dst, 0, move::CAPTURE);
		}
	}

	/* King moves */
	bitboard kings = ctm & b.get_piece_occ(type::KING);

	while (kings) {
		int src = bb::poplsb(kings);
		bitboard atk = attacks::king(src);

		bitboard quiets = atk & ~b.get_global_occ();
		bitboard captures = atk & opp;

		while (quiets) {
			int dst = bb::poplsb(quiets);
			out[count++] = move::make(src, dst);
		}

		while (captures) {
			int dst = bb::poplsb(captures);
			out[count++] = move::make(src, dst, 0, move::CAPTURE);
		}
	}

	/* Castling moves */

	/* We perform most of the legal move tests here, notably the noattack and occ tests */
	bitboard castle_rank = (color_to_move == color::WHITE) ? RANK_1 : RANK_8;
	bitboard noattack_ks = castle_rank & (FILE_E | FILE_F | FILE_G);
	bitboard noattack_qs = castle_rank & (FILE_E | FILE_D | FILE_C);
	bitboard no_occ_ks = castle_rank & (FILE_F | FILE_G);
	bitboard no_occ_qs = castle_rank & (FILE_B | FILE_C | FILE_D);

	static int king_src[2] = { square::E1, square::E8 };
	static int qs_dst[2] = { square::C1, square::C8 };
	static int ks_dst[2] = { square::G1, square::G8 };

	static int qsmask[2] = { CASTLE_WHITE_Q, CASTLE_BLACK_Q };
	static int ksmask[2] = { CASTLE_WHITE_K, CASTLE_BLACK_K };

	/* Kingside */
	if (ply.back().castle_rights & ksmask[color_to_move]) {
		/* occ test */
		if (!(b.get_global_occ() & no_occ_ks)) {
			/* noattack test */
			if (!b.mask_is_attacked(noattack_ks, !color_to_move)) {
				out[count++] = move::make(king_src[color_to_move], ks_dst[color_to_move], 0, move::CASTLE_KS);
			}
		}
	}

	/* Queenside */
	if (ply.back().castle_rights & qsmask[color_to_move]) {
		/* occ test */
		if (!(b.get_global_occ() & no_occ_qs)) {
			/* noattack test */
			if (!b.mask_is_attacked(noattack_qs, !color_to_move)) {
				out[count++] = move::make(king_src[color_to_move], qs_dst[color_to_move], 0, move::CASTLE_QS);
			}
		}
	}

	assert(count <= MAX_PL_MOVES);
	return count;
}

int position::pseudolegal_moves_evasions(int* out) {
	int count = 0;

	bitboard ctm = b.get_color_occ(color_to_move);
	bitboard opp = b.get_color_occ(!color_to_move);

	bitboard ep_mask = 0;
	int ep_square = ply.back().en_passant_square;

	/* King moves */
	bitboard kings = ctm & b.get_piece_occ(type::KING);

	/* Also grab attackers */
	int king_sq = bb::getlsb(kings);
	bitboard attackers = b.attacks_on(king_sq) & opp;

	while (kings) {
		int src = bb::poplsb(kings);
		bitboard atk = attacks::king(src);

		bitboard quiets = atk & ~b.get_global_occ();
		bitboard captures = atk & opp;

		while (quiets) {
			int dst = bb::poplsb(quiets);
			out[count++] = move::make(src, dst);
		}

		while (captures) {
			int dst = bb::poplsb(captures);
			out[count++] = move::make(src, dst, 0, move::CAPTURE);
		}
	}

	if (bb::popcount(attackers) > 1) {
		/* double check, king moves only */
		return count;
	}

	/* Only one attacker -- look for pieces that can capture it */
	int attacker_square = bb::getlsb(attackers);

	/* If checker is sliding piece, add blocking moves */
	bitboard block_dsts = bb::between(king_sq, attacker_square);

	if (!square::is_null(ep_square)) {
		ep_mask = bb::mask(ep_square);
	}

	/* Pawn moves */
	bitboard pawns = ctm & b.get_piece_occ(type::PAWN);

	bitboard promoting_rank = (color_to_move == color::WHITE) ? RANK_7 : RANK_2;
	bitboard starting_rank = (color_to_move == color::WHITE) ? RANK_2 : RANK_7;

	int adv_dir = (color_to_move == color::WHITE) ? NORTH : SOUTH;
	int left_dir = (color_to_move == color::WHITE) ? NORTHWEST : SOUTHWEST;
	int right_dir = (color_to_move == color::WHITE) ? NORTHEAST : SOUTHEAST;

	bitboard promoting_pawns = pawns & promoting_rank;

	/* Promoting left captures */
	bitboard promoting_left_cap = bb::shift(promoting_pawns & ~FILE_A, left_dir) & attackers;

	while (promoting_left_cap) {
		int dst = bb::poplsb(promoting_left_cap);
		out[count++] = move::make(dst - left_dir, dst, type::QUEEN, move::PROMOTION | move::CAPTURE);
		out[count++] = move::make(dst - left_dir, dst, type::KNIGHT, move::PROMOTION | move::CAPTURE);
		out[count++] = move::make(dst - left_dir, dst, type::ROOK, move::PROMOTION | move::CAPTURE);
		out[count++] = move::make(dst - left_dir, dst, type::BISHOP, move::PROMOTION | move::CAPTURE);
	}

	/* Promoting right captures */
	bitboard promoting_right_cap = bb::shift(promoting_pawns & ~FILE_H, right_dir) & attackers;

	while (promoting_right_cap) {
		int dst = bb::poplsb(promoting_right_cap);
		out[count++] = move::make(dst - right_dir, dst, type::QUEEN, move::PROMOTION | move::CAPTURE);
		out[count++] = move::make(dst - right_dir, dst, type::KNIGHT, move::PROMOTION | move::CAPTURE);
		out[count++] = move::make(dst - right_dir, dst, type::ROOK, move::PROMOTION | move::CAPTURE);
		out[count++] = move::make(dst - right_dir, dst, type::BISHOP, move::PROMOTION | move::CAPTURE);
	}

	/* Promoting advances */
	bitboard promoting_advances = bb::shift(promoting_pawns, adv_dir) & ~b.get_global_occ() & block_dsts;

	while (promoting_advances) {
		int dst = bb::poplsb(promoting_advances);
		out[count++] = move::make(dst - adv_dir, dst, type::QUEEN, move::PROMOTION);
		out[count++] = move::make(dst - adv_dir, dst, type::KNIGHT, move::PROMOTION);
		out[count++] = move::make(dst - adv_dir, dst, type::ROOK, move::PROMOTION);
		out[count++] = move::make(dst - adv_dir, dst, type::BISHOP, move::PROMOTION);
	}

	/* Nonpromoting pawn moves */
	bitboard npm_pawns = pawns & ~promoting_pawns;

	/* Advances */
	bitboard npm_advances = bb::shift(npm_pawns, adv_dir) & ~b.get_global_occ() & block_dsts;

	while (npm_advances) {
		int dst = bb::poplsb(npm_advances);
		out[count++] = move::make(dst - adv_dir, dst);
	}

	bitboard npm_left_atk = bb::shift(npm_pawns & ~FILE_A, left_dir);
	bitboard npm_right_atk = bb::shift(npm_pawns & ~FILE_H, right_dir);

	/* Left captures */
	bitboard npm_left_cap = npm_left_atk & attackers;

	while (npm_left_cap) {
		int dst = bb::poplsb(npm_left_cap);
		out[count++] = move::make(dst - left_dir, dst, 0, move::CAPTURE);
	}

	/* Right captures */
	bitboard npm_right_cap = npm_right_atk & attackers;

	while (npm_right_cap) {
		int dst = bb::poplsb(npm_right_cap);
		out[count++] = move::make(dst - right_dir, dst, 0, move::CAPTURE);
	}

	if (ep_mask) {
		/* Left EP captures */
		bitboard npm_left_ep = npm_left_atk & ep_mask;

		while (npm_left_ep) {
			int dst = bb::poplsb(npm_left_ep);
			out[count++] = move::make(dst - left_dir, dst, 0, move::CAPTURE_EP);
		}

		/* Right EP captures */
		bitboard npm_right_ep = npm_right_atk & ep_mask;

		while (npm_right_ep) {
			int dst = bb::poplsb(npm_right_ep);
			out[count++] = move::make(dst - right_dir, dst, 0, move::CAPTURE_EP);
		}
	}

	/* Jumps */
	bitboard npm_jumps = bb::shift(pawns & starting_rank, adv_dir) & ~b.get_global_occ();
	npm_jumps = bb::shift(npm_jumps, adv_dir) & ~b.get_global_occ() & block_dsts;

	while (npm_jumps) {
		int dst = bb::poplsb(npm_jumps);
		out[count++] = move::make(dst - 2 * adv_dir, dst, 0, move::PAWN_JUMP);
	}

	/* Queen moves */
	bitboard queens = ctm & b.get_piece_occ(type::QUEEN);

	while (queens) {
		int src = bb::poplsb(queens);
		bitboard atk = attacks::queen(src, b.get_global_occ());

		bitboard quiets = atk & ~b.get_global_occ() & block_dsts;
		bitboard captures = atk & attackers;

		while (quiets) {
			int dst = bb::poplsb(quiets);
			out[count++] = move::make(src, dst);
		}

		while (captures) {
			int dst = bb::poplsb(captures);
			out[count++] = move::make(src, dst, 0, move::CAPTURE);
		}
	}

	/* Rook moves */
	bitboard rooks = ctm & b.get_piece_occ(type::ROOK);

	while (rooks) {
		int src = bb::poplsb(rooks);
		bitboard atk = attacks::rook(src, b.get_global_occ());

		bitboard quiets = atk & ~b.get_global_occ() & block_dsts;
		bitboard captures = atk & attackers;

		while (quiets) {
			int dst = bb::poplsb(quiets);
			out[count++] = move::make(src, dst);
		}

		while (captures) {
			int dst = bb::poplsb(captures);
			out[count++] = move::make(src, dst, 0, move::CAPTURE);
		}
	}

	/* Knight moves */
	bitboard knights = ctm & b.get_piece_occ(type::KNIGHT);

	while (knights) {
		int src = bb::poplsb(knights);
		bitboard atk = attacks::knight(src);

		bitboard quiets = atk & ~b.get_global_occ() & block_dsts;
		bitboard captures = atk & attackers;

		while (quiets) {
			int dst = bb::poplsb(quiets);
			out[count++] = move::make(src, dst);
		}

		while (captures) {
			int dst = bb::poplsb(captures);
			out[count++] = move::make(src, dst, 0, move::CAPTURE);
		}
	}

	/* Bishop moves */
	bitboard bishops = ctm & b.get_piece_occ(type::BISHOP);

	while (bishops) {
		int src = bb::poplsb(bishops);
		bitboard atk = attacks::bishop(src, b.get_global_occ());

		bitboard quiets = atk & ~b.get_global_occ() & block_dsts;
		bitboard captures = atk & attackers;

		while (quiets) {
			int dst = bb::poplsb(quiets);
			out[count++] = move::make(src, dst);
		}

		while (captures) {
			int dst = bb::poplsb(captures);
			out[count++] = move::make(src, dst, 0, move::CAPTURE);
		}
	}

	assert(count <= MAX_PL_MOVES);
	return count;
}

zobrist::Key position::get_tt_key() {
	return ply.back().key;
}

int position::num_repetitions() {
	int res = 0;

	for (size_t i = 0; i < ply.size(); ++i) {
		if (ply[i].key == ply.back().key) {
			++res;
		}
	}

	return res;
}

int position::halfmove_clock() {
	return ply.back().halfmove_clock;
}

bool position::is_draw_by_hrm() {
	// Check halfmove clock
	if (halfmove_clock() >= 100) {
		return true;
	}

	// Check threefold repetition
	int reps = 0;
	for (auto i : ply) {
		if (i.key == ply.back().key) {
			if (++reps >= 3) {
				return true;
			}
		}
	}

	// Check insufficient material
	bitboard nking = b.get_global_occ() & ~b.get_piece_occ(type::KING);

	if (!nking) {
		// Only two kings
		return true;
	}

	if ((nking == (b.get_piece_occ(type::KNIGHT) | b.get_piece_occ(type::BISHOP))) && bb::popcount(nking) == 1) {
		// King and minor piece vs. king
		return true;
	}

	if (nking == b.get_piece_occ(type::BISHOP) && bb::popcount(nking) == 2) {
		// Kings and two same-color bishops

		int first = bb::poplsb(nking);
		int second = bb::poplsb(nking);

		if (first % 2 == second % 2) {
			return true;
		}
	}

	// Game is not draw by HRM
	return false;
}


void position::_write_frame() {
	int reps = num_repetitions() - 1;
	float rb1 = reps & 1;
	float rb2 = reps >> 1;

	static int pbits_lookup[2][12] = {
		{ // white POV
			0, // wpawn : 00
			6, // bpawn : 01
			1, // wbishop : 10
			7, // bbishop : 11
			2, // wknight : 100
			8, // bknight : 101
			3, // wrook : 110
			9, // brook : 111
			4, // wqueen : 1000
			10, // bqueen : 1001
			5, // wking : 1010
			11, // bking : 1011
		},
		{ // black POV
			6, // wpawn : 00
			0, // bpawn : 01
			7, // wbishop : 10
			1, // bbishop : 11
			8, // wknight : 100
			2, // bknight : 101
			9, // wrook : 110
			3, // brook : 111
			10, // wqueen : 1000
			4, // bqueen : 1001
			11, // wking : 1010
			5, // bking : 1011
		},
	};

	// Write headers while we're iterating over the squares anyway
	float header[15];

	int move_number = ply.back().fullmove_number;
	int halfmove_clock = ply.back().halfmove_clock;

	for (int i = 0; i < 9; ++i) {
		header[i] = (float) ((move_number >> i) & 1);
	}

	for (int i = 0; i < 6; ++i) {
		header[9 + i] = (float)((halfmove_clock >> i) & 1);
	}

	for (size_t r = 0; r < 8; ++r) {
		for (size_t f = 0; f < 8; ++f) {
			int p = b.get_piece(square::at(r, f));

			for (int c = 0; c < 2; ++c) {
				size_t offset;

				if (c == color::WHITE) {
					offset = (size_t) r * 680 + f * 85;
				} else {
					offset = (size_t) (7 - r) * 680 + (7 - f) * 85;
				}

				// Write square header
				memcpy(&input[c][offset], header, sizeof(float) * 15);

				// Write square piece data
				if (!piece::is_null(p)) {
					input[c][offset + 15 + pbits_lookup[c][p]] = 1.0f;
				}

				// Write repetition bits
				input[c][offset + 27] = rb1;
				input[c][offset + 28] = rb2;
			}
		}
	}
}

void position::_push_frame() {
	// 8 ranks * 8 squares * 2 colors * 14 bits per frame
	std::vector<std::vector<std::vector<std::vector<float>>>> frame;

	for (size_t r = 0; r < 8; ++r) {
		std::vector<std::vector<std::vector<float>>> rank;

		for (size_t f = 0; f < 8; ++f) {
			std::vector<std::vector<float>> colors;

			for (int c = 0; c < 2; ++c) {
				size_t offset = (size_t) r * 680 + f * 85;

				// Grab last frame
				std::vector<float> bits(&input[c][offset + 71], &input[c][offset + 71] + 14);

				// Shift frames back
				memmove(&input[c][offset + 29], &input[c][offset + 15], sizeof(float) * 14 * 4);

				// Zero most recent frame
				for (int i = 0; i < 14; ++i) {
					input[c][offset + 15 + i] = 0.0f;
				}

				colors.push_back(bits);
			}

			rank.push_back(colors);
		}

		frame.push_back(rank);
	}

	hist_frames.push_back(frame);
}

void position::_pop_frame() {
	auto ranks = hist_frames.back();

	// Rewrite headers with current state
	float header[15];

	int move_number = ply.back().fullmove_number;
	int halfmove_clock = ply.back().halfmove_clock;

	for (int i = 0; i < 9; ++i) {
		header[i] = (float) ((move_number >> i) & 1);
	}

	for (int i = 0; i < 6; ++i) {
		header[9 + i] = (float)((halfmove_clock >> i) & 1);
	}

	for (size_t r = 0; r < 8; ++r) {
		for (size_t f = 0; f < 8; ++f) {
			for (int c = 0; c < 2; ++c) {
				size_t offset = (size_t) r * 680 + f * 85;

				// Shift frames forward
				memmove(&input[c][offset + 15], &input[c][offset + 29], sizeof(float) * 14 * 4);

				// Grab last frame
				memcpy(&input[c][offset + 71], &ranks[r][f][c][0], sizeof(float) * 14);

				// Write square header
				memcpy(&input[c][offset], header, sizeof(float) * 15);
			}
		}
	}

	hist_frames.pop_back();
}

std::string position::dump() {
	std::string output;

	output += "board: \n";
	output += b.to_pretty();
	output += "\n";

	output += "history: ";

	for (unsigned i = 1; i < ply.size(); ++i) {
		output += move::to_uci(ply[i].last_move) + " ";
	}

	output += "\n";

	output += "check: " + std::string(check() ? "yes" : "no") + std::string("\n");
	output += "test_check(ctm): " + std::string(test_check(color_to_move) ? "yes" : "no") + std::string("\n");
	output += "test_check(!ctm): " + std::string(test_check(!color_to_move) ? "yes" : "no") + std::string("\n");

	int moves[MAX_PL_MOVES];
	int num_moves = pseudolegal_moves(moves);

	output += "pseudolegal moves: ";

	for (int i = 0; i < num_moves; ++i) {
		output += move::to_uci(moves[i]) + " ";
	}

	output += "\n";

	return output;
}

std::optional<int> position::is_game_over() {
	if (is_draw_by_hrm()) {
		return 0;
	}

	// Generate legal moves
	int moves[MAX_PL_MOVES], num_legal = 0;
	int num_pl_moves = pseudolegal_moves(moves);

	for (int i = 0; i < num_pl_moves; ++i) {
		if (make_move(moves[i])) {
			++num_legal;
		}

		unmake_move();
	}

	if (!num_legal) {
		if (check()) {
			return (color_to_move == color::WHITE) ? -1 : 1;
		} else {
			return 0;
		}
	}

	return std::optional<int>();
}

std::vector<int> position::legal_moves() {
	std::vector<int> output;

	// Generate legal moves
	int moves[MAX_PL_MOVES], num_legal = 0;
	int num_pl_moves = pseudolegal_moves(moves);

	for (int i = 0; i < num_pl_moves; ++i) {
		if (make_move(moves[i])) {
			output.push_back(moves[i]);
		}

		unmake_move();
	}

	return output;
}
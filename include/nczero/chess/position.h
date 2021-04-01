/* vim: set ts=4 sw=4 noet: */

/*
 * This file is subject to the terms and conditions defined in
 * LICENSE.txt, included in this source code distribution.
 */

#pragma once

#include <nczero/chess/board.h>
#include <nczero/chess/move.h>
#include <nczero/chess/piece.h>
#include <nczero/chess/square.h>
#include <nczero/chess/type.h>

#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace neocortex {
	namespace chess {
		constexpr int CASTLE_WHITE_K = 1;
		constexpr int CASTLE_WHITE_Q = 2;
		constexpr int CASTLE_BLACK_K = 4;
		constexpr int CASTLE_BLACK_Q = 8;

		constexpr int MAX_PL_MOVES = 100;

		class position {
		public:
			struct State {
				int last_move = move::null();
				int en_passant_square = square::null();
				int castle_rights = 0xF;
				int captured_piece = piece::null();
				int captured_square = square::null();
				int halfmove_clock = 0;
				int fullmove_number = 1;
				int in_check = 0;

				zobrist::Key key = 0;
			};

			/**
			* Constructs a standard start position.
			*/
			position();

			/**
			* Constructs a position for a FEN.
			*
			* @param fen Input FEN.
			*/
			position(std::string fen);

			/**
			* Converts a position to a FEN.
			*
			* @retrun FEN string.
			*/
			std::string to_fen();

			/**
			* Gets a reference to the position's current board.
			*
			* @return Board reference.
			*/
			board& get_board();

			/**
			* Gets the current color to move.
			*
			* @return Color to move.
			*/
			int get_color_to_move();

			/**
			* Tries to make a move.
			* 
			* @param move Pseudolegal move.
			* @return true if move is legal, false otherwise.
			*/
			bool make_move(int move);

			/**
			* Tries to match a uci move to a pseudolegal move.
			* This is SLOW and should only be used for testing or user input.
			* 
			* @param m Input move, to be matched.
			* @param m Matched move dest if non NULL.
			* @return true if move is legal, false otherwise.
			*/
			bool make_matched_move(int move, int* matched_out = NULL);

			/**
			* Unmakes a move.
			* 
			* @return Move unmade.
			*/
			int unmake_move();

			/**
			* Gets a mask of valid en passant squares.
			*
			* @return En-passant mask.
			*/
			bitboard en_passant_mask(); /* gets a mask of valid en passant capture targets, or 0 if none */

			/**
			* Gets the position's zobrist key for TT indexing.
			*
			* @return Zobrist key.
			*/
			zobrist::Key get_tt_key();

			/**
			* Test if the position is a check.
			*
			* @return true if color to move is in check, false otherwise.
			*/
			bool check();

			/**
			* Test if the last move made was a capture.
			*
			* @return true if a capture was made, false otherwise.
			*/
			bool capture();

			/**
			* Test if the last move made was an en passant capture.
			*
			* @return true if last move was en passant, false otherwise.
			*/
			bool en_passant();

			/**
			* Test if the last move made was a promotion.
			*
			* @return true if last move was a promotion, false otherwise.
			*/
			bool promotion();

			/**
			* Test if the last move made was a castle.
			*
			* @return true if last move was a castle, false otherwise.
			*/
			bool castle();

			/**
			* Gets the number of times the current position has occurred throughout the game.
			*
			* @return Number of instances. Will always be at least 1.
			*/
			int num_repetitions();

			/**
			* Gets the game's halfmove clock.
			*
			* @return Halfmove clock.
			*/
			int halfmove_clock();

			/**
			* Gets the game's move number.
			*
			* @return Move number.
			*/
			int move_number();

			/**
			* Gets the last move.
			*
			* @return Last move.
			*/
			int last_move();

			/**
			* Gets the pseudolegal moves for the position.
			* 
			* @param dst Buffer to fill with moves. Must be MAX_PL_MOVES size.
			* @return Number of moves generated.
			*/
			int pseudolegal_moves(int* dst);

			/**
			* Gets evasion moves for a position.
			*
			* @param dst Buffer to fill with moves. Must be MAX_PL_MOVES size.
			* @return Number of moves generated.
			*/
			int pseudolegal_moves_evasions(int* dst);

			/**
			* Get a printable debug dump of the position.
			* 
			* @return Printable debug string.
			*/
			std::string dump();

			/**
			* Tests if the game is a draw by halfmove rule, repetition, or material.
			* @return true if game is draw by HRM, false otherwise
			*/
			bool is_draw_by_hrm();

			/**
			* Tests if the game is a over.
			* @return wPOV value if game over, none otherwise
			*/
			std::optional<int> is_game_over();

			/**
			* Gets a reference to the board input layer.
			* @return reference to board input layer
			*/
			std::vector<float>& get_input();

			/**
			 * Generates legal moves for the position.
			 * @return vector of legal moves.
			 */
			std::vector<int> legal_moves();

		private:
			board b;
			std::vector<State> ply;
			int color_to_move;
			std::vector<float> input[2];

			// hist_ply * 8 ranks * 8 files * 2 colors * 14 bits
			std::vector<std::vector<std::vector<std::vector<std::vector<float>>>>> hist_frames;

			/**
			* Writes the current frame into the input layer.
			* Updates first history ply and writes square headers.
			*/
			void _write_frame();

			/**
			* Pushes the current input frame onto the history stack.
			* Sets the first history ply to 0s.
			*/
			void _push_frame();

			/**
			* Shift the history plies back 1, overwriting the first history ply.
			* Also updates the square headers to the current move number and HMC.
			*/
			void _pop_frame();

			/**
			* Tests if the position is in check.
			* @param col Color to test if in check
			* @return true if <col> is in check, false otherwise
			*/
			bool test_check(int col) {
				return (b.attacks_on(bb::getlsb(b.get_piece_occ(type::KING) & b.get_color_occ(col))) & b.get_color_occ(!col)) != 0;
			}
		};

		inline int position::get_color_to_move() {
			return color_to_move;
		}

		inline bool position::capture() {
			return (ply.size() > 1) && (ply.back().last_move & (move::CAPTURE | move::CAPTURE_EP));
		}

		inline bool position::check() {
			return ply.back().in_check;
		}

		inline bool position::en_passant() {
			return (ply.size() > 1) && ply.back().last_move & move::CAPTURE_EP;
		}

		inline bool position::castle() {
			return (ply.size() > 1) && ply.back().last_move & (move::CASTLE_KS | move::CASTLE_QS);
		}

		inline bool position::promotion() {
			return (ply.size() > 1) && ply.back().last_move & move::PROMOTION;
		}

		inline int position::move_number() {
			return ply.back().fullmove_number;
		}

		inline int position::last_move() {
			return ply.back().last_move;
		}

		inline std::vector<float>& position::get_input() {
			return input[color_to_move];
		}
	}
}

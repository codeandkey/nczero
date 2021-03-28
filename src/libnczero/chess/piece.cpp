/* vim: set ts=4 sw=4 noet: */

/*
 * This file is subject to the terms and conditions defined in
 * LICENSE.txt, included in this source code distribution.
 */

#include <nczero/chess/color.h>
#include <nczero/chess/piece.h>
#include <nczero/chess/type.h>
#include <nczero/util.h>

#include <cctype>

using namespace neocortex::chess;

char piece::get_uci(int piece) {
	if (is_null(piece) || piece > 12) {
		return '?';
	}

	return "PpBbNnRrQqKk"[piece];
}

int piece::from_uci(char uci) {
	char typechar = tolower(uci);
	int ptype = type::from_uci(typechar);

	return make((uci == typechar) ? color::BLACK : color::WHITE, ptype);
}

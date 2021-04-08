/* vim: set ts=4 sw=4 noet: */

/*
 * This file is subject to the terms and conditions defined in
 * LICENSE.txt, included in this source code distribution.
 */

#pragma once

#include "platform.h"

#include <iostream>
#include <chrono>
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>

namespace neocortex {
	namespace timer {
		/* Time manipulation */
#ifdef NEOCORTEX_WIN32
		typedef clock_t time_point;
#else
		typedef struct timespec time_point;
#endif

		/**
		 * Gets a string representing the current time.
		 *
		 * @return Current time string.
		 */
		std::string timestring();

		/**
		 * Gets a reference to the current time.
		 *
		 * @return Current time point.
		 */
		time_point time_now();

		/**
		 * Gets the elapsed time in seconds since a reference point.
		 *
		 * @param reference Reference point.
		 * @return Seconds elapsed since the reference point.
		 */
		double time_elapsed(time_point reference);

		/**
		 * Gets the elapsed time in milliseconds since a reference point.
		 *
		 * @param reference Reference point.
		 * @return Milliseconds elapsed since the reference point.
		 */
		int time_elapsed_ms(time_point reference);
	}
}

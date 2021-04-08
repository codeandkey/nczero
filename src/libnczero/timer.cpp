/* vim: set ts=4 sw=4 noet: */

/*
 * This file is subject to the terms and conditions defined in
 * LICENSE.txt, included in this source code distribution.
 */

#include <nczero/timer.h>

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>

#if defined NEOCORTEX_LINUX || defined NEOCORTEX_APPLE
#include <errno.h>
#include <cstring>
#endif

using namespace neocortex;

std::string timer::timestring() {
	std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm tm;

#ifdef NEOCORTEX_WIN32
	if (localtime_s(&tm, &t)) {
		throw std::runtime_error("localtime() failed.");
	}
#else
	if (!localtime_r(&t, &tm)) {
		throw std::runtime_error("localtime() failed.");
	}
#endif

	std::stringstream ss;
	ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

	return ss.str();
}

timer::time_point timer::time_now() {
#ifdef NEOCORTEX_WIN32
	LARGE_INTEGER ctr;
	if (!QueryPerformanceCounter(&ctr)) {
		throw std::runtime_error("QueryPerformanceFrequency failed");
	}
	return ctr.QuadPart;
#else
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now;
#endif
}

double timer::time_elapsed(time_point reference) {
#ifdef NEOCORTEX_WIN32
	static LARGE_INTEGER freq;
	static bool freqstat = QueryPerformanceFrequency(&freq);
	static float freq_f = double(freq.QuadPart);
	if (!freqstat) {
		throw std::runtime_error("QueryPerformanceFrequency failed");
	}
	return double(time_now() - reference) / freq_f;
#else
	timer::time_point now = time_now();
	double elapsed = (now.tv_sec - reference.tv_sec);
	elapsed += (now.tv_nsec - reference.tv_nsec) / 1000000000.0;

	return elapsed;
#endif
}

int timer::time_elapsed_ms(time_point reference) {
	return time_elapsed(reference) * 1000;
}

/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "assert.h"

#include "util.h"
#include "log.h"
#include "util/io.h"

void _ts_assert_fail(
	const char *cond, const char *msg, const char *func, const char *file, int line, bool use_log
) {
	use_log = use_log && log_initialized();

	if(use_log) {
		if(msg) {
			_taisei_log(LOG_FAKEFATAL, func, file, line,
				"%s:%i: assertion `%s` failed: %s", file, line, cond, msg);
		} else {
			_taisei_log(LOG_FAKEFATAL, func, file, line,
				"%s:%i: assertion `%s` failed", file, line, cond);
		}

		log_sync(true);
	} else {
		if(msg) {
			tsfprintf(stderr,
				"%s:%i: %s(): assertion `%s` failed: %s\n", file, line, func, cond, msg);
		} else {
			tsfprintf(stderr,
				"%s:%i: %s(): assertion `%s` failed\n", file, line, func, cond);
		}

		fflush(stderr);
	}
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
void _emscripten_trap(void) {
	EM_ASM({
		throw new Error("You just activated my trap card!");
	});
}
#endif

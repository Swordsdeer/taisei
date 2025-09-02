/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

typedef struct Stats Stats;

struct Stats {
	struct {
		int lives_used;
		int bombs_used;
		int continues_used;
	} total, stage;
};

void stats_init(Stats *stats);

void stats_track_life_used(Stats *stats);
void stats_track_bomb_used(Stats *stats);
void stats_track_continue_used(Stats *stats);

void stats_stage_reset(Stats *stats);

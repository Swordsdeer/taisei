/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "stages/common_imports.h"  // IWYU pragma: export
#include "../scuttle.h"  // IWYU pragma: export
#include "../wriggle.h"  // IWYU pragma: export

DECLARE_EXTERN_TASK_WITH_INTERFACE(stage3_spell_firefly_storm, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage3_spell_light_singularity, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage3_spell_moonlight_rocket, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage3_spell_moths_to_a_flame, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage3_spell_logic_bomb, BossAttack);

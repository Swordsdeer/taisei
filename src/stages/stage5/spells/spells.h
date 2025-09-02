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
#include "../iku.h"  // IWYU pragma: export

DECLARE_EXTERN_TASK_WITH_INTERFACE(stage5_midboss_static_bomb, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage5_spell_atmospheric_discharge, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage5_spell_artificial_lightning, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage5_spell_induction, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage5_spell_natural_cathode, BossAttack);
DECLARE_EXTERN_TASK_WITH_INTERFACE(stage5_spell_overload, BossAttack);

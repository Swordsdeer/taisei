/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "spells.h"

static ProjPrototype *kepler_pick_bullet(int tier) {
	switch(tier) {
		case 0:  return pp_soul;
		case 1:  return pp_bigball;
		case 2:  return pp_ball;
		default: return pp_flea;
	}
}

DECLARE_TASK(kepler_bullet, { BoxedProjectile parent; int tier; cmplx offset; cmplx pos; });

TASK(kepler_bullet_spawner, { BoxedProjectile proj; int tier; cmplx offset; }) {
	TASK_BIND(ARGS.proj);
	int interval = 1.5*difficulty_value(30, 25, 15, 10);
	int max_children = difficulty_value(4, 4, 5, 5) / (ARGS.tier + 1);

	for(int i = 0; i < max_children; i++) {
		if(ARGS.tier < difficulty_value(3,4,4,4)) {
			INVOKE_TASK(kepler_bullet,
				.parent = ARGS.proj,
				.tier = ARGS.tier + 1,
				.offset = ARGS.offset,
			);
		}
		WAIT(interval);
	}
}

DEFINE_TASK(kepler_bullet) {
	cmplx pos = ARGS.pos;
	Projectile *parent = ENT_UNBOX(ARGS.parent);

	if(parent != NULL) {
		pos = parent->pos;
	}

	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = kepler_pick_bullet(ARGS.tier),
		.pos = pos + ARGS.offset,
		.color = RGB(0.3 + 0.3 * ARGS.tier, 0.6 - 0.3 * ARGS.tier, 1.0),

	));

	p->move.retention = 0;
	p->move.attraction = 2 * I;
	p->move.attraction_exponent = 0;
	p->move.velocity = I * cnormalize(ARGS.offset);

	INVOKE_SUBTASK(kepler_bullet_spawner, ENT_BOX(p), ARGS.tier, 20 * cnormalize(p->pos - pos));

	for(;;) {
		parent = ENT_UNBOX(ARGS.parent);
		if(parent != NULL) {
			p->move.attraction_point = parent->pos;
		} else {
			p->move.attraction = 0;
			p->move.retention = 1;
			break;
		}
		YIELD;
	}
}

TASK(kepler_scythe, { BoxedEllyScythe scythe; cmplx center; }) {
	EllyScythe *scythe = TASK_BIND(ARGS.scythe);
	scythe->spin = 0.7;
	scythe->move = move_from_towards(scythe->pos, ARGS.center + 100, 0.03);
	WAIT(60);
	scythe->move.attraction_point = ARGS.center;
	scythe->move.attraction = I;
	scythe->move.attraction_exponent = 0;
	scythe->move.retention = 0;
}

DEFINE_EXTERN_TASK(stage6_spell_kepler) {
	Boss *boss = stage6_elly_init_scythe_attack(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS.base);

	INVOKE_SUBTASK(kepler_scythe, ARGS.scythe, boss->pos);

	elly_clap(boss, 20);

	for(int t = 0;; t++) {
		int c = 2;
		play_sfx("shot_special1");
		for(int i = 0; i < c; i++) {
			cmplx dir = cdir(M_TAU / c * i + 0.6 * t);

			Projectile *p = PROJECTILE(
				.proto = pp_soul,
				.pos = boss->pos,
				.color = RGBA(0.3, 0.6, 1.0, 0.5),
				.move = move_linear(dir),
				.flags = PFLAG_MANUALANGLE,
				.angle = rng_angle(),
				.angle_delta = M_TAU/59,
			);

			INVOKE_TASK_DELAYED(20, kepler_bullet,
				.parent = ENT_BOX(p),
				.tier = 1,
				.offset = 10 * dir,
				.pos = p->pos,
			);
		}

		WAIT(20);
	}
}

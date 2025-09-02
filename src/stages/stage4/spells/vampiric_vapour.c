/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "spells.h"

static Projectile *vapor_particle(cmplx pos, const Color *clr) {
	return PARTICLE(
		.sprite = "stain",
		.color = clr,
		.timeout = 60,
		.draw_rule = pdraw_timeout_scalefade(0.2, 2.0, 0.6, 0),
		.pos = pos,
		.angle = rng_angle(),
	);
}

// XXX replace me by common_interpolate once merged
TASK(interpolate, { float *clr; float target; float step; }) {
	for(;;) {
		fapproach_p(ARGS.clr, ARGS.target, ARGS.step);
		YIELD;
	}
}

TASK(kurumi_vampvape_proj, { int delay; cmplx pos; cmplx vel; }) {
	Projectile *p = TASK_BIND(PROJECTILE(
		.proto = pp_thickrice,
		.pos = ARGS.pos,
		.color = RGBA(1.0, 0.5, 0.5, 0.0),
		.move = move_linear(ARGS.vel),
		.flags = PFLAG_NOSPAWNFLARE,
	));

	WAIT(ARGS.delay);

	p->color = *RGBA(0.3, 0.8, 0.8, 0.0);
	projectile_set_prototype(p, pp_bullet);

	cmplx vel = cdir(rng_sreal()*0.01) * (global.plr.pos - p->pos) * 0.001;
	p->move = move_linear(vel);

	real deflection = difficulty_value(0.004, 0.004, 0.005, 0.005);
	p->move.retention = 1.02 + deflection * sin(global.frames*314) * I;

	if(rng_chance(0.5)) {
		Projectile *v = vapor_particle(p->pos, color_mul_scalar(COLOR_COPY(&p->color), 0.3));

		if(rng_chance(0.5)) {
			v->flags |= PFLAG_REQUIREDPARTICLE;
		}
	}

	PARTICLE(
		.sprite = "flare",
		.color = RGB(1, 1, 1),
		.timeout = 30,
		.draw_rule = pdraw_timeout_scalefade(3.0, 0, 0.6, 0),
		.pos = p->pos,
	);

	play_sfx("shot3");

	INVOKE_SUBTASK(interpolate, &p->color.a, 1, 0.025);
	while(cabs(p->move.velocity) < 2) {
		YIELD;
	}
	p->move.retention = 1.0;
}

TASK(kurumi_vampvape_slave, { cmplx pos; cmplx target; int time_offset; }) {
	cmplx direction = cnormalize(ARGS.target - ARGS.pos);
	cmplx acceleration = 0.2 * direction;

	create_laser(ARGS.pos, 50, 100, RGBA(1.0, 0.3, 0.3, 0.0),
		laser_rule_accelerated(0, acceleration));

	int travel_time = sqrt(2 * cabs(ARGS.target - ARGS.pos) / cabs(acceleration));
	WAIT(travel_time);
	real step = difficulty_value(7,9,7,5);
	int step_count = VIEWPORT_H / step;

	if(re(direction) < 0) {
		return;
	}

	int count = difficulty_value(2, 3, 3, 3);
	float speed = difficulty_value(0.5, 0.7, 0.9, 0.95);

	RNG_ARRAY(rand, count);

	for(int i = ARGS.time_offset; i < ARGS.time_offset + step_count; i++, YIELD) {
		real y = step * (i + 0.5 - ARGS.time_offset);

		for(int j = 0; j < count; j++) {
			cmplx p = CMPLX(VIEWPORT_W * ((j + 0.5 + 0.4 * rng_sreal()) / (real)count), y);
			cmplx dir = cdir(M_TAU * sin(vrng_angle(rand[j]) + 245 * i + j * j * 3501));

			if(cabs(p - global.plr.pos) > 60) {
				INVOKE_TASK(kurumi_vampvape_proj, 160, p, speed * dir);

				if(rng_chance(0.5)) {
					RNG_NEXT(rand);
					vapor_particle(p, RGBA(0.5, 0.125 * vrng_real(rand[0]), 0.125 * vrng_real(rand[1]), 0.1));
				}
			}
		}
		play_sfx_ex("redirect", 3, false);
	}
}

DEFINE_EXTERN_TASK(kurumi_vampvape) {
	Boss *b = INIT_BOSS_ATTACK(&ARGS);
	BEGIN_BOSS_ATTACK(&ARGS);

	b->move = move_from_towards(b->pos, BOSS_DEFAULT_GO_POS, 0.04);

	for(int t = 0;; t++) {
		INVOKE_SUBTASK(common_charge, b->pos, RGBA(1, 0.3, 0.2, 0), 50, .sound = COMMON_CHARGE_SOUNDS);
		WAIT(50);
		play_sfx("laser1");
		INVOKE_SUBTASK(kurumi_vampvape_slave,
			       .pos = b->pos,
			       .target = 0,
			       .time_offset = t
		);
		INVOKE_SUBTASK(kurumi_vampvape_slave,
			       .pos = b->pos,
			       .target = VIEWPORT_W,
			       .time_offset = 1.23*t
		);
		WAIT(210);

		aniplayer_queue(&b->ani,"muda",4);
		aniplayer_queue(&b->ani,"main",0);
		WAIT(140);
	}
}

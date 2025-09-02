/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "rules.h"

#include "global.h"

static cmplx laser_rule_linear_impl(Laser *l, real t, void *ruledata) {
	LaserRuleLinearData *rd = ruledata;
	return l->pos + t * rd->velocity;
}

LaserRule laser_rule_linear(cmplx velocity) {
	LaserRuleLinearData rd = {
		.velocity = velocity,
	};
	return MAKE_LASER_RULE(laser_rule_linear_impl, rd);
}

IMPL_LASER_RULE_DATAGETTER(laser_get_ruledata_linear,
	laser_rule_linear_impl, LaserRuleLinearData)

static cmplx laser_rule_accelerated_impl(Laser *l, real t, void *ruledata) {
	LaserRuleAcceleratedData *rd = ruledata;
	return l->pos + t * (rd->velocity + t * rd->half_accel);
}

LaserRule laser_rule_accelerated(cmplx velocity, cmplx accel) {
	LaserRuleAcceleratedData rd = {
		.velocity = velocity,
		.half_accel = accel * 0.5,
	};
	return MAKE_LASER_RULE(laser_rule_accelerated_impl, rd);
}

IMPL_LASER_RULE_DATAGETTER(laser_get_ruledata_accelerated,
	laser_rule_accelerated_impl, LaserRuleAcceleratedData)

static cmplx laser_rule_sine_impl(Laser *l, real t, void *ruledata) {
	LaserRuleSineData *rd = ruledata;
	cmplx line_vel = rd->velocity;
	cmplx line_dir = line_vel / cabs(line_vel);
	cmplx line_normal = im(line_dir) - I*re(line_dir);
	cmplx sine_ofs = line_normal * rd->amplitude * sin(rd->frequency * t + rd->phase);
	return l->pos + t * line_vel + sine_ofs;
}

LaserRule laser_rule_sine(cmplx velocity, cmplx amplitude, real frequency, real phase) {
	LaserRuleSineData rd = {
		.velocity = velocity,
		.amplitude = amplitude,
		.frequency = frequency,
		.phase = phase,
	};
	return MAKE_LASER_RULE(laser_rule_sine_impl, rd);
}

IMPL_LASER_RULE_DATAGETTER(laser_get_ruledata_sine,
	laser_rule_sine_impl, LaserRuleSineData)

static cmplx laser_rule_sine_expanding_impl(Laser *l, real t, void *ruledata) {
	LaserRuleSineExpandingData *rd = ruledata;
	real angle = carg(rd->velocity);
	real speed = cabs(rd->velocity);
	real s = (rd->frequency * t + rd->phase);
	return l->pos + cdir(angle + rd->amplitude * sin(s)) * t * speed;
}

LaserRule laser_rule_sine_expanding(cmplx velocity, real amplitude, real frequency, real phase) {
	LaserRuleSineExpandingData rd = {
		.velocity = velocity,
		.amplitude = amplitude,
		.frequency = frequency,
		.phase = phase,
	};
	return MAKE_LASER_RULE(laser_rule_sine_expanding_impl, rd);
}

IMPL_LASER_RULE_DATAGETTER(laser_get_ruledata_sine_expanding,
	laser_rule_sine_expanding_impl, LaserRuleSineExpandingData)

static cmplx laser_rule_arc_impl(Laser *l, real t, void *ruledata) {
	LaserRuleArcData *rd = ruledata;
	return l->pos + rd->radius * cdir(rd->turn_speed * (t + rd->time_ofs));
}

LaserRule laser_rule_arc(cmplx radius, real turnspeed, real timeofs) {
	LaserRuleArcData rd = {
		.radius = radius,
		.turn_speed = turnspeed,
		.time_ofs = timeofs,
	};
	return MAKE_LASER_RULE(laser_rule_arc_impl, rd);
}

IMPL_LASER_RULE_DATAGETTER(laser_get_ruledata_arc,
	laser_rule_arc_impl, LaserRuleArcData)

static cmplx laser_rule_dynamic_impl(Laser *l, real t, void *ruledata) {
	if(t == EVENT_BIRTH) {
		return 0;
	}

	LaserRuleDynamicData *rd = ruledata;
	assert(cotask_unbox(rd->control_task));
	LaserRuleDynamicTaskData *td = rd->task_data;

	assert(td->history.num_elements > 0);

	real tbase = (global.frames - l->birthtime) * l->speed;
	real tofsraw = t - tbase;
	real tofs = clamp(tofsraw, 1 - td->history.num_elements, 0);

	int i0 = floor(tofs);
	int i1 = ceil(tofs);
	real ifract = (tofs - floor(tofs));

	cmplx v0 = *NOT_NULL(ringbuf_peek_ptr(&td->history, -i0));
	cmplx v1 = *NOT_NULL(ringbuf_peek_ptr(&td->history, -i1));
	return clerp(v0, v1, ifract);
}

static LaserRule laser_rule_dynamic(
	BoxedTask control_task, LaserRuleDynamicTaskData *task_data
) {
	LaserRuleDynamicData rd = {
		.control_task = control_task,
		.task_data = task_data,
	};
	return MAKE_LASER_RULE(laser_rule_dynamic_impl, rd);
}

IMPL_LASER_RULE_DATAGETTER(laser_get_ruledata_dynamic,
	laser_rule_dynamic_impl, LaserRuleDynamicData)

TASK(laser_dynamic, {
	Laser **out_laser;
	cmplx pos;
	float timespan;
	float deathtime;
	const Color *color;
	MoveParams **out_move;
}) {
	int histsize = ceilf(ARGS.timespan) + 2;
	assume(histsize > 2);

	cmplx *history_data = TASK_MALLOC(sizeof(*history_data) * histsize);
	LaserRuleDynamicTaskData td = {
		.history = RING_BUFFER_INIT(history_data, histsize)
	};

	auto l = TASK_BIND(create_laser(
		ARGS.pos, ARGS.timespan, ARGS.deathtime, ARGS.color,
		laser_rule_dynamic(THIS_TASK, &td)
	));

	if(LIKELY(ARGS.out_move)) {
		*ARGS.out_move = &td.move;
	}

	*ARGS.out_laser = l;
	ringbuf_push(&td.history, l->pos);  // HACK: this works around some edge cases but we can probably
	ringbuf_push(&td.history, l->pos);  //       make the sampling function more robust instead?
	ringbuf_push(&td.history, l->pos);
	YIELD;

	for(;global.frames - l->birthtime <= l->deathtime; YIELD) {
		move_update(&l->pos, &td.move);
		ringbuf_push(&td.history, l->pos);
	}

	STALL;
}

Laser *create_dynamic_laser(
	cmplx pos, float time, float deathtime, const Color *color, MoveParams **out_move
) {
	Laser *l;
	INVOKE_TASK(laser_dynamic, &l, pos, time, deathtime, color, out_move);
	return l;
}

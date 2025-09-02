/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "renderer/api.h"
#include "resource.h"
#include "sprite.h"

typedef enum {
	ALIGN_LEFT = 0, // must be 0
	ALIGN_CENTER,
	ALIGN_RIGHT,
} Alignment;

typedef ulong charcode_t;
typedef struct Font Font;

typedef struct FontMetrics {
	float ascent;
	float descent;
	float max_glyph_height;
	float lineskip;
	float scale;
} FontMetrics;

typedef struct GlyphMetrics {
	float bearing_x;
	float bearing_y;
	float width;
	float height;
	float advance;
	float lsb_delta;
	float rsb_delta;
} GlyphMetrics;

typedef struct TextBBox {
	struct {
		float min;
		float max;
	} x;

	struct {
		float min;
		float max;
	} y;
} TextBBox;

// FIXME: this is a quite crude low-level-ish hack, and probably should be replaced with some kind of markup system.
typedef void (*GlyphDrawCallback)(Font *font, charcode_t charcode, SpriteInstanceAttribs *spr_instance, void *userdata);

typedef struct TextParams {
	const char *font;
	Font *font_ptr;
	const char *shader;
	ShaderProgram *shader_ptr;
	struct {
		GlyphDrawCallback func;
		void *userdata;
	} glyph_callback;
	union {
		struct { float x, y; };
		cmplxf as_cmplx;
	} pos;
	const Color *color;
	const ShaderCustomParams *shader_params;
	Texture *aux_textures[R_NUM_SPRITE_AUX_TEXTURES];
	float max_width;
	FloatRect *overlay_projection;
	BlendMode blend;
	Alignment align;
} TextParams;

DEFINE_RESOURCE_GETTER(Font, res_font, RES_FONT)
DEFINE_OPTIONAL_RESOURCE_GETTER(Font, res_font_optional, RES_FONT)

ShaderProgram *text_get_default_shader(void)
	attr_returns_nonnull;

const FontMetrics *font_get_metrics(Font *font) attr_nonnull(1) attr_returns_nonnull;

float font_get_ascent(Font *font) attr_nonnull(1);
float font_get_descent(Font *font) attr_nonnull(1);
float font_get_lineskip(Font *font) attr_nonnull(1);

const GlyphMetrics *font_get_char_metrics(Font *font, charcode_t c) attr_nonnull(1);

float text_draw(const char *text, const TextParams *params) attr_nonnull(1, 2);
float text_ucs4_draw(const uint32_t *text, const TextParams *params) attr_nonnull(1, 2);

float text_draw_wrapped(const char *text, float max_width, const TextParams *params) attr_nonnull(1, 3);

void text_render(const char *text, Font *font, Sprite *out_sprite, TextBBox *out_bbox) attr_nonnull(1, 2, 3, 4);

void text_ucs4_shorten(Font *font, uint32_t *text, float width) attr_nonnull(1, 2);

void text_wrap(Font *font, const char *src, float width, char *buf, size_t bufsize) attr_nonnull(1, 2, 4);

void text_bbox(Font *font, const char *text, uint maxlines, TextBBox *bbox) attr_nonnull(1, 2, 4);
void text_ucs4_bbox(Font *font, const uint32_t *text, uint maxlines, TextBBox *bbox) attr_nonnull(1, 2, 4);

float text_width_raw(Font *font, const char *text, uint maxlines) attr_nonnull(1, 2);
float text_ucs4_width_raw(Font *font, const uint32_t *text, uint maxlines) attr_nonnull(1, 2);

float text_width(Font *font, const char *text, uint maxlines) attr_nonnull(1, 2);
float text_ucs4_width(Font *font, const uint32_t *text, uint maxlines) attr_nonnull(1, 2);

float text_height_raw(Font *font, const char *text, uint maxlines) attr_nonnull(1, 2);
float text_ucs4_height_raw(Font *font, const uint32_t *text, uint maxlines) attr_nonnull(1, 2);

float text_height(Font *font, const char *text, uint maxlines) attr_nonnull(1, 2);
float text_ucs4_height(Font *font, const uint32_t *text, uint maxlines) attr_nonnull(1, 2);

// FIXME: come up with a better, stateless API for this
bool font_get_kerning_available(Font *font) attr_nonnull(1);
bool font_get_kerning_enabled(Font *font) attr_nonnull(1);
void font_set_kerning_enabled(Font *font, bool newval) attr_nonnull(1);

extern ResourceHandler font_res_handler;

#define FONT_PATH_PREFIX "res/fonts/"
#define FONT_EXTENSION ".font"

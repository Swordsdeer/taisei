/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "../common/backend.h"

void gles_init(RendererBackend *gles_backend, int major, int minor);
void gles_init_context(SDL_Window *w);
bool gles_texture_dump(Texture *tex, uint mipmap, uint layer, Pixmap *dst);

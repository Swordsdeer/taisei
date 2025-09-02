/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "arena.h"

#include "util.h"
#include "util/miscmath.h"
#include "../util.h"

#define ARENA_MIN_ALLOC 4096

// NOTE: if we cared about 64-bit Linux only, mmap would be nice here…
// Unfortunately, some of the platforms we support *cough*emscripten*cough*
// don't even have virtual memory, so we can't have nice infinitely growable
// contiguous arenas.

static void *_arena_alloc_page(size_t s) {
	return mem_alloc(s);
}

static void _arena_dealloc_page(MemArenaPage *p) {
	mem_free(p);
}

static MemArenaPage *_arena_new_page(MemArena *arena, size_t min_size) {
	auto alloc_size = topow2_u64(min_size + sizeof(MemArenaPage));
	alloc_size = max(alloc_size, ARENA_MIN_ALLOC);
	auto page_size = alloc_size - sizeof(MemArenaPage);
	MemArenaPage *p = _arena_alloc_page(alloc_size);
	p->size = page_size;
	p->arena = arena;
	alist_append(&arena->pages, p);
	arena->page_offset = 0;
	arena->total_allocated += page_size;
	return p;
}

static void _arena_delete_page(MemArena *arena, MemArenaPage *page) {
	assume(page->arena == arena);
	_arena_dealloc_page(page);
}

INLINE MemArenaPage *_arena_active_page(MemArena *arena) {
	auto page = NOT_NULL(arena->pages.last);
	assume(page->arena == arena);
	assume(page->next == NULL);
	return page;
}

static void *_arena_alloc(MemArena *arena, size_t size, size_t align) {
	auto page = _arena_active_page(arena);
	auto page_ofs = arena->page_offset;
	size_t required, alignofs;

	for(;;) {
		auto available = page->size - page_ofs;
		alignofs = (align - (uintptr_t)(page->data + page_ofs)) & (align - 1);
		required = alignofs + size;

		if(available < required) {
			page = _arena_new_page(arena, required);
			assert(arena->page_offset == 0);
			page_ofs = 0;
			continue;
		}

		break;
	}

	void *p = page->data + page_ofs + alignofs;
	arena->total_used += required;
	arena->page_offset += required;
	assert(arena->page_offset <= page->size);
	assert(((uintptr_t)p & (align - 1)) == 0);
	return p;
}

static bool _arena_free(MemArena *arena, void *p, size_t old_size) {
	auto page = _arena_active_page(arena);

	if(page->data + arena->page_offset - old_size == p) {
		assert(arena->page_offset >= old_size);
		arena->page_offset -= old_size;
		arena->total_used -= old_size;
		return true;
	}

	return false;
}

static void *_arena_realloc(MemArena *arena, void *p, size_t old_size, size_t new_size, size_t align) {
	assert(((uintptr_t)p & (align - 1)) == 0);

	if(_arena_free(arena, p, old_size)) {
		void *new_p = _arena_alloc(arena, new_size, align);

		if(p != new_p) {
			// If free succeeded and old_size >= new_size, alloc will never make a new page
			assert(old_size < new_size);
			memcpy(new_p, p, old_size);
		}

		return new_p;
	}

	// Couldn't free: the allocation isn't at the tip of the page.
	// If we aren't growing it, just leave it alone.
	if(old_size >= new_size) {
		return p;
	}

	void *new_p = _arena_alloc(arena, new_size, align);
	memcpy(new_p, p, old_size);

	return new_p;
}

void marena_init(MemArena *arena, size_t min_size) {
	*arena = (MemArena) { };
	_arena_new_page(arena, min_size);
}

void marena_deinit(MemArena *arena) {
	MemArenaPage *p;
	while((p = alist_pop(&arena->pages))) {
		_arena_delete_page(arena, p);
	}
}

void marena_reset(MemArena *arena) {
	auto used = arena->total_used;
	arena->total_used = 0;
	arena->page_offset = 0;

	if(arena->pages.first->next) {
		MemArenaPage *p;
		while((p = alist_pop(&arena->pages))) {
			_arena_delete_page(arena, p);
		}

		arena->total_allocated = 0;
		p = _arena_new_page(arena, used);
		assert(p == arena->pages.last);
	}

	assert(arena->pages.first != NULL);
	assert(arena->pages.first == arena->pages.last);
}

void *marena_alloc(MemArena *arena, size_t size) {
	return _arena_alloc(arena, size, alignof(max_align_t));
}

void *marena_alloc_array(MemArena *arena, size_t num_members, size_t size) {
	return marena_alloc_array_aligned(arena, num_members, size, alignof(max_align_t));
}

void *marena_alloc_array_aligned(MemArena *arena, size_t num_members, size_t size, size_t align) {
	return _arena_alloc(arena, mem_util_calc_array_size(num_members, size), align);
}

void *marena_alloc_aligned(MemArena *arena, size_t size, size_t align) {
	assume(align > 0);
	assume(!(align & (align - 1)));

	if(align < alignof(max_align_t)) {
		align = alignof(max_align_t);
	}

	return _arena_alloc(arena, size, align);
}

bool marena_free(MemArena *restrict arena, void *restrict p, size_t old_size) {
	return _arena_free(arena, p, old_size);
}

void *marena_realloc(MemArena *restrict arena, void *restrict p, size_t old_size, size_t new_size) {
	return _arena_realloc(arena, p, old_size, new_size, alignof(max_align_t));
}

void *marena_realloc_aligned(MemArena *restrict arena, void *restrict p, size_t old_size, size_t new_size, size_t align) {
	assume(align > 0);
	assume(!(align & (align - 1)));

	if(align < alignof(max_align_t)) {
		align = alignof(max_align_t);
	}

	return _arena_realloc(arena, p, old_size, new_size, align);
}

MemArenaSnapshot marena_snapshot(MemArena *arena) {
	return (MemArenaSnapshot) {
		.page = _arena_active_page(arena),
		.page_offset = arena->page_offset,
	};
}

bool marena_rollback(MemArena *restrict arena, const MemArenaSnapshot *restrict snapshot) {
	auto active_page = _arena_active_page(arena);

	if(active_page == snapshot->page) {
		if(UNLIKELY(snapshot->page_offset > arena->page_offset)) {
			return false;
		}

		size_t mem_diff = arena->page_offset - snapshot->page_offset;
		arena->page_offset = snapshot->page_offset;
		assert(arena->total_used >= mem_diff);
		arena->total_used -= mem_diff;
		return true;
	}

	// New page(s) have been allocated after the snapshot was taken.
	// We won't try to undo that, but we can at least reset the active page.

	assert(arena->total_used >= arena->page_offset);
	arena->total_used -= arena->page_offset;
	arena->page_offset = 0;

	return false;
}

#pragma once

#include "types.h"

/* ALWAYS ZERO INITIALISE OTHERWISE PIXELS WILL BE NON-ZERO!!! */
typedef struct RenderBuffer_t {
	s32 width, height, size;
	u32 *pixels;
} RenderBuffer;

/* all RB_ functions assume rb is a valid RenderBuffer pointer */

/* allocate pixels(will NOT check if they already exist!) */
int RB_alloc_pixels(RenderBuffer *rb);
/* allocate pixels for width and height  (will NOT check if they already exist!) */
int RB_alloc_resize_pixels(RenderBuffer *rb, s32 width, s32 height);
/* free rb32->pixels (if it's non-zero) */
int RB_free_pixels(RenderBuffer *rb);
/* Make sure rb is ZERO-INITIALISED */
/* free rb32->pixels if its non-zero, then allocate pixels for the new dimensions (this will clear the buffer too) */
int RB_resize(RenderBuffer *rb, s32 new_width, s32 new_height);
/* clear the screen */
void RB_clear(RenderBuffer *rb);
int RB_clone(const RenderBuffer *from, RenderBuffer *to);
int RB_rescale(RenderBuffer *rb, s32 x_scale, s32 y_scale);
int RB_rescale_clone(const RenderBuffer *rb, s32 x_scale, s32 y_scale, RenderBuffer *dest);

boolean RB_in_bounds(const RenderBuffer *rb, s32 x, s32 y);
u32 RB_sample(const RenderBuffer *rb, float x, float y);
u32 RB_sample_abs(const RenderBuffer *rb, float x, float y);

void RB_draw_pixel(RenderBuffer *rb, s32 x, s32 y, u32 colour);
void RB_draw_rect(RenderBuffer *rb, s32 x0, s32 y0, s32 x1, s32 y1, u32 colour);
void RB_draw_vertical(RenderBuffer *rb, s32 x, s32 y0, s32 y1, u32 colour);
void RB_draw_horizontal(RenderBuffer *rb, s32 x0, s32 x1, s32 y, u32 colour);
void RB_draw_line(RenderBuffer *rb, s32 x0, s32 y0, s32 x1, s32 y1, u32 colour);
void RB_draw_triangle(RenderBuffer *rb, s32 x0, s32 y0, s32 x1, s32 y1, s32 x2, s32 y2, u32 colour);
/* Will draw to the rectangle defined by (prb, width, height), using the same sized rectangle based at ps in source for colour,
	with source having a width of stride. It is assumed that the rectangle (ps, width, height) is fully contained in source. */
void RB_draw_buffer(RenderBuffer *rb, s32 xrb, s32 yrb, s32 xs, s32 ys, s32 width, s32 height, const u32 *source, s32 stride);
void RB_draw_renderbuffer(RenderBuffer *rb, s32 xrb, s32 yrb, const RenderBuffer *source);
void RB_draw_renderbuffer_sub(RenderBuffer *rb, s32 xrb, s32 yrb, s32 xs, s32 ys, s32 width, s32 height, const RenderBuffer *source);

void RB_draw_renderbuffer_sample(RenderBuffer *rb, s32 xrb, s32 yrb, s32 dest_width, s32 dest_height, const RenderBuffer *source);
void RB_draw_renderbuffer_sample_sub(RenderBuffer *rb, s32 xrb, s32 yrb, s32 dest_width, s32 dest_height,
	float xs, float ys, float src_width, float src_height, const RenderBuffer *source);

/* Make sure rb is ZERO-INITIALISED */
int RB_load_image_png(RenderBuffer *rb, const char *filename);
int RB_load_image_bmp(RenderBuffer *rb, const char *filename);

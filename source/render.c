#include "render.h"

#include "win32_tools.h"
#include "maths.h"
#include "assert_opt.h"
#include "memory_opt.h"

#include "lodepng.h"

#include <stdio.h>
#include <stdlib.h>

/* allocate pixels (will NOT check if they already exist!) */
int RB_alloc_pixels(RenderBuffer *rb) {
	assert(rb && "RB_alloc_pixels: rb == 0");
	rb->size = rb->width * rb->height;
	if (rb->size == 0) {
		rb->pixels = 0;
		return 0;
	}
	if ((rb->pixels = VAlloc(sizeof(u32) * rb->size)))
		return 0;
	return ERROR_RETURN;
}
/* allocate pixels for width and height  (will NOT check if they already exist!) */
int RB_alloc_resize_pixels(RenderBuffer *rb, s32 width, s32 height) {
	assert(rb && "RB_alloc_resize_pixels: rb == 0");
	rb->width = width;
	rb->height = height;
	return RB_alloc_pixels(rb);
}
/* free rb->pixels (if it's non-zero) */
int RB_free_pixels(RenderBuffer *rb) {
	assert(rb && "RB_free_pixels: rb == 0");
	if (rb->pixels)
		return !VFree(rb->pixels);
	return 0;
}
/* free rb->pixels if its non-zero, then allocate pixels for the new dimensions (this will clear the buffer too) */
int RB_resize(RenderBuffer *rb, s32 new_width, s32 new_height) {
	assert(rb && "RB_resize: rb == 0");
	if (rb->pixels && rb->size == new_width * new_height) {
		rb->width = new_width;
		rb->height = new_height;
		return 0;
	}
	if (RB_free_pixels(rb))
		return ERROR_RETURN; /* could not free pixels */
	return RB_alloc_resize_pixels(rb, new_width, new_height);
}
/* clear the screen */
void RB_clear(RenderBuffer *rb) {
	assert(rb && "RB_clear: rb == 0");
	memset(rb->pixels, 0, sizeof(u32) * rb->size);
}
int RB_clone(const RenderBuffer *from, RenderBuffer *to) {
	assert(from && "RB_clone: from == 0");
	assert(to && "RB_clone: to == 0");
	RB_free_pixels(to);
	int err = RB_resize(to, from->width, from->height);
	if (to->pixels && !err) memcpy(to->pixels, from->pixels, to->size * sizeof(u32));
	return err;
}
int RB_rescale(RenderBuffer *rb, s32 x_scale, s32 y_scale) {
	assert(rb && "RB_rescale: rb == 0");
	/* Create a temporary renderbuffer */
	RenderBuffer rb_scaled = { 0 };
	int err = RB_rescale_clone(rb, x_scale, y_scale, &rb_scaled);
	if (err) return err;
	/* Free original renderbuffer and put the new dims and pointer in it */
	err = RB_free_pixels(rb);
	if (err) return err;
	*rb = rb_scaled;
	return 0;
}
int RB_rescale_clone(const RenderBuffer *rb, s32 x_scale, s32 y_scale, RenderBuffer *dest) {
	assert(rb && "RB_rescale_clone: rb == 0");
	assert(dest && "RB_rescale_clone: dest == 0");
	assert(x_scale > 0 && y_scale > 0 && "Cannot rescale by factor <= 0!");
	/* Resize the renderbuffer with scaled dimensions */
	int err = RB_resize(dest, rb->width * x_scale, rb->height * y_scale);
	if (err) return err;

	/* Generate the scaled buffer */
	const u32 *pix_src = rb->pixels;
	u32 *pix_dest = dest->pixels;
	for (s32 y = 0; y < rb->height; ++y) {
		for (s32 x = 0; x < rb->width; ++x) { /* pix_src and pix_dest at the beginning of the row */
			for (s32 i = 0; i < x_scale; ++i) {
				*pix_dest++ = *pix_src;
			}
			pix_src++;
		} /* pix_src and pix_dest at the beginning of the next row */
		/* we now don't change pix_src until the next loop */
		const u32 *temp_src = pix_dest - dest->width;
		for (s32 j = 1; j < y_scale; ++j) {
			memcpy(pix_dest, temp_src, dest->width * sizeof(u32));
			pix_dest += dest->width;
		}
	}
	return 0;
}

boolean RB_in_bounds(const RenderBuffer *rb, s32 x, s32 y) {
	assert(rb && "RB_in_bounds: rb == 0");
	return 0 <= x && 0 <= y && x < rb->width && y < rb->height;
}
u32 RB_sample(const RenderBuffer *rb, float x, float y) {
	assert(rb && "RB_sample: rb == 0");
	x = truncate_int_part(x);
	y = truncate_int_part(y); /* now x, y in [0,1) */
	s32 xx = (s32)(x * (float)rb->width);
	s32 yy = (s32)(y * (float)rb->height);
	assert(RB_in_bounds(rb, xx, yy) && "Sampling point outside of renderbuffer bounds!");
	return rb->pixels[xx + yy * rb->width];
}
u32 RB_sample_abs(const RenderBuffer *rb, float x, float y) {
	assert(rb && "RB_sample: rb == 0");
	s32 xx = (s32)x;
	s32 yy = (s32)y;
	assert(RB_in_bounds(rb, xx, yy) && "Sampling point outside of renderbuffer bounds!");
	return rb->pixels[xx + yy * rb->width];
}

void RB_draw_pixel(RenderBuffer *rb, s32 x, s32 y, u32 colour) {
	assert(rb && "RB_draw_pixel: rb == 0");
	if (x >= 0 && y >= 0 && x < rb->width && y < rb->height)
		rb->pixels[x + y * rb->width] = colour;
}
void RB_draw_rect(RenderBuffer *rb, s32 x0, s32 y0, s32 x1, s32 y1, const u32 colour) {
	assert(rb && "RB_draw_rect: rb == 0");
	if (x1 < x0) SWAP(x0, x1); /* now we know x0 <= x1 */
	if (x1 < 0 || x0 >= rb->width) return;
	if (y1 < y0) SWAP(y0, y1); /* now we know y0 <= y1 */
	if (y1 < 0 || y0 >= rb->height) return;
	if (x0 < 0) x0 = 0;
	if (x1 >= rb->width) x1 = rb->width - 1;
	if (y0 < 0) y0 = 0;
	if (y1 >= rb->height) y1 = rb->height - 1;

	const s32 d = rb->width + x0 - x1;
	u32 *pix = rb->pixels + (x0 + y0 * rb->width);

	for (s32 y = y0; y < y1; ++y) {
		for (s32 x = x0; x < x1; ++x)
			*pix++ = colour;
		pix += d;
	}
}
void RB_draw_vertical(RenderBuffer *rb, s32 x, s32 y0, s32 y1, u32 colour) {
	assert(rb && "RB_draw_vertical: rb == 0");
	if (x < 0 || x >= rb->width) return;
	if (y1 < y0) SWAP(y0, y1); // ensure y0 <= y1
	if (y1 < 0 || y0 >= rb->height) return;
	if (y0 < 0) y0 = 0;
	if (y1 >= rb->height) y1 = rb->height - 1;
	u32 *pix = rb->pixels + (x + y0 * rb->width);
	for (; y0 <= y1; ++y0) {
		*pix = colour;
		pix += rb->width;
	}
}
void RB_draw_horizontal(RenderBuffer *rb, s32 x0, s32 x1, s32 y, u32 colour) {
	assert(rb && "RB_draw_horizontal: rb == 0");
	if (y < 0 || y >= rb->height) return;
	if (x1 < x0) SWAP(x0, x1); // ensure x0 <= x1
	if (x1 < 0 || x0 >= rb->width) return;
	if (x0 < 0) x0 = 0;
	if (x1 >= rb->width) x1 = rb->width - 1;
	u32 *pix = rb->pixels + (x0 + y * rb->width);
	for (; x0 <= x1; ++x0)
		*pix++ = colour;
}
void RB_draw_line(RenderBuffer *rb, s32 x0, s32 y0, s32 x1, s32 y1, u32 colour) {
	assert(rb && "RB_draw_line: rb == 0");
	const s32 dx = abs(x1 - x0);
	const s32 dy = -abs(y1 - y0);
	if (dx == 0) {
		if (dy == 0)
			RB_draw_pixel(rb, x0, y0, colour);
		else
			RB_draw_vertical(rb, x0, y0, y1, colour);
		return;
	} else if (dy == 0) {
		RB_draw_horizontal(rb, x0, x1, y0, colour);
		return;
	}
	const s32 sx = x0 < x1 ? 1 : -1;
	const s32 sy = y0 < y1 ? 1 : -1;
	const s32 dx2 = dx * 2;
	const s32 dy2 = dy * 2;
	s32 err = dx2 + dy2; /* error value e_xy */

	while (true) {
		if (sx == -1 && x0 < 0) break;
		if (sx == 1 && x0 >= rb->width) break;
		if (sy == -1 && y0 < 0) break;
		if (sy == 1 && y0 >= rb->height) break;

		RB_draw_pixel(rb, x0, y0, colour);

		if (x0 == x1 && y0 == y1) break;
		/* e_xy+e_x > 0 */
		if (err >= dy) {
			err += dy2;
			x0 += sx;
		}
		/* e_xy+e_y < 0 */
		if (err <= dx) {
			err += dx2;
			y0 += sy;
		}
	}
}
/* Writes x coordinates of line points into target (with a stride of 2,
	not increasing if the y coordinate doesn't change).
	Returns number of points written to target (so target+(ret*2)
	is the next empty address). Parameter left indicates whether
	to use the left or right pixel in the case of a horizontal line.
	The input is guarunteed to satisfy: y0 >= y1 */
s32 triangle_side(s32 *target, s32 x0, s32 y0, s32 x1, s32 y1, boolean left) {
	const s32 dx = abs(x1 - x0);
	const s32 dy = y1 - y0;
	if (dx == 0) {
		if (dy == 0) {
			//RB_draw_pixel(rb, x0, y0, colour);
			*target = x0;
			return 1;
		} else {
			//RB_draw_vertical(rb, x0, y0, y1, colour);
			const s32 len = y0 - y1 + 1;
			for (s32 i = 0; i<len; ++i) {
				*target = x0;
				target += 2;
			}
			return len;
		}
	} else if (dy == 0) {
		//RB_draw_horizontal(rb, x0, x1, y0, colour);
		*target = ((x0 <= x1) ^ left) ? x1 : x0;
		return 1;
	}
	const s32 sx = x0 < x1 ? 1 : -1;
	const s32 dx2 = dx * 2;
	const s32 dy2 = dy * 2;
	s32 err = dx2 + dy2; /* error value e_xy */

	while (true) {
		//RB_draw_pixel(rb, x0, y0, colour);
		*target = x0;

		if (x0 == x1 && y0 == y1) break;
		/* e_xy+e_x > 0 */
		if (err >= dy) {
			err += dy2;
			x0 += sx;
		}
		/* e_xy+e_y < 0 */
		if (err <= dx) {
			err += dx2;
			y0--;
			target += 2;
		}
	}
	return 1 - dy;
}
/* Returns >0 if to the left, <0 if to the right, and ==0 if parallel. */
s32 side(s32 x0, s32 y0, s32 x1, s32 y1, s32 px, s32 py) {
	return (px - x0) * (y1 - y0) - (py - y0) * (x1 - x0);
}
void RB_draw_triangle(RenderBuffer *rb, s32 x0, s32 y0, s32 x1, s32 y1, s32 x2, s32 y2, u32 colour) {
	assert(rb && "RB_draw_triangle: rb == 0");
	/* sort the points such that y0 >= y1 >= y2 */
	if (y0 < y1) {
		if (y1 < y2) {
			SWAP(y0, y2);
			SWAP(x0, x2);
		} else {
			SWAP(y0, y1);
			SWAP(x0, x1);
			if (y1 < y2) {
				SWAP(y1, y2);
				SWAP(x1, x2);
			}
		}
	} else if (y1 < y2) {
		SWAP(y1, y2);
		SWAP(x1, x2);
		if (y0 < y1) {
			SWAP(y0, y1);
			SWAP(x0, x1);
		}
	}

	const s32 tri_height = y0 - y2 + 1;

	if (tri_height == 1) {
		s32 minX = x0, maxX = x0;
		if (x1 < minX) minX = x1;
		else if (maxX < x1) maxX = x1;
		if (x2 < minX) minX = x2;
		else if (maxX < x2) maxX = x2;
		RB_draw_horizontal(rb, minX, maxX, y0, colour);
		return;
	} /* Now we know y0 > y2, hence it will be an entire side. */

	/* Returns 1 if to the left, -1 if to the right, and 0 if parallel. */
	const s32 mid_point_side = side(x0, y0, x2, y2, x1, y1);
	if (mid_point_side == 0) {
		RB_draw_line(rb, x0, y0, x2, y2, colour);
		return;
	}

	s32* sides = malloc_s(tri_height * 2 * sizeof(s32));
	if (mid_point_side > 0) { /* mid point p1 is to the left of p0->p2 */
		s32 len = triangle_side(sides + 1, x0, y0, x2, y2, false);
		assert(len == tri_height && "Triangle side points doesn't match height!");
		len = triangle_side(sides, x0, y0, x1, y1, true) - 1;
		len += triangle_side(sides + (len * 2), x1, y1, x2, y2, true);
		assert(len == tri_height && "Triangle side points doesn't match height!");
	} else { /* mid point p1 is to the right of p0->p2 */
		s32 len = triangle_side(sides, x0, y0, x2, y2, true);
		assert(len == tri_height && "Triangle side points doesn't match height!");
		len = triangle_side(sides + 1, x0, y0, x1, y1, false) - 1;
		len += triangle_side(sides + (len * 2 + 1), x1, y1, x2, y2, false);
		assert(len == tri_height && "Triangle side points doesn't match height!");
	}

	/* Draw the scanlines */
	for (s32 i = 0; i < tri_height; ++i)
		RB_draw_horizontal(rb, sides[i * 2], sides[i * 2 + 1], y0 - i, colour);
	
	free_s(sides);
}

/* Will draw to the rectangle defined by (prb, width, height), using the same sized rectangle based at ps in source for colour,
	with source having a width of stride. It is assumed that the rectangle (ps, width, height) is fully contained in source. */
void RB_draw_buffer(RenderBuffer *rb, s32 xrb, s32 yrb, s32 xs, s32 ys, s32 width, s32 height, const u32 *source, s32 stride) {
	assert(rb && "RB_draw_buffer: rb == 0");
	assert(source && "RB_draw_buffer: source == 0");
	if (xrb + width <= 0 || xrb >= rb->width || yrb + height <= 0 || yrb >= rb->height || width <= 0 || height <= 0)
		return;
	/* Now we know: xrb+width > 0, xrb < rb->width, yrb+height > 0, yrb < rb->height, width > 0, height > 0 */
	if (xrb < 0) {
		width += xrb; /* xrb+width > 0 --> now: width > 0 */
		xs -= xrb; /* xs = xs + |xrb| (so still +ve) */
		xrb = 0;
	} /* Now xrb >= 0 */
	if (xrb + width >= rb->width)
		width = rb->width - xrb; /* xrb < rb->width --> now: width = rb->width - xrb > 0*/
	if (yrb < 0) {
		height += yrb; /* yrb+height > 0 --> now: height > 0 */
		ys -= yrb; /* ys = ys + |yrb| (so still +ve) */
		yrb = 0;
	} /* Now yrb >= 0 */
	if (yrb + height >= rb->height)
		height = rb->height - yrb; /* yrb < rb->height --> now: height = rb->height - yrb > 0*/

	u32 *pixrb = rb->pixels + (xrb + yrb * rb->width);
	const u32 *pixs = source + (xs + ys * stride);
	for (s32 y = 0; y < height; ++y) {
		memcpy(pixrb, pixs, width * sizeof(u32));
		pixrb += rb->width;
		pixs += stride;
	}
}
void RB_draw_renderbuffer(RenderBuffer *rb, s32 xrb, s32 yrb, const RenderBuffer *source) {
	assert(rb && "RB_draw_renderbuffer: rb == 0");
	assert(source && "RB_draw_renderbuffer: source == 0");
	RB_draw_buffer(rb, xrb, yrb, 0, 0, source->width, source->height, source->pixels, source->width);
}
void RB_draw_renderbuffer_sub(RenderBuffer *rb, s32 xrb, s32 yrb, s32 xs, s32 ys, s32 width, s32 height, const RenderBuffer *source) {
	assert(rb && "RB_draw_renderbuffer_sub: rb == 0");
	assert(source && "RB_draw_renderbuffer_sub: source == 0");
	RB_draw_buffer(rb, xrb, yrb, xs, ys, width, height, source->pixels, source->width);
}

void RB_draw_renderbuffer_sample(RenderBuffer *rb, s32 xrb, s32 yrb, s32 dest_width, s32 dest_height, const RenderBuffer *source) {
	assert(rb && "RB_draw_renderbuffer_sample: rb == 0");
	assert(source && "RB_draw_renderbuffer_sample: source == 0");
	RB_draw_renderbuffer_sample_sub(rb, xrb, yrb, dest_width, dest_height, 0.0f, 0.0f, 1.0f, 1.0f, source);
}
void RB_draw_renderbuffer_sample_sub(RenderBuffer *rb, s32 xrb, s32 yrb, s32 dest_width, s32 dest_height,
	float xs, float ys, float src_width, float src_height, const RenderBuffer *source) {
	assert(rb && "RB_draw_renderbuffer_sample_sub: rb == 0");
	assert(source && "RB_draw_renderbuffer_sample_sub: source == 0");
	if (xrb + dest_width <= 0 || xrb >= rb->width || yrb + dest_height <= 0 || yrb >= rb->height || dest_width <= 0 || dest_height <= 0)
		return;
	/* Now we know: xrb+width > 0, xrb < rb->width, yrb+height > 0, yrb < rb->height, width > 0, height > 0 */

	if (xrb < 0) {
		const float delta_src = src_width * (float)xrb / (float)dest_width; /* xrb in src coorindates (-ve !!!) */
		xs -= delta_src;
		src_width += delta_src;
		dest_width += xrb; /* xrb+width > 0 --> now: width > 0 */
		xrb = 0;
	} /* Now xrb >= 0 */
	if (xrb + dest_width >= rb->width) {
		const s32 new_width = rb->width - xrb;
		src_width *= (float)new_width / (float)dest_width;
		dest_width = new_width; /* xrb < rb->width --> now: width = rb->width - xrb > 0*/
	}
	if (yrb < 0) {
		const float delta_src = src_height * (float)yrb / (float)dest_height; /* yrb in src coorindates (-ve !!!) */
		ys -= delta_src;
		src_height += delta_src;
		dest_height += yrb; /* yrb+height > 0 --> now: height > 0 */
		yrb = 0;
	} /* Now yrb >= 0 */
	if (yrb + dest_height >= rb->height) {
		const s32 new_height = rb->height - yrb;
		src_height *= (float)new_height / (float)dest_height;
		dest_height = new_height; /* yrb < rb->height --> now: height = rb->height - yrb > 0*/
	}
	u32 *pix_dest = rb->pixels + (xrb + yrb * rb->width);
	const u32 delta_dest = rb->width - dest_width;

	const float x_per_pix = src_width * (float)source->width / (float)dest_width;
	const float y_per_pix = src_height * (float)source->height / (float)dest_height;

	const float x_src_abs = xs * (float)source->width;
	float x_src = x_src_abs;
	float y_src = ys * (float)source->height;

	s32 curr_source_y_pixel = -1;
	for (s32 y = 0; y < dest_height; ++y) {
		if (curr_source_y_pixel < (s32)y_src) {
			curr_source_y_pixel = (s32)y_src;
			u32 col = 0xFF0000;
			s32 curr_source_x_pixel = -1;
			for (s32 x = 0; x < dest_width; ++x) {
				if (curr_source_x_pixel < (s32)x_src) {
					curr_source_x_pixel = (s32)x_src;
					col = RB_sample_abs(source, x_src, y_src);
				}
				*pix_dest++ = col;
				x_src += x_per_pix;
			}
			x_src = x_src_abs;
		} else {
			memcpy(pix_dest, pix_dest - rb->width, sizeof(u32) * dest_width);
			pix_dest += dest_width;
		}
		y_src += y_per_pix;
		pix_dest += delta_dest;
	}
}

int RB_load_image_png(RenderBuffer *rb, const char *filename) {
	assert(rb && "RB_load_image_png: rb == 0");
	assert(filename && "RB_load_image_png: filename == 0");
	unsigned char *image = 0;
	unsigned width, height;
	/* image data will be RGBA */
	int error = lodepng_decode32_file(&image, &width, &height, filename);
	if (error) {
		fprintf(stdout, "[RB_load_image_png] error %u: %s\n", error, lodepng_error_text(error));
		return ERROR_RETURN;
	}

	/* Copy image data to renderbuffer */
	error = RB_resize(rb, width, height);
	if (error) return ERROR_RETURN;
	u32 *pix = rb->pixels + rb->size - rb->width;
	const unsigned char *im = image;
	/* have to flip along y axis! */
	for (int y = 0; y < rb->height; ++y) {
		for (int x = 0; x < rb->width; ++x) {
			u32 r = *im++;
			u32 g = *im++;
			u32 b = *im++;
			u32 a = *im++;
			*pix++ = (a << 24) | (r << 16) | (g << 8) | b;
		}
		pix -= rb->width * 2;
	}

	/* NOT free_s as this was malloc'd by lodepng */
	free(image);

	return 0;
}
int RB_load_image_bmp(RenderBuffer *rb, const char *filename) {
	assert(rb && "RB_load_image_bmp: rb == 0");
	assert(filename && "RB_load_image_bmp: filename == 0");
	unsigned char *filedata = 0;
	size_t filesize = 0;
	{
		FILE *file = 0;
		if (fopen_s(&file, filename, "rb")) {
			fprintf(stdout, "[RB_load_image_bmp] failed to load %s\n", filename);
			return ERROR_RETURN;
		}
		fseek(file, 0, SEEK_END);
		filesize = ftell(file);
		if (filesize == 0) {
			fprintf(stdout, "[RB_load_image_bmp] file empty\n");
			return ERROR_RETURN;
		}
		fseek(file, 0, SEEK_SET);

		filedata = malloc_s(filesize);
		assert(filedata && "RB_load_image_bmp: malloc failed");
		const size_t size_read = fread(filedata, 1, filesize, file);
		assert(size_read == filesize && "RB_load_image_bmp: bytes read don't match file size");
		fclose(file);
	}
	static const size_t MINHEADER = 54; /* minimum BMP header size */
	if (filesize < MINHEADER) {
		fprintf(stdout, "[RB_load_image_bmp] header too small (%zu)\n", filesize);
		return ERROR_RETURN;
	}
	if (filedata[0] != 'B' || filedata[1] != 'M') {
		fprintf(stdout, "[RB_load_image_bmp] missing BM header (%c%c)\n", filedata[0], filedata[1]);
		return ERROR_RETURN;
	}
	const int pixeloffset = filedata[10] + 256 * filedata[11]; /* where the pixel data starts */
	const int width = filedata[18] + filedata[19] * 256; /* read width and height from BMP header */
	const int height = filedata[22] + filedata[23] * 256;
	/* read number of channels from BMP header */
	if (filedata[28] != 24 && filedata[28] != 32) {
		fprintf(stdout, "[RB_load_image_bmp] unsupported bit-depth (%d)\n", filedata[28]);
		return ERROR_RETURN;
	}
	const int numChannels = filedata[28] / 8;

	/* The amount of scanline bytes is width of image times channels, with extra bytes added if needed
		to make it a multiple of 4 bytes. */
	int scanlineBytes = width * numChannels;
	if (scanlineBytes & 3) scanlineBytes = (scanlineBytes & ~3) + 4;

	const int datasize = scanlineBytes * height;
	if (filesize < datasize + pixeloffset) {
		fprintf(stdout, "[RB_load_image_bmp] file too small to fit image (%zu < %zu)\n", filesize, (size_t)(datasize + pixeloffset));
		return ERROR_RETURN;
	}

	int err = RB_resize(rb, width, height);
	if (err) return ERROR_RETURN;
	const unsigned char *im = &filedata[pixeloffset];
	for (int y = 0; y < rb->height; ++y)
		for (int x = 0; x < rb->width; ++x) {
			const int bmpos = (rb->height - y - 1) * scanlineBytes + numChannels * x;
			u32 r, g, b, a;
			if (numChannels == 3) {
				r = im[bmpos + 2];
				g = im[bmpos + 1];
				b = im[bmpos + 0];
				a = 255;
			} else {
				r = im[bmpos + 3];
				g = im[bmpos + 2];
				b = im[bmpos + 1];
				a = im[bmpos + 0];
			}
			rb->pixels[y * width + x] = (a << 24) | (r << 16) | (g << 8) | b;
		}
	free_s(filedata);
	return 0;
}

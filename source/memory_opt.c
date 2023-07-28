#include "memory_opt.h"

//#include "maths.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>

static int malloc_count = 0;
static int realloc_count = 0;
static int free_count = 0;

void *malloc_s(size_t size) {
	if (size == 0) {
		fprintf(stdout, "[malloc_s] size 0 request\n");
		return 0;
	}
	void *ret = malloc(size);
	assert(ret && "[malloc_s] malloc failed");
	malloc_count++;
	return ret;
}
void *calloc_s(size_t size) {
	if (size == 0) {
		fprintf(stdout, "[calloc_s] size 0 request\n");
		return 0;
	}
	void *ret = calloc(size, 1);
	assert(ret && "[calloc_s] calloc failed");
	malloc_count++;
	return ret;
}
void *realloc_s(void *ptr, size_t size) {
	if (size == 0) {
		if (ptr) {
			free(ptr);
			free_count++;
		}
		fprintf(stdout, "[realloc_s] size 0 request\n");
		return 0;
	}
	void *ret = realloc(ptr, size);
	assert(ret && "[realloc_s] realloc failed");
	if (ptr) realloc_count++;
	else {
		//fprintf(stdout, "[realloc_s] ptr == 0\n");
		malloc_count++;
	}
	return ret;
}
void free_s(void *ptr) {
	if (ptr) {
		free(ptr);
		free_count++;
	}
}

void check_memory_leaks(void) {
	fprintf(stdout, "[check_memory_leaks] malloc_count = %d, realloc_count = %d, free_count = %d\n", malloc_count, realloc_count, free_count);
	fprintf(stdout, "[check_memory_leaks] malloc_count - free_count = %d\n", malloc_count - free_count);
}


void *buf__grow(const void *buf, size_t new_len, size_t elem_size) {
	assert(buf_cap(buf) <= (SIZE_MAX - 1) / 2 && "[buf__grow] capacity will overflow");
	size_t new_cap = MAX(2 * buf_cap(buf), MAX(new_len, 16));
	assert(new_len <= new_cap && "[buf__grow] capacity will be less than length");
	assert(new_cap <= (SIZE_MAX - offsetof(buffer_t, buf)) / elem_size && "[buf__grow] capacity will overflow");
	size_t new_size = offsetof(buffer_t, buf) + new_cap * elem_size;
	buffer_t *new_buf;
	if (buf) new_buf = realloc_s(buf__hdr(buf), new_size);
	else {
		new_buf = malloc_s(new_size);
		new_buf->len = 0;
	}
	new_buf->cap = new_cap;
	return new_buf->buf;
}
char *buf__printf(char *buf, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	size_t cap = buf_cap(buf) - buf_len(buf);
	size_t n = 1 + vsnprintf(buf_end(buf), cap, fmt, args);
	va_end(args);
	if (n > cap) {
		buf_fit(buf, n + buf_len(buf));
		va_start(args, fmt);
		size_t new_cap = buf_cap(buf) - buf_len(buf);
		n = 1 + vsnprintf(buf_end(buf), new_cap, fmt, args);
		assert(n <= new_cap);
		va_end(args);
	}
	buf__hdr(buf)->len += n - 1;
	return buf;
}

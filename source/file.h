#pragma once

#include "string_wrapper.h"

#include <stdio.h>

#define FILE_BUFFER_SIZE 256

typedef struct {
	FILE* fp;
	size_t pos;
	char buffer[FILE_BUFFER_SIZE + 1];
} file_t;

int file_open(file_t *file, const char *filename);
void file_close(file_t *file);
boolean file_fill_buffer(file_t *file);
void file_skip_whitespace(file_t *file);
boolean file_read_line(file_t *file, string *line);

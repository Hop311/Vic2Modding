#include "file.h"

#include "types.h"
#include "assert_opt.h"

#include <string.h>

int file_open(file_t *file, const char *filename) {
	assert(file && "file_open: file == 0");
	assert(filename && "file_open: filename == 0");
	int err = fopen_s(&file->fp, filename, "r");
	if (err) return err;
	assert(file->fp && "file_open: fopen_s failed");
	file->pos = FILE_BUFFER_SIZE;
	return 0;
}
void file_close(file_t *file) {
	assert(file && "file_close: file == 0");
	if (file->fp) {
		fclose(file->fp);
		file->fp = 0;
	}
}
boolean file_fill_buffer(file_t *file) {
	assert(file && "file_fill_buffer: file == 0");
	assert(file->fp && "file_fill_buffer: file->fp == 0");
	file->buffer[FILE_BUFFER_SIZE] = '\0';
	const size_t size_read = fread(file->buffer, sizeof(char), FILE_BUFFER_SIZE, file->fp);
	if (size_read < FILE_BUFFER_SIZE) {
		if (size_read == 0)
			return false;
		memset(&file->buffer[size_read], 0, FILE_BUFFER_SIZE - size_read);
	}
	file->pos = 0;
	return true;
}
void file_skip_whitespace(file_t *file) {
	assert(file && "file_skip_whitespace: file == 0");
	while (is_whitespace(file->buffer[file->pos]))
		file->pos++;
}
boolean file_read_line(file_t *file, string *line) {
	assert(file && "file_read_line: file == 0");
	assert(file->fp && "file_read_line: file->fp == 0");
	assert(line && "file_read_line: line == 0");
	string_clear(line);
	do {
		if (file->pos >= FILE_BUFFER_SIZE)
			if (!file_fill_buffer(file)) {
				if (line->length) break;
				else return false;
			}
		const size_t start_pos = file->pos;
		while (file->pos < FILE_BUFFER_SIZE && !is_newline(file->buffer[file->pos]) && file->buffer[file->pos] != '\0')
			file->pos++;
		if (file->pos < FILE_BUFFER_SIZE && file->buffer[file->pos] == '\0') {
			string_append_c(line, &file->buffer[start_pos]);
			file->pos = FILE_BUFFER_SIZE;
			break;
		} else {
			file->buffer[file->pos] = '\0';
			string_append_c(line, &file->buffer[start_pos]);
			if (file->pos < FILE_BUFFER_SIZE) {
				file->pos++;
				break;
			}
		}
	} while(true);
	return true;
}

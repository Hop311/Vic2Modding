#pragma once

#include "file.h"

#include <stdio.h>

/* Helper functions */
boolean is_alpha(char c);
boolean is_number(char c);
boolean is_alphanumeric(char c);
boolean is_symbol(char c);
boolean is_string_signifier(char c);

/* Date */
typedef union date_s {
	struct {
		u16 year;
		u8 month, day;
	};
	u32 packed;
} date_t;

boolean date_valid(const date_t *date);
date_t date_default(void);
boolean date_equal(const date_t *dateA, const date_t *dateB);
void date_print(FILE *const stream, const date_t *date);
size_t date_sprint(char *const buffer, size_t buffer_count, const date_t *date);

/*	Token types:
	 - alphanumeric (starting with letter or _)
	 - symbols ({ } = etc)
	 - string literal (" to ")
	 - int literal (including -ve)
	 - decimal literal (including -ve)
*/

struct token_t {
	enum token_type_t {
		UNKNOWN, ALPHANUMERIC, SYMBOL, STRING, INT_TOKEN, DECIMAL_TOKEN, DATE_TOKEN
	} type;
	union token_data_t {
		string str;
		char sym;
		int i;
		double d;
		date_t date;
	} data;
};
void token_init_unknown(struct token_t *token);
void token_init_alphanumeric(struct token_t *token, const string *str, boolean copy);
void token_init_symbol(struct token_t *token, char sym);
void token_init_string(struct token_t *token, const string *str, boolean copy);
void token_init_int(struct token_t *token, int i);
void token_init_decimal(struct token_t *token, double d);
void token_init_date(struct token_t *token, const date_t *date);
void token_free(struct token_t *token);
struct token_t token_move(struct token_t *token);
void token_print(FILE *const stream, const struct token_t *token);
size_t token_sprint(char *const buffer, size_t buffer_count, const struct token_t *token);

/* str here contains a pointer to another string's characters, so we can increment it
	to move through the string without losing its beginning */
boolean string_next_token(string *str, struct token_t *token);

struct token_source_t {
	file_t file;
	string filename;
	string line, unread;
	size_t line_number;
	struct token_t peek_token;
};

int token_source_init(struct token_source_t *src, const char *filename);
void token_source_free(struct token_source_t *src);
void token_source_clear_line(struct token_source_t *src);
/* returns true if another token is found, false if the file ends with no token */
boolean token_source_next(struct token_source_t *src, struct token_t *token);
boolean token_source_peek(struct token_source_t *src);
int token_source_expect_symbol(struct token_source_t *src, char sym, const char *func_name, const char *purpose);
int token_source_expect_alphanumeric(struct token_source_t *src, const char *alphanumeric, const char *func_name, const char *purpose);

int token_source_get_int(struct token_source_t *src, int *i, const char *func_name, const char *purpose);
int token_source_get_decimal(struct token_source_t *src, double *d, const char *func_name, const char *purpose);
int token_source_get_decimal_or_int(struct token_source_t *src, double *d, const char *func_name, const char *purpose);
int token_source_get_string(struct token_source_t *src, string *str, const char *func_name, const char *purpose);
int token_source_get_alphanumeric(struct token_source_t *src, string *str, const char *func_name, const char *purpose);
int token_source_get_date(struct token_source_t *src, date_t *date, const char *func_name, const char *purpose);
int token_source_get_bool(struct token_source_t *src, boolean *b, const char *func_name, const char *purpose);
/* NOT csv = also includes the { } either side of the color values 
	YES csv = skips any semicolons inside the color */
int token_source_get_color(struct token_source_t *src, u8 *col, boolean csv, const char *func_name, const char *purpose);
/* use just before a "= { ... }" */
int token_source_skip_block(struct token_source_t *src, const char *func_name, const char *purpose);

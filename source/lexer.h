#pragma once

#include "parser.h"

struct lexeme_t {
	struct token_t key;
	struct lexeme_t **values;
	boolean compound;
	struct lexeme_t *parent_lexeme, *next_lexeme, *prev_lexeme;
};
void lexeme_free(struct lexeme_t *lex);
struct lexeme_t *lexeme_new(void);
struct lexeme_t *lexeme_new_alphanumeric(const string *name, boolean copy, boolean compound);
struct lexeme_t *lexeme_new_alphanumeric_c(const char *name, boolean compound);
struct lexeme_t *lexeme_new_bool(boolean b, boolean compound);
struct lexeme_t *lexeme_new_int(int i, boolean compound);
struct lexeme_t *lexeme_new_decimal(double d, boolean compound);
void lexeme_delete(struct lexeme_t *lex);
void lexeme_add_value(struct lexeme_t *parent, struct lexeme_t *child);
void lexeme_add_color(struct lexeme_t *parent, u32 color);
void lexeme_print(const struct lexeme_t *lex);
boolean lexeme_is_named_group(const struct lexeme_t *lex);
boolean lexeme_get_color(const struct lexeme_t *root, u8 *col);
boolean lexeme_get_bool(const struct lexeme_t *root, boolean *b);
boolean lexeme_get_date(const struct lexeme_t *root, date_t *date);
boolean lexeme_get_int(const struct lexeme_t *root, int *i);
boolean lexeme_get_decimal(const struct lexeme_t *root, double *d);
boolean lexeme_get_int_or_decimal(const struct lexeme_t *root, double *d);
boolean lexeme_get_alphanumeric(const struct lexeme_t *root, string *str);
boolean lexeme_get_string(const struct lexeme_t *root, string *str);

int lexer_process_file(const char *filename, struct lexeme_t *lex_root);

int lexer_check_file(const char *filepath, const char *filename);
int lexer_check_all_in_folder(const char *base_folder);

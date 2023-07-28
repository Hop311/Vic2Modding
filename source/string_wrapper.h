#pragma once

#include "types.h"

#include <stdlib.h>

boolean is_whitespace(char c);
boolean is_newline(char c);

typedef struct string_t {
	char *text;
	size_t length;
} string;

/* zero-initialise, otherwise text will be freed! */

string string_make(const char *text);
string string_make_repeated(const char *text, size_t n);
void string_clear(string *str);
void string_set(string *strDest, const string *strSrc);
void string_set_c(string *str, const char *text);
void string_append(string *strA, const string *strB);
void string_append_c(string *str, const char *text);
void string_extract(string *str, const char *text, size_t length);
string string_move(string *str);
boolean string_empty(const string *str);
boolean string_equal(const string *strA, const string *strB);
boolean string_equal_c(const string *strA, const char *strB);

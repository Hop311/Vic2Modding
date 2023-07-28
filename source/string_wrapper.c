#include "string_wrapper.h"

#include "assert_opt.h"
#include "memory_opt.h"

#include <string.h>

boolean is_whitespace(char c) {
	return c == ' ' || c == '\t' || c == '\v' || c == '\f';
}
boolean is_newline(char c) {
	return c == '\n' || c == '\r';
}

string string_make(const char *text) {
	string ret = { 0 };
	if (text) {
		ret.length = strlen(text);
		if (ret.length) {
			ret.text = malloc_s(ret.length + 1);
			assert(ret.text && "string_make: malloc failed");
			memcpy(ret.text, text, ret.length + 1);
		}
	}
	return ret;
}
string string_make_repeated(const char *text, size_t n) {
	string ret = { 0 };
	if (n == 0 || text == 0 || text[0] == 0) return ret;
	const size_t len = strlen(text);
	ret.length = n * len;
	ret.text = malloc_s(ret.length + 1);
	assert(ret.text && "string_make_repeated: malloc failed");
	for (int i = 0; i < n; ++i)
		memcpy(&ret.text[n * len], text, len);
	return ret;
}
void string_clear(string *str) {
	assert(str && "string_clear: str == 0");
	if (str->text) {
		free_s(str->text);
		str->text = 0;
	}
	str->length = 0;
}
void string_set(string *strDest, const string *strSrc) {
	assert(strDest && "string_set: strDest == 0");
	assert(strSrc && "string_set: strSrc == 0");
	if (strSrc->text == 0 || strSrc->length == 0) {
		if (strDest->text) {
			free_s(strDest->text);
			strDest->text = 0;
		}
		strDest->length = 0;
		return;
	}
	if (strDest->text == 0)
		strDest->text = malloc_s(strSrc->length + 1);
	else if (strDest->length != strSrc->length) {
		free_s(strDest->text);
		strDest->text = malloc_s(strSrc->length + 1);
	}
	assert(strDest->text && "string_set: str->text == 0");
	memcpy(strDest->text, strSrc->text, strSrc->length + 1);
	strDest->length = strSrc->length;
}
void string_set_c(string *str, const char *text) {
	assert(str && "string_set_c: str == 0");
	string tmp = string_make(text);
	string_set(str, &tmp);
	string_clear(&tmp);
}
void string_append(string *strA, const string *strB) {
	assert(strA && "string_append: strA == 0");
	assert(strB && "string_append: strB == 0");
	if (strB->text == 0 || strB->length == 0) return;
	if (strA->text == 0 || strA->length == 0) {
		string_set(strA, strB);
		return;
	}

	char *new_text = realloc_s(strA->text, strA->length + strB->length + 1);
	assert(new_text && "string_append: realloc failed");
	memcpy(&new_text[strA->length], strB->text, strB->length + 1);
	strA->text = new_text;
	strA->length += strB->length;
}
void string_append_c(string *str, const char *text) {
	assert(str && "string_append_c: str == 0");
	string tmp = string_make(text);
	string_append(str, &tmp);
	string_clear(&tmp);
}
void string_extract(string *str, const char *text, size_t length) {
	assert(str && "string_extract: str == 0");
	string_clear(str);
	if (text == 0 || length == 0) return;
	str->text = malloc_s(length + 1);
	assert(str->text && "string_extract: malloc failed");
	str->length = length;
	memcpy(str->text, text, length);
	str->text[length] = '\0';
}
string string_move(string *str) {
	assert(str && "string_move: str == 0");
	string ret = *str;
	str->length = 0;
	str->text = 0;
	return ret;
}
boolean string_empty(const string *str) {
	assert(str && "string_empty: str == 0");
	return str->text == 0 || str->length == 0;
}
boolean string_equal(const string *strA, const string *strB) {
	assert(strA && "string_equal: strA == 0");
	assert(strB && "string_equal: strB == 0");
	const boolean emptyA = string_empty(strA), emptyB = string_empty(strB);
	if (emptyA || emptyB) return emptyA && emptyB;
	if (strA->length != strB->length) return false;
	for (int i = 0; i < strA->length; ++i)
		if (strA->text[i] != strB->text[i]) return false;
	return true;
}
boolean string_equal_c(const string *strA, const char *strB) {
	string tmp = string_make(strB);
	boolean ret = string_equal(strA, &tmp);
	string_clear(&tmp);
	return ret;
}

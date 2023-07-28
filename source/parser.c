#include "parser.h"

#include "assert_opt.h"
#include "memory_opt.h"

#include <string.h>

/* DATE */
const u8 days_per_month[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
boolean date_valid(const date_t *date) {
	assert(date && "date_valid: date == 0");
	return 1 <= date->month && date->month <= 12 && 1 <= date->day && date->day <= days_per_month[date->month - 1];
}
date_t date_default(void) {
	local const date_t ret = { .year = 1836, .month = 1, .day = 1 };
	return ret;
}
boolean date_equal(const date_t *dateA, const date_t *dateB) {
	assert(dateA && "date_equal: dateA == 0");
	assert(dateB && "date_equal: dateB == 0");
	return dateA->year == dateB->year && dateA->month == dateB->month && dateA->day == dateB->day;
}
void date_print(FILE *const stream, const date_t *date) {
	assert(date && "date_print: date == 0");
	fprintf(stdout, "%d.%d.%d", date->year, date->month, date->day);
}
size_t date_sprint(char *const buffer, size_t buffer_count, const date_t *date) {
	assert(buffer && "date_sprint: buffer == 0");
	assert(date && "date_sprint: date == 0");
	return sprintf_s(buffer, buffer_count, "%d.%d.%d", date->year, date->month, date->day);
}

void token_init_unknown(struct token_t *token) {
	assert(token && "token_init_unknown: token == 0");
	token->type = UNKNOWN;
}
void token_init_alphanumeric(struct token_t *token, const string *str, boolean copy) {
	assert(token && "token_init_alphanumeric: token == 0");
	assert(str && "token_init_alphanumeric: str == 0");
	token->type = ALPHANUMERIC;
	assert(str->length && "token_init_alphanumeric: str is empty");
	if (copy) {
		token->data.str.length = str->length;
		token->data.str.text = malloc_s(str->length + 1);
		assert(token->data.str.text && "token_init_alphanumeric: malloc failed");
		memcpy(token->data.str.text, str->text, str->length + 1);
	} else
		token->data.str = *str;
}
void token_init_symbol(struct token_t *token, char sym) {
	assert(token && "token_init_symbol: token == 0");
	token->type = SYMBOL;
	token->data.sym = sym;
}
void token_init_string(struct token_t *token, const string *str, boolean copy) {
	assert(token && "token_init_string: token == 0");
	assert(str && "token_init_string: str == 0");
	token->type = STRING;
	if (copy) string_set(&token->data.str, str);
	else token->data.str = *str;
}
void token_init_int(struct token_t *token, int i) {
	assert(token && "token_init_int: token == 0");
	token->type = INT_TOKEN;
	token->data.i = i;
}
void token_init_decimal(struct token_t *token, double d) {
	assert(token && "token_init_decimal: token == 0");
	token->type = DECIMAL_TOKEN;
	token->data.d = d;
}
void token_init_date(struct token_t *token, const date_t *date) {
	assert(token && "token_init_decimal: token == 0");
	token->type = DATE_TOKEN;
	token->data.date = *date;
}

void token_free(struct token_t *token) {
	assert(token && "token_free: token == 0");
	if (token->type == ALPHANUMERIC || token->type == STRING)
		string_clear(&token->data.str);
	token->type = UNKNOWN;
}
struct token_t token_move(struct token_t *token) {
	struct token_t ret = *token;
	memset(token, 0, sizeof(struct token_t));
	return ret;
}

void token_print(FILE *const stream, const struct token_t *token) {
	assert(token && "token_print: token == 0");
	if (token->type == UNKNOWN)
		fprintf(stream, "UNKOWN");
	else if (token->type == ALPHANUMERIC)
		fprintf(stream, "ALPHANUMERIC:%s", token->data.str.text);
	else if (token->type == SYMBOL)
		fprintf(stream, "SYMBOL:%c", token->data.sym);
	else if (token->type == STRING)
		fprintf(stream, "STRING:%s", token->data.str.text);
	else if (token->type == INT_TOKEN)
		fprintf(stream, "INT:%d", token->data.i);
	else if (token->type == DECIMAL_TOKEN)
		fprintf(stream, "DECIMAL:%f", token->data.d);
	else if (token->type == DATE_TOKEN)
		fprintf(stream, "DATE:%d.%d.%d", token->data.date.year, token->data.date.month, token->data.date.day);
	else
		fprintf(stream, "ERROR");
}
size_t token_sprint(char *const buffer, size_t buffer_count, const struct token_t *token) {
	assert(token && "token_print: token == 0");
	if (token->type == UNKNOWN)
		return sprintf_s(buffer, buffer_count, "UNKOWN");
	else if (token->type == ALPHANUMERIC)
		return sprintf_s(buffer, buffer_count, "%s", token->data.str.text);
	else if (token->type == SYMBOL)
		return sprintf_s(buffer, buffer_count, "%c", token->data.sym);
	else if (token->type == STRING)
		return sprintf_s(buffer, buffer_count, "\"%s\"", token->data.str.text);
	else if (token->type == INT_TOKEN)
		return sprintf_s(buffer, buffer_count, "%d", token->data.i);
	else if (token->type == DECIMAL_TOKEN)
		return sprintf_s(buffer, buffer_count, "%f", token->data.d);
	else if (token->type == DATE_TOKEN)
		return date_sprint(buffer, buffer_count, &token->data.date);
	else
		return sprintf_s(buffer, buffer_count, "ERROR");
}

boolean is_alpha(char c) {
	return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || (c & 0b10000000);
}
boolean is_number(char c) {
	return ('0' <= c && c <= '9');
}
boolean is_alphanumeric(char c) {
	//return is_alpha(c) || is_number(c) || c == '_' || c == '-' || c == '\'' || c == '.' || c == ':';
	return !is_symbol(c) && !is_whitespace(c) && !is_string_signifier(c);
}
boolean is_symbol(char c) {
	return c == '=' || c == '{' || c == '}' || c == ';' || c == ',';
}
boolean is_string_signifier(char c) {
	return c == '\'' || c == '"';
}

boolean parse_token(string *str, struct token_t *token) {
	/*	Token types:
			- alphanumeric (starting with letter or _)
			- symbols ({ } = etc)
			- string literal (" to ")
			- int literal (including -ve)
			- float literal (including -ve)
	*/
	if (is_alphanumeric(str->text[0])) {
		size_t length = 1;
		int points = str->text[0] == '.';
		boolean can_be_numeric = is_number(str->text[0]) || str->text[0] == '-' || points;
		while (length < str->length && is_alphanumeric(str->text[length])) {
			if (can_be_numeric && !is_number(str->text[length])) {
				if (str->text[length] == '.' && points < 2) points++;
				else can_be_numeric = false;
			}
			length++;
		}
		/* Can be: alphanumeric, int, decimal or date */
		if (can_be_numeric) {
			string tmp = { 0 };
			string_extract(&tmp, str->text, length);
			assert(tmp.text && "parse_token: (number) tmp.text == 0");
			if (points == 0)
				token_init_int(token, atoi(tmp.text));
			else if (points == 1)
				token_init_decimal(token, atof(tmp.text));
			else if (points == 2) {
				date_t date = date_default();
				int pos = 0;
				if (tmp.text[0] == '-') {
					pos++;
					fprintf(stdout, "[parse_token] Cannot have negative year: %s\n", tmp.text);
				}
				if (tmp.text[pos] == '.') {
					pos++;
					fprintf(stdout, "[parse_token] Date is missing year: %s\n", tmp.text);
				} else {
					int pos2 = pos;
					while (tmp.text[++pos2] && tmp.text[pos2] != '.');
					string tmp2 = { 0 };
					string_extract(&tmp2, &tmp.text[pos], pos2 - pos);
					date.year = atoi(tmp2.text);
					string_clear(&tmp2);
					pos = pos2 + 1;
				}
				if (tmp.text[pos] == '.') {
					pos++;
					fprintf(stdout, "[parse_token] Date is missing month: %s\n", tmp.text);
				} else {
					int pos3 = pos;
					while (tmp.text[++pos3] && tmp.text[pos3] != '.');
					string tmp3 = { 0 };
					string_extract(&tmp3, &tmp.text[pos], pos3 - pos);
					u8 m = atoi(tmp3.text);
					string_clear(&tmp3);
					if (m < 1 || m > 12) {
						fprintf(stdout, "[parse_token] Invalid month (%d) in %s\n", m, tmp.text);
					} else date.month = m;
					pos = pos3 + 1;
				}
				if (tmp.text[pos] == '.') {
					pos++;
					fprintf(stdout, "[parse_token] Date is missing day: %s\n", tmp.text);
				} else {
					int pos4 = pos;
					while (tmp.text[++pos4] && tmp.text[pos4] != '.');
					string tmp4 = { 0 };
					string_extract(&tmp4, &tmp.text[pos], pos4 - pos);
					u8 d = atoi(tmp4.text);
					string_clear(&tmp4);
					if (d < 1 || d > days_per_month[date.month - 1]) {
						fprintf(stdout, "[parse_token] Invalid day (%d) in %s\n", d, tmp.text);
					} else date.day = d;
				}
				string_clear(&tmp);
				token_init_date(token, &date);
			} else {
				token_init_unknown(token);
				fprintf(stdout, "[parse_token] Incompatible point-count (%u) in: %s\n", points, tmp.text);
				string_clear(&tmp);
				return false;
			}
			string_clear(&tmp);
			str->text += length;
			str->length -= length;
			return true;
		} else {
			string tmp = { 0 };
			string_extract(&tmp, str->text, length);
			token_init_alphanumeric(token, &tmp, false);
			str->text += length;
			str->length -= length;
			return true;
		}
	} else if (is_symbol(str->text[0])) {
		token_init_symbol(token, str->text[0]);
		str->text++;
		str->length--;
		return true;
	} else if (is_string_signifier(str->text[0])) {	/* STRING */
		size_t length = 1;
		while (length < str->length && str->text[length] != str->text[0]) length++;
		string tmp = { 0 };
		string_extract(&tmp, str->text + 1, length - 1);
		token_init_string(token, &tmp, false);
		if (length < str->length) length++;
		str->text += length;
		str->length -= length;
		return true;
	} else {
		token_init_unknown(token);
		fprintf(stdout, "[parse_token] Unknown token: %s\n", str->text);
		return false;
	}
}

boolean string_next_token(string *str, struct token_t *token) {
	assert(str && "string_next_token: str == 0");
	assert(token && "string_next_token: token == 0");
	while (str->length > 0 && is_whitespace(str->text[0])) {
		str->text++;
		str->length--;
	}
	if (str->length == 0 || str->text[0] == '#' || (str->length > 1 && str->text[0] == '-' && str->text[1] == '-')) {
		token_init_unknown(token);
		return false;
	}
	//return parse_token(str, token);
	const char *line = str->text;
	boolean ret = parse_token(str, token);
	
	/*fprintf(stdout, "TOKEN: ");
	token_print(stdout, token);
	fprintf(stdout, " (from: %s)\n", line);*/
	
	return ret;
}

int token_source_init(struct token_source_t *src, const char *filename) {
	assert(src && "token_source_init: src == 0");
	assert(filename && "token_source_init: filename == 0");
	string_set_c(&src->filename, filename);
	string_set_c(&src->line, 0);
	src->unread = src->line;
	src->line_number = 0;
	int ret = file_open(&src->file, filename);
	if (ret) fprintf(stdout, "[token_source_init] Failed to open file: %s (error code: %d)\n", filename, ret);
	return ret;
}
void token_source_free(struct token_source_t *src) {
	assert(src && "token_source_free: src == 0");
	file_close(&src->file);
	string_clear(&src->filename);
	string_clear(&src->line);
	token_free(&src->peek_token);
	memset(src, 0, sizeof(struct token_source_t));
}
void token_source_clear_line(struct token_source_t *src) {
	assert(src && "token_source_clear_line: src == 0");
	string_clear(&src->line);
	src->unread = src->line;
}
/* returns true if another token is found, false if the file ends with no token */
boolean token_source_next(struct token_source_t *src, struct token_t *token) {
	assert(src && "token_source_next: src == 0");
	assert(src->file.fp && "token_source_next: src->file.fp == 0");
	assert(token && "token_source_free: token == 0");
	token_free(token);

	if (src->peek_token.type == UNKNOWN) {
		while (true) {
			while (src->unread.length == 0) {
				if (!file_read_line(&src->file, &src->line)) {	/* file reading fails */
					file_close(&src->file);
					return false;
				}
				src->unread = src->line;
				src->line_number++;
			}
			if (string_next_token(&src->unread, token)) return true;
			else src->unread.length = 0;
		}
	} else {
		*token = src->peek_token;
		memset(&src->peek_token, 0, sizeof(struct token_t));
		return true;
	}
}
boolean token_source_peek(struct token_source_t *src) {
	assert(src && "token_source_next: src == 0");
	assert(src->file.fp && "token_source_next: src->file.fp == 0");
	if (src->peek_token.type == UNKNOWN) {
		return token_source_next(src, &src->peek_token);
	} else return true;
}
int token_source_expect_symbol(struct token_source_t *src, char sym, const char *func_name, const char *purpose) {
	assert(src && "token_source_expect_symbol: src == 0");
	assert(is_symbol(sym) && "token_source_expect_symbol: invalid symbol");
	struct token_t token = { 0 };
	if (!token_source_next(src, &token)) {
		token_free(&token);
		fprintf(stdout, "[%s] Missing token (expected '%c' for %s) [line:%zu|%s]\n", func_name, sym, purpose, src->line_number, src->filename.text);
		return ERROR_RETURN;
	}
	if (token.type != SYMBOL || token.data.sym != sym) {
		fprintf(stdout, "[%s] Invalid token (expected '%c' for %s): ", func_name, sym, purpose);
		token_print(stdout, &token);
		fprintf(stdout, " [line:%zu|%s]\n", src->line_number, src->filename.text);
		token_free(&token);
		return ERROR_RETURN;
	}
	return 0;
}
int token_source_expect_alphanumeric(struct token_source_t *src, const char *alphanumeric, const char *func_name, const char *purpose) {
	assert(src && "token_source_expect_alphanumeric: src == 0");
	assert((alphanumeric && alphanumeric[0]) && "token_source_expect_alphanumeric: invalid alphanumeric");
	struct token_t token = { 0 };
	if (!token_source_next(src, &token)) {
		token_free(&token);
		fprintf(stdout, "[%s] Missing token (expected \"%s\" for %s) [line:%zu|%s]\n", func_name, alphanumeric, purpose, src->line_number, src->filename.text);
		return ERROR_RETURN;
	}
	if (token.type != ALPHANUMERIC || !string_equal_c(&token.data.str, alphanumeric)) {
		fprintf(stdout, "[%s] Invalid token (expected \"%s\" for %s): ", func_name, alphanumeric, purpose);
		token_print(stdout, &token);
		fprintf(stdout, " [line:%zu|%s]\n", src->line_number, src->filename.text);
		token_free(&token);
		return ERROR_RETURN;
	}
	token_free(&token);
	return 0;
}


int token_source_get_int(struct token_source_t *src, int *i, const char *func_name, const char *purpose) {
	assert(src && "token_source_get_int: src == 0");
	assert(i && "token_source_get_int: i == 0");
	struct token_t token = { 0 };
	if (!token_source_next(src, &token)) {
		token_free(&token);
		fprintf(stdout, "[%s] Missing token (expected int for %s) [line:%zu]\n", func_name, purpose, src->line_number);
		return ERROR_RETURN;
	}
	if (token.type != INT_TOKEN) {
		fprintf(stdout, "[%s] Invalid token (expected int for %s): ", func_name, purpose);
		token_print(stdout, &token);
		fprintf(stdout, " [line:%zu]\n", src->line_number);
		token_free(&token);
		return ERROR_RETURN;
	}
	*i = token.data.i;
	return 0;
}
int token_source_get_decimal(struct token_source_t *src, double *d, const char *func_name, const char *purpose) {
	assert(src && "token_source_get_decimal: src == 0");
	assert(d && "token_source_get_decimal: d == 0");
	struct token_t token = { 0 };
	if (!token_source_next(src, &token)) {
		token_free(&token);
		fprintf(stdout, "[%s] Missing token (expected decimal for %s) [line:%zu]\n", func_name, purpose, src->line_number);
		return ERROR_RETURN;
	}
	if (token.type != DECIMAL_TOKEN) {
		fprintf(stdout, "[%s] Invalid token (expected decimal for %s): ", func_name, purpose);
		token_print(stdout, &token);
		fprintf(stdout, " [line:%zu]\n", src->line_number);
		token_free(&token);
		return ERROR_RETURN;
	}
	*d = token.data.d;
	return 0;
}
int token_source_get_decimal_or_int(struct token_source_t *src, double *d, const char *func_name, const char *purpose) {
	assert(src && "token_source_get_decimal_or_int: src == 0");
	assert(d && "token_source_get_decimal_or_int: d == 0");
	struct token_t token = { 0 };
	if (!token_source_next(src, &token)) {
		token_free(&token);
		fprintf(stdout, "[%s] Missing token (expected decimal or int for %s) [line:%zu]\n", func_name, purpose, src->line_number);
		return ERROR_RETURN;
	}
	if (token.type == DECIMAL_TOKEN)
		*d = token.data.d;
	else if (token.type == INT_TOKEN)
		*d = (double)token.data.i;
	else {
		fprintf(stdout, "[%s] Invalid token (expected decimal or int for %s): ", func_name, purpose);
		token_print(stdout, &token);
		fprintf(stdout, " [line:%zu]\n", src->line_number);
		token_free(&token);
		return ERROR_RETURN;
	}
	return 0;
}
int token_source_get_string(struct token_source_t *src, string *str, const char *func_name, const char *purpose) {
	assert(src && "token_source_get_string: src == 0");
	assert(str && "token_source_get_string: str == 0");
	string_clear(str);
	struct token_t token = { 0 };
	if (!token_source_next(src, &token)) {
		token_free(&token);
		fprintf(stdout, "[%s] Missing token (expected string for %s) [line:%zu]\n", func_name, purpose, src->line_number);
		return ERROR_RETURN;
	}
	if (token.type != STRING) {
		fprintf(stdout, "[%s] Invalid token (expected string for %s): ", func_name, purpose);
		token_print(stdout, &token);
		fprintf(stdout, " [line:%zu]\n", src->line_number);
		token_free(&token);
		return ERROR_RETURN;
	}
	*str = string_move(&token.data.str);
	return 0;
}
int token_source_get_alphanumeric(struct token_source_t *src, string *str, const char *func_name, const char *purpose) {
	assert(src && "token_source_get_alphanumeric: src == 0");
	assert(str && "token_source_get_alphanumeric: str == 0");
	string_clear(str);
	struct token_t token = { 0 };
	if (!token_source_next(src, &token)) {
		token_free(&token);
		fprintf(stdout, "[%s] Missing token (expected alphanumeric for %s) [line:%zu]\n", func_name, purpose, src->line_number);
		return ERROR_RETURN;
	}
	if (token.type != ALPHANUMERIC) {
		fprintf(stdout, "[%s] Invalid token (expected alphanumeric for %s): ", func_name, purpose);
		token_print(stdout, &token);
		fprintf(stdout, " [line:%zu]\n", src->line_number);
		token_free(&token);
		return ERROR_RETURN;
	}
	*str = string_move(&token.data.str);
	return 0;
}
int token_source_get_date(struct token_source_t *src, date_t *date, const char *func_name, const char *purpose) {
	assert(src && "token_source_get_date: src == 0");
	assert(date && "token_source_get_date: date == 0");
	*date = date_default();
	struct token_t token = { 0 };
	if (!token_source_next(src, &token)) {
		token_free(&token);
		fprintf(stdout, "[%s] Missing token (expected date for %s) [line:%zu]\n", func_name, purpose, src->line_number);
		return ERROR_RETURN;
	}
	if (token.type != DATE_TOKEN) {
		fprintf(stdout, "[%s] Invalid token (expected date for %s): ", func_name, purpose);
		token_print(stdout, &token);
		fprintf(stdout, " [line:%zu]\n", src->line_number);
		token_free(&token);
		return ERROR_RETURN;
	}
	*date = token.data.date;
	return 0;
}
int token_source_get_bool(struct token_source_t *src, boolean *b, const char *func_name, const char *purpose) {
	assert(src && "token_source_get_bool: src == 0");
	assert(b && "token_source_get_bool: b == 0");
	struct token_t token = { 0 };
	if (!token_source_next(src, &token)) {
		token_free(&token);
		fprintf(stdout, "[%s] Missing token (expected bool for %s) [line:%zu]\n", func_name, purpose, src->line_number);
		return ERROR_RETURN;
	}
	if (token.type != ALPHANUMERIC) {
		fprintf(stdout, "[%s] Invalid token (expected bool for %s): ", func_name, purpose);
		token_print(stdout, &token);
		fprintf(stdout, " [line:%zu]\n", src->line_number);
		token_free(&token);
		return ERROR_RETURN;
	}
	if (string_equal_c(&token.data.str, "yes"))
		*b = true;
	else if (string_equal_c(&token.data.str, "no"))
		*b = false;
	else {
		token_free(&token);
		fprintf(stdout, "[%s] Expected bool for %s, couldn't understand: %s [line:%zu]\n", func_name, purpose, token.data.str.text, src->line_number);
		return ERROR_RETURN;
	}
	token_free(&token);
	return 0;
}
int token_source_get_color(struct token_source_t *src, u8 *col, boolean csv, const char *func_name, const char *purpose) {
	assert(src && "token_source_get_color: src == 0");
	assert(col && "token_source_get_color: col == 0");
	if (!csv && token_source_expect_symbol(src, '{', func_name, purpose)) return ERROR_RETURN;
	int tmp_color = 0;
	struct token_t token = { 0 };
	for (int i = 0; i < 3; ++i)
		if (token_source_next(src, &token)) {
			if (csv && token.type == SYMBOL && token.data.sym == ';')
				if (!token_source_next(src, &token)) return ERROR_RETURN;
			if (token.type == INT_TOKEN) {
				tmp_color = token.data.i;
			} else if (token.type == DECIMAL_TOKEN) {
				if (0.0 <= token.data.d && token.data.d <= 1.0) tmp_color = (int)(token.data.d * 255.0);
				else tmp_color = (int)token.data.d;
			} else {
				fprintf(stdout, "[%s] Invalid token (expected int for color for %s): ", func_name, purpose);
				token_print(stdout, &token);
				fprintf(stdout, " [line:%zu]\n", src->line_number);
				token_free(&token);
				return ERROR_RETURN;
			}
			if (tmp_color < 0) {
				fprintf(stdout, "[%s] color[%d] for %s is negative (%d) [line:%zu]\n", func_name, i, purpose, tmp_color, src->line_number);
				tmp_color = 0;
			} else if (tmp_color > 255) {
				fprintf(stdout, "[%s] color[%d] for %s is greater than 255 (%d) [line:%zu]\n", func_name, i, purpose, tmp_color, src->line_number);
				tmp_color = 255;
			}
			col[i] = (u8)tmp_color;
		} else return ERROR_RETURN;
	if (!csv && token_source_expect_symbol(src, '}', func_name, purpose)) return ERROR_RETURN;
	return 0;
}

int token_source_skip_block(struct token_source_t *src, const char *func_name, const char *purpose) {
	assert(src && "token_source_skip_block: src == 0");
	if (token_source_expect_symbol(src, '=', func_name, purpose)) return ERROR_RETURN;
	if (token_source_expect_symbol(src, '{', func_name, purpose)) return ERROR_RETURN;
	int bracket_depth = 1;
	boolean has_eq = false;
	struct token_t token = { 0 };
	while (bracket_depth > 0) {
		if (token_source_next(src, &token)) {
			if (!has_eq && token.type == SYMBOL && token.data.sym == '=')
				has_eq = true;
			else if (has_eq && token.type == SYMBOL && token.data.sym == '{') {
				bracket_depth++;
				has_eq = false;
			} else if (!has_eq && token.type == SYMBOL && token.data.sym == '}')
				bracket_depth--;
			else if (token.type == ALPHANUMERIC || token.type == STRING || token.type == INT_TOKEN || token.type == DECIMAL_TOKEN)
				has_eq = false;
			else {
				fprintf(stdout, "[%s] Invalid token (in block skipped for %s): ", func_name, purpose);
				token_print(stdout, &token);
				fprintf(stdout, " [line:%zu]\n", src->line_number);
				return ERROR_RETURN;
			}
		} else return ERROR_RETURN;
	}
	return 0;
}

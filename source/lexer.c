#include "lexer.h"

#include "assert_opt.h"
#include "memory_opt.h"
#include "parser.h"

#include <windows.h>

boolean token_is_data(const struct token_t *token) {
	return token->type == ALPHANUMERIC || token->type == STRING || token->type == INT_TOKEN || token->type == DECIMAL_TOKEN || token->type == DATE_TOKEN;
}

void lexeme_free(struct lexeme_t *lex) {
	assert(lex && "lexeme_free: lex == 0");
	token_free(&lex->key);
	for_buf(i, lex->values) lexeme_delete(lex->values[i]);
	buf_free(lex->values);
}
struct lexeme_t *lexeme_new(void) {
	struct lexeme_t *ret = calloc_s(sizeof(struct lexeme_t));
	assert(ret && "lexeme_new: ret == 0");
	return ret;
}
struct lexeme_t *lexeme_new_alphanumeric(const string *name, boolean copy, boolean compound) {
	assert(name && "lexeme_new_alphanumeric: name == 0");
	struct lexeme_t *ret = lexeme_new();
	token_init_alphanumeric(&ret->key, name, copy);
	ret->compound = compound;
	return ret;
}
struct lexeme_t *lexeme_new_alphanumeric_c(const char *name, boolean compound) {
	assert(name && "lexeme_new_alphanumeric_c: name == 0");
	string tmp = string_make(name);
	return lexeme_new_alphanumeric(&tmp, false, compound);
}
struct lexeme_t *lexeme_new_bool(boolean b, boolean compound) {
	struct lexeme_t *ret = lexeme_new();
	string bool_text = string_make(b ? "yes" : "no");
	token_init_alphanumeric(&ret->key, &bool_text, false);
	ret->compound = compound;
	return ret;
}
struct lexeme_t *lexeme_new_int(int i, boolean compound) {
	struct lexeme_t *ret = lexeme_new();
	token_init_int(&ret->key, i);
	ret->compound = compound;
	return ret;
}
struct lexeme_t *lexeme_new_decimal(double d, boolean compound) {
	struct lexeme_t *ret = lexeme_new();
	token_init_decimal(&ret->key, d);
	ret->compound = compound;
	return ret;
}
void lexeme_delete(struct lexeme_t *lex) {
	lexeme_free(lex);
	free_s(lex);
}
void lexeme_add_value(struct lexeme_t *parent, struct lexeme_t *child) {
	assert(parent && "lexeme_add_value: parent == 0");
	assert(child && "lexeme_add_value: child == 0");
	assert(child->parent_lexeme == 0 && "lexeme_add_value: child->parent_lexeme already set");

	/*fprintf(stdout, "ADDING: ");
	token_print(stdout, &child->key);
	fprintf(stdout, " to ");
	token_print(stdout, &parent->key);
	fprintf(stdout, "\n");*/

	child->parent_lexeme = parent;
	if (parent->values) {
		child->prev_lexeme = *buf_back(parent->values);
		assert(child->prev_lexeme && "lexeme_add_value: zero in lexeme value list");
		assert(child->prev_lexeme->next_lexeme == 0 && "lexeme_add_value: last lexeme already has next filled in");
		child->prev_lexeme->next_lexeme = child;
	}
	buf_push(parent->values, child);
}
void lexeme_add_color(struct lexeme_t *parent, u32 color) {
	assert(parent && "lexeme_add_color: parent == 0");
	struct lexeme_t *color_r_l = lexeme_new_int((color >> 16) & 255, false);
	struct lexeme_t *color_g_l = lexeme_new_int((color >>  8) & 255, false);
	struct lexeme_t *color_b_l = lexeme_new_int((color >>  0) & 255, false);
	lexeme_add_value(parent, color_r_l);
	lexeme_add_value(parent, color_g_l);
	lexeme_add_value(parent, color_b_l);
	parent->compound = true;
}
#define LEXEME_PRINT_BUF_SIZE 256
internal void internal_lexeme_print(const struct lexeme_t *lex, size_t indent) {
	assert(lex && "lexeme_print: lex == 0");
	char buffer[LEXEME_PRINT_BUF_SIZE] = { 0 };
	memset(buffer, '\t', indent);
	size_t pos = indent;
	pos += token_sprint(buffer + pos, LEXEME_PRINT_BUF_SIZE - pos, &lex->key);
	if (lex->values) {
		if (lex->compound) {
			if (buf_len(lex->values)) {
				fprintf(stdout, "%s = {\n", buffer);
				for_buf(i, lex->values)
					internal_lexeme_print(lex->values[i], indent + 1);
				buffer[indent] = 0;
				fprintf(stdout, "%s}\n", buffer);
			} else fprintf(stdout, "%s = { }\n", buffer);
		} else {
			buffer[pos++] = ' ';
			buffer[pos++] = '=';
			buffer[pos++] = ' ';
			pos += token_sprint(buffer + pos, LEXEME_PRINT_BUF_SIZE - pos, &lex->values[0]->key);
			fprintf(stdout, "%s\n", buffer);
		}
	} else fprintf(stdout, "%s\n", buffer);
}
void lexeme_print(const struct lexeme_t *lex) {
	assert(lex && "lexeme_print: lex == 0");
	internal_lexeme_print(lex, 0);
}
boolean lexeme_is_named_group(const struct lexeme_t *lex) {
	return lex->compound && lex->key.type == ALPHANUMERIC;
}
boolean lexeme_get_color(const struct lexeme_t *root, u8 *col) {
	assert(root && "lexeme_get_color: root == 0");
	assert(col && "lexeme_get_color: col == 0");
	if (!root->compound || root->values == 0 || buf_len(root->values) != 3) {
		fprintf(stdout, "[lexeme_get_color] Invalid color: must have exactly 3 integer or decimal values\n");
		return false;
	}
	for_buf(i, root->values) {
		struct lexeme_t *val = root->values[i];
		if (val->compound || val->values || (val->key.type != DECIMAL_TOKEN && val->key.type != INT_TOKEN)) {
			fprintf(stdout, "[lexeme_get_color] Invalid color component: must be non-compound integer or decimal with no child value\n");
			col[i] = 0;
		} else if (val->key.type == DECIMAL_TOKEN) {
			double c = val->key.data.d;
			if (c <= 0.0) col[i] = 0;
			else {
				if (c <= 1.0) c *= 255.0;
				if (c >= 255.0) col[i] = 255;
				else col[i] = (int)c;
			}
		} else if (val->key.type == INT_TOKEN) {
			int c = val->key.data.i;
			if (c <= 0) col[i] = 0;
			else if (c >= 255) col[i] = 255;
			else col[i] = c;
		}
	}
	return true;
}
boolean lexeme_get_bool(const struct lexeme_t *root, boolean *b) {
	assert(root && "lexeme_get_bool: root == 0");
	assert(b && "lexeme_get_bool: b == 0");
	if (root->compound || root->values == 0 || buf_len(root->values) != 1 || root->values[0]->key.type != ALPHANUMERIC) {
		fprintf(stdout, "[lexeme_get_color] Invalid bool: must have exactly 1 alphanumeric yes/no value\n");
		return false;
	}
	if (string_equal_c(&root->values[0]->key.data.str, "yes")) *b = true;
	else if (string_equal_c(&root->values[0]->key.data.str, "no")) *b = false;
	else {
		fprintf(stdout, "[lexeme_get_color] Invalid bool value: %s\n", root->values[0]->key.data.str.text);
		return false;
	}
	return true;
}
boolean lexeme_get_date(const struct lexeme_t *root, date_t *date) {
	assert(root && "lexeme_get_date: root == 0");
	assert(date && "lexeme_get_date: date == 0");
	if (root->compound || root->values == 0 || buf_len(root->values) != 1 || root->values[0]->key.type != DATE_TOKEN) {
		fprintf(stdout, "[lexeme_get_date] Invalid date: must have exactly 1 date value (Y.M.D)\n");
		return false;
	}
	*date = root->values[0]->key.data.date;
	return true;
}
boolean lexeme_get_int(const struct lexeme_t *root, int *i) {
	assert(root && "lexeme_get_int: root == 0");
	assert(i && "lexeme_get_int: i == 0");
	if (root->compound || root->values == 0 || buf_len(root->values) != 1 || root->values[0]->key.type != INT_TOKEN) {
		fprintf(stdout, "[lexeme_get_int] Invalid int: must have exactly 1 int value\n");
		return false;
	}
	*i = root->values[0]->key.data.i;
	return true;
}
boolean lexeme_get_decimal(const struct lexeme_t *root, double *d) {
	assert(root && "lexeme_get_decimal: root == 0");
	assert(d && "lexeme_get_decimal: d == 0");
	if (root->compound || root->values == 0 || buf_len(root->values) != 1 || root->values[0]->key.type != DECIMAL_TOKEN) {
		fprintf(stdout, "[lexeme_get_decimal] Invalid decimal: must have exactly 1 decimal value\n");
		return false;
	}
	*d = root->values[0]->key.data.d;
	return true;
}
boolean lexeme_get_int_or_decimal(const struct lexeme_t *root, double *d) {
	assert(root && "lexeme_get_int_or_decimal: root == 0");
	assert(d && "lexeme_get_int_or_decimal: d == 0");
	if (!root->compound && root->values && buf_len(root->values) == 1) {
		if (root->values[0]->key.type == DECIMAL_TOKEN) {
			*d = root->values[0]->key.data.d;
			return true;
		} else if (root->values[0]->key.type == INT_TOKEN) {
			*d = (double)root->values[0]->key.data.i;
			return true;
		}
	}
	fprintf(stdout, "[lexeme_get_int_or_decimal] Invalid decimal: must have exactly 1 int or decimal value\n");
	return false;
}
boolean lexeme_get_alphanumeric(const struct lexeme_t *root, string *str) {
	assert(root && "lexeme_get_alphanumeric: root == 0");
	assert(str && "lexeme_get_alphanumeric: str == 0");
	string_clear(str);
	if (root->compound || root->values == 0 || buf_len(root->values) != 1 || root->values[0]->key.type != ALPHANUMERIC) {
		fprintf(stdout, "[lexeme_get_alphanumeric] Invalid alphanumeric\n");
		return false;
	}
	string_set(str, &root->values[0]->key.data.str);
	return true;
}
boolean lexeme_get_string(const struct lexeme_t *root, string *str) {
	assert(root && "lexeme_get_string: root == 0");
	assert(str && "lexeme_get_string: str == 0");
	string_clear(str);
	if (root->compound || root->values == 0 || buf_len(root->values) != 1 || root->values[0]->key.type != STRING) {
		fprintf(stdout, "[lexeme_get_string] Invalid string\n");
		return false;
	}
	string_set(str, &root->values[0]->key.data.str);
	return true;
}

int lexeme_read(struct token_source_t *src, struct lexeme_t *lex);
int lexeme_read_compound(struct token_source_t *src, struct lexeme_t *parent) {
	parent->compound = true;
	struct token_t token = { 0 };
	while (token_source_peek(src)) {
		if (src->peek_token.type == SYMBOL && src->peek_token.data.sym == '}') {
			return token_source_next(src, &token);
		} else {
			struct lexeme_t *val = lexeme_new();
			if (!lexeme_read(src, val)) {
				lexeme_delete(val);
				return false;
			}
			val->parent_lexeme = parent;
			if (parent->values) {
				val->prev_lexeme = *buf_back(parent->values);
				assert(val->prev_lexeme->next_lexeme == 0 && "lexeme_read: back value already has next value set");
				val->prev_lexeme->next_lexeme = val;
			}
			buf_push(parent->values, val);
		}
	}
	token_free(&token);
	fprintf(stdout, "lexeme_read_compound: unclosed { brackets [line:% zu | % s]\n", src->line_number, src->filename.text);
	return false;
}

int lexeme_read(struct token_source_t *src, struct lexeme_t *lex) {
	assert(src && "lexeme_read: src == 0");
	assert(lex && "lexeme_read: lex == 0");
	lexeme_free(lex);
	struct token_t token = { 0 };
	if (!token_source_next(src, &token)) return false;
	if (token.type == SYMBOL && token.data.sym == '{') {
		string compound_name = string_make("compound");
		token_init_alphanumeric(&lex->key, &compound_name, false);
		return lexeme_read_compound(src, lex);
	}
	if (!token_is_data(&token)) {
		fprintf(stdout, "[lexeme_read] Invalid token (expected data key): ");
		token_print(stdout, &token);
		fprintf(stdout, " [line:%zu|%s]\n", src->line_number, src->filename.text);
		return false;
	}
	lex->key = token_move(&token);
	if (token_source_peek(src) && src->peek_token.type == SYMBOL && src->peek_token.data.sym == '=') {
		if (!token_source_next(src, &token)) return false; // pushing through the '='
		if (!token_source_next(src, &token)) return false; // getting '{' or data
		if (token_is_data(&token)) {
			struct lexeme_t *val = lexeme_new();
			val->key = token_move(&token);
			val->parent_lexeme = lex;
			buf_push(lex->values, val);
			return true;
		} else if (token.type == SYMBOL && token.data.sym == '{') {
			return lexeme_read_compound(src, lex);
		} else {
			fprintf(stdout, "[lexeme_read] Invalid token (expected data value or '{'): ");
			token_print(stdout, &token);
			token_free(&token);
			fprintf(stdout, " [line:%zu|%s]\n", src->line_number, src->filename.text);
			return false;
		}
	} else return true;
}

int lexer_process_file(const char *filename, struct lexeme_t *lex_root) {
	assert(filename && "lexer_process_source: filename == 0");
	assert(lex_root && "lexer_process_source: lex_root == 0");
	{
		/* root = { src contents... } */
		lexeme_free(lex_root);
		string root_name = string_make("root");
		token_init_alphanumeric(&lex_root->key, &root_name, false);
		lex_root->compound = true;
	}
	struct token_source_t src = { 0 };
	int err = token_source_init(&src, filename);
	if (err) return err;
	struct lexeme_t lex = { 0 };
	while (lexeme_read(&src, &lex)) {
		struct lexeme_t *val = lexeme_new();
		*val = lex;
		memset(&lex, 0, sizeof(struct lexeme_t));
		buf_push(lex_root->values, val);
		//lexeme_print(lex_root);
	}
	lexeme_free(&lex);
	token_source_free(&src);
	return 0;
}

const char *get_filename_ext(const char *filename) {
	const char *dot = strrchr(filename, '.');
	if (!dot || dot == filename) return "";
	return dot + 1;
}

int lexer_check_file(const char *filepath, const char *filename) {
	assert(filepath && "lexer_check_file: filepath == 0");
	assert(filepath[0] && "lexer_check_file: filepath[0] == 0");
	assert(filename && "lexer_check_file: filename == 0");
	assert(filename[0] && "lexer_check_file: filename[0] == 0");

	const char *file_ext = get_filename_ext(filename);
	if (strcmp(file_ext, "bmp") == 0 || strcmp(file_ext, "dds") == 0 || strcmp(file_ext, "csv") == 0) return 0;

	struct token_source_t src = { 0 };
	int err = token_source_init(&src, filepath);
	if (err) return err;
	struct token_t token = { 0 };
	boolean can_eq = false, is_eq = false;
	int depth = 0;
	while (token_source_next(&src, &token)) {
		//token_print(stdout, &token);
		//fprintf(stdout, "\n");
		if (!is_eq && !can_eq && token.type == SYMBOL && token.data.sym == ',') continue;
		if (can_eq) {
			can_eq = false;
			if (token.type == SYMBOL && token.data.sym == '=') {
				is_eq = true;
				continue;
			}
		}

		if (is_eq && token_is_data(&token)) is_eq = false;
		else if (token.type == SYMBOL && token.data.sym == '{') {
			is_eq = false;
			depth++;
			//fprintf(stdout, "OPENING BRACKETS (NEW DEPTH %d)\n", depth);
		} else if (!is_eq && token.type == SYMBOL && token.data.sym == '}') {
			if (depth) {
				depth--;
				//fprintf(stdout, "CLOSING BRACKETS (NEW DEPTH %d)\n", depth);
			} else {
				fprintf(stdout, "[lexer_check_file] Closing bracket } with no matching opening bracket [line:%zu|%s]\n", src.line_number, filename);
				err = ERROR_RETURN;
			}
		} else if (!is_eq && token_is_data(&token)) can_eq = true;
		else {
			fprintf(stdout, "[lexer_check_file] Invalid token: ");
			token_print(stdout, &token);
			fprintf(stdout, " [line:%zu|%s]\n", src.line_number, filename);
			err = ERROR_RETURN;
		}
	}
	token_free(&token);
	token_source_free(&src);

	if (is_eq) {
		fprintf(stdout, "[lexer_check_file] Trailing = at end of file [line:%zu|%s]\n", src.line_number, filename);
		err = ERROR_RETURN;
	}
	if (depth) {
		fprintf(stdout, "[lexer_check_file] Unclosed opening brackets } : %d [line:%zu|%s]\n", depth, src.line_number, filename);
		err = ERROR_RETURN;
	}
	return err;
}
internal int internal_lexer_check_all_in_folder(const char *base_folder, int *files_read) {
	WIN32_FIND_DATA foundFile;
	HANDLE hFind = NULL;
	char sPath[2048];
	/* Specify a file mask. *.* = We want everything! */
	sprintf_s(sPath, 2048, "%s/*.*", base_folder);
	if ((hFind = FindFirstFile(sPath, &foundFile)) == INVALID_HANDLE_VALUE) {
		fprintf(stdout, "[lexer_check_all_in_folder] could not find base folder: %s\n", base_folder);
		return ERROR_RETURN;
	}
	int err = 0;
	do {
		if (strcmp(foundFile.cFileName, ".") != 0 && strcmp(foundFile.cFileName, "..") != 0) {
			/* Build up our file path using the passed in [sDir] and the file/foldername we just found: */
			sprintf_s(sPath, 2048, "%s/%s", base_folder, foundFile.cFileName);

			/* Is the entity a File or Folder? */
			if (foundFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)	/* Folder */
				internal_lexer_check_all_in_folder(sPath, files_read);
			else														/* File */
				if (lexer_check_file(sPath, foundFile.cFileName)) {
					err++;
					fprintf(stdout, "[lexer_check_all_in_folder] Failed to read file: %s\n\t(at %s)\n", foundFile.cFileName, sPath);
				} else
					*files_read += 1;
		}
	} while (FindNextFile(hFind, &foundFile));
	FindClose(hFind);
	return err;
}
int lexer_check_all_in_folder(const char *base_folder) {
	assert(base_folder && "lexer_check_all_in_folder: base_folder == 0");
	int files_read = 0;
	int files_lost = internal_lexer_check_all_in_folder(base_folder, &files_read);
	if (files_lost) {
		fprintf(stdout, "[lexer_check_all_in_folder] Failed to check all files (%d succeeded, %d failed) in %s\n", files_read, files_lost, base_folder);
		return ERROR_RETURN;
	}
	fprintf(stdout, "[lexer_check_all_in_folder] Successfully checked %d files in %s\n", files_read, base_folder);
	return 0;
}
#include "database_parsing.h"

#include "assert_opt.h"
#include "memory_opt.h"

#include <string.h>
#include <windows.h>

int database_load_all(struct database_t *db) {
	assert(db && "database_load_all: db == 0");
	/* LOAD ORDER:
		- COMMON: trade goods, ideologies, issues, national values, religions, government_types, countries, cultures, country defines
		- MAP: province definitions, default.map (sea starts), states, provinces
		database objects can contain pointers to other database objects ONLY IF they are loaded after the object */
	int err;
	/* ==================== COMMON ==================== */
	/* load trade goods */
	err = read_trade_goods(db, MOD_FOLDER "common/goods.txt"); if_err_ret else db->load_status.common.trade_goods = true;
	/* load ideologies */
	err = read_ideologies(db, MOD_FOLDER "common/ideologies.txt"); if_err_ret else db->load_status.common.ideologies = true;
	/* load issues */
	err = read_issues(db, MOD_FOLDER "common/issues.txt"); if_err_ret else db->load_status.common.issues = true;
	/* load national values */
	err = read_national_values(db, MOD_FOLDER "common/nationalvalues.txt"); if_err_ret else db->load_status.common.national_values = true;
	/* load religions */
	err = read_religions(db, MOD_FOLDER "common/religion.txt"); if_err_ret else db->load_status.common.religions = true;
	/* load government types (requires ideologies) */
	err = read_government_types(db, MOD_FOLDER "common/governments.txt"); if_err_ret else db->load_status.common.government_types = true;

	/* load countries */
	err = read_countries(db, MOD_FOLDER "common/countries.txt"); if_err_ret else db->load_status.common.countries = true;
	/* load cultures (requires countries) */
	err = read_cultures(db, MOD_FOLDER "common/cultures.txt"); if_err_ret else db->load_status.common.cultures = true;
	/* load country defines (requires countries, ideology, issues) */
	err = read_country_defines(db); if_err_ret else db->load_status.common.country_defines = true;

	/* ==================== MAP ==================== */
	/* load province ids/colors */
	err = read_province_defines(db, MOD_FOLDER "map/definition.csv"); if_err_ret else db->load_status.map.province_defines = true;
	/* load default.map (requires province_defines) */
	err = read_sea_starts(db, MOD_FOLDER "map/default.map"); if_err_ret else db->load_status.map.sea_starts = true;
	/* load states (requires province_defines) */
	err = read_states(db, MOD_FOLDER "map/region.txt"); if_err_ret else db->load_status.map.states = true;
	update_province_states(db);
	/* load province shapes (requires province_defines) */
	err = read_province_shapes(db, MOD_FOLDER "map/provinces.bmp"); if_err_ret else db->load_status.map.province_shapes = true;
	
	/* ==================== UNITS ==================== */
	/* load units */
	//err = read_units_folder(db, MOD_FOLDER "units"); if_err_ret else db->load_status.units = true;

	/* ==================== HISTORY ==================== */
	/* load country histories */
	//err = read_country_histories(db, MOD_FOLDER "history/countries"); if_err_ret else db->load_status.history.countries = true;
	/* load province histories */
	err = read_province_histories(db, MOD_FOLDER "history/provinces"); if_err_ret else db->load_status.history.provinces = true;

	return 0;
}


int token_source_get_tag(struct token_source_t *src, struct tag_t *tag, const char *func_name, const char *purpose) {
	assert(src && "token_source_get_tag: src == 0");
	assert(tag && "token_source_get_tag: tag == 0");
	memset(tag, 0, sizeof(struct tag_t));
	struct token_t token = { 0 };
	if (!token_source_next(src, &token)) {
		token_free(&token);
		fprintf(stdout, "[%s] Missing token (expected alphanumeric for tag for %s) [line:%zu]\n", func_name, purpose, src->line_number);
		return ERROR_RETURN;
	}
	if (token.type != ALPHANUMERIC) {
		fprintf(stdout, "[%s] Invalid token (expected alphanumeric for tag for %s): ", func_name, purpose);
		token_print(stdout, &token);
		fprintf(stdout, " [line:%zu]\n", src->line_number);
		token_free(&token);
		return ERROR_RETURN;
	}
	if (token.data.str.length != 3) {
		fprintf(stdout, "[%s] Invalid TAG for %s: %s [line:%zu]\n", func_name, purpose, token.data.str.text, src->line_number);
		token_free(&token);
		return ERROR_RETURN;
	}
	memcpy(tag->text, token.data.str.text, 3);
	token_free(&token);
	if (!tag_valid(tag)) {
		fprintf(stdout, "[%s] Invalid TAG for %s: %s [line:%zu]\n", func_name, purpose, tag->text, src->line_number);
		return ERROR_RETURN;
	}
	return 0;
}
int token_source_get_country(struct token_source_t *src, struct database_t *db, struct country_t **country, const char *func_name, const char *purpose) {
	assert(src && "token_source_get_country: src == 0");
	assert(db && "token_source_get_country: db == 0");
	assert(country && "token_source_get_country: country == 0");
	struct tag_t tag = { 0 };
	if (token_source_get_tag(src, &tag, func_name, purpose)) return ERROR_RETURN;
	*country = database_get_country(db, &tag);
	if (*country == 0) {
		fprintf(stdout, "[%s] Unrecognised TAG for %s: %s [line:%zu]\n", func_name, purpose, tag.text, src->line_number);
		return ERROR_RETURN;
	}
	return 0;
}

boolean lexeme_get_tag(struct lexeme_t *root, struct tag_t *tag) {
	assert(root && "lexeme_get_tag: root == 0");
	assert(tag && "lexeme_get_tag: tag == 0");
	memset(tag, 0, sizeof(struct tag_t));
	if (root->compound || root->values == 0 || buf_len(root->values) != 1 || root->values[0]->key.type != ALPHANUMERIC || root->values[0]->key.data.str.length != 3) {
		fprintf(stdout, "[lexeme_get_tag] Invalid lexeme for TAG\n");
		return false;
	}
	memcpy(tag->text, root->values[0]->key.data.str.text, 3);
	if (!tag_valid(tag)) {
		fprintf(stdout, "[lexeme_get_tag] Invalid TAG: %s\n", tag->text);
		return false;
	}
	return true;
}
boolean lexeme_get_country(struct lexeme_t *root, struct database_t *db, struct country_t **country) {
	assert(root && "lexeme_get_country: root == 0");
	assert(db && "lexeme_get_country: db == 0");
	assert(country && "lexeme_get_country: country == 0");
	struct tag_t tag = { 0 };
	if (!lexeme_get_tag(root, &tag)) return false;
	*country = database_get_country(db, &tag);
	if (*country == 0) {
		fprintf(stdout, "[lexeme_get_country] Unrecognised TAG: %s\n", tag.text);
		return false;
	}
	return true;
}

int read_all_in_folder(struct database_t *db, read_file_func_t read_file_func, const char *base_folder, int *files_read, const char *func_name) {
	WIN32_FIND_DATA foundFile;
	HANDLE hFind = NULL;
	char sPath[2048];
	/* Specify a file mask. *.* = We want everything! */
	sprintf_s(sPath, 2048, "%s/*.*", base_folder);
	if ((hFind = FindFirstFile(sPath, &foundFile)) == INVALID_HANDLE_VALUE) {
		fprintf(stdout, "[%s] could not find base folder: %s\n", func_name, base_folder);
		return ERROR_RETURN;
	}
	int err = 0;
	do {
		if (strcmp(foundFile.cFileName, ".") != 0 && strcmp(foundFile.cFileName, "..") != 0) {
			/* Build up our file path using the passed in [sDir] and the file/foldername we just found: */
			sprintf_s(sPath, 2048, "%s/%s", base_folder, foundFile.cFileName);

			/* Is the entity a File or Folder? */
			if (foundFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)	/* Folder */
				read_all_in_folder(db, read_file_func, sPath, files_read, func_name);
			else														/* File */
				if (read_file_func(db, sPath, foundFile.cFileName)) {
					err++;
					fprintf(stdout, "[%s] Failed to read file: %s\n\t(at %s)\n", func_name, foundFile.cFileName, sPath);
				} else
					*files_read += 1;
		}
	} while (FindNextFile(hFind, &foundFile));
	FindClose(hFind);
	return err;
}

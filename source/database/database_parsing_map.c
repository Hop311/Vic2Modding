#include "database_parsing.h"

#include "assert_opt.h"
#include "memory_opt.h"

/* MAP: province definitions, default.map(sea starts), states, province shapes */

int read_province_defines(struct database_t *db, const char *filename) {
	assert(db && "read_province_defines: db == 0");
	assert(filename && "read_province_defines: filename == 0");
	struct token_source_t src = { 0 };
	int err = token_source_init(&src, filename);
	if_err_ret
	struct token_t token = { 0 };
	while (token_source_next(&src, &token)) {
		if (token.type == INT_TOKEN) {
			if (token.data.i <= 0 || token.data.i >= 1 << 16) {
				fprintf(stdout, "[read_province_defines] Invalid province id (%d). [line:%zu]\n", token.data.i, src.line_number);
				err_break;
			}
			if (database_get_province(db, token.data.i)) {
				fprintf(stdout, "[read_province_defines] Duplicate province id (%d). [line:%zu]\n", token.data.i, src.line_number);
				err_break;
			}
			struct province_t province = { 0 };
			province.id = (u16)token.data.i;
			u8 col[3] = { 0 };
			if (token_source_get_color(&src, col, true, __func__, "province color")) err_break;
			province.color = to_color(col);
			struct province_t *tmp_province = database_get_province_col(db, province.color);
			if (tmp_province) {
				fprintf(stdout, "[read_province_defines] Duplicate province color id %06x for provinces %d and %d. [line:%zu]\n",
					province.color, tmp_province->id, province.id, src.line_number);
				err_break;
			}
			database_add_province(db, &province);
			token_source_clear_line(&src);
		} else {
			token_free(&token);
			token_source_clear_line(&src);
		}
	}
	token_free(&token);
	token_source_free(&src);

	fprintf(stdout, "[read_province_defines] Loaded %zu provinces.\n", buf_len(db->provinces));
	return err;
}

int read_sea_starts(struct database_t *db, const char *filename) {
	assert(db && "read_sea_starts: db == 0");
	assert(filename && "read_sea_starts: filename == 0");
	struct lexeme_t *root_l = lexeme_new();
	if (lexer_process_file(filename, root_l)) {
		lexeme_delete(root_l);
		return ERROR_RETURN;
	}
	int err = 0;
	for_buf(i, root_l->values) {	// for each definition...
		struct lexeme_t *arg_l = root_l->values[i];
		if (arg_l->key.type == ALPHANUMERIC) {
			if (string_equal_c(&arg_l->key.data.str, "max_provinces")) {
				int tmp = 0;
				if (!lexeme_get_int(arg_l, &tmp)) {
					fprintf(stdout, "[read_sea_starts] Could not read max_provinces int\n");
					err = ERROR_RETURN;
				} else {
					if (buf_len(db->provinces) > tmp)
						fprintf(stdout, "[read_sea_starts] Actual province count (%d) exceeds max_provinces (%d).\n",
							(int)buf_len(db->provinces), tmp);
					// TODO PROPERLY USE THIS VALUE?
				}
			} else if (string_equal_c(&arg_l->key.data.str, "sea_starts")) {
				for_buf(j, arg_l->values) {
					struct lexeme_t *prov_l = arg_l->values[j];
					if (prov_l->key.type == INT_TOKEN) {
						struct province_t *prov = database_get_province(db, prov_l->key.data.i);
						if (prov) {
							if (prov->sea_start) fprintf(stdout, "[read_sea_starts] Province %d already has sea_start\n", prov->id);
							else prov->sea_start = true;
						} else {
							fprintf(stdout, "[read_sea_starts] Unrecognised province id: %d\n", prov_l->key.data.i);
							err = ERROR_RETURN;
						}
					} else {
						fprintf(stdout, "[read_sea_starts] Invalid token (expected province id): ");
						token_print(stdout, &prov_l->key);
						fprintf(stdout, "\n");
						err = ERROR_RETURN;
					}
				}
			} else if (string_equal_c(&arg_l->key.data.str, "definitions") || string_equal_c(&arg_l->key.data.str, "provinces") || string_equal_c(&arg_l->key.data.str, "positions")
				|| string_equal_c(&arg_l->key.data.str, "terrain") || string_equal_c(&arg_l->key.data.str, "rivers") || string_equal_c(&arg_l->key.data.str, "terrain_definition")
				|| string_equal_c(&arg_l->key.data.str, "tree_definition") || string_equal_c(&arg_l->key.data.str, "continent") || string_equal_c(&arg_l->key.data.str, "adjacencies")
				|| string_equal_c(&arg_l->key.data.str, "region") || string_equal_c(&arg_l->key.data.str, "region_sea") || string_equal_c(&arg_l->key.data.str, "province_flag_sprite")) {
				// TODO WHAT TO DO WITH THESE FILE LOCS?
			} else if (string_equal_c(&arg_l->key.data.str, "border_heights") || string_equal_c(&arg_l->key.data.str, "terrain_sheet_heights")) {
				// TODO WHAT TO DO WITH THESE VALUE?
			} else if (string_equal_c(&arg_l->key.data.str, "tree")) {
				// TODO WHAT TO DO WITH THIS VALUE?
			} else if (string_equal_c(&arg_l->key.data.str, "border_cutoff")) {
				// TODO WHAT TO DO WITH THIS VALUE?
			} else {
				fprintf(stdout, "[read_sea_starts] Unknown alphanumeric %s in default.map.\n", arg_l->key.data.str.text);
				err = ERROR_RETURN;
			}
		} else {
			fprintf(stdout, "[read_sea_starts] Invalid token (expected alphanumeric): ");
			token_print(stdout, &arg_l->key);
			fprintf(stdout, "\n");
			err = ERROR_RETURN;
		}
	}
	lexeme_delete(root_l);
	if_err_ret

	db->land_province_count = 0;
	db->sea_province_count = 0;
	for (int i = 0; i < buf_len(db->provinces); ++i) {
		if (db->provinces[i].sea_start) db->sea_province_count++;
		else db->land_province_count++;
	}
	fprintf(stdout, "[read_provinces] %zu (%f%%) land provinces and %zu (%f%%) sea provinces.\n",
		db->land_province_count, 100.0f * (float)db->land_province_count / (float)buf_len(db->provinces),
		db->sea_province_count, 100.0f * (float)db->sea_province_count / (float)buf_len(db->provinces));
	return err;
}

void update_province_states(struct database_t *db) {
	assert(db && "update_province_states: db == 0");
	assert(db->load_status.map.states && db->load_status.map.province_defines && "[update_province_states] cannot update provinces' state pointers before both are all loaded");
	for (int i = 0; i < buf_len(db->provinces); ++i)
		db->provinces[i].state = 0;
	for (int i = 0; i < buf_len(db->states); ++i)
		for (int j = 0; j < buf_len(db->states[i].provinces); ++j)
			db->states[i].provinces[j]->state = &db->states[i];
}
int read_states(struct database_t *db, const char *filename) {
	assert(db && "read_states: db == 0");
	assert(filename && "read_states: filename == 0");
	struct lexeme_t *root_l = lexeme_new();
	if (lexer_process_file(filename, root_l)) {
		lexeme_delete(root_l);
		return ERROR_RETURN;
	}
	int err = 0;
	for_buf(i, root_l->values) {	// for each state...
		struct lexeme_t *state_l = root_l->values[i];
		if (state_l->key.type == ALPHANUMERIC) {
			if (database_get_state(db, &state_l->key.data.str)) {
				fprintf(stdout, "[read_states] Duplicate state id (%s).\n", state_l->key.data.str.text);
				err = ERROR_RETURN;
			} else {
				struct state_t new_state = { 0 };
				string_set(&new_state.name, &state_l->key.data.str);
				for_buf(j, state_l->values) {
					struct lexeme_t *prov_l = state_l->values[j];
					if (prov_l->key.type == INT_TOKEN) {
						struct province_t *prov = database_get_province(db, prov_l->key.data.i);
						if (prov) {
							if (state_contains_province(&new_state, prov)) fprintf(stdout, "[read_states] Duplicate province id (%d) in state %s.\n",
								prov->id, new_state.name.text);
							else state_add_province(&new_state, prov);
						} else {
							fprintf(stdout, "[read_states] Invalid province id (%d) for state %s.\n", prov_l->key.data.i, new_state.name.text);
							err = ERROR_RETURN;
						}
					} else {
						fprintf(stdout, "[read_states] Invalid token (expected province id): ");
						token_print(stdout, &prov_l->key);
						fprintf(stdout, "\n");
						err = ERROR_RETURN;
					}
				}
				database_add_state(db, &new_state);
			}
		} else {
			fprintf(stdout, "[read_states] Invalid token (expected state id): ");
			token_print(stdout, &state_l->key);
			fprintf(stdout, "\n");
			err = ERROR_RETURN;
		}
	}
	lexeme_delete(root_l);

	fprintf(stdout, "[read_states] Loaded %zu states.\n", buf_len(db->states));

	return err;
}

int read_province_shapes(struct database_t *db, const char *filename) {
	assert(db && "read_province_shapes: db == 0");
	assert(filename && "read_province_shapes: filename == 0");
	int err = RB_load_image_bmp(&db->map.province_col, filename);
	if_err_ret
	db->map.width = db->map.province_col.width;
	db->map.height = db->map.province_col.height;
	db->map.size = db->map.province_col.size;
	RB_alloc_resize_pixels(&db->map.province_id, db->map.width, db->map.height);
	int left = 0;
	for (int i = 0; i < db->map.size; ++i) {
		u32 col = db->map.province_col.pixels[i];
		if (i >= db->map.width && db->map.province_col.pixels[i - db->map.width] == col)
			db->map.province_id.pixels[i] = db->map.province_id.pixels[i - db->map.width];
		else if ((i % db->map.width) != 0 && db->map.province_col.pixels[i - 1] == col)
			db->map.province_id.pixels[i] = db->map.province_id.pixels[i - 1];
		else {
			struct province_t *prov = database_get_province_col(db, col);
			db->map.province_id.pixels[i] = prov ? prov->id : 0xFF0000;
		}
	}
	return 0;
}


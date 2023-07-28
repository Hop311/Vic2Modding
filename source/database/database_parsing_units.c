#include "database_parsing.h"

#include "assert_opt.h"
#include "memory_opt.h"

void read_trade_good_list(struct database_t *db, double **list, struct lexeme_t *lex) {
	assert(db && "read_trade_good_list: db == 0");
	assert(list && "read_trade_good_list: list == 0");
	assert(lex && "read_trade_good_list: lex == 0");
	if (!lex->compound || lex->values == 0 || buf_len(lex->values) == 0) {
		fprintf(stdout, "[read_trade_good_list] Empty trade good list");
		return;
	}
	if (*list == 0) *list = create_trade_good_list(db);
	for_buf(i, lex->values) {
		struct lexeme_t *good_l = lex->values[i];
		if (good_l->key.type != ALPHANUMERIC || good_l->compound) {
			fprintf(stdout, "[read_trade_good_list] Invalid trade good: ");
			token_print(stdout, &good_l->key);
			fprintf(stdout, "\n");
		} else {
			const struct trade_good_t *good = database_get_trade_good(db, &good_l->key.data.str);
			if (good) {
				double tmp = 0.0;
				if (!lexeme_get_int_or_decimal(good_l, &tmp))
					fprintf(stdout, "[read_trade_good_list] Invalid trade good (%s) amount (must be int or decimal)\n", good->name.text);
				else add_to_trade_good_list(db, *list, good, tmp, true);
			} else fprintf(stdout, "[read_trade_good_list] Invalid trade good: %s\n", good_l->key.data.str.text);
		}
	}
}

int read_unit(struct database_t *db, const char *filepath, const char *filename) {
	assert(db && "read_unit: db == 0");
	assert(filename && "read_unit: filename == 0");
	struct lexeme_t *root_l = lexeme_new();
	if (lexer_process_file(filepath, root_l)) {
		lexeme_delete(root_l);
		return ERROR_RETURN;
	}
	int err = 0;
	for_buf(i, root_l->values) {	// for each unit...
		struct lexeme_t *unit_l = root_l->values[i];
		if (lexeme_is_named_group(unit_l)) {
			if (database_get_unit(db, &unit_l->key.data.str)) {
				fprintf(stdout, "[read_unit] Duplicate unit with name: %s\n", unit_l->key.data.str.text);
				err = ERROR_RETURN;
			} else {
				struct unit_t unit = { 0 }; //unit_default();
				string_set(&unit.name, &unit_l->key.data.str);
				for_buf(j, unit_l->values) {	// for each unit arg...
					struct lexeme_t *arg_l = unit_l->values[j];
					if (arg_l->key.type == ALPHANUMERIC) {
						if (string_equal_c(&arg_l->key.data.str, "icon")) {
							int tmp = 0;
							if (!lexeme_get_int(arg_l, &tmp)) {
								fprintf(stdout, "[read_unit] Icon for %s is not an integer\n", unit.name.text);
								err = ERROR_RETURN;
							} else unit.icon = tmp;
						} else if (string_equal_c(&arg_l->key.data.str, "naval_icon")) {
							int tmp = 0;
							if (!lexeme_get_int(arg_l, &tmp)) {
								fprintf(stdout, "[read_unit] Naval icon for %s is not an integer\n", unit.name.text);
								err = ERROR_RETURN;
							} else unit.naval_icon = tmp;
						} else if (string_equal_c(&arg_l->key.data.str, "type")) {
							string tmp = { 0 };
							if (!lexeme_get_alphanumeric(arg_l, &tmp)) {
								fprintf(stdout, "[read_unit] Could not read terrain type alphanumeric for %s\n", unit.name.text);
								err = ERROR_RETURN;
							} else {
								enum unit_terrain_type_t tt;
								for (tt = 0; tt < UNIT_TERRAIN_TYPE_COUNT && !string_equal_c(&tmp, unit_terrain_type_strings[tt]); ++tt);
								if (tt == UNIT_TERRAIN_TYPE_COUNT) {
									fprintf(stdout, "[read_unit] Unknown terrain type for %s: %s\n",
										unit.name.text, tmp.text);
									err = ERROR_RETURN;
								} else unit.type = tt;
							}
							string_clear(&tmp);
						} else if (string_equal_c(&arg_l->key.data.str, "unit_type")) {
							string tmp = { 0 };
							if (!lexeme_get_alphanumeric(arg_l, &tmp)) {
								fprintf(stdout, "[read_unit] Could not read unit type alphanumeric for %s\n", unit.name.text);
								err = ERROR_RETURN;
							} else {
								enum unit_type_t ut;
								for (ut = 0; ut < UNIT_TYPE_COUNT && !string_equal_c(&tmp, unit_type_strings[ut]); ++ut);
								if (ut == UNIT_TYPE_COUNT) {
									fprintf(stdout, "[read_unit] Unknown unit type for %s: %s\n",
										unit.name.text, tmp.text);
									err = ERROR_RETURN;
								} else unit.unit_type = ut;
							}
							string_clear(&tmp);
						} else if (string_equal_c(&arg_l->key.data.str, "sprite")) {
							if (!lexeme_get_alphanumeric(arg_l, &unit.sprite)) {
								fprintf(stdout, "[read_unit] Could not read sprite alphanumeric for %s\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "move_sound")) {
							if (!lexeme_get_alphanumeric(arg_l, &unit.move_sound)) {
								fprintf(stdout, "[read_unit] Could not read move_sound alphanumeric for %s\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "select_sound")) {
							if (!lexeme_get_alphanumeric(arg_l, &unit.select_sound)) {
								fprintf(stdout, "[read_unit] Could not read select_sound alphanumeric for %s\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "sprite_override")) {
							if (!lexeme_get_alphanumeric(arg_l, &unit.sprite_override)) {
								fprintf(stdout, "[read_unit] Could not read sprite_override alphanumeric for %s\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "sprite_mount")) {
							if (!lexeme_get_alphanumeric(arg_l, &unit.sprite_mount)) {
								fprintf(stdout, "[read_unit] Could not read sprite_mount alphanumeric for %s\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "sprite_mount_attach_node")) {
							if (!lexeme_get_alphanumeric(arg_l, &unit.sprite_mount_attach_node)) {
								fprintf(stdout, "[read_unit] Could not read sprite_mount_attach_node alphanumeric for %s\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "capital")) {
							if (!lexeme_get_bool(arg_l, &unit.capital)) {
								fprintf(stdout, "[read_unit] Could not read capital bool for %s\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "sail")) {
							if (!lexeme_get_bool(arg_l, &unit.sail)) {
								fprintf(stdout, "[read_unit] Could not read sail bool for %s\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "active")) {
							if (!lexeme_get_bool(arg_l, &unit.active)) {
								fprintf(stdout, "[read_unit] Could not read active bool for %s\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "transport")) {
							if (!lexeme_get_bool(arg_l, &unit.transport)) {
								fprintf(stdout, "[read_unit] Could not read transport bool for %s\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "floating_flag")) {
							if (!lexeme_get_bool(arg_l, &unit.floating_flag)) {
								fprintf(stdout, "[read_unit] Could not read floating_flag bool for %s\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "can_build_overseas")) {
							if (!lexeme_get_bool(arg_l, &unit.can_build_overseas)) {
								fprintf(stdout, "[read_unit] Could not read can_build_overseas bool for %s\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "colonial_points")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.colonial_points)) {
								fprintf(stdout, "[read_unit] Could not read colonial points int or decimal for %s\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "priority")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.priority)) {
								fprintf(stdout, "[read_unit] Could not read priority int or decimal for %s\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "max_strength")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.max_strength)) {
								fprintf(stdout, "[read_unit] Could not read max_strength int or decimal for %s\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "default_organisation")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.default_organisation)) {
								fprintf(stdout, "[read_unit] Could not read default_organisation int or decimal for %s\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "maximum_speed")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.maximum_speed)) {
								fprintf(stdout, "[read_unit] Could not read maximum_speed int or decimal for %s\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "weighted_value")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.weighted_value)) {
								fprintf(stdout, "[read_unit] Could not read weighted_value int or decimal for %s\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "build_time")) {
							if (!lexeme_get_int(arg_l, &unit.build_time)) {
								fprintf(stdout, "[read_unit] Could not read build_time int for %s\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "build_cost")) {
							read_trade_good_list(db, &unit.build_cost, arg_l);
						} else if (string_equal_c(&arg_l->key.data.str, "min_port_level")) {
							int tmp = 0;
							if (!lexeme_get_int(arg_l, &tmp)) {
								fprintf(stdout, "[read_unit] Minimum port level for %s is not an integer\n", unit.name.text);
								err = ERROR_RETURN;
							} else unit.min_port_level = tmp;
						} else if (string_equal_c(&arg_l->key.data.str, "limit_per_port")) {
							int tmp = 0;
							if (!lexeme_get_int(arg_l, &tmp)) {
								fprintf(stdout, "[read_unit] Limit per port for %s is not an integer\n", unit.name.text);
								err = ERROR_RETURN;
							} else unit.limit_per_port = tmp;
						} else if (string_equal_c(&arg_l->key.data.str, "supply_consumption_score")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.supply_consumption_score)) {
								fprintf(stdout, "[read_unit] Supply consumption score for %s is not an integer or decimal\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "supply_consumption")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.supply_consumption)) {
								fprintf(stdout, "[read_unit] Supply consumption for %s is not an integer or decimal\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "supply_cost")) {
							read_trade_good_list(db, &unit.supply_cost, arg_l);
						} else if (string_equal_c(&arg_l->key.data.str, "reconnaissance")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.reconnaissance)) {
								fprintf(stdout, "[read_unit] Reconnaissance for %s is not an integer or decimal\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "attack")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.attack)) {
								fprintf(stdout, "[read_unit] Attack for %s is not an integer or decimal\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "defence")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.defence)) {
								fprintf(stdout, "[read_unit] Defence for %s is not an integer or decimal\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "discipline")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.discipline)) {
								fprintf(stdout, "[read_unit] Discipline for %s is not an integer or decimal\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "support")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.support)) {
								fprintf(stdout, "[read_unit] Support for %s is not an integer or decimal\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "maneuver")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.maneuver)) {
								fprintf(stdout, "[read_unit] Maneuver for %s is not an integer or decimal\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "siege")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.siege)) {
								fprintf(stdout, "[read_unit] Siege for %s is not an integer or decimal\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "hull")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.hull)) {
								fprintf(stdout, "[read_unit] Hull for %s is not an integer or decimal\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "gun_power")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.gun_power)) {
								fprintf(stdout, "[read_unit] Gun power for %s is not an integer or decimal\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "fire_range")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.fire_range)) {
								fprintf(stdout, "[read_unit] Fire range for %s is not an integer or decimal\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "evasion")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.evasion)) {
								fprintf(stdout, "[read_unit] Evasion for %s is not an integer or decimal\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "torpedo_attack")) {
							if (!lexeme_get_int_or_decimal(arg_l, &unit.torpedo_attack)) {
								fprintf(stdout, "[read_unit] Torpedo attack for %s is not an integer or decimal\n", unit.name.text);
								err = ERROR_RETURN;
							}
						} else {
							fprintf(stdout, "[read_unit] Unrecognised unit specification for %s: %s\n", unit.name.text, arg_l->key.data.str.text);
							err = ERROR_RETURN;
						}
					} else {
						fprintf(stdout, "[read_unit] Invalid token (expected alphanumeric unit definition for %s): ", unit.name.text);
						token_print(stdout, &arg_l->key);
						fprintf(stdout, "\n");
						err = ERROR_RETURN;
					}
				}
				database_add_unit(db, &unit);
			}
		} else {
			fprintf(stdout, "[read_unit] Invalid token (expected alphanumeric unit definition): ");
			token_print(stdout, &unit_l->key);
			fprintf(stdout, "\n");
			err = ERROR_RETURN;
		}
	}
	lexeme_delete(root_l);
	return err;
}
int read_units_folder(struct database_t *db, const char *base_folder) {
	assert(db && "read_units_folder: db == 0");
	assert(base_folder && "read_units_folder: base_folder == 0");
	int files_read = 0;
	if (read_all_in_folder(db, read_unit, base_folder, &files_read, __func__)) {
		fprintf(stdout, "[read_units_folder] Failed to read all units (%d)\n", files_read);
		return ERROR_RETURN;
	}
	fprintf(stdout, "[read_units_folder] Successfully read %d units.\n", files_read);
	return 0;
}

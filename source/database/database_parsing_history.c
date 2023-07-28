#include "database_parsing.h"

#include "assert_opt.h"
#include "memory_opt.h"

#include <string.h>

/* PROVINCE HISTORY (filename can be left 0 and it will be automatically extracted) */
int read_province_history(struct database_t *db, const char *filepath, const char *filename) {
	assert(db && "read_province_history: db == 0");
	assert(filepath && "read_province_history: filepath == 0");
	assert(filepath[0] && "read_province_history: filepath[0] == 0");
	if (filename == 0) {
		int start_pos = (int)strlen(filepath);
		while (start_pos > 0 && filepath[--start_pos] != '/'); /* now filepath[start_pos] should be on the '/' or at 0 */
		if (filepath[start_pos] == '/') start_pos++;
		filename = &filepath[start_pos];
	}
	int pos = -1;
	while (filename[++pos] && is_number(filename[pos])); /* now filename[pos] is \0 or a non-number */
	if (pos < 1) {
		fprintf(stdout, "[read_province_history] province history file missing province id: %s\n", filename);
		return ERROR_RETURN;
	}
	string tmp = { 0 };
	string_extract(&tmp, filename, pos);
	assert(tmp.text && "read_province_history: tmp.text == 0");
	const int prov_id = atoi(tmp.text);
	if (prov_id < 1) {
		fprintf(stdout, "[read_province_history] province history file invalid province id %s (%d) (for %s)\n", tmp.text, prov_id, filename);
		return ERROR_RETURN;
	}
	string_clear(&tmp);
	struct province_t *prov = database_get_province(db, prov_id);
	if (prov == 0) {
		fprintf(stdout, "[read_province_history] province history file could not find province by id (%d) for %s\n", prov_id, filename);
		return ERROR_RETURN;
	}

	/* DEBUGGING */
	if (prov->history_defined) fprintf(stdout, "[read_province_history] province %d already defined, now trying again with %s\n", prov_id, filename);
	else {
		prov->history_defined = true;
		if (prov->sea_start) fprintf(stdout, "[read_province_history] province %d (a sea tile) is being defined by %s\n", prov_id, filename);
	}

	struct lexeme_t *root_l = lexeme_new();
	if (lexer_process_file(filepath, root_l)) {
		lexeme_delete(root_l);
		return ERROR_RETURN;
	}
	int err = 0;
	for_buf(i, root_l->values) {	// for each definition...
		struct lexeme_t *arg_l = root_l->values[i];
		if (arg_l->key.type == ALPHANUMERIC) {
			if (string_equal_c(&arg_l->key.data.str, "owner")) {
				if (!lexeme_get_country(arg_l, db, &prov->owner)) {
					fprintf(stdout, "[read_province_history] Could not read owner TAG for province %d\n", prov->id);
					err = ERROR_RETURN;
				}
			} else if (string_equal_c(&arg_l->key.data.str, "controller")) {
				if (!lexeme_get_country(arg_l, db, &prov->controller)) {
					fprintf(stdout, "[read_province_history] Could not read controller TAG for province %d\n", prov->id);
					err = ERROR_RETURN;
				}
			} else if (string_equal_c(&arg_l->key.data.str, "add_core")) {
				struct country_t *country = 0;
				if (!lexeme_get_country(arg_l, db, &country)) {
					fprintf(stdout, "[read_province_history] Could not read add_core TAG for province %d\n", prov->id);
					err = ERROR_RETURN;
				} else province_add_core(prov, country);
			} else if (string_equal_c(&arg_l->key.data.str, "trade_goods")) {
				string tmp = { 0 };
				if (!lexeme_get_alphanumeric(arg_l, &tmp)) {
					fprintf(stdout, "[read_province_history] Could not read trade_goods alphanumeric for province %d\n", prov->id);
					err = ERROR_RETURN;
				} else {
					struct trade_good_t *good = database_get_trade_good(db, &tmp);
					if (good) {
						if (prov->rgo) fprintf(stdout, "[read_province_history] replacing rgo in province %d (%s to %s)\n",
							prov->id, prov->rgo->name.text, good->name.text);
						prov->rgo = good;
					} else
						fprintf(stdout, "[read_province_history] unrecognised rgo for province %d: %s\n",
							prov->id, tmp.text);
				}
				string_clear(&tmp);
			} else if (string_equal_c(&arg_l->key.data.str, "life_rating")) {
				int tmp = 0;
				if (!lexeme_get_int(arg_l, &tmp)) {
					fprintf(stdout, "[read_province_history] Could not read life_rating int for province %d\n", prov->id);
					err = ERROR_RETURN;
				} else prov->life_rating = tmp;
			} else if (string_equal_c(&arg_l->key.data.str, "railroad")) {
				int tmp = 0;
				if (!lexeme_get_int(arg_l, &tmp)) {
					fprintf(stdout, "[read_province_history] Could not read railroad int for province %d\n", prov->id);
					err = ERROR_RETURN;
				} else prov->railroad = tmp;
			} else if (string_equal_c(&arg_l->key.data.str, "naval_base")) {
				int tmp = 0;
				if (!lexeme_get_int(arg_l, &tmp)) {
					fprintf(stdout, "[read_province_history] Could not read naval_base int for province %d\n", prov->id);
					err = ERROR_RETURN;
				} else prov->naval_base = tmp;
			} else if (string_equal_c(&arg_l->key.data.str, "fort")) {
				int tmp = 0;
				if (!lexeme_get_int(arg_l, &tmp)) {
					fprintf(stdout, "[read_province_history] Could not read fort int for province %d\n", prov->id);
					err = ERROR_RETURN;
				} else prov->fort = tmp;
			} else if (string_equal_c(&arg_l->key.data.str, "colonial") || string_equal_c(&arg_l->key.data.str, "colony")) {
				int tmp = 0;
				if (!lexeme_get_int(arg_l, &tmp)) {
					fprintf(stdout, "[read_province_history] Could not read colonial int for province %d\n", prov->id);
					err = ERROR_RETURN;
				} else prov->colonial = tmp;
			} else if (string_equal_c(&arg_l->key.data.str, "set_province_flag")) {
				string tmp = { 0 };
				if (!lexeme_get_alphanumeric(arg_l, &tmp)) {
					fprintf(stdout, "[read_province_history] Could not read province flag alphanumeric for province %d\n", prov->id);
					err = ERROR_RETURN;
				} else {
					if (province_has_flag(prov, &tmp)) {
						fprintf(stdout, "[read_province_history] Repeated province flag %s for province %d\n", tmp.text, prov->id);
						string_clear(&tmp);
						err = ERROR_RETURN;
					} else buf_push(prov->flags, tmp);
				}
			} else if (string_equal_c(&arg_l->key.data.str, "state_building")) {
				// TODO WHAT TO DO WITH BUILDINGS ????
			} else if (string_equal_c(&arg_l->key.data.str, "party_loyalty")) {
				// TODO WHAT TO DO WITH LOYALTY ????
			} else if (string_equal_c(&arg_l->key.data.str, "is_slave")) {
				// TODO WHAT TO DO WITH IS_SLAVE ????
			} else if (string_equal_c(&arg_l->key.data.str, "terrain")) {
				// TODO WHAT TO DO WITH TERRAIN ????
			} else {
				fprintf(stdout, "[read_province_history] Unrecognised alphanumeric in definition of %d: %s\n",
					prov->id, arg_l->key.data.str.text);
				err_break;
			}
		} else if (arg_l->key.type == DATE_TOKEN) {
			// TODO proper date block reading
			if (!(date_equal(&arg_l->key.data.date, &ACW_START_DATE) || date_equal(&arg_l->key.data.date, &DOMINIONS_START_DATE))) {
				fprintf(stdout, "[read_province_history] Non ACW/Dominion start date for province %d: ", prov->id);
				date_print(stdout, &arg_l->key.data.date);
				fprintf(stdout, "\n");
			}
		} else {
			fprintf(stdout, "[read_province_history] Invalid token (expected alphanumeric for definition of %d): ", prov->id);
			token_print(stdout, &arg_l->key);
			fprintf(stdout, "\n");
			err_break;
		}
	}
	lexeme_delete(root_l);
	return err;
}
int read_province_histories(struct database_t *db, const char *base_folder) {
	assert(db && "read_province_histories: db == 0");
	assert(base_folder && "read_province_histories: base_folder == 0");
	int files_read = 0;
	if (read_all_in_folder(db, read_province_history, base_folder, &files_read, __func__)) {
		fprintf(stdout, "[read_province_histories] Failed to read all province histories (%d/%d)\n", files_read, (int)db->land_province_count);
		return ERROR_RETURN;
	}
	fprintf(stdout, "[read_province_histories] Successfully read province histories for %d/%d provinces\n", files_read, (int)db->land_province_count);
	return 0;
}

/* COUNTRY HISTORY (filename can be left 0 and it will be automatically extracted) */
int read_country_history(struct database_t *db, const char *filepath, const char *filename) {
	assert(db && "read_country_history: db == 0");
	assert(filepath && "read_country_history: filepath == 0");
	assert(filepath[0] && "read_country_history: filepath[0] == 0");
	if (filename == 0) {
		int start_pos = (int)strlen(filepath);
		while (start_pos > 0 && filepath[--start_pos] != '/'); /* now filepath[start_pos] should be on the '/' or at 0 */
		if (filepath[start_pos] == '/') start_pos++;
		filename = &filepath[start_pos];
	}
	if (filename[0] == 0 || filename[1] == 0 || filename[2] == 0) {
		fprintf(stdout, "[read_country_history] country history file missing tag: %s\n", filename);
		return ERROR_RETURN;
	}
	struct tag_t tag = { 0 };
	memcpy(tag.text, filename, 3);
	if (!tag_valid(&tag)) {
		fprintf(stdout, "[read_country_history] country history file invalid tag %s (for %s)\n", tag.text, filename);
		return ERROR_RETURN;
	}
	struct country_t *country = database_get_country(db, &tag);
	if (country == 0) {
		fprintf(stdout, "[read_country_history] could not find country with tag %s for %s\n", tag.text, filename);
		return ERROR_RETURN;
	}
	/* DEBUGGING */
	if (country->history_defined) fprintf(stdout, "[read_country_history] country %s already defined, now trying again with %s\n", country->tag.text, filename);
	else country->history_defined = true;

	struct lexeme_t *root_l = lexeme_new();
	if (lexer_process_file(filepath, root_l)) {
		lexeme_delete(root_l);
		return ERROR_RETURN;
	}
	int err = 0;
	for_buf(i, root_l->values) {	// for each definition...
		struct lexeme_t *arg_l = root_l->values[i];
		if (arg_l->key.type == ALPHANUMERIC) {
			if (string_equal_c(&arg_l->key.data.str, "capital")) {
				int tmp = 0;
				if (!lexeme_get_int(arg_l, &tmp)) {
					fprintf(stdout, "[read_country_history] Could not read province ID for capital of %s\n", country->tag.text);
					err = ERROR_RETURN;
				} else {
					struct province_t *prov = database_get_province(db, tmp);
					if (prov) {
						if (country->capital) fprintf(stdout, "[read_country_history] changing %s's capital from %d to %d\n",
							country->tag.text, country->capital->id, prov->id);
						country->capital = prov;
					} else {
						fprintf(stdout, "[read_country_history] Unrecognised province %d for %s's capital\n",
							prov->id, country->tag.text);
						err = ERROR_RETURN;
					}
				}
			} else if (string_equal_c(&arg_l->key.data.str, "primary_culture")) {
				string tmp = { 0 };
				if (!lexeme_get_alphanumeric(arg_l, &tmp)) {
					fprintf(stdout, "[read_country_history] Could not read alphanumeric for primary culture of %s\n", country->tag.text);
					err = ERROR_RETURN;
				} else {
					struct culture_t *culture = database_get_culture(db, &tmp);
					if (culture) {
						string_clear(&tmp);
						if (country->primary_culture) fprintf(stdout, "[read_country_history] changing %s's primary culture from %s to %s\n",
							country->tag.text, country->primary_culture->name.text, culture->name.text);
						country->primary_culture = culture;
					} else {
						fprintf(stdout, "[read_country_history] Unrecognised primary culture %s for %s\n",
							tmp.text, country->tag.text);
						string_clear(&tmp);
						err = ERROR_RETURN;
					}
				}
			} else if (string_equal_c(&arg_l->key.data.str, "culture")) {
				string tmp = { 0 };
				if (!lexeme_get_alphanumeric(arg_l, &tmp)) {
					fprintf(stdout, "[read_country_history] Could not read alphanumeric for accepted culture of %s\n", country->tag.text);
					err = ERROR_RETURN;
				} else {
					struct culture_t *culture = database_get_culture(db, &tmp);
					if (culture) {
						string_clear(&tmp);
						if (country_has_accepted(country, culture)) fprintf(stdout, "[read_country_history] Adding %s as an accepted culture for %s again\n",
							culture->name.text, country->tag.text);
						else country_add_accepted_culture(country, culture);
					} else {
						fprintf(stdout, "[read_country_history] Unrecognised accepted culture %s for %s\n",
							tmp.text, country->tag.text);
						string_clear(&tmp);
						err = ERROR_RETURN;
					}
				}
			} else if (string_equal_c(&arg_l->key.data.str, "religion")) {
				string tmp = { 0 };
				if (!lexeme_get_alphanumeric(arg_l, &tmp)) {
					fprintf(stdout, "[read_country_history] Could not read alphanumeric for religion of %s\n", country->tag.text);
					err = ERROR_RETURN;
				} else {
					struct religion_t *religion = database_get_religion(db, &tmp);
					if (religion) {
						string_clear(&tmp);
						if (country->religion) fprintf(stdout, "[read_country_history] changing %s's religion from %s to %s\n",
							country->tag.text, country->religion->name.text, religion->name.text);
						country->religion = religion;
					} else {
						fprintf(stdout, "[read_country_history] Unrecognised religion %s for %s\n",
							tmp.text, country->tag.text);
						string_clear(&tmp);
						err = ERROR_RETURN;
					}
				}
			} else if (string_equal_c(&arg_l->key.data.str, "government")) {
				string tmp = { 0 };
				if (!lexeme_get_alphanumeric(arg_l, &tmp)) {
					fprintf(stdout, "[read_country_history] Could not read alphanumeric for government type of %s\n", country->tag.text);
					err = ERROR_RETURN;
				} else {
					struct government_type_t *gov = database_get_government_type(db, &tmp);
					if (gov) {
						string_clear(&tmp);
						if (country->government) fprintf(stdout, "[read_country_history] changing %s's government type from %s to %s\n",
							country->tag.text, country->government->name.text, gov->name.text);
						country->government = gov;
					} else {
						fprintf(stdout, "[read_country_history] Unrecognised government type %s for %s\n",
							tmp.text, country->tag.text);
						string_clear(&tmp);
						err = ERROR_RETURN;
					}
				}
			} else if (string_equal_c(&arg_l->key.data.str, "plurality")) {
				if (!lexeme_get_int_or_decimal(arg_l, &country->plurality)) {
					fprintf(stdout, "[read_country_history] Could not read plurality int or decimal for %s\n", country->tag.text);
					err = ERROR_RETURN;
				}
			} else if (string_equal_c(&arg_l->key.data.str, "nationalvalue")) {
				string tmp = { 0 };
				if (!lexeme_get_alphanumeric(arg_l, &tmp)) {
					fprintf(stdout, "[read_country_history] Could not read national value alphanumeric for %s\n", country->tag.text);
					err = ERROR_RETURN;
				} else {
					struct national_value_t *nv = database_get_national_value(db, &tmp);
					if (nv) {
						string_clear(&tmp);
						if (country->nv) fprintf(stdout, "[read_country_history] changing %s's national value from %s to %s\n",
							country->tag.text, country->nv->name.text, nv->name.text);
						country->nv = nv;
					} else {
						fprintf(stdout, "[read_country_history] Unrecognised national value %s for %s\n",
							tmp.text, country->tag.text);
						string_clear(&tmp);
						err = ERROR_RETURN;
					}
				}
			} else if (string_equal_c(&arg_l->key.data.str, "literacy")) {
				if (!lexeme_get_decimal(arg_l, &country->literacy)) {
					fprintf(stdout, "[read_country_history] Could not read literacy decimal for %s\n", country->tag.text);
					err = ERROR_RETURN;
				}
			} else if (string_equal_c(&arg_l->key.data.str, "non_state_culture_literacy")) {
				if (!lexeme_get_decimal(arg_l, &country->non_state_culture_literacy)) {
					fprintf(stdout, "[read_country_history] Could not read non_state_culture_literacy decimal for %s\n", country->tag.text);
					err = ERROR_RETURN;
				}
			} else if (string_equal_c(&arg_l->key.data.str, "civilized")) {
				if (!lexeme_get_bool(arg_l, &country->civilized)) {
					fprintf(stdout, "[read_country_history] Could not read civilized bool for %s\n", country->tag.text);
					err = ERROR_RETURN;
				}
			} else if (string_equal_c(&arg_l->key.data.str, "is_releasable_vassal")) {
				if (!lexeme_get_bool(arg_l, &country->is_releasable_vassal)) {
					fprintf(stdout, "[read_country_history] Could not read is_releasable_vassal bool for %s\n", country->tag.text);
					err = ERROR_RETURN;
				}
			} else if (string_equal_c(&arg_l->key.data.str, "prestige")) {
				if (!lexeme_get_int_or_decimal(arg_l, &country->prestige)) {
					fprintf(stdout, "[read_country_history] Could not read prestige int or decimal for %s\n", country->tag.text);
					err = ERROR_RETURN;
				}
			} else if (string_equal_c(&arg_l->key.data.str, "set_country_flag")) {
				string tmp = { 0 };
				if (!lexeme_get_alphanumeric(arg_l, &tmp)) {
					fprintf(stdout, "[read_country_history] Could not read country flag alphanumeric for %s\n", country->tag.text);
					err = ERROR_RETURN;
				} else {
					if (country_has_flag(country, &tmp)) {
						fprintf(stdout, "[read_country_history] Repeated country flag %s for %s\n", tmp.text, country->tag.text);
						string_clear(&tmp);
						err = ERROR_RETURN;
					} else buf_push(country->flags, tmp);
				}
			} else if (string_equal_c(&arg_l->key.data.str, "ruling_party")) {
				string tmp = { 0 };
				if (!lexeme_get_alphanumeric(arg_l, &tmp)) {
					fprintf(stdout, "[read_country_history] Could not read ruling_party alphanumeric for %s\n", country->tag.text);
					err = ERROR_RETURN;
				} else {
					struct party_t *party = country_get_party(country, &tmp);
					if (party) {
						string_clear(&tmp);
						if (country->ruling_party) fprintf(stdout, "[read_country_history] changing %s's ruling party from %s to %s\n",
							country->tag.text, country->ruling_party->name.text, party->name.text);
						country->ruling_party = party;
					} else {
						fprintf(stdout, "[read_country_history] Unrecognised party %s for %s\n",
							tmp.text, country->tag.text);
						err = ERROR_RETURN;
					}
				}
			} else if (string_equal_c(&arg_l->key.data.str, "upper_house")) {
				size_t ideologies_left = buf_len(db->ideologies);
				for_buf(j, arg_l->values) {
					struct lexeme_t *ideo_l = arg_l->values[j];
					if (ideo_l->key.type == ALPHANUMERIC && !ideo_l->compound) {
						const struct ideology_t *ideo = database_get_ideology(db, &ideo_l->key.data.str);
						if (ideo) {
							const size_t idx = database_ideology_index(db, ideo);
							if (country->upper_house == 0) country_upper_house_init(db, country);
							if (!lexeme_get_int_or_decimal(ideo_l, &country->upper_house[idx])) {
								fprintf(stdout, "[read_country_history] Could not read upper house %s count for %s\n", ideo->name.text, country->tag.text);
								err = ERROR_RETURN;
							} else ideologies_left--;
						} else {
							fprintf(stdout, "[read_country_history] Invalid upper house ideology for %s: %s\n", country->tag.text, ideo_l->key.data.str.text);
							err = ERROR_RETURN;
						}
					} else {
						fprintf(stdout, "[read_country_history] Invalid upper house ideology for %s: ", country->tag.text);
						token_print(stdout, &ideo_l->key);
						fprintf(stdout, "\n");
						err = ERROR_RETURN;
					}
				}
				if (ideologies_left) {
					fprintf(stdout, "[read_country_history] Upper house for %s missing %zu ideologies\n", country->tag.text, ideologies_left);
					err = ERROR_RETURN;
				}
			} else if (string_equal_c(&arg_l->key.data.str, "consciousness")) {
				if (!lexeme_get_int_or_decimal(arg_l, &country->consciousness)) {
					fprintf(stdout, "[read_province_history] Could not read consciousness int or decimal for %s\n", country->tag.text);
					err = ERROR_RETURN;
				}
			} else if (string_equal_c(&arg_l->key.data.str, "nonstate_consciousness")) {
				if (!lexeme_get_int_or_decimal(arg_l, &country->nonstate_consciousness)) {
					fprintf(stdout, "[read_province_history] Could not read nonstate_consciousness int or decimal for %s\n", country->tag.text);
					err = ERROR_RETURN;
				}
			} else if (string_equal_c(&arg_l->key.data.str, "last_election")) {
				if (!lexeme_get_date(arg_l, &country->last_election)) {
					fprintf(stdout, "[read_province_history] Could not read last_election date for %s\n", country->tag.text);
					err = ERROR_RETURN;
				}
			} else if (string_equal_c(&arg_l->key.data.str, "oob")) {
				if (!lexeme_get_string(arg_l, &country->oob_location)) {
					fprintf(stdout, "[read_province_history] Could not read oob string for %s\n", country->tag.text);
					err = ERROR_RETURN;
				}
			} else {
				struct reform_group_t *rg = database_get_reform_group(db, &arg_l->key.data.str);
				if (rg) {
					string tmp = { 0 };
					if (!lexeme_get_alphanumeric(arg_l, &tmp)) {
						fprintf(stdout, "[read_province_history] Could not read %s reform for %s\n", rg->name.text, country->tag.text);
						err = ERROR_RETURN;
					} else {
						struct reform_t *ref = database_get_reform(db, &tmp);
						if (ref) {
							string_clear(&tmp);
							if (ref->group != rg) {
								fprintf(stdout, "[read_country_history] Reform group mismatch in history of %s: %s vs %s\n",
									country->tag.text, rg->name.text, ref->group ? ref->group->name.text : "NO_GROUP");
								err = ERROR_RETURN;
							}
							const size_t index = database_reform_group_index(db, rg);
							if (country->reforms == 0) country_reforms_init(db, country);
							if (country->reforms[index]) fprintf(stdout, "[read_country_history] Changing %s reform from %s to %s for %s\n",
								rg->name.text, country->reforms[index]->name.text, ref->name.text, country->tag.text);
							country->reforms[index] = ref;
						} else {
							fprintf(stdout, "[read_country_history] Unrecognised reform (in group %s) in history of %s: %s\n",
								rg->name.text, country->tag.text, arg_l->key.data.str.text);
							string_clear(&tmp);
							err = ERROR_RETURN;
						}
					}
				} else {
					fprintf(stdout, "[read_country_history] Unrecognised alphanumeric in history of %s: %s\n",
						country->tag.text, arg_l->key.data.str.text);
					err = ERROR_RETURN;
				}
			}
		} else if (arg_l->key.type == DATE_TOKEN) {
			// TODO proper date block reading
			if (!(date_equal(&arg_l->key.data.date, &ACW_START_DATE) || date_equal(&arg_l->key.data.date, &DOMINIONS_START_DATE))) {
				fprintf(stdout, "[read_country_history] Non ACW/Dominion start date for %s: ", country->tag.text);
				date_print(stdout, &arg_l->key.data.date);
				fprintf(stdout, "\n");
			}
		} else {
			fprintf(stdout, "[read_country_history] Invalid token (expected alphanumeric for history of %s): ", country->tag.text);
			token_print(stdout, &arg_l->key);
			fprintf(stdout, "\n");
			err = ERROR_RETURN;
		}
	}
	lexeme_delete(root_l);
	return err;
}
int read_country_histories(struct database_t *db, const char *base_folder) {
	assert(db && "read_country_histories: db == 0");
	assert(base_folder && "read_country_histories: base_folder == 0");
	int files_read = 0;
	if (read_all_in_folder(db, read_country_history, base_folder, &files_read, __func__)) {
		fprintf(stdout, "[read_country_histories] Failed to read all country histories (%d/%d)\n", files_read, (int)buf_len(db->countries));
		return ERROR_RETURN;
	}
	fprintf(stdout, "[read_country_histories] Successfully read histories for %d/%d countries\n", files_read, (int)buf_len(db->countries));
	return 0;
}

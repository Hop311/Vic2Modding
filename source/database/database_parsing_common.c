#include "database_parsing.h"

#include "assert_opt.h"
#include "memory_opt.h"

#include <string.h>

/* COMMON: trade goods, ideologies, national values, religions, government_types, countries, cultures, country defines */

int read_trade_goods(struct database_t *db, const char *filename) {
	assert(db && "read_trade_goods: db == 0");
	assert(filename && "read_trade_goods: filename == 0");
	struct lexeme_t *root_l = lexeme_new();
	if (lexer_process_file(filename, root_l)) {
		lexeme_delete(root_l);
		return ERROR_RETURN;
	}
	int err = 0;
	for_buf(i, root_l->values) {	// for each trade good group...
		struct lexeme_t *group_l = root_l->values[i];
		if (lexeme_is_named_group(group_l)) {
			if (database_get_trade_good_group(db, &group_l->key.data.str)) {
				fprintf(stdout, "[read_trade_goods] Duplicate trade good group with name: %s\n", group_l->key.data.str.text);
				err = ERROR_RETURN;
			} else {
				struct trade_good_group_t group = { 0 };
				string_set(&group.name, &group_l->key.data.str);
				for_buf(j, group_l->values) {	// for each trade good...
					struct lexeme_t *good_l = group_l->values[j];
					if (lexeme_is_named_group(good_l)) {
						if (database_get_trade_good(db, &good_l->key.data.str)) {
							fprintf(stdout, "[read_trade_goods] Duplicate trade good with name: %s\n", good_l->key.data.str.text);
							err = ERROR_RETURN;
						} else {
							struct trade_good_t good = trade_good_default();
							string_set(&good.name, &good_l->key.data.str);
							for_buf(k, good_l->values) {	// for each trade good arg...
								struct lexeme_t *arg_l = good_l->values[k];
								if (arg_l->key.type == ALPHANUMERIC) {
									if (string_equal_c(&arg_l->key.data.str, "cost")) {
										if (!lexeme_get_int_or_decimal(arg_l, &good.cost)) {
											fprintf(stdout, "[read_trade_goods] Cost for %s is not a number\n", good.name.text);
											err = ERROR_RETURN;
										}
									} else if (string_equal_c(&arg_l->key.data.str, "color")) {
										u8 col[3] = { 0 };
										if (lexeme_get_color(arg_l, col))
											good.color = to_color(col);
										else {
											fprintf(stdout, "[read_trade_goods] Could not read color for %s\n", good.name.text);
											err = ERROR_RETURN;
										}
									} else if (string_equal_c(&arg_l->key.data.str, "available_from_start")) {
										if (!lexeme_get_bool(arg_l, &good.available_from_start)) {
											fprintf(stdout, "[read_trade_goods] Could not read available_from_start bool for %s\n", good.name.text);
											err = ERROR_RETURN;
										}
									} else if (string_equal_c(&arg_l->key.data.str, "tradeable")) {
										if (!lexeme_get_bool(arg_l, &good.tradeable)) {
											fprintf(stdout, "[read_trade_goods] Could not read tradeable bool for %s\n", good.name.text);
											err = ERROR_RETURN;
										}
									} else if (string_equal_c(&arg_l->key.data.str, "money")) {
										if (!lexeme_get_bool(arg_l, &good.money)) {
											fprintf(stdout, "[read_trade_goods] Could not read money bool for %s\n", good.name.text);
											err = ERROR_RETURN;
										}
									} else if (string_equal_c(&arg_l->key.data.str, "overseas_penalty")) {
										if (!lexeme_get_bool(arg_l, &good.overseas_penalty)) {
											fprintf(stdout, "[read_trade_goods] Could not read overseas_penalty bool for %s\n", good.name.text);
											err = ERROR_RETURN;
										}
									} else {
										fprintf(stdout, "[read_trade_goods] Unrecognised trade good specification for %s: %s\n", good.name.text, arg_l->key.data.str.text);
										err = ERROR_RETURN;
									}
								} else {
									fprintf(stdout, "[read_trade_goods] Invalid token (expected alphanumeric trade good specifications for %s): ", good.name.text);
									token_print(stdout, &arg_l->key);
									fprintf(stdout, "\n");
									err = ERROR_RETURN;
								}
							}
							database_add_trade_good(db, &good);
							trade_good_group_add_trade_good(&group, (struct trade_good_t *)buf_len(db->trade_goods));
						}
					} else {
						fprintf(stdout, "[read_trade_goods] Invalid token (expected alphanumeric trade good definition for %s): ", group.name.text);
						token_print(stdout, &group_l->key);
						fprintf(stdout, "\n");
						err = ERROR_RETURN;
					}
				}
				database_add_trade_good_group(db, &group);
			}
		} else {
			fprintf(stdout, "[read_trade_goods] Invalid token (expected alphanumeric trade good group definition): ");
			token_print(stdout, &group_l->key);
			fprintf(stdout, "\n");
			err = ERROR_RETURN;
		}
	}

	lexeme_delete(root_l);

	for_buf(i, db->trade_good_groups) {
		struct trade_good_group_t *group = &db->trade_good_groups[i];
		for_buf(j, group->trade_goods) {
			group->trade_goods[j] = &db->trade_goods[(size_t)group->trade_goods[j] - 1];
			group->trade_goods[j]->group = group;
		}
	}

	fprintf(stdout, "[read_trade_goods] Loaded %zu trade_goods into %zu trade_good groups.\n", buf_len(db->trade_goods), buf_len(db->trade_good_groups));
	return err;
}
int write_trade_goods(struct database_t *db, const char *filename) {
	assert(db && "write_trade_goods: db == 0");
	assert(filename && "write_trade_goods: filename == 0");
	struct lexeme_t *root_l = root_l = lexeme_new_alphanumeric_c("root", true);

	for_buf(i, db->trade_good_groups) { // for each trade good group...
		struct trade_good_group_t *group = &db->trade_good_groups[i];
		struct lexeme_t *group_l = lexeme_new_alphanumeric(&group->name, true, true);
		for_buf(j, group->trade_goods) { // for each trade good in the group...
			struct trade_good_t *good = group->trade_goods[j];
			struct lexeme_t *good_l = lexeme_new_alphanumeric(&good->name, true, true);
			{	// cost
				struct lexeme_t *cost_l = lexeme_new_alphanumeric_c("cost", false);
				struct lexeme_t *cost_val_l = lexeme_new_decimal(good->cost, false);
				lexeme_add_value(cost_l, cost_val_l);
				lexeme_add_value(good_l, cost_l);
			}
			{	// color
				struct lexeme_t *color_l = lexeme_new_alphanumeric_c("color", true);
				lexeme_add_color(color_l, good->color);
				lexeme_add_value(good_l, color_l);
			}
			if (!good->available_from_start) {
				struct lexeme_t *available_from_start_l = lexeme_new_alphanumeric_c("available_from_start", false);
				struct lexeme_t *no_l = lexeme_new_alphanumeric_c("no", false);
				lexeme_add_value(available_from_start_l, no_l);
				lexeme_add_value(good_l, available_from_start_l);
			}
			if (!good->tradeable) {
				struct lexeme_t *tradeable_l = lexeme_new_alphanumeric_c("tradeable", false);
				struct lexeme_t *no_l = lexeme_new_alphanumeric_c("no", false);
				lexeme_add_value(tradeable_l, no_l);
				lexeme_add_value(good_l, tradeable_l);
			}
			if (good->money) {
				struct lexeme_t *money_l = lexeme_new_alphanumeric_c("money", false);
				struct lexeme_t *yes_l = lexeme_new_alphanumeric_c("yes", false);
				lexeme_add_value(money_l, yes_l);
				lexeme_add_value(good_l, money_l);
			}
			if (good->overseas_penalty) {
				struct lexeme_t *overseas_penalty_l = lexeme_new_alphanumeric_c("overseas_penalty", false);
				struct lexeme_t *yes_l = lexeme_new_alphanumeric_c("yes", false);
				lexeme_add_value(overseas_penalty_l, yes_l);
				lexeme_add_value(good_l, overseas_penalty_l);
			}
			lexeme_add_value(group_l, good_l);
		}
		lexeme_add_value(root_l, group_l);
	}
	lexeme_print(root_l);
	lexeme_delete(root_l);
	return 0;
}

int read_ideologies(struct database_t *db, const char *filename) {
	assert(db && "read_ideologies: db == 0");
	assert(filename && "read_ideologies: filename == 0");
	struct lexeme_t *root_l = lexeme_new();
	if (lexer_process_file(filename, root_l)) {
		lexeme_delete(root_l);
		return ERROR_RETURN;
	}
	int err = 0;
	for_buf(i, root_l->values) {	// for each ideology group...
		struct lexeme_t *group_l = root_l->values[i];
		if (lexeme_is_named_group(group_l)) {
			if (database_get_ideology_group(db, &group_l->key.data.str)) {
				fprintf(stdout, "[read_ideologies] Duplicate ideology group with name: %s\n", group_l->key.data.str.text);
				err = ERROR_RETURN;
			} else {
				struct ideology_group_t group = { 0 };
				string_set(&group.name, &group_l->key.data.str);
				for_buf(j, group_l->values) {	// for each ideology...
					struct lexeme_t *ideology_l = group_l->values[j];
					if (lexeme_is_named_group(ideology_l)) {
						if (database_get_ideology(db, &ideology_l->key.data.str)) {
							fprintf(stdout, "[read_ideologies] Duplicate ideology with name: %s\n", ideology_l->key.data.str.text);
							err = ERROR_RETURN;
						} else {
							struct ideology_t ideology = ideology_default();
							string_set(&ideology.name, &ideology_l->key.data.str);
							for_buf(k, ideology_l->values) {	// for each ideology arg...
								struct lexeme_t *arg_l = ideology_l->values[k];
								if (arg_l->key.type == ALPHANUMERIC) {
									if (string_equal_c(&arg_l->key.data.str, "uncivilized")) {
										if (!lexeme_get_bool(arg_l, &ideology.uncivilized)) {
											fprintf(stdout, "[read_ideologies] Could not read uncivilized bool for %s\n", ideology.name.text);
											err = ERROR_RETURN;
										}
									} else if (string_equal_c(&arg_l->key.data.str, "color")) {
										u8 col[3] = { 0 };
										if (lexeme_get_color(arg_l, col))
											ideology.color = to_color(col);
										else {
											fprintf(stdout, "[read_ideologies] Could not read color for %s\n", ideology.name.text);
											err = ERROR_RETURN;
										}
									} else if (string_equal_c(&arg_l->key.data.str, "date")) {
										if (!lexeme_get_date(arg_l, &ideology.date)) {
											fprintf(stdout, "[read_ideologies] Could not read date for %s\n", ideology.name.text);
											err = ERROR_RETURN;
										}
									} else if (string_equal_c(&arg_l->key.data.str, "can_reduce_militancy")) {
										if (!lexeme_get_bool(arg_l, &ideology.can_reduce_militancy)) {
											fprintf(stdout, "[read_ideologies] Could not read can_reduce_militancy bool for %s\n", ideology.name.text);
											err = ERROR_RETURN;
										}
									} else if (string_equal_c(&arg_l->key.data.str, "add_political_reform") || string_equal_c(&arg_l->key.data.str, "remove_political_reform") ||
										string_equal_c(&arg_l->key.data.str, "add_social_reform") || string_equal_c(&arg_l->key.data.str, "remove_social_reform") ||
										string_equal_c(&arg_l->key.data.str, "add_military_reform") || string_equal_c(&arg_l->key.data.str, "add_economic_reform")) {
										// TODO parse these modifiers
									} else {
										fprintf(stdout, "[read_ideologies] Unrecognised ideology specification for %s: %s\n", ideology.name.text, arg_l->key.data.str.text);
										err = ERROR_RETURN;
									}
								} else {
									fprintf(stdout, "[read_ideologies] Invalid token (expected alphanumeric ideology specifications for %s): ", ideology.name.text);
									token_print(stdout, &arg_l->key);
									fprintf(stdout, "\n");
									err = ERROR_RETURN;
								}
							}
							database_add_ideology(db, &ideology);
							ideology_group_add_ideology(&group, (struct ideology_t *)buf_len(db->ideologies));
						}
					} else {
						fprintf(stdout, "[read_ideologies] Invalid token (expected alphanumeric ideology definition for %s): ", group.name.text);
						token_print(stdout, &group_l->key);
						fprintf(stdout, "\n");
						err = ERROR_RETURN;
					}
				}
				database_add_ideology_group(db, &group);
			}
		} else {
			fprintf(stdout, "[read_ideologies] Invalid token (expected alphanumeric ideology group definition): ");
			token_print(stdout, &group_l->key);
			fprintf(stdout, "\n");
			err = ERROR_RETURN;
		}
	}

	lexeme_delete(root_l);

	for_buf(i, db->ideology_groups) {
		struct ideology_group_t *group = &db->ideology_groups[i];
		for_buf(j, group->ideologies) {
			group->ideologies[j] = &db->ideologies[(size_t)group->ideologies[j] - 1];
			group->ideologies[j]->group = group;
		}
	}

	fprintf(stdout, "[read_ideologies] Loaded %zu ideologies into %zu ideology groups.\n", buf_len(db->ideologies), buf_len(db->ideology_groups));
	return err;
}

int read_issues(struct database_t *db, const char *filename) {
	assert(db && "read_issues: db == 0");
	assert(filename && "read_issues: filename == 0");
	struct lexeme_t *root_l = lexeme_new();
	if (lexer_process_file(filename, root_l)) {
		lexeme_delete(root_l);
		return ERROR_RETURN;
	}
	int err = 0;
	for_buf(n, root_l->values) {	// for each parent group...
		struct lexeme_t *parent = root_l->values[n];
		if (!lexeme_is_named_group(parent)) {
			fprintf(stdout, "[read_issues] Unrecognised parent group:");
			token_print(stdout, &parent->key);
			fprintf(stdout, "\n");
			err = ERROR_RETURN;
			continue;
		}
		if (string_equal_c(&parent->key.data.str, "party_issues")) {
			for_buf(i, parent->values) {	// for each issue group...
				struct lexeme_t *group_l = parent->values[i];
				if (lexeme_is_named_group(group_l)) {
					if (database_get_issue_group(db, &group_l->key.data.str)) {
						fprintf(stdout, "[read_issues] Duplicate issue group with name: %s\n", group_l->key.data.str.text);
						err = ERROR_RETURN;
					} else {
						struct issue_group_t group = { 0 };
						string_set(&group.name, &group_l->key.data.str);
						for_buf(j, group_l->values) {	// for each issue...
							struct lexeme_t *issue_l = group_l->values[j];
							if (lexeme_is_named_group(issue_l)) {
								if (database_get_issue(db, &issue_l->key.data.str)) {
									fprintf(stdout, "[read_issues] Duplicate issue with name: %s\n", issue_l->key.data.str.text);
									err = ERROR_RETURN;
								} else {
									struct issue_t issue = { 0 };
									string_set(&issue.name, &issue_l->key.data.str);
									/*for_buf(k, issue_l->values) {	// for each issue arg...
										struct lexeme_t *arg_l = issue_l->values[k];
										if (arg_l->key.type == ALPHANUMERIC) {
											// TODO parse these modifiers
										} else {
											fprintf(stdout, "[read_issues] Invalid token (expected alphanumeric issue specifications for %s): ", issue.name.text);
											token_print(stdout, &arg_l->key);
											fprintf(stdout, "\n");
											err = ERROR_RETURN;
										}
									}*/
									database_add_issue(db, &issue);
									issue_group_add_issue(&group, (struct issue_t *)buf_len(db->issues));
								}
							} else {
								fprintf(stdout, "[read_issues] Invalid token (expected alphanumeric issue definition for %s): ", group.name.text);
								token_print(stdout, &group_l->key);
								fprintf(stdout, "\n");
								err = ERROR_RETURN;
							}
						}
						database_add_issue_group(db, &group);
					}
				} else {
					fprintf(stdout, "[read_issues] Invalid token (expected alphanumeric issue group definition): ");
					token_print(stdout, &group_l->key);
					fprintf(stdout, "\n");
					err = ERROR_RETURN;
				}
			}
		} else {
			enum reform_type_t type;
			for (type = 0; type < REFORM_TYPE_COUNT && !string_equal_c(&parent->key.data.str, reform_type_strings[type]); type++);
			if (type < REFORM_TYPE_COUNT) {
				for_buf(m, parent->values) {
					struct lexeme_t *group_l = parent->values[m];
					if (group_l->key.type == ALPHANUMERIC) {
						if (database_get_reform_group(db, &group_l->key.data.str)) {
							fprintf(stdout, "[read_reforms] Duplicate reform group with name: %s\n", group_l->key.data.str.text);
							err = ERROR_RETURN;
						}
						struct reform_group_t group = { 0 };
						string_set(&group.name, &group_l->key.data.str);
						for_buf(p, group_l->values) {
							struct lexeme_t *reform_l = group_l->values[p];
							if (reform_l->key.type == ALPHANUMERIC) {
								if (string_equal_c(&reform_l->key.data.str, "next_step_only")) {
									//if (!lexeme_get_bool(reform_l, &group.next_step_only)) {
									//	fprintf(stdout, "[read_reforms] Could not read next_step_only bool for %s\n", group.name.text);
									//	err = ERROR_RETURN;
									//}
									// TODO WHAT TO DO WITH next_step_only???
								} else if (string_equal_c(&reform_l->key.data.str, "administrative")) {
									//if (!lexeme_get_bool(reform_l, &administrative.next_step_only)) {
									//	fprintf(stdout, "[read_reforms] Could not read administrative bool for %s\n", group.name.text);
									//	err = ERROR_RETURN;
									//}
									// TODO WHAT TO DO WITH administrative???
								} else {
									if (database_get_reform(db, &reform_l->key.data.str)) {
										fprintf(stdout, "[read_reforms] Duplicate reform with name: %s\n", reform_l->key.data.str.text);
										err = ERROR_RETURN;
									}
									struct reform_t reform = { .type = type };
									string_set(&reform.name, &reform_l->key.data.str);
									// TODO process effects here
									database_add_reform(db, &reform);
									reform_group_add_reform(&group, (struct reform_t *)buf_len(db->reforms));
								}
							} else {
								fprintf(stdout, "[read_reforms] Invalid token (expected alphanumeric reform definition for %s): ", group.name.text);
								token_print(stdout, &reform_l->key);
								fprintf(stdout, "\n");
								err = ERROR_RETURN;
							}
						}
						if (err) {
							reform_group_free(&group);
							err = ERROR_RETURN;
						}
						database_add_reform_group(db, &group);
					} else {
						fprintf(stdout, "[read_reforms] Invalid token (expected alphanumeric reform group): ");
						token_print(stdout, &group_l->key);
						fprintf(stdout, "\n");
						err = ERROR_RETURN;
					}
				}
			} else {
				fprintf(stdout, "[read_issues] Unrecognised alphanumeric (expected issue or reform group definition): %s\n", parent->key.data.str.text);
				err = ERROR_RETURN;
			}
		}
	}

	lexeme_delete(root_l);

	for_buf(i, db->issue_groups) {
		struct issue_group_t *group = &db->issue_groups[i];
		for_buf(j, group->issues) {
			group->issues[j] = &db->issues[(size_t)group->issues[j] - 1];
			group->issues[j]->group = group;
		}
	}

	for_buf(i, db->reform_groups) {
		struct reform_group_t *group = &db->reform_groups[i];
		for_buf(j, group->reforms) {
			group->reforms[j] = &db->reforms[(size_t)group->reforms[j] - 1];
			group->reforms[j]->group = group;
		}
	}

	fprintf(stdout, "[read_issues] Loaded %zu issues into %zu issue groups.\n", buf_len(db->issues), buf_len(db->issue_groups));
	fprintf(stdout, "[read_reforms] Loaded %zu reforms into %zu reform groups.\n", buf_len(db->reforms), buf_len(db->reform_groups));
	return err;
}

int read_national_values(struct database_t *db, const char *filename) {
	assert(db && "read_national_values: db == 0");
	assert(filename && "read_national_values: filename == 0");
	struct lexeme_t *root_l = lexeme_new();
	if (lexer_process_file(filename, root_l)) {
		lexeme_delete(root_l);
		return ERROR_RETURN;
	}
	int err = 0;
	for_buf(i, root_l->values) {
		struct lexeme_t *nv_l = root_l->values[i];
		if (!lexeme_is_named_group(nv_l)) {
			fprintf(stdout, "[read_national_values] National values must be alphanumeric compound tags\n");
			err = ERROR_RETURN;
			continue;
		}
		if (database_get_national_value(db, &nv_l->key.data.str)) {
			fprintf(stdout, "[read_national_values] Duplicate national_value with name: %s\n", nv_l->key.data.str.text);
			err = ERROR_RETURN;
			continue;
		}
		struct national_value_t national_value = { 0 };
		string_set(&national_value.name, &nv_l->key.data.str);
		// TODO read modifiers
		database_add_national_value(db, &national_value);
	}

	lexeme_delete(root_l);

	fprintf(stdout, "[read_national_values] Loaded %zu national_values.\n", buf_len(db->national_values));
	return err;
}

int read_religions(struct database_t *db, const char *filename) {
	assert(db && "read_religions: db == 0");
	assert(filename && "read_religions: filename == 0");
	struct lexeme_t *root_l = lexeme_new();
	if (lexer_process_file(filename, root_l)) {
		lexeme_delete(root_l);
		return ERROR_RETURN;
	}
	int err = 0;
	for_buf(i, root_l->values) {	// for each religion group...
		struct lexeme_t *group_l = root_l->values[i];
		if (lexeme_is_named_group(group_l)) {
			if (database_get_religion_group(db, &group_l->key.data.str)) {
				fprintf(stdout, "[read_religions] Duplicate religion group with name: %s\n", group_l->key.data.str.text);
				err = ERROR_RETURN;
			} else {
				struct religion_group_t group = { 0 };
				string_set(&group.name, &group_l->key.data.str);
				for_buf(j, group_l->values) {	// for each religion...
					struct lexeme_t *religion_l = group_l->values[j];
					if (lexeme_is_named_group(religion_l)) {
						if (database_get_religion(db, &religion_l->key.data.str)) {
							fprintf(stdout, "[read_religions] Duplicate religion with name: %s\n", religion_l->key.data.str.text);
							err = ERROR_RETURN;
						} else {
							struct religion_t religion = { 0 };
							string_set(&religion.name, &religion_l->key.data.str);
							for_buf(k, religion_l->values) {	// for each religion arg...
								struct lexeme_t *arg_l = religion_l->values[k];
								if (arg_l->key.type == ALPHANUMERIC) {
									if (string_equal_c(&arg_l->key.data.str, "icon")) {
										int tmp = 0;
										if (!lexeme_get_int(arg_l, &tmp)) {
											fprintf(stdout, "[read_religions] Could not read icon int for %s\n", religion.name.text);
											err = ERROR_RETURN;
										} else religion.icon = tmp;
									} else if (string_equal_c(&arg_l->key.data.str, "color")) {
										u8 col[3] = { 0 };
										if (lexeme_get_color(arg_l, col))
											religion.color = to_color(col);
										else {
											fprintf(stdout, "[read_religions] Could not read color for %s\n", religion.name.text);
											err = ERROR_RETURN;
										}
									} else if (string_equal_c(&arg_l->key.data.str, "pagan")) {
										if (!lexeme_get_bool(arg_l, &religion.pagan)) {
											fprintf(stdout, "[read_religions] Could not read pagan bool for %s\n", religion.name.text);
											err = ERROR_RETURN;
										}
									} else {
										fprintf(stdout, "[read_religions] Unrecognised religion specification for %s: %s\n", religion.name.text, arg_l->key.data.str.text);
										err = ERROR_RETURN;
									}
								} else {
									fprintf(stdout, "[read_religions] Invalid token (expected alphanumeric religion specifications for %s): ", religion.name.text);
									token_print(stdout, &arg_l->key);
									fprintf(stdout, "\n");
									err = ERROR_RETURN;
								}
							}
							database_add_religion(db, &religion);
							religion_group_add_religion(&group, (struct religion_t *)buf_len(db->religions));
						}
					} else {
						fprintf(stdout, "[read_religions] Invalid token (expected alphanumeric religion definition for %s): ", group.name.text);
						token_print(stdout, &group_l->key);
						fprintf(stdout, "\n");
						err = ERROR_RETURN;
					}
				}
				database_add_religion_group(db, &group);
			}
		} else {
			fprintf(stdout, "[read_religions] Invalid token (expected alphanumeric religion group definition): ");
			token_print(stdout, &group_l->key);
			fprintf(stdout, "\n");
			err = ERROR_RETURN;
		}
	}

	lexeme_delete(root_l);

	for_buf(i, db->religion_groups) {
		struct religion_group_t *group = &db->religion_groups[i];
		for_buf(j, group->religions) {
			group->religions[j] = &db->religions[(size_t)group->religions[j] - 1];
			group->religions[j]->group = group;
		}
	}

	fprintf(stdout, "[read_religions] Loaded %zu religions into %zu religion groups.\n", buf_len(db->religions), buf_len(db->religion_groups));
	return err;
}

int read_government_types(struct database_t *db, const char *filename) {
	assert(db && "read_government_types: db == 0");
	assert(filename && "read_government_types: filename == 0");
	struct lexeme_t *root_l = lexeme_new();
	if (lexer_process_file(filename, root_l)) {
		lexeme_delete(root_l);
		return ERROR_RETURN;
	}
	int err = 0;
	for_buf(i, root_l->values) {	// for each government_type group...
		struct lexeme_t *gov_l = root_l->values[i];
		if (lexeme_is_named_group(gov_l)) {
			if (database_get_government_type(db, &gov_l->key.data.str)) {
				fprintf(stdout, "[read_government_types] Duplicate government type with name: %s\n", gov_l->key.data.str.text);
				err = ERROR_RETURN;
			} else {
				struct government_type_t gov = { 0 };
				government_type_init(db, &gov);
				string_set(&gov.name, &gov_l->key.data.str);
				for_buf(k, gov_l->values) {	// for each government_type arg...
					struct lexeme_t *arg_l = gov_l->values[k];
					if (arg_l->key.type == ALPHANUMERIC) {
						if (string_equal_c(&arg_l->key.data.str, "election")) {
							if (!lexeme_get_bool(arg_l, &gov.election)) {
								fprintf(stdout, "[read_government_types] Could not read election bool for %s\n", gov.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "duration")) {
							int tmp = 0;
							if (!lexeme_get_int(arg_l, &tmp)) {
								fprintf(stdout, "[read_government_types] Could not read duration int for %s\n", gov.name.text);
								err = ERROR_RETURN;
							}
							gov.duration = tmp;
						} else if (string_equal_c(&arg_l->key.data.str, "appoint_ruling_party")) {
							if (!lexeme_get_bool(arg_l, &gov.appoint_ruling_party)) {
								fprintf(stdout, "[read_government_types] Could not read appoint_ruling_party bool for %s\n", gov.name.text);
								err = ERROR_RETURN;
							}
						} else if (string_equal_c(&arg_l->key.data.str, "flagType")) {
							if (arg_l->compound || arg_l->values == 0 || arg_l->values[0]->key.type != ALPHANUMERIC) {
								fprintf(stdout, "[read_government_types] Unrecognised flagType for %s\n", gov.name.text);
								err = ERROR_RETURN;
							} else {
								enum flag_type_t f;
								for (f = 0; f < FLAG_TYPE_COUNT && !string_equal_c(&arg_l->values[0]->key.data.str, flag_type_strings[f]); ++f);
								if (f == FLAG_TYPE_COUNT) {
									fprintf(stdout, "[read_government_types] Unknown flag type for %s: %s\n",
										gov.name.text, arg_l->values[0]->key.data.str.text);
									err = ERROR_RETURN;
								} else gov.flag = f;
							}
						} else {
							struct ideology_t *ideo = database_get_ideology(db, &arg_l->key.data.str);
							if (ideo) {
								if (!lexeme_get_bool(arg_l, &gov.ideologies[database_ideology_index(db, ideo)])) {
									fprintf(stdout, "[read_government_types] Could not read %s bool for %s\n", ideo->name.text, gov.name.text);
									err = ERROR_RETURN;
								}
							} else {
								fprintf(stdout, "[read_government_types] Unrecognised government type specification for %s: %s\n", gov.name.text, arg_l->key.data.str.text);
								err = ERROR_RETURN;
							}
						}
					} else {
						fprintf(stdout, "[read_government_types] Invalid token (expected alphanumeric government type specifications for %s): ", gov.name.text);
						token_print(stdout, &arg_l->key);
						fprintf(stdout, "\n");
						err = ERROR_RETURN;
					}
				}
				database_add_government_type(db, &gov);
			}
		} else {
			fprintf(stdout, "[read_government_types] Invalid token (expected alphanumeric government type group definition): ");
			token_print(stdout, &gov_l->key);
			fprintf(stdout, "\n");
			err = ERROR_RETURN;
		}
	}

	lexeme_delete(root_l);

	fprintf(stdout, "[read_government_types] Loaded %zu government_types.\n", buf_len(db->government_types));
	return err;
}

int read_countries(struct database_t *db, const char *filename) {
	assert(db && "read_countries: db == 0");
	assert(filename && "read_countries: filename == 0");
	struct lexeme_t *root_l = lexeme_new();
	if (lexer_process_file(filename, root_l)) {
		lexeme_delete(root_l);
		return ERROR_RETURN;
	}
	int err = 0;
	for_buf(i, root_l->values) {	// for each country
		struct lexeme_t *country_l = root_l->values[i];
		if (country_l->key.type == ALPHANUMERIC && country_l->key.data.str.length == 3) {
			struct country_t country = { 0 };
			memcpy(country.tag.text, country_l->key.data.str.text, 3);
			if (!tag_valid(&country.tag)) {
				fprintf(stdout, "[read_countries] Invalid TAG: %s\n", country.tag.text);
				err = ERROR_RETURN;
				continue;
			}
			if (database_get_country(db, &country.tag)) {
				fprintf(stdout, "[read_countries] Repeated tag: %s\n", country.tag.text);
				err = ERROR_RETURN;
				continue;
			}
			if (country_l->compound || country_l->values == 0 || country_l->values[0]->key.type != STRING) {
				fprintf(stdout, "[read_countries] Invalid defines location for %s\n", country.tag.text);
				err = ERROR_RETURN;
				continue;
			}
			string_set(&country.defines_location, &country_l->values[0]->key.data.str);
			database_add_country(db, &country);
		} else if (country_l->key.type == ALPHANUMERIC && string_equal_c(&country_l->key.data.str, "dynamic_tags")) {
			/* dynamic_tags */
			// TODO what to do with this? (mark the tags after it as dynamic?)
		} else {
			fprintf(stdout, "[read_countries] Invalid token (expected TAG): ");
			token_print(stdout, &country_l->key);
			fprintf(stdout, "\n");
			err = ERROR_RETURN;
		}
	}
	lexeme_delete(root_l);

	fprintf(stdout, "[read_countries] Loaded %zu tags.\n", buf_len(db->countries));
	return err;
}

int read_cultures(struct database_t *db, const char *filename) {
	assert(db && "read_cultures: db == 0");
	assert(filename && "read_cultures: filename == 0");
	struct lexeme_t *root_l = lexeme_new();
	if (lexer_process_file(filename, root_l)) {
		lexeme_delete(root_l);
		return ERROR_RETURN;
	}
	int err = 0;
	for_buf(i, root_l->values) {	// for each culture group...
		struct lexeme_t *group_l = root_l->values[i];
		if (lexeme_is_named_group(group_l)) {
			if (database_get_culture_group(db, &group_l->key.data.str)) {
				fprintf(stdout, "[read_cultures] Duplicate culture group with name: %s\n", group_l->key.data.str.text);
				err = ERROR_RETURN;
			} else {
				struct culture_group_t group = { 0 };
				string_set(&group.name, &group_l->key.data.str);
				for_buf(j, group_l->values) {	// for each culture...
					struct lexeme_t *culture_l = group_l->values[j];
					if (culture_l->key.type == ALPHANUMERIC) {
						if (string_equal_c(&culture_l->key.data.str, "is_overseas")) {
							// TODO - check for valid bool
						} else if (string_equal_c(&culture_l->key.data.str, "leader")) {
							string tmp = { 0 };
							if (!lexeme_get_alphanumeric(culture_l, &tmp)) {
								fprintf(stdout, "[read_cultures] Could not read leader alphanumeric for %s\n", group.name.text);
								err = ERROR_RETURN;
							} else {
								enum leader_t l;
								for (l = 0; l < LEADER_COUNT && !string_equal_c(&tmp, leader_strings[l]); ++l);
								if (l == LEADER_COUNT) {
									fprintf(stdout, "[read_cultures] Unknown leader type for %s: %s\n",
										group.name.text, tmp.text);
									err = ERROR_RETURN;
								} else group.leader = l;
							}
							string_clear(&tmp);
						} else if (string_equal_c(&culture_l->key.data.str, "unit")) {
							string tmp = { 0 };
							if (!lexeme_get_alphanumeric(culture_l, &tmp)) {
								fprintf(stdout, "[read_cultures] Could not read unit alphanumeric for %s\n", group.name.text);
								err = ERROR_RETURN;
							} else {
								enum graphical_culture_t u;
								for (u = 0; u < GRAPHICAL_CULTURE_COUNT && !string_equal_c(&tmp, graphical_culture_strings[u]); ++u);
								if (u == GRAPHICAL_CULTURE_COUNT) {
									fprintf(stdout, "[read_cultures] Unknown graphical culture (unit) type for %s: %s\n",
										group.name.text, tmp.text);
									err = ERROR_RETURN;
								} else group.unit = u;
							}
							string_clear(&tmp);
						} else if (string_equal_c(&culture_l->key.data.str, "union")) {
							if (!lexeme_get_country(culture_l, db, &group.cultural_union)) {
								fprintf(stdout, "[read_cultures] Could not read union TAG for %s\n", group.name.text);
								err = ERROR_RETURN;
							}
						} else {
							if (database_get_culture(db, &culture_l->key.data.str)) {
								fprintf(stdout, "[read_cultures] Duplicate culture with name: %s\n", culture_l->key.data.str.text);
								err = ERROR_RETURN;
							} else {
								struct culture_t culture = { 0 };
								string_set(&culture.name, &culture_l->key.data.str);
								for_buf(k, culture_l->values) {	// for each culture arg...
									struct lexeme_t *arg_l = culture_l->values[k];
									if (arg_l->key.type == ALPHANUMERIC) {
										if (string_equal_c(&arg_l->key.data.str, "color")) {
											u8 col[3] = { 0 };
											if (lexeme_get_color(arg_l, col))
												culture.color = to_color(col);
											else {
												fprintf(stdout, "[read_cultures] Could not read color for %s\n", culture.name.text);
												err = ERROR_RETURN;
											}
										} else if (string_equal_c(&arg_l->key.data.str, "radicalism")) {
											int tmp = 0;
											if (!lexeme_get_int(arg_l, &tmp)) {
												fprintf(stdout, "[read_cultures] Could not read radicalism int for %s\n", culture.name.text);
												err = ERROR_RETURN;
											} else culture.radicalism = tmp;
										} else if (string_equal_c(&arg_l->key.data.str, "primary")) {
											if (!lexeme_get_country(arg_l, db, &culture.primary)) {
												fprintf(stdout, "[read_cultures] Could not read primary TAG for %s\n", culture.name.text);
												err = ERROR_RETURN;
											}
										} else if (string_equal_c(&arg_l->key.data.str, "first_names")) {
											if (!arg_l->compound || buf_len(arg_l->values) < 1) {
												fprintf(stdout, "[read_cultures] Could not read primary TAG for %s\n", culture.name.text);
												err = ERROR_RETURN;
											} else {
												for_buf(fn, arg_l->values) { // for each first name...
													struct lexeme_t *first_name_l = arg_l->values[fn];
													if ((first_name_l->key.type == ALPHANUMERIC || first_name_l->key.type == STRING) && !string_empty(&first_name_l->key.data.str) && !first_name_l->compound)
														buf_push(culture.first_names, string_make(first_name_l->key.data.str.text));
													else {
														fprintf(stdout, "[read_cultures] Invalid first name (expected alphanumeric or string for %s): ", culture.name.text);
														token_print(stdout, &arg_l->key);
														fprintf(stdout, "\n");
														err = ERROR_RETURN;
													}
												}
											}
										} else if (string_equal_c(&arg_l->key.data.str, "last_names")) {
											if (!arg_l->compound || buf_len(arg_l->values) < 1) {
												fprintf(stdout, "[read_cultures] Could not read primary TAG for %s\n", culture.name.text);
												err = ERROR_RETURN;
											} else {
												for_buf(fn, arg_l->values) { // for each last name...
													struct lexeme_t *last_name_l = arg_l->values[fn];
													if ((last_name_l->key.type == ALPHANUMERIC || last_name_l->key.type == STRING) && !string_empty(&last_name_l->key.data.str) && !last_name_l->compound)
														buf_push(culture.last_names, string_make(last_name_l->key.data.str.text));
													else {
														fprintf(stdout, "[read_cultures] Invalid last name (expected alphanumeric or string for %s): ", culture.name.text);
														token_print(stdout, &arg_l->key);
														fprintf(stdout, "\n");
														err = ERROR_RETURN;
													}
												}
											}
										} else {
											fprintf(stdout, "[read_cultures] Unrecognised alphanumeric (expected definition of culture %s): %s.\n",
												culture.name.text, arg_l->key.data.str.text);
											err = ERROR_RETURN;
										}
									} else {
										fprintf(stdout, "[read_cultures] Invalid token (expected alphanumeric culture specifications for %s): ", culture.name.text);
										token_print(stdout, &arg_l->key);
										fprintf(stdout, "\n");
										err = ERROR_RETURN;
									}
								}
								database_add_culture(db, &culture);
								culture_group_add_culture(&group, (struct culture_t *)buf_len(db->cultures));
							}
						}
					} else {
						fprintf(stdout, "[read_cultures] Invalid token (expected alphanumeric culture definition for %s): ", group.name.text);
						token_print(stdout, &group_l->key);
						fprintf(stdout, "\n");
						err = ERROR_RETURN;
					}
				}
				database_add_culture_group(db, &group);
			}
		} else {
			fprintf(stdout, "[read_cultures] Invalid token (expected alphanumeric culture group definition): ");
			token_print(stdout, &group_l->key);
			fprintf(stdout, "\n");
			err = ERROR_RETURN;
		}
	}

	lexeme_delete(root_l);

	for_buf(i, db->culture_groups) {
		struct culture_group_t *group = &db->culture_groups[i];
		for_buf(j, group->cultures) {
			group->cultures[j] = &db->cultures[(size_t)group->cultures[j] - 1];
			group->cultures[j]->group = group;
		}
	}

	fprintf(stdout, "[read_cultures] Loaded %zu cultures into %zu culture groups.\n", buf_len(db->cultures), buf_len(db->culture_groups));
	return err;
}

int read_single_country_defines(struct database_t *db, struct country_t *country) {
	assert(db && "read_single_country_defines: db == 0");
	assert(country && "read_single_country_defines: country == 0");
	string filename = string_make(MOD_FOLDER "common/");
	string_append(&filename, &country->defines_location);
	struct lexeme_t *root_l = lexeme_new();
	if (lexer_process_file(filename.text, root_l)) {
		string_clear(&filename);
		lexeme_delete(root_l);
		return ERROR_RETURN;
	}
	string_clear(&filename);
	int err = 0;
	for_buf(i, root_l->values) {	// for each definition...
		struct lexeme_t *arg_l = root_l->values[i];
		if (arg_l->key.type == ALPHANUMERIC) {
			if (string_equal_c(&arg_l->key.data.str, "color")) {
				u8 col[3] = { 0 };
				if (lexeme_get_color(arg_l, col))
					country->color = to_color(col);
				else {
					fprintf(stdout, "[read_single_country_defines] Could not read color for %s\n", country->tag.text);
					err = ERROR_RETURN;
				}
			} else if (string_equal_c(&arg_l->key.data.str, "graphical_culture")) {
				string tmp = { 0 };
				if (!lexeme_get_alphanumeric(arg_l, &tmp)) {
					fprintf(stdout, "[read_single_country_defines] Could not read graphical_culture alphanumeric for %s\n", country->tag.text);
					err = ERROR_RETURN;
				} else {
					enum graphical_culture_t gc;
					for (gc = 0; gc < GRAPHICAL_CULTURE_COUNT && !string_equal_c(&tmp, graphical_culture_strings[gc]); ++gc);
					if (gc == GRAPHICAL_CULTURE_COUNT) {
						fprintf(stdout, "[read_single_country_defines] Unknown graphical culture (unit) type for %s: %s\n",
							country->tag.text, tmp.text);
						err = ERROR_RETURN;
					} else country->graphical_culture = gc;
				}
				string_clear(&tmp);
			} else if (string_equal_c(&arg_l->key.data.str, "party")) {
				if (!arg_l->compound) {
					fprintf(stdout, "[read_single_country_defines] Invalid party lexeme for %s.\n", country->tag.text);
					err = ERROR_RETURN;
					continue;
				}
				struct party_t party = { .name = string_make("unnamed") };
				int completion = 9;
				for_buf(j, arg_l->values) {
					struct lexeme_t *party_l = arg_l->values[j];
					if (party_l->key.type == ALPHANUMERIC) {
						if (string_equal_c(&party_l->key.data.str, "name")) {
							if (!lexeme_get_string(party_l, &party.name)) {
								fprintf(stdout, "[read_single_country_defines] Could not read party name string for %s\n", country->tag.text);
								err = ERROR_RETURN;
							}
							completion--;
						} else if (string_equal_c(&party_l->key.data.str, "start_date")) {
							if (!lexeme_get_date(party_l, &party.start)) {
								fprintf(stdout, "[read_single_country_defines] Could not read party start date for %s\n", country->tag.text);
								err = ERROR_RETURN;
							}
							completion--;
						} else if (string_equal_c(&party_l->key.data.str, "end_date")) {
							if (!lexeme_get_date(party_l, &party.end)) {
								fprintf(stdout, "[read_single_country_defines] Could not read party end date for %s\n", country->tag.text);
								err = ERROR_RETURN;
							}
							completion--;
						} else if (string_equal_c(&party_l->key.data.str, "ideology")) {
							string tmp = { 0 };
							if (!lexeme_get_alphanumeric(party_l, &tmp)) {
								fprintf(stdout, "[read_single_country_defines] Could not read ideology alphanumeric for %s\n", country->tag.text);
								err = ERROR_RETURN;
							} else {
								struct ideology_t *i = database_get_ideology(db, &tmp);
								if (i) party.ideology = i;
								else fprintf(stdout, "[read_single_country_defines] Unknown ideology for %s: %s\n",
									country->tag.text, tmp.text);
							}
							string_clear(&tmp);
							completion--;
						} else if (string_equal_c(&party_l->key.data.str, "social_policy")) {
							/* (issue added in HPM, here I just skip over it) */
						} else {
							struct issue_group_t *group = database_get_issue_group(db, &party_l->key.data.str);
							if (group) {
								string tmp = { 0 };
								if (!lexeme_get_alphanumeric(party_l, &tmp)) {
									fprintf(stdout, "[read_single_country_defines] Could not read issue alphanumeric for %s\n", country->tag.text);
									err = ERROR_RETURN;
								} else {
									struct issue_t *issue = database_get_issue(db, &tmp);
									if (issue) {
										if (issue->group != group) {
											fprintf(stdout, "[read_single_country_defines] Issue group mismatch: %s is claimed to be in %s, while actually being ", issue->name.text, group->name.text);
											if (issue->group) fprintf(stdout, "in %s", issue->group->name.text);
											else fprintf(stdout, "empty");
											fprintf(stdout, "\n");
											err = ERROR_RETURN;
										}
										const size_t index = database_issue_group_index(db, group);
										if (party.issues == 0) party_issues_init(db, &party);
										if (party.issues[index]) fprintf(stdout, "[read_single_country_defines] Changing %s policy from %s to %s for %s\n",
											group->name.text, party.issues[index]->name.text, issue->name.text, party.name.text);
										party.issues[index] = issue;
									} else {
										fprintf(stdout, "[read_single_country_defines] Unknown issue for %s for %s: %s\n",
											group->name.text, country->tag.text, tmp.text);
										err = ERROR_RETURN;
									}
								}
								string_clear(&tmp);
								completion--;
							} else {
								fprintf(stdout, "[read_single_country_defines] Unrecognised alphanumeric in definition of %s party in %s: ",
									party.name.text, country->tag.text);
								token_print(stdout, &party_l->key);
								fprintf(stdout, "\n");
								err = ERROR_RETURN;
							}
						}
					} else {
						fprintf(stdout, "[read_single_country_defines] Invalid token (expected alphanumeric for definition of party %s in %s): ",
							party.name.text, country->tag.text);
						token_print(stdout, &party_l->key);
						fprintf(stdout, "\n");
						err = ERROR_RETURN;
					}
				}
				if (completion) fprintf(stdout, "[read_single_country_defines] Incomplete party: %s in %s, %d items missing\n",
					party.name.text, country->tag.text, completion);
				country_add_party(country, &party);
			} else if (string_equal_c(&arg_l->key.data.str, "unit_names")) {
				// TODO proper unit names reading
			} else {
				struct government_type_t *gov = database_get_government_type(db, &arg_l->key.data.str);
				if (gov) {
					u8 col[3] = { 0 };
					if (lexeme_get_color(arg_l, col))
						fprintf(stdout, "[read_single_country_defines] read unique %s color for %s: %d %d %d\n",
							gov->name.text, country->tag.text, col[0], col[1], col[2]);
					else {
						fprintf(stdout, "[read_single_country_defines] Could not read %s color for %s\n", gov->name.text, country->tag.text);
						err = ERROR_RETURN;
					}
				} else {
					fprintf(stdout, "[read_single_country_defines] Unrecognised alphanumeric (expected definition of %s): %s.\n",
						country->tag.text, arg_l->key.data.str.text);
					err = ERROR_RETURN;
				}
			}
		} else {
			fprintf(stdout, "[read_single_country_defines] Invalid token (expected alphanumeric country defines definition): ");
			token_print(stdout, &arg_l->key);
			fprintf(stdout, "\n");
			err = ERROR_RETURN;;
		}
	}

	lexeme_delete(root_l);
	return err;
}

int read_country_defines(struct database_t *db) {
	assert(db && "read_country_defines: db == 0");
	int err = 0;
	for (int i = 0; i < buf_len(db->countries); ++i) {
		err |= read_single_country_defines(db, &db->countries[i]);
		if (err) {
			fprintf(stdout, "[read_country_defines] Failed to load defines for %s.\n", db->countries[i].tag.text);
			break;
		}
	}
	if (err == 0) fprintf(stdout, "[read_country_defines] Successfully loaded all country defines\n");
	return err;
}

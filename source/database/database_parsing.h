#pragma once

#include "database.h"
#include "lexer.h"

#define err_break { err = ERROR_RETURN; break; }do{}while(0)
#define if_err_ret if(err) { return err; }
#define skip_token if (!token_source_next(&src, &token)) err_break

internal const date_t ACW_START_DATE = { .year = 1861, .month = 1, .day = 1 };
internal const date_t DOMINIONS_START_DATE = { .year = 1836, .month = 1, .day = 2 };

internal const char *flag_type_strings[FLAG_TYPE_COUNT] = { "communist", "republic", "fascist", "monarchy" };

internal const char *graphical_culture_strings[GRAPHICAL_CULTURE_COUNT] = { "Generic", "BritishGC", "EuropeanGC", "MiddleEasternGC",
	"ChineseGC", "IndianGC", "AfricanGC", "UsGC", "RussianGC", "FrenchGC", "PrussianGC", "ItalianGC", "AustriaHungaryGC", "SwedishGC",
	"SpanishGC", "OttomanGC", "MoroccoGC", "ZuluGC", "AsianGC", "SouthAmericanGC", "ConfederateGC", "JapaneseGC" };

internal const char *leader_strings[LEADER_COUNT] = { "european", "russian", "arab", "asian", "indian", "nativeamerican", "southamerican", "african", "polar_bear" };

internal const char *reform_type_strings[REFORM_TYPE_COUNT] = { "political_reforms", "social_reforms", "economic_reforms", "military_reforms" };

internal const char *unit_terrain_type_strings[UNIT_TERRAIN_TYPE_COUNT] = { "land", "naval" };
internal const char *unit_type_strings[UNIT_TYPE_COUNT] = { "infantry", "cavalry", "support", "special", "transport", "light_ship", "big_ship" };

int token_source_get_tag(struct token_source_t * src, struct tag_t * tag, const char *func_name, const char *purpose);
int token_source_get_country(struct token_source_t *src, struct database_t *db, struct country_t **country, const char *func_name, const char *purpose);

boolean lexeme_get_tag(struct lexeme_t *root, struct tag_t *tag);
boolean lexeme_get_country(struct lexeme_t *root, struct database_t *db, struct country_t **country);

typedef int(*read_file_func_t)(struct database_t *db, const char *filepath, const char *filename);
int read_all_in_folder(struct database_t * db, read_file_func_t read_file_func, const char *base_folder, int *files_read, const char *func_name);

/* trade goods */
int read_trade_goods(struct database_t *db, const char *filename);
int write_trade_goods(struct database_t *db, const char *filename);
/* ideologies */
int read_ideologies(struct database_t *db, const char *filename);
/* issues */
int read_issues(struct database_t * db, const char *filename);
/* national values */
int read_national_values(struct database_t *db, const char *filename);
/* religions */
int read_religions(struct database_t *db, const char *filename);
/* government types (requires ideologies) */
int read_government_types(struct database_t *db, const char *filename);

/* countries */
int read_countries(struct database_t *db, const char *filename);
/* cultures (requires countries) */
int read_cultures(struct database_t *db, const char *filename);
/* country defines (requires countries) */
int read_country_defines(struct database_t *db);

/* ==================== MAP ==================== */
/* province ids/colors */
int read_province_defines(struct database_t *db, const char *filename);
/* default.map (requires province_defines) */
int read_sea_starts(struct database_t *db, const char *filename);
/* states (requires province_defines) */
int read_states(struct database_t *db, const char *filename);
void update_province_states(struct database_t * db);
/* province shapes (requires province_defines) */
int read_province_shapes(struct database_t *db, const char *filename);

/* ==================== UNITS ==================== */
/* units */
int read_units_folder(struct database_t *db, const char *filename);

/* ==================== HISTORY ==================== */
/* country histories */
int read_country_histories(struct database_t *db, const char *filename);
/* province histories */
int read_province_histories(struct database_t *db, const char *filename);

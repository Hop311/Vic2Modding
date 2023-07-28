#pragma once

#include "string_wrapper.h"
#include "render.h"

#include "parser.h"

#define MOD_FOLDER "C:/Program Files (x86)/Steam/steamapps/common/Victoria 2/"

/* Helper functions */
u32 to_color(const u8 color[3]);

#define for_all_database_lists(func)		\
	func(country, countries)				\
	func(province, provinces)				\
	func(state, states)						\
	func(trade_good, trade_goods)			\
	func(trade_good_group, trade_good_groups)\
	func(culture, cultures)					\
	func(culture_group, culture_groups)		\
	func(religion, religions)				\
	func(religion_group, religion_groups)	\
	func(ideology, ideologies)				\
	func(ideology_group, ideology_groups)	\
	func(national_value, national_values)	\
	func(government_type, government_types)	\
	func(issue, issues)						\
	func(issue_group, issue_groups)			\
	func(reform, reforms)					\
	func(reform_group, reform_groups)		\
	func(unit, units)

#define for_all_database_lists_named(func)	\
	func(state, states)						\
	func(trade_good, trade_goods)			\
	func(trade_good_group, trade_good_groups)\
	func(culture, cultures)					\
	func(culture_group, culture_groups)		\
	func(religion, religions)				\
	func(religion_group, religion_groups)	\
	func(ideology, ideologies)				\
	func(ideology_group, ideology_groups)	\
	func(national_value, national_values)	\
	func(government_type, government_types)	\
	func(issue, issues)						\
	func(issue_group, issue_groups)			\
	func(reform, reforms)					\
	func(reform_group, reform_groups)		\
	func(unit, units)

#define for_all_database_ref_lists(func)			\
	func(state, province, provinces)				\
	func(trade_good_group, trade_good, trade_goods)	\
	func(religion_group, religion, religions)		\
	func(ideology_group, ideology, ideologies)		\
	func(issue_group, issue, issues)				\
	func(reform_group, reform, reforms)

/* forward declaration */
#define template_ref_list_forward_declare(type, list_type, list_name) struct type##_t;
	for_all_database_ref_lists(template_ref_list_forward_declare)
#undef template_ref_list_forward_declare
struct country_t;
struct province_t;
struct culture_group_t;
struct database_t;

enum graphical_culture_t {
	Generic, BritishGC, EuropeanGC, MiddleEasternGC, ChineseGC, IndianGC, AfricanGC, UsGC, RussianGC, FrenchGC, PrussianGC,
	ItalianGC, AustriaHungaryGC, SwedishGC, SpanishGC, OttomanGC, MoroccoGC, ZuluGC, AsianGC, SouthAmericanGC, ConfederateGC,
	JapaneseGC, GRAPHICAL_CULTURE_COUNT
};
enum leader_t {
	european, russian, arab, asian, indian, nativeamerican, southamerican, african, polar_bear, LEADER_COUNT
};
enum flag_type_t {
	communist, republic, fascist, monarchy, FLAG_TYPE_COUNT
};
enum reform_type_t {
	political, social, economic, military, REFORM_TYPE_COUNT
};
enum unit_terrain_type_t {
	land, naval, UNIT_TERRAIN_TYPE_COUNT
};
enum unit_type_t {
	infantry, cavalry, support, special, transport, light_ship, big_ship, UNIT_TYPE_COUNT
};

struct tag_t {
	char text[3];
	char zero;	/* leave zero = 0 as string terminator */
};

boolean tag_valid(const struct tag_t *tag);
boolean tag_equal(const struct tag_t *tagA, const struct tag_t *tagB);

/* National Value */
struct national_value_t {
	string name;
};

/* Trade Good */
struct trade_good_t {
	string name;
	struct trade_good_group_t *group;
	double cost;
	u32 color;
	boolean available_from_start, overseas_penalty, money, tradeable;
};

struct trade_good_t trade_good_default(void);
double *create_trade_good_list(const struct database_t *db);
void add_to_trade_good_list(struct database_t *db, double *list, const struct trade_good_t *good, double amount, boolean warn_repeated);

/* Unit */
struct unit_t {
	string name;
	u8 icon, naval_icon;
	enum unit_terrain_type_t type;
	boolean capital, sail;
	string sprite;
	boolean active;
	enum unit_type_t unit_type;
	string move_sound, select_sound;
	string sprite_override, sprite_mount, sprite_mount_attach_node;
	boolean transport, floating_flag;
	double colonial_points;

	double priority, max_strength, default_organisation;
	double maximum_speed, weighted_value;
	boolean can_build_overseas;

	int build_time;
	double *build_cost;

	s8 min_port_level, limit_per_port;
	double supply_consumption_score;

	double supply_consumption, *supply_cost;
	/* land */
	double reconnaissance, attack, defence, discipline, support, maneuver, siege;
	/* naval */
	double hull, gun_power, fire_range, evasion, torpedo_attack;
};

/* Ideology */
struct ideology_t {
	string name;
	struct ideology_group_t *group;
	boolean uncivilized, can_reduce_militancy;
	u32 color;
	date_t date;
};

struct ideology_t ideology_default(void);

struct government_type_t {
	string name;
	/* will be fixed to number of ideologies */
	boolean *ideologies;
	boolean election, appoint_ruling_party;
	u8 duration;
	enum flag_type_t flag;
};

void government_type_init(const struct database_t *db, struct government_type_t *gov);

/* Issue */
struct issue_t {
	string name;
	struct issue_group_t *group;
};

/* Reform */
struct reform_t {
	string name;
	struct reform_group_t *group;
	enum reform_type_t type;
};

/* Political Party */
struct party_t {
	string name;
	date_t start, end;
	struct ideology_t *ideology;
	struct issue_t **issues;
};

void party_issues_init(const struct database_t *db, struct party_t *party);
void party_free(struct party_t *party);

/* Culture */
struct culture_t {
	string name;
	struct culture_group_t *group;
	u32 color;
	u8 radicalism;
	struct country_t *primary;
	string *first_names, *last_names;
};

/* Culture Group */
struct culture_group_t {
	string name;
	struct culture_t **cultures;

	enum leader_t leader;
	enum graphical_culture_t unit;
	struct country_t *cultural_union;
};

void culture_group_add_culture(struct culture_group_t *culture_group, struct culture_t *culture);
boolean culture_group_contains_culture(const struct culture_group_t *culture_group, const struct culture_t *culture);

/* Religion */
struct religion_t {
	string name;
	struct religion_group_t *group;
	u8 icon;
	u32 color;
	boolean pagan;
};

/* Country */
struct country_t {
	struct tag_t tag;
	string defines_location, oob_location;
	u32 color;
	enum graphical_culture_t graphical_culture;
	struct province_t *capital;
	struct culture_t *primary_culture;
	struct culture_t **accepted_cultures;
	struct religion_t *religion;
	struct government_type_t *government;
	double plurality;
	struct national_value_t *nv;
	double literacy, non_state_culture_literacy;
	boolean civilized, is_releasable_vassal;
	double prestige;
	struct party_t *ruling_party;
	struct party_t *parties;
	double *upper_house;
	struct reform_t **reforms;
	double consciousness, nonstate_consciousness;
	date_t last_election;
	string *flags;

	boolean history_defined;
};

void country_upper_house_init(const struct database_t *db, struct country_t *country);
void country_reforms_init(const struct database_t *db, struct country_t *country);
void country_add_party(struct country_t *country, const struct party_t *party);
struct party_t *country_get_party(struct country_t *country, const string *party);
void country_add_accepted_culture(struct country_t *country, struct culture_t *culture);
boolean country_has_accepted(const struct country_t *country, const struct culture_t *culture);
boolean country_has_flag(const struct country_t *country, const string *flag);

/* Province */
struct province_t {
	u16 id;
	u32 color;
	struct state_t *state;
	struct country_t *owner, *controller;
	struct country_t **cores;
	struct trade_good_t *rgo;
	u8 life_rating, railroad, naval_base, fort, colonial;
	boolean sea_start;
	string *flags;

	boolean history_defined;
};

boolean province_has_core(const struct province_t *province, const struct country_t *country);
void province_add_core(struct province_t *province, struct country_t *country);
boolean province_has_flag(const struct province_t *province, const string *flag);

#define template_ref_list_declare(type, list_type, list_name) struct type##_t {							\
																string name;							\
																struct list_type##_t **list_name; };	\
							void type##_add_##list_type(struct type##_t *type, struct list_type##_t *list_type);
	for_all_database_ref_lists(template_ref_list_declare)
#undef template_ref_list_declare

#define template_ref_list_contains_dec(type, list_type, list_name) boolean type##_contains_##list_type(const struct type##_t *type, const struct list_type##_t *list_type);
	for_all_database_ref_lists(template_ref_list_contains_dec)
#undef template_ref_list_contains_dec

boolean state_contains_province(const struct state_t *state, const struct province_t *prov);

/* Database */
struct database_t {

#define template_list(type,plural) struct type##_t *plural;
	for_all_database_lists(template_list)
#undef template_list

	struct map_t {
		s32 width, height, size;
		RenderBuffer province_id, province_col, province_owner;
	} map;

	size_t land_province_count, sea_province_count;

	struct load_status_t {
		struct common_loaded_t {
			boolean countries;
			boolean country_defines;	/* requires countries, ideology, issues */
			boolean cultures;	/* requires countries */
			boolean trade_goods;
			boolean ideologies;
			boolean issues;
			boolean government_types;	/* requires ideologies */
			boolean national_values;
			boolean religions;
		} common;
		struct map_loaded_t {
			boolean sea_starts;	/* requires province_defines */
			boolean province_defines;
			boolean province_shapes;	/* requires province_defines */
			boolean states;	/* requires province_defines */
		} map;
		boolean units;	/* requires trade goods */
		struct history_loaded_t {	/* foo.[all things in foo <and these ones are redundantly listed as they are also required for other things in the list>] */
			boolean countries;	/* requires common.[countries,cultures,<ideologies>,government_types,national_values,religions], provinces.definitions*/
			boolean provinces; /* requires definitions */
		} history;
	} load_status;
};

int database_load_all(struct database_t *db);
void database_free_all(struct database_t *db);

/* these add exact copies, without making new pointers */
#define template_list_add_dec(type, plural) void database_add_##type(struct database_t *db, const struct type##_t *type);
	for_all_database_lists(template_list_add_dec)
#undef template_add_dec
#define template_list_free_dec(type, plural) void type##_free(struct type##_t *type);
	for_all_database_lists(template_list_free_dec)
#undef template_list_free_dec
#define template_list_index_dec(type, plural) size_t database_##type##_index(struct database_t *db, const struct type##_t *type);
		for_all_database_lists(template_list_index_dec)
#undef template_list_index_dec

/* Getters */
struct country_t *database_get_country(struct database_t *db, const struct tag_t *tag);
struct province_t *database_get_province(struct database_t *db, int id);
struct province_t *database_get_province_col(struct database_t *db, u32 color);

#define template_list_get_by_name_dec(type,plural) struct type##_t *database_get_##type(struct database_t *db, const string *name);
	for_all_database_lists_named(template_list_get_by_name_dec)
#undef template_list_get_by_name_dec

typedef u32(*map_mode_t)(struct province_t *prov);
void database_apply_mapmode(struct database_t *db, RenderBuffer *rb, map_mode_t map_mode);

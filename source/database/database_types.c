#include "database.h"

#include "assert_opt.h"
#include "memory_opt.h"

u32 to_color(const u8 color[3]) {
	return ((color[0] & 0xFF) << 16) | ((color[1] & 0xFF) << 8) | (color[2] & 0xFF);
}

/* TAGS */
boolean tag_valid(const struct tag_t *tag) {
	assert(tag && "tag_valid: tag == 0");
	for (int i = 0; i < 3; ++i)
		if (!(('A' <= tag->text[i] && tag->text[i] <= 'Z') || (i && '0' <= tag->text[i] && tag->text[i] <= '9'))) return false;
	return true;
}
boolean tag_equal(const struct tag_t *tagA, const struct tag_t *tagB) {
	assert(tagA && "tag_equal: tagA == 0");
	assert(tagB && "tag_equal: tagB == 0");
	for (int i = 0; i < 3; ++i)
		if (tagA->text[i] != tagB->text[i]) return false;
	return true;
}

#define template_free_named(type) void type##_free(struct type##_t *type) {				\
									assert(type && #type "_free: " #type " == 0");	\
									string_clear(&type->name); }

/* NATIONAL VALUE */
template_free_named(national_value)

/* TRADE GOODS */
struct trade_good_t trade_good_default(void) {
	local const struct trade_good_t ret = { .cost = 1.0, .color = 0xFF0000, .available_from_start = true, .overseas_penalty = false, .money = false, .tradeable = true };
	return ret;
}
template_free_named(trade_good)
double *create_trade_good_list(const struct database_t *db) {
	assert(db && "create_trade_good_list: db == 0");
	assert(db->trade_goods && "create_trade_good_list: db->trade_goods == 0");
	double *ret = calloc_s(buf_len(db->trade_goods) * sizeof(double));
	assert(ret && "create_trade_good_list: calloc_s failed");
	return ret;
}
void add_to_trade_good_list(struct database_t *db, double *list, const struct trade_good_t *good, double amount, boolean warn_repeated) {
	assert(db && "add_to_trade_good_list: db == 0");
	assert(list && "add_to_trade_good_list: list == 0");
	assert(good && "add_to_trade_good_list: good == 0");
	if (amount == 0.0) {
		fprintf(stdout, "[add_to_trade_good_list] Adding 0 %s to a trade good list.", good->name.text);
		return;
	}
	size_t idx = database_trade_good_index(db, good);
	assert(0 <= idx && idx < buf_len(list) && "add_to_trade_good_list: good not in database's trade good list");
	if (warn_repeated && list[idx] != 0.0) fprintf(stdout, "[add_to_trade_good_list] %s repeated in trade good list.", good->name.text);
	list[idx] += amount;
}

/* UNIT */
template_free_named(unit)

/* IDEOLOGY */
struct ideology_t ideology_default(void) {
	local const struct ideology_t ret = { .uncivilized = true };
	return ret;
}
template_free_named(ideology)

/* GOVERNMENT TYPE */
void government_type_init(const struct database_t *db, struct government_type_t *gov) {
	assert(db && "[government_type_init] db == 0");
	assert(gov && "[government_type_init] gov == 0");
	assert(db->load_status.common.ideologies && "[government_type_init] ideologies must be loaded before government types");
	assert(gov->ideologies == 0 && "[government_type_init] gov->ideologies is already allocated");
	gov->ideologies = calloc_s(buf_len(db->ideologies) * sizeof(boolean));
}
void government_type_free(struct government_type_t *gov) {
	assert(gov && "[government_type_free] gov == 0");
	string_clear(&gov->name);
	free_s(gov->ideologies);
}

/* ISSUE */
template_free_named(issue)

/* REFORM */
template_free_named(reform)

/* PARTY */
void party_issues_init(const struct database_t *db, struct party_t *party) {
	assert(db && "party_init: db == 0");
	assert(party && "party_init: party == 0");
	assert(db->load_status.common.issues && "party_init: issues must be loaded before parties");
	assert(party->issues == 0 && "party_init: party->issues is already allocated");
	party->issues = calloc_s(buf_len(db->issue_groups) * sizeof(struct issue_t *));
}
void party_free(struct party_t *party) {
	assert(party && "party_free: party == 0");
	string_clear(&party->name);
	free_s(party->issues);
}

/* CULTURE */
void culture_free(struct culture_t *culture) {
	assert(culture && "culture_free: culture == 0");
	string_clear(&culture->name);
	for_buf(i, culture->first_names)
		string_clear(&culture->first_names[i]);
	buf_free(culture->first_names);
	for_buf(i, culture->last_names)
		string_clear(&culture->last_names[i]);
	buf_free(culture->last_names);
}

/* RELIGION */
template_free_named(religion)

/* COUNTRY */
void country_upper_house_init(const struct database_t *db, struct country_t *country) {
	assert(db && "country_upper_house_init: db == 0");
	assert(country && "country_upper_house_init: country == 0");
	assert(db->load_status.common.ideologies && "country_upper_house_init: ideologies must be loaded before country histories");
	assert(country->upper_house == 0 && "country_upper_house_init: country->upper_house is already allocated");
	country->upper_house = calloc_s(buf_len(db->ideologies) * sizeof(double));
}
void country_reforms_init(const struct database_t *db, struct country_t *country) {
	assert(db && "country_reforms_init: db == 0");
	assert(country && "country_reforms_init: country == 0");
	assert(db->load_status.common.issues && "country_reforms_init: reforms must be loaded before country histories");
	assert(country->reforms == 0 && "country_reforms_init: country->reforms is already allocated");
	country->reforms = calloc_s(buf_len(db->reform_groups) * sizeof(struct reform_t *));
}
void country_free(struct country_t *country) {
	assert(country && "country_free: country == 0");
	string_clear(&country->defines_location);
	string_clear(&country->oob_location);
	for_buf(i, country->parties)
		party_free(&country->parties[i]);
	buf_free(country->parties);
	buf_free(country->accepted_cultures);
	free_s(country->upper_house);
	free_s(country->reforms);
	for_buf(i, country->flags)
		string_clear(&country->flags[i]);
	buf_free(country->flags);
}
void country_add_party(struct country_t *country, const struct party_t *party) {
	assert(country && "country_add_party: country == 0");
	assert(party && "country_add_party: party == 0");
	buf_push(country->parties, *party);
}
struct party_t *country_get_party(struct country_t *country, const string *party) {
	assert(country && "country_get_party: country == 0");
	assert(party && "country_get_party: party == 0");
	for_buf(i, country->parties)
		if (string_equal(&country->parties[i].name, party))
			return &country->parties[i];
	return 0;
}
void country_add_accepted_culture(struct country_t *country, struct culture_t *culture) {
	assert(country && "country_add_accepted_culture: country == 0");
	assert(culture && "country_add_accepted_culture: culture == 0");
	buf_push(country->accepted_cultures, culture);
}
boolean country_has_accepted(const struct country_t *country, const struct culture_t *culture) {
	assert(country && "country_has_accepted: country == 0");
	assert(culture && "country_has_accepted: culture == 0");
	for_buf(i, country->accepted_cultures)
		if (country->accepted_cultures[i] == culture) return true;
	return false;
}
boolean country_has_flag(const struct country_t *country, const string *flag) {
	assert(country && "country_has_flag: country == 0");
	assert(flag && "country_has_flag: flag == 0");
	for_buf(i, country->flags)
		if (string_equal(&country->flags[i], flag)) return true;
	return false;
}

/* PROVINCE */
void province_free(struct province_t *province) {
	assert(province && "province_free: country == 0");
	buf_free(province->cores);
	for_buf(i, province->flags)
		string_clear(&province->flags[i]);
	buf_free(province->flags);
}
boolean province_has_core(const struct province_t *province, const struct country_t *country) {
	assert(province && "province_has_core: province == 0");
	assert(country && "province_has_core: country == 0");
	for_buf(i, province->cores)
		if (province->cores[i] == country) return true;
	return false;
}
void province_add_core(struct province_t *province, struct country_t *country) {
	assert(province && "province_add_core: province == 0");
	assert(country && "province_add_core: country == 0");
	if (province_has_core(province, country)) {
		fprintf(stdout, "[province_add_core] readding %s core to province %d\n", country->tag.text, province->id);
		return;
	}
	buf_push(province->cores, country);
}
boolean province_has_flag(const struct province_t *province, const string *flag) {
	assert(province && "province_has_flag: province == 0");
	assert(flag && "province_has_flag: flag == 0");
	for_buf(i, province->flags)
		if (string_equal(&province->flags[i], flag)) return true;
	return false;
}

#define template_free_named_ref_list(type, list_type, list_name) void type##_free(struct type##_t *type) {\
														assert(type && #type "_free: " #type " == 0");	\
														string_clear(&type->name);						\
														buf_free(type->list_name); }

#define template_list_add_ref(type, list_type, list_name) void type##_add_##list_type(struct type##_t *type, struct list_type##_t *list_type) {	\
									assert(type && #type "_add_" #list_type ": " #type " == 0");								\
									assert(list_type && #type "_add_" #list_type ": " #list_type " == 0");						\
									buf_push(type->list_name, list_type); }

#define template_list_contains_ref(type, list_type, list_name) boolean type##_contains_##list_type(const struct type##_t *type, const struct list_type##_t *list_type) {	\
																assert(type && #type "_contains_" #list_type ": " #type " == 0");							\
																assert(list_type && #type "_contains_" #list_type ": " #list_type " == 0");					\
																for_buf(i, type->list_name)																	\
																	if (type->list_name[i] == list_type) return true;										\
																return false; }

	for_all_database_ref_lists(template_free_named_ref_list)
	for_all_database_ref_lists(template_list_add_ref)
	for_all_database_ref_lists(template_list_contains_ref)
	/* CULTURE GROUP */
	template_free_named_ref_list(culture_group, culture, cultures)
	template_list_add_ref(culture_group, culture, cultures)
	template_list_contains_ref(culture_group, culture, cultures)

#undef template_free_named
#undef template_free_named_ref_list
#undef template_list_add_ref
#undef template_list_contains_ref

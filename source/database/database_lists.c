#include "database.h"

#include "assert_opt.h"
#include "memory_opt.h"

#include <stdio.h>

/* LIST ADDERS */
/* adds an exact copy, without making new pointers */
#define template_list_add(type,plural) void database_add_##type(struct database_t *db, const struct type##_t *type) {	\
									assert(db && "database_add_" #type ": db == 0");					\
									assert(type && "database_add_" #type ": " #type " == 0");			\
									buf_push(db->plural,*type); }
	for_all_database_lists(template_list_add)
#undef template_add

/* LIST GETTERS */
struct country_t *database_get_country(struct database_t* db, const struct tag_t *tag) {
	assert(db && "database_add_country: db == 0");
	assert(tag_valid(tag) && "database_get_country: invalid tag");
	for_buf(i, db->countries)
		if (tag_equal(tag, &db->countries[i].tag))
			return &db->countries[i];
	return 0;
}
struct province_t *database_get_province(struct database_t *db, int id) {
	assert(db && "database_get_province: db == 0");
	if (id <= 0 || id > buf_len(db->provinces)) return 0;
	struct province_t *ret = &db->provinces[id - 1];
	assert(ret->id == id && "[database_get_province] province id doesn't match index");
	return ret;
	/*for_buf(i, db->provinces) {
		if (db->provinces.list[i].id == id)
			return &db->provinces[i];
	}
	return 0;*/
}
struct province_t *database_get_province_col(struct database_t *db, u32 color) {
	assert(db && "database_get_province_col: db == 0");
	color &= 0xFFFFFF;
	for_buf(i, db->provinces)
		if (db->provinces[i].color == color)
			return &db->provinces[i];
	return 0;
}
#define template_list_get_by_name(type,plural) struct type##_t *database_get_##type(struct database_t* db, const string *name) {	\
												assert(db && "database_get_" #type ": db == 0");					\
												assert(name && "database_get_" #type ": name == 0");				\
												for_buf(i, db->plural) {											\
													if (string_equal(&db->plural[i].name, name))					\
														return &db->plural[i];										\
												} return 0; }
	for_all_database_lists_named(template_list_get_by_name)
#undef template_list_get_by_name

#define template_list_index(type, plural) size_t database_##type##_index(struct database_t *db, const struct type##_t *type) {			\
											assert(db && "database_" #type "_index: db == 0");	\
											assert(type && "database_" #type "_index: " #type " == 0");	\
											return (size_t)(type - db->plural); }
		for_all_database_lists(template_list_index)
#undef template_list_index

void database_free_all(struct database_t *db) {
	/* LOAD ORDER: ideologies, countries, [country defines], trade goods, map, states, [province histories], cultures, religions
		[ ] indicates freed with a previous entry */

#define template_list_free(type,plural) for_buf(i, db->plural)			\
											type##_free(&db->plural[i]);\
										buf_free(db->plural);
		for_all_database_lists(template_list_free)
#undef template_list_free

	RB_free_pixels(&db->map.province_col);
	RB_free_pixels(&db->map.province_id);
	RB_free_pixels(&db->map.province_owner);
}

void database_apply_mapmode(struct database_t *db, RenderBuffer *rb, map_mode_t map_mode) {
	assert(db && "database_apply_mapmode: db == 0");
	assert(rb && "database_apply_mapmode: rb == 0");
	assert(map_mode && "database_apply_mapmode: map_mode == 0");
	assert(db->map.size && db->map.province_id.pixels && "database_apply_mapmode: province id map not loaded");
	RB_resize(rb, db->map.width, db->map.height);
	assert(rb->pixels && "database_apply_mapmode: failed to allocate rb");
	int left = 0, up = 0;
	for (int i = 0; i < rb->size; ++i) {
		const u32 id = db->map.province_id.pixels[i];
		if (i >= db->map.width && db->map.province_id.pixels[i - db->map.width] == id)
			rb->pixels[i] = rb->pixels[i - db->map.width];
		else if ((i % db->map.width) != 0 && db->map.province_id.pixels[i - 1] == id)
			rb->pixels[i] = rb->pixels[i - 1];
		else {
			struct province_t *prov = database_get_province(db, id);
			if (prov) rb->pixels[i] = map_mode(prov);
			else rb->pixels[i] = 0xFF0000;
		}
	}
}


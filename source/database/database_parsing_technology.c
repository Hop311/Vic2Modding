#include "database_parsing.h"

#include "assert_opt.h"
#include "memory_opt.h"

int read_technology_groups(database_t *db, const char *filename) {
	assert(db && "read_units: db == 0");
	assert(filename && "read_units: filename == 0");
	struct token_source_t src = { 0 };
	int err = token_source_init(&src, filename);
	if_err_ret
	struct token_t token = { 0 };
	while (token_source_next(&src, &token)) {
		if (token.type == ALPHANUMERIC) {
			if (string_equal_c(&token.data.str, "folders")) {
				if (token_source_expect_symbol(&src, '=', __func__, "folders")) err_break;
				if (token_source_expect_symbol(&src, '{', __func__, "folders")) err_break;
				while (token_source_next(&src, &token)) {

				}
				if (err) break;
			} else if (string_equal_c(&token.data.str, "schools") {

			} else {
				fprintf(stdout, "[read_units] Unrecognised alphanumeric (expected tech folders or schools): %s [line:%zu]\n", token.data.str.text, src.line_number);
					err_break;
			}
		} else {
			fprintf(stdout, "[read_units] Invalid token (expected alphanumeric tech folders or schools): ");
				token_print(stdout, &token);
				fprintf(stdout, " [line:%zu]\n", src.line_number);
				err_break;
		}
	}
	token_free(&token);
	token_source_free(&src);

	fprintf(stdout, "[read_units] Loaded %zu units.\n", buf_len(db->units));
	return err;
}

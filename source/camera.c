/* included in winmain.c */

typedef struct {
	vec2f pos;
	vec2 screen_dims;
	float zoom;
} camera_t;

camera_t camera = { .pos = { 0 }, .zoom = 0.4f, .screen_dims = { 0 } };

vec2f screen_to_worldf(vec2f v) {
	v.x -= (float)(camera.screen_dims.x / 2);
	v.y -= (float)(camera.screen_dims.y / 2);
	v.x = v.x / camera.zoom;
	v.y = v.y / camera.zoom;
	v.x += camera.pos.x;
	v.y += camera.pos.y;
	return v;
}
vec2 screen_to_world(vec2 v) {
	vec2f vf = { .x = (float)v.x, .y = (float)v.y };
	vf = screen_to_worldf(vf);
	v.x = (s32)vf.x;
	v.y = (s32)vf.y;
	return v;
}
vec2f world_to_screenf(vec2f v) {
	v.x -= camera.pos.x;
	v.y -= camera.pos.y;
	v.x = v.x * camera.zoom;
	v.y = v.y * camera.zoom;
	v.x += (float)(camera.screen_dims.x / 2);
	v.y += (float)(camera.screen_dims.y / 2);
	return v;
}
vec2 world_to_screen(vec2 v) {
	vec2f vf = { .x = (float)v.x, .y = (float)v.y };
	vf = world_to_screenf(vf);
	v.x = (s32)vf.x;
	v.y = (s32)vf.y;
	return v;
}

void world_to_screen_quad(vec2 *corner, vec2 *dims) {
	assert(corner && "world_to_screen_quad: corner == 0");
	assert(dims && "world_to_screen_quad: dims == 0");

	corner->x += dims->x / 2;
	corner->y += dims->y / 2;

	*corner = world_to_screen(*corner);

	dims->x = (s32)((float)dims->x * camera.zoom);
	dims->y = (s32)((float)dims->y * camera.zoom);

	corner->x -= dims->x / 2;
	corner->y -= dims->y / 2;
}

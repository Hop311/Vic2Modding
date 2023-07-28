#include "types.h"

#include "win32_tools.h"
#include "render.h"
#include "maths.h"
#include "assert_opt.h"
#include "memory_opt.h"

#include "lexer.h"
#include "database.h"

#include <stdio.h>
#include <time.h>

#include <wingdi.h>

#include "camera.c"

#define START_DIMS 1280,720

#define profile_out if(0) fprintf

/* Engine */
HWND main_window;
boolean running = true;
RenderBuffer renderbuffer = { 0 };
BITMAPINFO win32_bitmap_info;
vec2 mouse_pos;

/* Content */
struct database_t database = { 0 };
RenderBuffer map = { 0 };

u32 map_mode_owner(struct province_t * prov) {
	if (prov->owner) return prov->owner->color;
	if (!prov->sea_start) return 0xAAAAAA;
	return prov->color;
}
u32 map_mode_rgo(struct province_t *prov) {
	if (prov->rgo) return prov->rgo->color;
	if (!prov->sea_start) return 0xFF0000;
	return prov->color;
}
u32 map_mode_state(struct province_t *prov) {
	if (prov->state) return prov->state->provinces[0]->color;
	if (!prov->sea_start) return 0xFF00FF;
	return prov->color;
}
#include "database_parsing.h"
void init_map(void) {

	/*lexer_check_all_in_folder(MOD_FOLDER "common");
	lexer_check_all_in_folder(MOD_FOLDER "decisions");
	lexer_check_all_in_folder(MOD_FOLDER "events");
	lexer_check_all_in_folder(MOD_FOLDER "history");
	lexer_check_all_in_folder(MOD_FOLDER "interface");
	lexer_check_all_in_folder(MOD_FOLDER "inventions");
	lexer_check_all_in_folder(MOD_FOLDER "map");
	lexer_check_all_in_folder(MOD_FOLDER "poptypes");
	lexer_check_all_in_folder(MOD_FOLDER "technologies");
	lexer_check_all_in_folder(MOD_FOLDER "units");
	lexer_check_file(MOD_FOLDER "settings.txt", "settings.txt");*/

	int err = database_load_all(&database);
	if (err) return;

	database_apply_mapmode(&database, &map, map_mode_owner);

	/*struct lexeme_t *root = lexeme_new();
	lexer_process_file(MOD_FOLDER "common/ideologies.txt", root);
	lexeme_print(root);
	lexeme_delete(root);*/

	//write_trade_goods(&database, "test");
}
void deinit_map(void) {
	RB_free_pixels(&map);
	database_free_all(&database);
}

#define KEY_UP 0
#define KEY_DOWN 1
#define KEY_LEFT 2
#define KEY_RIGHT 3
boolean keys[4] = { 0 };

static const float base_speed = 15.0f;

void tick(void) {
	const float speed = base_speed / camera.zoom;
	if (keys[KEY_UP]) camera.pos.y += speed;
	if (keys[KEY_DOWN]) camera.pos.y -= speed;
	if (keys[KEY_LEFT]) camera.pos.x -= speed;
	if (keys[KEY_RIGHT]) camera.pos.x += speed;
}

void render(void) {
	RB_clear(&renderbuffer);

	vec2 corner = { .x = 0, .y = 0 };
	vec2 dims = { .x = map.width, .y = map.height };
	world_to_screen_quad(&corner, &dims);
	RB_draw_renderbuffer_sample(&renderbuffer, corner.x, corner.y, dims.x, dims.y, &map);
	//RB_draw_renderbuffer_sample(&renderbuffer, 0, 0, renderbuffer.width, renderbuffer.height, map);
}

/* Timinig */
double frequency_counter;
double seconds_elapsed(u64 last_counter) {
	LARGE_INTEGER current_counter;
	QueryPerformanceCounter(&current_counter);
	return (double)(current_counter.QuadPart - last_counter) / frequency_counter;
}
u64 get_perf_counter(void) {
	LARGE_INTEGER current_counter;
	QueryPerformanceCounter(&current_counter);
	return current_counter.QuadPart;
}

LRESULT CALLBACK window_callback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CREATE:
	{
		char title[64] = { 0 };
		int ret = GetWindowText(hwnd, title, 64);
		fprintf(stdout, "Window created: %s\n", ret ? title : "[untitled]");
	} break;
	case WM_CLOSE:
	{
		int ret = MessageBox(main_window, "Are you sure you want to quit?", "Escape", MB_OKCANCEL);
		if (ret == IDOK) {
			DestroyWindow(hwnd);
			running = false;
		}
	} break;
	case WM_DESTROY:
	{
		/* destroy any child windows/toolbars? */
		PostQuitMessage(0);
	} break;
	case WM_SIZE:
	{
		RECT rect;
		GetClientRect(hwnd, &rect);
		const s32 width = rect.right - rect.left;
		const s32 height = rect.bottom - rect.top;

		fprintf(stdout, "Window size: %d x %d\n", width, height);

		RB_resize(&renderbuffer, width, height);

		camera.screen_dims.x = width;
		camera.screen_dims.y = height;

		/* update the bitmapinfo */
		win32_bitmap_info.bmiHeader.biWidth = width;
		win32_bitmap_info.bmiHeader.biHeight = height;
	} break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}
void keyboard_input(u32 vk_code, boolean was_down, boolean is_down, boolean alt_down) {
	switch (vk_code) {
	case VK_ESCAPE: {
		SendMessage(main_window, WM_CLOSE, 0, 0);
	} break;
	case 'W':
	case VK_UP: { keys[KEY_UP] = is_down; } break;
	case 'S':
	case VK_DOWN: { keys[KEY_DOWN] = is_down; } break;
	case 'A':
	case VK_LEFT: { keys[KEY_LEFT] = is_down; } break;
	case 'D':
	case VK_RIGHT: { keys[KEY_RIGHT] = is_down; } break;
	}
}
void message_process(MSG message) {
	switch (message.message) {
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		u32 vk_code = (u32)message.wParam;
		boolean was_down = ((message.lParam & (1 << 30)) != 0);
		boolean is_down = ((message.lParam & (1 << 31)) == 0);
		boolean alt_down = ((message.lParam & (1 << 29)) != 0);
		keyboard_input(vk_code, was_down, is_down, alt_down);
	} break;

	case WM_MOUSEMOVE:
	{
		POINT mouse_pos_new;
		GetCursorPos(&mouse_pos_new);
		ScreenToClient(main_window, &mouse_pos_new);
		mouse_pos_new.y = renderbuffer.height - mouse_pos_new.y;

		mouse_pos.x = mouse_pos_new.x;
		mouse_pos.y = mouse_pos_new.y;

	} break;
	case WM_MOUSEWHEEL:
	{
		short delta = GET_WHEEL_DELTA_WPARAM(message.wParam);
		assert(delta % 120 == 0 && "mousewheel delta not a multiple of 120");
		delta /= 120; /* +ve delta means scrolling up/forwards */
		camera.zoom *= (float)(10 + delta) / 10.0f;
	} break;
	case WM_LBUTTONDOWN:	/* Left mouse button */
	{
		const vec2 world_pos = screen_to_world(mouse_pos);
		if (world_pos.x >= 0 && world_pos.x < map.width && world_pos.y >= 0 && world_pos.y < map.height) {
			const size_t index = world_pos.x + world_pos.y * map.width;
			const u32 id = database.map.province_id.pixels[index];
			const struct province_t *prov = database_get_province(&database, id);
			fprintf(stdout, "[CLICK] col = #%06x, ", map.pixels[index]);
			if (prov) fprintf(stdout, "id = %d, owner=%s, rgo=%s, state=%s, sea_start=%s\n",
				prov->id, prov->owner ? prov->owner->tag.text : "NONE", prov->rgo ? prov->rgo->name.text : "NONE",
				prov->state ? prov->state->name.text : "NONE", prov->sea_start ? "yes" : "no");
			else fprintf(stdout, "NO PROVINCE\n");
		}
	} break;
	default:
	{	/* send messages to windows */
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow) {

	AllocConsole();
	FILE *res = 0;
	freopen_s(&res, "CONIN$", "r", stdin);
	freopen_s(&res, "CONOUT$", "w", stdout);
	freopen_s(&res, "CONOUT$", "w", stderr);

	WNDCLASSEX window_class = { 0 };
	HDC hdc;

	/* Registering the Window Class */
	if (init_WNDClass(&window_class, hInstance, window_callback)) return ERROR_RETURN;
	/* Creating the Window */
	if (create_HWND(&main_window, &window_class, hInstance, "Vic2Modding", START_DIMS)) return ERROR_RETURN;

	/* Graphics setup */
	hdc = GetDC(main_window);
	win32_bitmap_info.bmiHeader.biSize = sizeof(win32_bitmap_info.bmiHeader);
	win32_bitmap_info.bmiHeader.biPlanes = 1;
	win32_bitmap_info.bmiHeader.biBitCount = 32;
	win32_bitmap_info.bmiHeader.biCompression = BI_RGB;

	init_map();

	/* Activate window */
	ShowWindow(main_window, nCmdShow);
	UpdateWindow(main_window);

	/* FPS setup */
	const int refresh_rate = GetDeviceCaps(hdc, VREFRESH);
	double last_dt = 1.0 / (double)refresh_rate, second_counter = 0.0;
	int frames = 0;
	const double target_dt = last_dt;
	{
		LARGE_INTEGER frequency_counter_large;
		QueryPerformanceFrequency(&frequency_counter_large);
		frequency_counter = (double)frequency_counter_large.QuadPart;
	}
	u64 last_counter = get_perf_counter();

	/* Message Loop */
	MSG message = { 0 };
	while (running) {

		profile_out(stdout, "[TICK] start = %f\n", seconds_elapsed(last_counter));

		/* Input */
		while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
			message_process(message);
		tick();

		profile_out(stdout, "[RENDER] start = %f\n", seconds_elapsed(last_counter));

		/* Render */
		{
			render();
			SetStretchBltMode(hdc, HALFTONE);
			StretchDIBits(hdc, 0, 0, win32_bitmap_info.bmiHeader.biWidth, win32_bitmap_info.bmiHeader.biHeight,
				0, 0, renderbuffer.width, renderbuffer.height,
				renderbuffer.pixels, &win32_bitmap_info, DIB_RGB_COLORS, SRCCOPY);
			frames++;
		}
		profile_out(stdout, "[TIMING] start = %f\n", seconds_elapsed(last_counter));

		/* Timing */
		{
			if (true) {
				const double dt_before_sleep = min(0.1, seconds_elapsed(last_counter));
				int sleep = (int)((target_dt - dt_before_sleep) * 1000.0);
				if (sleep > 1)
					Sleep(sleep - 1);
			}
			last_dt = min(0.1, seconds_elapsed(last_counter));
			second_counter += last_dt;
			if (second_counter >= 1.0) {
				second_counter -= 1.0;
				//fprintf(stdout, "[FPS:%d]\n", frames);
				frames = 0;
			}
			last_counter = get_perf_counter();
		}
	}

	deinit_map();
	RB_free_pixels(&renderbuffer);

	check_memory_leaks();
	int i = getchar();

	return (int)message.wParam;
}

#include "win32_tools.h"

#include "types.h"

#define WINDOW_STYLE_EX WS_EX_CLIENTEDGE	// WS_EX_STATICEDGE WS_EX_WINDOWEDGE WS_EX_TOOLWINDOW WS_EX_DLGMODALFRAME
#define WINDOW_STYLE WS_OVERLAPPEDWINDOW	// WS_BORDER WS_CAPTION WS_CHILD WS_CLIPCHILDREN WS_CLIPSIBLINGS WS_DLGFRAME

void ErrorMessage(const char *msg) {
	MessageBox(NULL, msg, "Error!", MB_ICONEXCLAMATION | MB_OK);
}

int init_WNDClass(WNDCLASSEX *window_class, HINSTANCE hInstance, WNDPROC window_callback) {
	window_class->cbSize = sizeof(WNDCLASSEX);
	window_class->style = CS_HREDRAW | CS_VREDRAW;
	window_class->lpfnWndProc = window_callback;
	//window_class->cbClsExtra = 0;
	//window_class->cbWndExtra = 0;
	window_class->hInstance = hInstance;
	//window_class->hIcon = LoadIcon(NULL, IDI_APPLICATION);
	//window_class->hCursor = LoadCursor(NULL, IDC_ARROW);
	//window_class->hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	//window_class->lpszMenuName = NULL;
	window_class->lpszClassName = "WNDCLASS_mapgame";
	//window_class->hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	if (!RegisterClassEx(window_class)) {
		ErrorMessage("Window Registration Failed!");
		return ERROR_RETURN;
	}
	return 0;
}

int create_HWND(HWND *window, WNDCLASSEX *window_class, HINSTANCE hInstance, const char *name, int width, int height) {
	*window = CreateWindowEx(WINDOW_STYLE_EX,	window_class->lpszClassName, name, WINDOW_STYLE, CW_USEDEFAULT, CW_USEDEFAULT, width, height,
		// parent, menu
		NULL, NULL, hInstance, NULL);
	if (*window == NULL) {
		ErrorMessage("Window Creation Failed!");
		return ERROR_RETURN;
	}
	return 0;
}


LPVOID VAlloc(SIZE_T dwSize) {
	return VirtualAlloc(0, dwSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}
BOOL VFree(LPVOID lpAddress) {
	return VirtualFree(lpAddress, 0, MEM_RELEASE);
}

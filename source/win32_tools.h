#pragma once

#include <windows.h>

void ErrorMessage(const char *msg);

/* Initialise and register a standard window class, returns 0 if successful. */
int init_WNDClass(WNDCLASSEX *window_class, HINSTANCE hInstance, WNDPROC window_callback);
int create_HWND(HWND *window, WNDCLASSEX *window_class, HINSTANCE hInstance, const char *name, int width, int height);


/* returns 0 on error, non-zero is pointer to allocated memory */
LPVOID VAlloc(SIZE_T dwSize);
/* return true (non-zero) on success, 0 on error */
BOOL VFree(LPVOID lpAddress);
#include "SDL_main.h"
#include <windows.h>

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	return SDL_WinRTRunApp(SDL_main, NULL);
}

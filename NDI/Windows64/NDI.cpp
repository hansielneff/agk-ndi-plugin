#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <stdio.h>

#include "../AGKLibraryCommands.h"

extern "C" DLL_EXPORT void StartBroadcast() {
	agk::CreateDummySprite();
}

/*
----------------------------------------------------------------------------------------------------
FILE NAME:			Main.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Ng Juin Herng, juinherng.ng@digipen.edu

DESCRIPTION:		Main entry point of the game. Contains the main function that initializes the game
					and runs the game loop.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/


#include <windows.h>
#include <crtdbg.h>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	bool isDebug =
	#if _DEBUG
			true;
		// Check for memory leaks
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	#else
			false;
	#endif

	// Suppress compiler warnings about the following unused parameters
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	// Simple message box to test if the solution is setup properly
	MessageBox(NULL, "Test Solution", "LetsEatMonsters", MB_OK);

	return 0;
}

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

#include "config-manager.hpp"
#include <iostream>

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
	// MessageBox(NULL, "Test Solution", "LetsEatMonsters", MB_OK);

	ConfigManager::Settings s;              // defaults
	ConfigManager::load("config.txt", s);     // keep defaults if file missing

	// Register a minimal window class
	WNDCLASSA wc = {};
	wc.lpfnWndProc = DefWindowProcA;
	wc.hInstance = hInstance;
	wc.lpszClassName = "MyGameWindow";
	RegisterClassA(&wc);

	DWORD style = s.fullscreen ? WS_POPUP : WS_OVERLAPPEDWINDOW;

	HWND hwnd = CreateWindowEx(
		0,
		wc.lpszClassName,
		"My Game",
		style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		s.resolution.width, s.resolution.height,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);

	// Apply fullscreen by changing display settings
	if (s.fullscreen) {
		DEVMODE dm = {};
		dm.dmSize = sizeof(dm);
		dm.dmPelsWidth = s.resolution.width;
		dm.dmPelsHeight = s.resolution.height;
		dm.dmBitsPerPel = 32;
		dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
		ChangeDisplaySettings(&dm, CDS_FULLSCREEN);

		SetWindowLong(hwnd, GWL_STYLE, WS_POPUP);
		SetWindowPos(hwnd, HWND_TOP, 0, 0,
			s.resolution.width, s.resolution.height,
			SWP_SHOWWINDOW);
	}

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	// Build a message box to confirm settings
	char buf[256];
		_snprintf_s(buf, _TRUNCATE,
			"parameters:\n"
		"- window resolution: %dx%d\n"
		"- fullscreen mode: %s\n"
		"- audio volume: %.2f",
		s.resolution.width, s.resolution.height,
		s.fullscreen ? "true" : "false",
		s.audio_volume);
	MessageBoxA(hwnd, buf, "Game Config", MB_OK);

	// Basic message loop (so the window stays up)
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Restore display mode when exiting fullscreen
	if (s.fullscreen) {
		ChangeDisplaySettings(nullptr, 0);
	}

	return (int)msg.wParam;
}

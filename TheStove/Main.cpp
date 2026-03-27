/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Main.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (100%)

 DESCRIPTION:		Entry point for the desktop application. Sets up debug-time
					CRT diagnostics and delegates the runtime lifecycle to the
					Application wrapper.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "Core/Application.hpp"
#include "Core/Logger.hpp"

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

#define DBG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DBG_NEW
#endif

 /**
  * @brief Enters the application, runs the main lifecycle, and returns the exit code.
  * @return Zero on success, or a non-zero value when startup or runtime fails.
  */
int main() {
#ifdef _DEBUG
	// Enable CRT leak reporting before any heap allocations happen in debug builds.
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
	TS_LOG_DEBUG("=== Memory leak detection enabled ===");
#endif

	Application app;
	const int exitCode = app.Run();

#ifdef _DEBUG
	// Leave a clear marker around the leak report in the debug log.
	TS_LOG_DEBUG("=== Memory Leak Report ===");
	TS_LOG_DEBUG("Checking for memory leaks...");
	TS_LOG_DEBUG("If no leaks are detected, no additional output will appear below.");
	TS_LOG_DEBUG("=== End of Memory Leak Report ===");
#endif

	return exitCode;
}

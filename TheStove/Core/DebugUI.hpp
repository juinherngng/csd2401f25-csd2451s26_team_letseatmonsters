/*
----------------------------------------------------------------------------------------------------
FILE NAME:			DebugUI.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Glenn Yeo Yi Heng, g.yeo@digipen.edu

DESCRIPTION:		The declarations of functions for the debugger window.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <streambuf>
#include <ostream>
#include <string>

#include "Precompiled.hpp"
#include "AudioManager.hpp"
#include "../Graphics/GraphicsEngine.hpp"

namespace CoreFramework { class CoreEngine; }

struct SystemPerformance
{
	std::string name; // Name of the system
	float percentageOf; // The %tage of the total game loop
};

enum class FPSMode
{
	Unlimited,
	VSYNC,
	Capped
};

namespace Debug
{
	class DebuggerApp
	{
	public:
		// Ctor
		DebuggerApp();

		// Dtor
		~DebuggerApp();

		//Shutdown
		void Shutdown();

		// Initializes the debugger app - now takes CoreEngine pointer
		bool InitializeDebuggerApp(GLFWwindow* externalWindow, CoreFramework::CoreEngine* coreEnginePtr);

		// Updates debugger state (logic, hotkeys, toggles)
		void UpdateDebuggerApp();

		// Loads the ImGui window every frame
		void RenderDebuggerApp();

		// Runs a full debugger frame (Update + Render)
		void RunDebuggerApp();

		// Logs an error into a txt file as a crash error
		void LogError(const std::string& errorMessage);

		// Updates each systems %tage usage of the current engine
		void UpdateSystemTimes(float loopTime);

		bool IsActive() const { return openedDebugger; }

		void AddDebugLine(const std::string& txt);

		void ClearDebugLog();

		void ShowDebugLog();

		void SetRenderStats(int objects, int batches, int instanced, int draws);

	public:
		float fps = 0; // FPS 
		float msperFrame = 0; // MS/frame
		// Vector of SystemPerformance structs to store data for system performance
		std::vector<SystemPerformance> sysPerformance;

		// FPS control
		FPSMode fpsMode = FPSMode::VSYNC; // By default

		bool openedDebugger; // Shows Whether debugger window is visible

		std::vector<std::string> debuglines;
	private:
		GLFWwindow* debugWindow; // The host window
		CoreFramework::CoreEngine* coreEngine; // Pointer to CoreEngine (not owned)
		bool isInitialised; // Shows if the debugger was initialised or not

		// Crash logging
		std::ofstream crashlogFile; // The file stream to log errors to

		// Audio Values
		float bgm = 0.0f, vfx = 0.0f;

		int totalObjects = 0;
		int totalBatches = 0;
		int instancedObjects = 0;
		int drawCalls = 0;

	};
	extern DebuggerApp gDebugger;
}

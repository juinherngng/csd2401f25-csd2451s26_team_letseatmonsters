/*
----------------------------------------------------------------------------------------------------
FILE NAME:			ImGuiDebugger.hpp
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

#include "Precompiled.hpp"
//#include "Core.hpp"

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

class DebuggerApp
{
public:
	// Ctor
	DebuggerApp();

	// Dtor
	~DebuggerApp();

	// Initializes the debugger app
	bool InitializeDebuggerApp(GLFWwindow* externalWindow);

	// Updates debugger state (logic, hotkeys, toggles)
	void UpdateDebuggerApp();

	// Loads the ImGui window every frame
	void RenderDebuggerApp();

	// Runs a full debugger frame (Update + Render)
	void RunDebuggerApp();

	// Logs an error into a txt file as a crash error
	void LogError(const std::string& errorMessage);

	// Called in CoreEngine per frame
	//void UpdateSystemTimes(const std::vector<CoreFramework::SystemInterface*>& systems, float loopTime);

	bool IsActive() const { return openedDebugger; }

public:
	float fps = 0; // FPS 
	float msperFrame = 0; // MS/frame
	// Vector of SystemPerformance structs to store data for system performance
	std::vector<SystemPerformance> sysPerformance;

	// FPS control
	FPSMode fpsMode = FPSMode::VSYNC; // By default
private:
	GLFWwindow* debugWindow; // The host window
	bool openedDebugger; // Shows Whether debugger window is visible
	bool isInitialised; // Shows if the debugger was initialised or not

	// Crash logging
	std::ofstream crashlogFile; // The file stream to log errors to
};
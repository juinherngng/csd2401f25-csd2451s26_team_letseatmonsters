/*
----------------------------------------------------------------------------------------------------
FILE NAME:			ImGuiDebugger.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Glenn Yeo Yi Heng, g.yeo@digipen.edu

DESCRIPTION:		The declarations of functions for the debugger window.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <iostream>

#pragma once

class DebuggerApp
{
public: 
	// Ctor
	DebuggerApp();

	// Dtor
	~DebuggerApp();

	// Initializes the debugger app
	bool InitializeDebuggerApp(int width, int height, const char* appName);

	// Updates debugger state (logic, hotkeys, toggles)
	void UpdateDebuggerApp();

	// Loads the ImGui window every frame
	void RenderDebuggerApp();

	// Runs a full debugger frame (Update + Render)
	void RunDebuggerApp();

private:
	GLFWwindow* debugWindow; // The host window
	bool openedDebugger; // Shows Whether debugger window is visible
	bool isGay;
};
/*
----------------------------------------------------------------------------------------------------
FILE NAME:			ImGuiDebugger.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Glenn Yeo Yi Heng, g.yeo@digipen.edu

DESCRIPTION:		The definitions of functions for the debugger window.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include "ImGuiDebugger.hpp"

// Constructor
DebuggerApp::DebuggerApp() : debugWindow{ nullptr }, openedDebugger{ true } {}

// Destructor
DebuggerApp::~DebuggerApp()
{
	// Cleanup ImGui
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// Cleanup GLFW if window still open
	if (debugWindow)
	{
		glfwDestroyWindow(debugWindow);
		glfwTerminate();
	}
}

bool DebuggerApp::InitializeDebuggerApp(int width, int height, const char* appName)
{
	if (!glfwInit())
	{
		return false;
	}

	// Set OpenGL version (3.3 core)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create the window with size of width and height, with name appName
	debugWindow = glfwCreateWindow(width, height, appName, nullptr, nullptr);
	if (!debugWindow)
	{
		return false;
	}

	glfwMakeContextCurrent(debugWindow);
	glfwSwapInterval(1); // Vsync

	// Load OpenGL functions using GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Failed to initialize OpenGL context\n";
		return false;
	}

	// Setup ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(debugWindow, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	return true;
}

void DebuggerApp::UpdateDebuggerApp()
{
	// Close the debugger if esc was pressed
	if (ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		openedDebugger = !openedDebugger;
	}
}

void DebuggerApp::RenderDebuggerApp()
{
	if (!openedDebugger)
	{
		return;
	}
	
	// Start ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowSize(ImVec2(300, 300));
	// Create my window
	if (ImGui::Begin("Debug Infomation", &openedDebugger))
	{
		ImGui::Text("----Frame Infomation----");
		ImGui::Text("Current FPS : %.1f", ImGui::GetIO().Framerate);
		ImGui::Text("----Render Infomation----");

		if (ImGui::Button("Press to Bas"))
		{
			// Calls when button pressed
		}

		ImGui::SameLine();

		if (ImGui::Button("Press to Coom"))
		{

		}

		ImGui::Checkbox("Is Gay", &isGay);
	}

	ImGui::End();

	// Render in my ImGui
	ImGui::Render();
	glViewport(0, 0, 800, 600);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Swaps buffers
	glfwSwapBuffers(debugWindow);
}

void DebuggerApp::RunDebuggerApp()
{
	while (!glfwWindowShouldClose(debugWindow) && openedDebugger)
	{
		// Poll events first so Update() gets latest input and Render() can draw based on updated state
		glfwPollEvents();
		UpdateDebuggerApp(); // Checks for updates done in the window
		RenderDebuggerApp(); // Loads the ImGui window every frame
	}
}
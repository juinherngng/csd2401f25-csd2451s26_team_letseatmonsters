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
DebuggerApp::DebuggerApp() : debugWindow{ nullptr }, openedDebugger{ true }, isInitialised{ true }
{
	crashlogFile.open("Debug_Log.txt", std::ios::app); // Set to append mode

	if (crashlogFile.is_open())
	{
		crashlogFile << "---- Debugger Started ---- \n";
	}
}


// Destructor
DebuggerApp::~DebuggerApp()
{
	// Close crashlog file
	if (crashlogFile.is_open())
	{
		crashlogFile << " ---- Debugger closing ---- \n";
		crashlogFile.close();
	}

	// Cleanup ImGui
	if (isInitialised)
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}

bool DebuggerApp::InitializeDebuggerApp(GLFWwindow* externalWindow)
{
	if (!glfwInit())
	{
		return false;
	}

	debugWindow = externalWindow;
	glfwMakeContextCurrent(debugWindow);

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

	isInitialised = true;
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

	// Create my window
	if (ImGui::Begin("Debug Infomation", &openedDebugger))
	{
		static int selectedfpsMode = 1;
		const char* fpsModes[] = { "Vsync", "Unlimited", "Capped" };
		int fpsmodeCount = IM_ARRAYSIZE(fpsModes);

		ImGui::Text("----Frame Infomation----");
		ImGui::Text("[Current FPS : %.1f FPS ] [ms/frame : %.1f ms]", fps, msperFrame);
		if (ImGui::Combo("FPS Modes", &selectedfpsMode, fpsModes, fpsmodeCount))
		{
			if (selectedfpsMode == 0)
			{
				fpsMode = FPSMode::VSYNC;
				glfwSwapInterval(1); // Enables VSYNC
			}
			else if (selectedfpsMode == 1)
			{
				fpsMode = FPSMode::Unlimited;
				glfwSwapInterval(0); // Unlimited FPS based on device
			}
			else if (selectedfpsMode == 2)
			{
				fpsMode = FPSMode::Capped;
			}
		}

		if (fpsMode == FPSMode::Capped)
		{
			ImGui::SliderInt("Cap FPS to", &targetFPS, 60, 240);
		}

		//ImGui::Checkbox("Unlimited", FPSMode::Unlimited);
		ImGui::Text("----System Usage Infomation");
		for (auto& performance : systemP)
		{
			ImGui::Text("%s: %1.f%%", performance.name.c_str(), performance.percentageOf);
		}

		ImGui::Text("----Render Infomation----");
		ImGui::Text("Squares rendered: ");
		ImGui::Text("Sprites rendered: ");

		if (ImGui::Button("Press to Bas"))
		{
			// Calls when button pressed
		}

		ImGui::SameLine();

		if (ImGui::Button("Press to Coom"))
		{

		}
	}

	ImGui::End();

	// Render in my ImGui
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

}

void DebuggerApp::RunDebuggerApp()
{

	UpdateDebuggerApp(); // Checks for updates done in the window


	RenderDebuggerApp(); // Loads the ImGui window every frame
}

void DebuggerApp::LogError(const std::string& errorMessage)
{
	if (crashlogFile.is_open())
	{
		// Setting the timestamp of when the error occurred
		std::time_t now = std::time(nullptr);
		char buffer[64];
		// Turns time_t into readable C-string of time in the format
		// weekday, month, day of month, local-time(24HR), year
		ctime_s(buffer, sizeof(buffer), &now);
		buffer[strcspn(buffer, "\n")] = 0; // Remove the newline

		crashlogFile << "[" << buffer << "] " << errorMessage << "\n";
		crashlogFile.flush();
	}
}


//void DebuggerApp::UpdateSystemTimes(const std::vector<CoreFramework::SystemInterface*>& systems, float totalDt)
//{
//	systemP.clear();
//
//	for (auto& sys : systems)
//	{
//		float percent = (totalDt > 0.0f) ? (sys->lastDt / totalDt) * 100.0f : 0.0f;
//		systemP.push_back({ sys->GetName(), percent });
//	}
//}

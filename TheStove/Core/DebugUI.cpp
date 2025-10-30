/*
----------------------------------------------------------------------------------------------------
FILE NAME:			DebugUI.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Glenn Yeo Yi Heng, g.yeo@digipen.edu

DESCRIPTION:		The definitions of functions for the debugger window.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include "DebugUI.hpp"
#include "Core.hpp"

namespace Debug
{
	// Constructor
	DebuggerApp::DebuggerApp() : debugWindow{ nullptr }, coreEngine{ nullptr }, openedDebugger{ true }, isInitialised{ false }
	{
		crashlogFile.open("Debug_Log.txt", std::ios::app); // Set to append mode

		if (crashlogFile.is_open())
		{
			crashlogFile << "---- Debugger Started ---- \n";
		}
		else
		{
			std::cerr << "Crash Log File was not opened!" << std::endl;
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
		else
		{
			std::cerr << "Crash Log File was not opened at start!" << std::endl;
		}
	}


	// Implicit dtor for the debugger
	void DebuggerApp::Shutdown() {
		isInitialised = false; // only mark state, do not shutdown ImGui here
		std::cout << "Debugger Destructed with Shutdown\n";
	}

	bool DebuggerApp::InitializeDebuggerApp(GLFWwindow* externalWindow, CoreFramework::CoreEngine* coreEnginePtr)
	{
		if (!glfwInit())
		{
			return false;
		}

		debugWindow = externalWindow;
		coreEngine = coreEnginePtr;
		glfwMakeContextCurrent(debugWindow);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cerr << "Failed to initialize OpenGL context\n";
			return false;
		}

		isInitialised = true;
		return true;
	}

	void DebuggerApp::UpdateDebuggerApp()
	{
		// Close the debugger if esc was pressed
		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			openedDebugger = !openedDebugger;
			std::cout << "CLOSING DEBUGGER" << std::endl;
		}

		// update system performance %tages
		UpdateSystemTimes(coreEngine->GetDeltaTime());
	}

	void DebuggerApp::RenderDebuggerApp()
	{
		if (!openedDebugger)
		{
			return;
		}

		if (ImGui::GetCurrentContext() == nullptr) {
			return;
		}

		try {
			ImGuiIO& io = ImGui::GetIO();
			(void)io; // Suppress unused variable warning

			if (ImGui::GetWindowDrawList() == nullptr) {
				return; // Not in a valid frame scope
			}
		}
		catch (...) {
			return; // If any exception occurs, don't render
		}

		// Create my window
		if (ImGui::Begin("Debug Infomation", &openedDebugger))
		{
			static int selectedfpsMode = 0;
			const char* fpsModes[] = { "Vsync", "Unlimited" };
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
			}

			ImGui::Text("----System Usage Infomation----");
			for (auto& performance : sysPerformance)
			{
				ImGui::Text("%s: %1.f%%", performance.name.c_str(), performance.percentageOf);
			}

			ImGui::Text("----Render Infomation----");
			ImGui::Text("Squares rendered: ");
			ImGui::Text("Sprites rendered: ");

			ImGui::Text("---- Audio ----");
			if (ImGui::Button("Play: boiling sound"))
			{
				if (coreEngine)
				{
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>())
					{
						// test play audio
						bgm = audioMgr->GetBgmVolume();
						audioMgr->PlaySound("boiling sound", bgm, false);
						DebuggerApp::AddDebugLine("Playing: boiling sound\n");
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Play: background music"))
			{
				if (coreEngine)
				{
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>())
					{
						// test play bgm
						bgm = audioMgr->GetBgmVolume();
						audioMgr->PlaySound("background music", bgm, false);
						DebuggerApp::AddDebugLine("Playing: background music\n");
					}
				}
			}

			if (ImGui::Button("Stop boiling sound"))
			{
				if (coreEngine)
				{
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>())
					{
						// test stopping audio
						audioMgr->StopSound("boiling sound");
						DebuggerApp::AddDebugLine("Stopping: boiling sound\n");
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Stop background music"))
			{
				if (coreEngine)
				{
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>())
					{
						// test stopping bgm
						audioMgr->StopSound("background music");
						DebuggerApp::AddDebugLine("Stopping: background music\n");
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Stop all audio"))
			{
				if (coreEngine)
				{
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>())
					{
						// test stopping all audio
						audioMgr->StopAllSounds();
						DebuggerApp::AddDebugLine("Stopping: all audio\n");
					}
				}
			}
		}

		ImGui::End();

		// Show the debug log infomation window
		// Only show debug log if we can safely call ImGui
		try {
			ShowDebugLog();
		}
		catch (...) {
			// Ignore any ImGui errors in debug log
		}
	}

	// Currently not in use
	void DebuggerApp::RunDebuggerApp()
	{

		UpdateDebuggerApp(); // Checks for updates done in the window
		RenderDebuggerApp(); // Loads the ImGui window every frame
	}


	// Logs an error that is later outputted to the crash log txt file
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


	// Updates all system times in the systems manager
	void DebuggerApp::UpdateSystemTimes(float totalDt)
	{
		sysPerformance.clear();

		// Access systems from the stored CoreEngine pointer
		if (!coreEngine) return;

		const auto& systems = coreEngine->GetSystems();

		// For each system found in systems, record down their name and %tage usage of the current engine
		for (auto const& sys : systems)
		{
			float percent = (totalDt > 0.0f) ? (sys->lastDt / totalDt) * 100.0f : 0.0f;
			sysPerformance.push_back({ sys->GetName(), percent });
		}
	}

	// Adds line passed in to the debug log ImGui window
	void DebuggerApp::AddDebugLine(const std::string& txt)
	{
		debuglines.push_back(txt);
	}

	// Clears the Debug log infomation window
	void DebuggerApp::ClearDebugLog()
	{
		debuglines.clear();
	}

	// Opens up an ImGui window for debug log infomation to be outputted here
	void DebuggerApp::ShowDebugLog()
	{
		if (ImGui::GetCurrentContext() == nullptr) {
			return;
		}

		ImGui::Begin("Debug Log from std::cout");

		// Clear logs if the button was pressed
		if (ImGui::Button("Clear Logs"))
		{
			ClearDebugLog();
		}

		// For everyline stored, print it out
		for (const auto& line : debuglines)
		{
			ImGui::TextUnformatted(line.c_str());
		}

		ImGui::End();
	}
}
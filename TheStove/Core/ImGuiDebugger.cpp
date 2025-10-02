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

namespace Debug
{
	// Constructor
	DebuggerApp::DebuggerApp() : debugWindow{ nullptr }, openedDebugger{ true }, isInitialised{ true }
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

		// Cleanup ImGui
		if (isInitialised)
		{
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
		}
		else
		{
			std::cerr << "Debugger was not initalised at start!" << std::endl;
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

		// update system performance %tages
		UpdateSystemTimes(CoreFramework::gDt);
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

		// Create docking environment
		//ImGuiWindowFlags windowFlags = ImGuiWIndowFlags_NoDocking;
		
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
				if (auto* audioMgr = CoreFramework::CORE->GetSystem<AudioManager>())
				{
					// test play audio
					bgm = audioMgr->GetBgmVolume();
					audioMgr->PlaySound("boiling sound", bgm, false);
					std::cout << "Playing 'boiling sound'\n";
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Play: background music"))
			{
				if (auto* audioMgr = CoreFramework::CORE->GetSystem<AudioManager>())
				{
					// test play audio
					bgm = audioMgr->GetBgmVolume();
					audioMgr->PlaySound("background music", bgm, false);
					std::cout << "Playing 'background music'\n";
				}
			}

			if (ImGui::Button("Stop boiling sound"))
			{
				if (auto* audioMgr = CoreFramework::CORE->GetSystem<AudioManager>())
				{
					// test stopping audio
					audioMgr->StopSound("boiling sound");
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Stop background music"))
			{
				if (auto* audioMgr = CoreFramework::CORE->GetSystem<AudioManager>())
				{
					// test stopping audio
					audioMgr->StopSound("background music");
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Stop all audio"))
			{
				if (auto* audioMgr = CoreFramework::CORE->GetSystem<AudioManager>())
				{
					// test stopping audio
					audioMgr->StopAllSounds();
				}
			}
		}

		ImGui::End();

		// Render in my ImGui
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	}

	// Currently not in use
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

	void DebuggerApp::UpdateSystemTimes(float totalDt)
	{
		sysPerformance.clear();

		// Access the actual systems vector from the global engine in main
		const auto& systems = CoreFramework::CORE->GetSystems();

		// For each system found in systems, record down their name and %tage usage of the current engine
		for (auto& sys : systems)
		{
			float percent = (totalDt > 0.0f) ? (sys->lastDt / totalDt) * 100.0f : 0.0f;
			sysPerformance.push_back({ sys->GetName(), percent });
		}
	}

	/*void DebuggerApp::UpdateAudioList()
	{
		const auto& audios = AudioManager::
	}*/
}

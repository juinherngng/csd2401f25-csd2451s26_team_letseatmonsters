/*
----------------------------------------------------------------------------------------------------
FILE NAME:			DebugUI.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Glenn Yeo Yi Heng, g.yeo@digipen.edu
CO-AUTHORS: 		Ng Juin Herng, juinherng.ng@digipen.edu

DESCRIPTION:		The definitions of functions for the debugger window.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include "DebugUI.hpp"
#include "Core.hpp"
#include "../Graphics/SceneManager.hpp"

namespace Debug
{
	DebuggerApp gDebugger;

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
		ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(),
			ImGuiCond_FirstUseEver);
		ImGui::Begin("Debug Information###DebugInfo", &openedDebugger);
		{
			static int selectedfpsMode = 0;
			const char* fpsModes[] = { "Vsync", "Unlimited" };
			int fpsmodeCount = IM_ARRAYSIZE(fpsModes);

			ImGui::Text("----Frame Infomation----");
			ImGui::Text("[Current FPS : %.1f FPS ] [ms/frame : %.1f ms]", fps, msperFrame);
			if (ImGui::Combo("FPS Modes", &selectedfpsMode, fpsModes, fpsmodeCount))
			{
				if (coreEngine)
				{
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>())
					{
						audioMgr->PlayUIClickSound();
					}
				}
				
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

			if (scene_) {
				static int stressTestCount = 2500;
				ImGui::InputInt("Object Count", &stressTestCount, 100, 500);
				stressTestCount = glm::clamp(stressTestCount, 0, 10000);

				// Check if scene has objects
				bool hasObjects = (totalObjects > 0);

				// Disable button if true
				if (hasObjects) {
					ImGui::BeginDisabled();
				}

				if (ImGui::Button("Generate Stress Test")) {
					if (coreEngine) {
						if (auto* audioMgr = coreEngine->GetSystem<AudioManager>()) {
							audioMgr->PlayUIClickSound();
						}
					}
					scene_->GenerateStressTest(stressTestCount);
					scene_->SetSimulationActive(true);
					AddDebugLine("Generated stress test with " + std::to_string(stressTestCount) + " objects\n");
				}

				// Re-enable UI if it was disabled
				if (hasObjects) {
					ImGui::EndDisabled();
					ImGui::SameLine();
					ImGui::TextDisabled("(Clear objects first)");
				}

				ImGui::SameLine();

				// Simulation toggle
				bool simActive = scene_->IsSimulationActive();
				if (ImGui::Checkbox("Simulation Active", &simActive)) {
					if (coreEngine) {
						if (auto* audioMgr = coreEngine->GetSystem<AudioManager>()) {
							audioMgr->PlayUIClickSound();
						}
					}
					scene_->SetSimulationActive(simActive);
					AddDebugLine(simActive ? "Simulation started\n" : "Simulation paused\n");
				}
				if (ImGui::Button("Clear All Objects")) {
					if (coreEngine) {
						if (auto* audioMgr = coreEngine->GetSystem<AudioManager>()) {
							audioMgr->PlayUIClickSound();
						}
					}
					scene_->RequestClearAll();
					scene_->SetSimulationActive(false);
					AddDebugLine("Cleared all objects from scene\n");
				}
			}
			else {
				ImGui::TextDisabled("(Scene not connected)");
			}


			ImGui::Separator();
			ImGui::Text("----Render Infomation----");
			ImGui::Text("Total Objects: %d", totalObjects);
			ImGui::Text("Total Batches: %d", totalBatches);
			ImGui::Text("Instanced Objects: %d", instancedObjects);
			ImGui::Text("Draw Calls: %d", drawCalls);
			
			ImGui::Separator();
			ImGui::Text("---- Audio ----");
			if (ImGui::Button("Play: boiling sound"))
			{
				if (coreEngine)
				{
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>())
					{
						audioMgr->PlayUIClickSound();
						
						// test play audio
						bgm = audioMgr->GetBgmVolume();

						// Publish via MessageBus instead of calling AudioManager directly
						coreEngine->GetMessageBus().Post<CoreFramework::PlayAudioMessage>("boiling_sound", 1.0f, false);
						// audioMgr->PlaySound("boiling_sound", bgm, false); // Direct call (not via MessageBus)
						DebuggerApp::AddDebugLine("Playing: boiling sound (via MessageBus)\n");
						AddDebugLine("Published PLAY_AUDIO message for boiling_sound\n");
					}
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Stop boiling sound"))
			{
				if (coreEngine)
				{
					// UI click sound
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>())
					{
						audioMgr->PlayUIClickSound();
					}

					// Publish stop message via MessageBus
					coreEngine->GetMessageBus().Post<CoreFramework::StopAudioMessage>("boiling_sound");
					DebuggerApp::AddDebugLine("Stopping: boiling sound (via MessageBus)\n");
					AddDebugLine("Published STOP_AUDIO message for boiling_sound\n");
				}
			}

			if (ImGui::Button("Play: grilling sound"))
			{
				if (coreEngine)
				{
					// UI click sound
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>())
					{
						audioMgr->PlayUIClickSound();
					}

					// Publish via MessageBus instead of calling AudioManager directly
					coreEngine->GetMessageBus().Post<CoreFramework::PlayAudioMessage>("grilling_sound", 1.0f, false);
					DebuggerApp::AddDebugLine("Playing: grilling sound (via MessageBus)\n");
					AddDebugLine("Published PLAY_AUDIO message for grilling_sound\n");
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Stop grilling sound"))
			{
				if (coreEngine)
				{
					// UI click sound
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>())
					{
						audioMgr->PlayUIClickSound();
					}

					// Publish stop message via MessageBus
					coreEngine->GetMessageBus().Post<CoreFramework::StopAudioMessage>("grilling_sound");
					DebuggerApp::AddDebugLine("Stopping: grilling sound (via MessageBus)\n");
					AddDebugLine("Published STOP_AUDIO message for grilling_sound\n");
				}
			}

			if (ImGui::Button("Play: background music"))
			{
				if (coreEngine)
				{
					// UI click sound
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>())
					{
						audioMgr->PlayUIClickSound();
					}
					
					// Publish via MessageBus instead of calling AudioManager directly
					coreEngine->GetMessageBus().Post<CoreFramework::PlayAudioMessage>("background_music", 1.0f, false);
					DebuggerApp::AddDebugLine("Playing: background music (via MessageBus)\n");
					AddDebugLine("Published PLAY_AUDIO message for background_music\n");
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Stop background music"))
			{
				if (coreEngine)
				{
					// UI click sound
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>())
					{
						audioMgr->PlayUIClickSound();
					}
					
					// Publish stop message via MessageBus
					coreEngine->GetMessageBus().Post<CoreFramework::StopAudioMessage>("background_music");
					DebuggerApp::AddDebugLine("Stopping: background music (via MessageBus)\n");
					AddDebugLine("Published STOP_AUDIO message for background_music\n");
				}
			}

			if (ImGui::Button("Stop all audio"))
			{
				if (coreEngine)
				{
					// Publish stop all message (empty string = stop all)
					coreEngine->GetMessageBus().Post<CoreFramework::StopAudioMessage>("");
					DebuggerApp::AddDebugLine("Stopping: all audio (via MessageBus)\n");
					
					// Play UI click sound AFTER stopping all audio
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>())
					{
						audioMgr->PlayUIClickSound();
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
		ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(),
			ImGuiCond_FirstUseEver);
		ImGui::Begin("Console Log###ConsoleLog");

		// Clear logs if the button was pressed
		if (ImGui::Button("Clear Logs"))
		{
			if (coreEngine)
			{
				if (auto* audioMgr = coreEngine->GetSystem<AudioManager>())
				{
					audioMgr->PlayUIClickSound();
				}
			}
			ClearDebugLog();
		}

		// For everyline stored, print it out
		for (const auto& line : debuglines)
		{
			ImGui::TextUnformatted(line.c_str());
		}

		ImGui::End();
	}

	void DebuggerApp::SetRenderStats(int objects, int batches, int instanced, int draws) {
		totalObjects = objects;
		totalBatches = batches;
		instancedObjects = instanced;
		drawCalls = draws;
	}
}

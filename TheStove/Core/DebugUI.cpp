/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			DebugUI.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Glenn Yeo Yi Heng, g.yeo@digipen.edu    (15%)
 CO-AUTHORS: 		Ng Juin Herng, juinherng.ng@digipen.edu (65%)
					Seah Wang Hua, wanghua.seah"digipen.edu (20%)

 DESCRIPTION:		The definitions of functions for the debugger window.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
#pragma once

#include <algorithm>
#include <imgui_internal.h>
#include <unordered_set>
#include <glm/gtc/matrix_transform.hpp>

#include "../Graphics/GraphicsEngine.hpp"
#include "../Graphics/SceneManager.hpp"

#include "Core.hpp"
#include "DebugUI.hpp"
#include "FilePaths.hpp"

namespace Debug {
	DebuggerApp gDebugger;

	// Constructor
	DebuggerApp::DebuggerApp() : debugWindow{ nullptr }, coreEngine{ nullptr }, openedDebugger{ true }, isInitialised{ false } {
		crashlogFile.open("Debug_Log.txt", std::ios::app); // Set to append mode

		if (crashlogFile.is_open()) {
			crashlogFile << "---- Debugger Started ---- \n";
		}
		else {
			std::cerr << "Crash Log File was not opened!" << std::endl;
		}
	}

	// Destructor
	DebuggerApp::~DebuggerApp() {
		// Close crashlog file
		if (crashlogFile.is_open()) {
			crashlogFile << " ---- Debugger closing ---- \n";
			crashlogFile.close();
		}
		else {
			std::cerr << "Crash Log File was not opened at start!" << std::endl;
		}
	}


	// Implicit dtor for the debugger
	void DebuggerApp::Shutdown() {
		// Shutdown font system components
		if (fontSystemInitialized)
		{
			FontSystem::TextRenderer::Instance().Shutdown();
			FontSystem::FontManager::Instance().Shutdown();
			fontSystemInitialized = false;
		}
		
		isInitialised = false; // only mark state, do not shutdown ImGui here
		std::cout << "Debugger Destructed with Shutdown\n";
	}

	bool DebuggerApp::InitializeDebuggerApp(GLFWwindow* externalWindow, CoreFramework::CoreEngine* coreEnginePtr) {
		if (!glfwInit()) {
			return false;
		}

		debugWindow = externalWindow;
		coreEngine = coreEnginePtr;

		glfwMakeContextCurrent(debugWindow);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			std::cerr << "Failed to initialize OpenGL context\n";
			return false;
		}

		isInitialised = true;
		
		// Initialize font system
		InitializeFontSystem();
		
		return true;
	}

	void DebuggerApp::InitializeFontSystem()
	{
		// Initialize FontManager
		if (!FontSystem::FontManager::Instance().Initialize())
		{
			std::cerr << "Failed to initialize FontManager\n";
			return;
		}

		// Initialize TextRenderer
		if (!FontSystem::TextRenderer::Instance().Initialize())
		{
			std::cerr << "Failed to initialize TextRenderer\n";
			FontSystem::FontManager::Instance().Shutdown();
			return;
		}

	// Load fonts from assets folder
		// Load ChrustyRock font
		FontSystem::Font* fontChrusty = FontSystem::FontManager::Instance().LoadFont(
			"chrusty", 
			FilePaths::Fonts::CHRUSTY_ROCK,
			48  // Font size
		);

		// Load ToThePoint font as fallback/default
		FontSystem::Font* fontToThePoint = FontSystem::FontManager::Instance().LoadFont(
			"tothepoint", 
			FilePaths::Fonts::TO_THE_POINT,
			48  // Font size
		);

		// Check if at least one font loaded successfully
		if (!fontChrusty && !fontToThePoint)
		{
			std::cerr << "Failed to load any fonts from assets folder\n";
			FontSystem::TextRenderer::Instance().Shutdown();
			FontSystem::FontManager::Instance().Shutdown();
			return;
		}

		// Use ChrustyRock as the primary font if it loaded, otherwise use ToThePoint
		FontSystem::Font* primaryFont = fontChrusty ? fontChrusty : fontToThePoint;
		FontSystem::Font* secondaryFont = fontToThePoint ? fontToThePoint : fontChrusty;

		// Setup first text object with primary font
		text1.SetFont(primaryFont);
		text1.SetText("FPS Counter");
		text1.SetPosition(glm::vec2(50.0f, 50.0f));
		text1.SetColor(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green
		text1.SetScale(0.5f);

		// Setup second text object with secondary font
		text2.SetFont(secondaryFont);
		text2.SetText("TheStove Engine");
		text2.SetPosition(glm::vec2(50.0f, 750.0f));
		text2.SetColor(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow
		text2.SetScale(0.75f);

		fontSystemInitialized = true;
		std::cout << "Font system initialized successfully!\n";
		std::cout << "Loaded fonts from assets folder:\n";
		if (fontChrusty) std::cout << "  - ChrustyRock-ORLA.ttf\n";
		if (fontToThePoint) std::cout << "  - ToThePointRegular-n9y4.ttf\n";
		AddDebugLine("Font system initialized with fonts from assets folder\n");
	}

	void DebuggerApp::RenderTextOverlays()
	{
		if (!fontSystemInitialized)
			return;

		// Get the current window dimensions
		int width = GraphicsEngine::Instance().GetWidth();
		int height = GraphicsEngine::Instance().GetHeight();

		// Create orthographic projection for screen space rendering
		glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(width), 
										  static_cast<float>(height), 0.0f);

		// Update first text with current FPS
		std::string fpsText = "FPS: " + std::to_string(static_cast<int>(fps));
		text1.SetText(fpsText);

		// Render both text objects
		FontSystem::TextRenderer::Instance().RenderText(text1, projection);
		FontSystem::TextRenderer::Instance().RenderText(text2, projection);
	}

	void DebuggerApp::UpdateDebuggerApp()
	{
		// Close the debugger if esc was pressed
		if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			openedDebugger = !openedDebugger;
			std::cout << "CLOSING DEBUGGER" << std::endl;
		}
	}

	void DebuggerApp::RenderDebuggerApp() {
		// Always update system performance, even if window is closed
		if (coreEngine) {
			UpdateSystemTimes(coreEngine->GetDeltaTime());

			// Update render stats from GraphicsEngine system
			if (auto* gfxEngine = coreEngine->GetSystem<GraphicsEngine>()) {
				SetRenderStats(
					gfxEngine->GetTotalObjects(),
					gfxEngine->GetBatchCount(),
					gfxEngine->GetInstancedObjectCount(),
					gfxEngine->GetDrawCallCount()
				);
			}
		}

		if (!openedDebugger) {
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

		// Minimal, clean padding for this window
		ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(), ImGuiCond_FirstUseEver);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 4.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 6.0f));

		ImGui::Begin("Debug Information###DebugInfo", &openedDebugger);

		{
			static int selectedfpsMode = 0;
			const char* fpsModes[] = { "Vsync", "Unlimited" };
			int fpsmodeCount = IM_ARRAYSIZE(fpsModes);

			ImGui::SeparatorText("Frame Information");
			ImGui::Text("FPS: %.1f  (%.1f ms / frame)", fps, msperFrame);

			ImGui::SetNextItemWidth(140.0f);
			if (ImGui::Combo("FPS Modes", &selectedfpsMode, fpsModes, fpsmodeCount)) {
				if (coreEngine) {
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>()) {
						audioMgr->PlayUIClickSound();
					}
				}

				if (selectedfpsMode == 0) {
					fpsMode = FPSMode::VSYNC;
					glfwSwapInterval(1); // Enables VSYNC
				}
				else if (selectedfpsMode == 1) {
					fpsMode = FPSMode::Unlimited;
					glfwSwapInterval(0); // Unlimited FPS based on device
				}
			}

			ImGui::SeparatorText("System Usage");

			// Display options row
			static bool showDetailedStats = false;
			static bool showFramePercentage = false;

			ImGui::Checkbox("Details", &showDetailedStats);
			ImGui::SameLine();
			ImGui::Checkbox("Show frame %", &showFramePercentage);
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Toggle between relative system distribution (default)\nand absolute frame-time usage");
			}

			ImGui::SameLine();
			if (ImGui::Button("Reset peaks")) {
				if (coreEngine) {
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>()) {
						audioMgr->PlayUIClickSound();
					}
				}

				for (auto& performance : sysPerformance) {
					performance.peakPercentage = showFramePercentage?performance.percentageOfFrame:performance.percentageOf;
					performance.avgPercentage = showFramePercentage?performance.percentageOfFrame:performance.percentageOf;
					performance.sampleCount = 1;
				}
			}

			ImGui::Separator();

			// Display mode explanation
			if (showFramePercentage) {
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.0f, 1.0f), "Showing %% of frame time (can exceed 100%% total)");
			}
			else {
				ImGui::TextColored(ImVec4(0.0f, 0.7f, 0.0f, 1.0f), "Showing relative distribution (total = 100%%)");
			}

			// Display each system with a colored progress bar
			// Also calculate total time from CURRENT data while displaying
			float totalSystemTimeMs = 0.0f;
			for (auto& performance : sysPerformance) {
				// Accumulate total time from current frame's data
				totalSystemTimeMs += performance.lastTimeMs;

				// Choose which percentage to display
				float displayPercent = showFramePercentage?performance.percentageOfFrame:performance.percentageOf;

				// Determine color based on performance percentage
				ImVec4 barColor;
				if (showFramePercentage) {
					// For frame percentage, use different thresholds
					if (displayPercent < 10.0f)
						barColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green - Good
					else if (displayPercent < 30.0f)
						barColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow - Warning
					else
						barColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red - Critical
				}
				else {
					// For relative percentage, use system time thresholds
					if (displayPercent < 20.0f)
						barColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green - Good
					else if (displayPercent < 40.0f)
						barColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow - Warning
					else
						barColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red - Critical
				}

				// Draw system name and percentage
				ImGui::Text("%s:", performance.name.c_str());
				ImGui::SameLine(200.0f); // Align the percentage values
				ImGui::Text("%.2f%% (%.3f ms)", displayPercent, performance.lastTimeMs);

				// Show detailed stats if enabled
				if (showDetailedStats) {
					ImGui::Indent(20.0f);
					ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "Avg: %.2f%% | Peak: %.2f%%",
									   performance.avgPercentage, performance.peakPercentage);
					if (showFramePercentage) {
						// Also show the relative distribution
						ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "Relative: %.2f%%", performance.percentageOf);
					}
					else {
						// Also show the frame percentage
						ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "Frame %%: %.2f%%", performance.percentageOfFrame);
					}

					ImGui::Unindent(20.0f);
				}

				// Draw progress bar (clamp at 100% for display purposes)
				float barValue = showFramePercentage?
					std::min(displayPercent / 100.0f, 1.0f):
					displayPercent / 100.0f;

				ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
				ImGui::ProgressBar(barValue, ImVec2(-1.0f, 0.0f), "");
				ImGui::PopStyleColor();
			}

			// Display total system usage
			ImGui::Separator();
			if (showFramePercentage) {
				// Protect against division by zero
				float safeFrameTime = (msperFrame > 0.0f)?msperFrame:0.001f;

				// Calculate total frame percentage based on actual time vs frame budget
				float totalFramePercent = (totalSystemTimeMs / safeFrameTime) * 100.0f;
				ImGui::Text("Total Frame Usage: %.2f%% (%.3f ms / %.3f ms)",
							totalFramePercent, totalSystemTimeMs, msperFrame);

				// Overall performance bar for frame usage
				ImVec4 totalBarColor = totalFramePercent < 60.0f?
					ImVec4(0.0f, 1.0f, 0.0f, 1.0f):
					(totalFramePercent < 80.0f?ImVec4(1.0f, 1.0f, 0.0f, 1.0f):ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_PlotHistogram, totalBarColor);
				ImGui::ProgressBar(std::min(totalFramePercent / 100.0f, 1.0f), ImVec2(-1.0f, 0.0f), "");
				ImGui::PopStyleColor();
			}
			else {
				// For relative mode, show total time and confirm percentages sum to 100%
				ImGui::Text("Total System Time: %.3f ms (100%% distribution)", totalSystemTimeMs);

				// Overall performance bar - should always be at 100% in relative mode
				ImVec4 totalBarColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
				ImGui::PushStyleColor(ImGuiCol_PlotHistogram, totalBarColor);
				ImGui::ProgressBar(1.0f, ImVec2(-1.0f, 0.0f), "");
				ImGui::PopStyleColor();
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

				// Simulation toggle
				bool simActive = scene_->IsSimulationActive();
				if (ImGui::Checkbox("Simulation Active", &simActive)) {
					if (coreEngine) {
						if (auto* audioMgr = coreEngine->GetSystem<AudioManager>()) {
							audioMgr->PlayUIClickSound();
						}
					}

					scene_->SetSimulationActive(simActive);
					AddDebugLine(simActive?"Simulation started\n":"Simulation paused\n");
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

			ImGui::SeparatorText("Render");
			ImGui::Text("Total objects: %d", totalObjects);
			ImGui::Text("Total Batches: %d", totalBatches);
			ImGui::Text("Instanced Objects: %d", instancedObjects);
			ImGui::Text("Draw Calls: %d", drawCalls);
			
			// Font System Controls
			ImGui::Separator();
			ImGui::Text("---- Text Overlays ----");
			if (fontSystemInitialized)
			{
				static char text1Buffer[256] = "FPS Counter";
				static char text2Buffer[256] = "TheStove Engine";
				static float text1Pos[2] = { 50.0f, 50.0f };
				static float text2Pos[2] = { 50.0f, 750.0f };
				static float text1Scale = 0.5f;
				static float text2Scale = 0.75f;
				static float text1Color[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
				static float text2Color[4] = { 1.0f, 1.0f, 0.0f, 1.0f };

				ImGui::Text("Text 1 (FPS Display):");
				ImGui::InputText("##text1", text1Buffer, sizeof(text1Buffer));
				ImGui::SliderFloat2("Position##text1", text1Pos, 0.0f, 1200.0f);
				ImGui::SliderFloat("Scale##text1", &text1Scale, 0.1f, 2.0f);
				ImGui::ColorEdit4("Color##text1", text1Color);

				ImGui::Separator();
				ImGui::Text("Text 2 (Title):");
				ImGui::InputText("##text2", text2Buffer, sizeof(text2Buffer));
				ImGui::SliderFloat2("Position##text2", text2Pos, 0.0f, 800.0f);
				ImGui::SliderFloat("Scale##text2", &text2Scale, 0.1f, 2.0f);
				ImGui::ColorEdit4("Color##text2", text2Color);

				// Apply changes to text objects
				text1.SetPosition(glm::vec2(text1Pos[0], text1Pos[1]));
				text1.SetScale(text1Scale);
				text1.SetColor(glm::vec4(text1Color[0], text1Color[1], text1Color[2], text1Color[3]));
				
				text2.SetText(text2Buffer);
				text2.SetPosition(glm::vec2(text2Pos[0], text2Pos[1]));
				text2.SetScale(text2Scale);
				text2.SetColor(glm::vec4(text2Color[0], text2Color[1], text2Color[2], text2Color[3]));
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Font System not initialized!");
			}
			
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
			if (ImGui::Button("Stop boiling sound")) {
				if (coreEngine) {
					// UI click sound
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>()) {
						audioMgr->PlayUIClickSound();
					}

					// Publish stop message via MessageBus
					coreEngine->GetMessageBus().Post<CoreFramework::StopAudioMessage>("boiling_sound");
					DebuggerApp::AddDebugLine("Stopping: boiling sound (via MessageBus)\n");
					AddDebugLine("Published STOP_AUDIO message for boiling_sound\n");
				}
			}

			if (ImGui::Button("Play: grilling sound")) {
				if (coreEngine) {
					// UI click sound
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>()) {
						audioMgr->PlayUIClickSound();
					}

					// Publish via MessageBus instead of calling AudioManager directly
					coreEngine->GetMessageBus().Post<CoreFramework::PlayAudioMessage>("grilling_sound", 1.0f, false);
					DebuggerApp::AddDebugLine("Playing: grilling sound (via MessageBus)\n");
					AddDebugLine("Published PLAY_AUDIO message for grilling_sound\n");
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Stop grilling sound")) {
				if (coreEngine) {
					// UI click sound
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>()) {
						audioMgr->PlayUIClickSound();
					}

					// Publish stop message via MessageBus
					coreEngine->GetMessageBus().Post<CoreFramework::StopAudioMessage>("grilling_sound");
					DebuggerApp::AddDebugLine("Stopping: grilling sound (via MessageBus)\n");
					AddDebugLine("Published STOP_AUDIO message for grilling_sound\n");
				}
			}

			if (ImGui::Button("Play: background music")) {
				if (coreEngine) {
					// UI click sound
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>()) {
						audioMgr->PlayUIClickSound();
					}

					// Publish via MessageBus instead of calling AudioManager directly
					coreEngine->GetMessageBus().Post<CoreFramework::PlayAudioMessage>("background_music", 1.0f, false);
					DebuggerApp::AddDebugLine("Playing: background music (via MessageBus)\n");
					AddDebugLine("Published PLAY_AUDIO message for background_music\n");
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Stop background music")) {
				if (coreEngine) {
					// UI click sound
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>()) {
						audioMgr->PlayUIClickSound();
					}

					// Publish stop message via MessageBus
					coreEngine->GetMessageBus().Post<CoreFramework::StopAudioMessage>("background_music");
					DebuggerApp::AddDebugLine("Stopping: background music (via MessageBus)\n");
					AddDebugLine("Published STOP_AUDIO message for background_music\n");
				}
			}

			if (ImGui::Button("Stop all audio")) {
				if (coreEngine) {
					// Publish stop all message (empty string = stop all)
					coreEngine->GetMessageBus().Post<CoreFramework::StopAudioMessage>("");
					DebuggerApp::AddDebugLine("Stopping: all audio (via MessageBus)\n");
					AddDebugLine("Published STOP_AUDIO message for ALL sounds\n");

					// Play UI click sound AFTER stopping all audio
					if (auto* audioMgr = coreEngine->GetSystem<AudioManager>()) {
						audioMgr->PlayUIClickSound();
					}
				}
			}
		}

		ImGui::End();
		ImGui::PopStyleVar(3);

		// Render the transition panel (make it appear)
		ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(), ImGuiCond_FirstUseEver);
		DrawTransitionPanel();

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
	void DebuggerApp::RunDebuggerApp() {

		UpdateDebuggerApp(); // Checks for updates done in the window
		RenderDebuggerApp(); // Loads the ImGui window every frame
	}

	// Logs an error that is later outputted to the crash log txt file
	void DebuggerApp::LogError(const std::string& errorMessage) {
		if (crashlogFile.is_open()) {
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
	void DebuggerApp::UpdateSystemTimes(float frameDt) {
		// Access systems from the stored CoreEngine pointer
		if (!coreEngine) return;

		const auto& systems = coreEngine->GetSystems();

		// Calculate the total time spent in all systems
		float totalSystemTime = 0.0f;
		for (auto const& sys : systems) {
			totalSystemTime += sys->lastDt;
		}

		// Prevent division by zero
		if (totalSystemTime <= 0.0f) {
			totalSystemTime = 0.001f; // Use a small epsilon value
		}

		// Prevent division by zero for frame time
		float safeFrameDt = (frameDt > 0.0f)?frameDt:0.001f;

		// Build a set of current system names for cleanup detection
		std::unordered_set<std::string> currentSystemNames;
		for (auto const& sys : systems) {
			currentSystemNames.insert(sys->GetName());
		}

		// Update existing systems or add new ones
		for (auto const& sys : systems) {
			// Calculate percentage based on total system time (relative distribution)
			float percentOfSystems = (sys->lastDt / totalSystemTime) * 100.0f;

			// Calculate percentage based on frame time (absolute usage)
			float percentOfFrame = (sys->lastDt / safeFrameDt) * 100.0f;

			float timeMs = sys->lastDt * 1000.0f; // Convert to milliseconds

			// Find existing performance entry or create new one
			auto it = std::find_if(sysPerformance.begin(), sysPerformance.end(),
								   [&](const SystemPerformance& perf) { return perf.name == sys->GetName(); });

			if (it != sysPerformance.end()) {
				// Update existing entry
				it->percentageOf = percentOfSystems;
				it->percentageOfFrame = percentOfFrame;
				it->lastTimeMs = timeMs;

				// Update peak (using frame percentage for peak tracking)
				if (percentOfFrame > it->peakPercentage)
					it->peakPercentage = percentOfFrame;

				// Update running average (using frame percentage)
				it->sampleCount++;
				it->avgPercentage = ((it->avgPercentage * (it->sampleCount - 1)) + percentOfFrame) / it->sampleCount;

				// Reset average every 1000 samples to prevent overflow and keep it current
				if (it->sampleCount > 1000) {
					it->sampleCount = 1;
					it->avgPercentage = percentOfFrame;
				}
			}
			else {
				// Add new entry
				SystemPerformance newPerf;
				newPerf.name = sys->GetName();
				newPerf.percentageOf = percentOfSystems;
				newPerf.percentageOfFrame = percentOfFrame;
				newPerf.lastTimeMs = timeMs;
				newPerf.peakPercentage = percentOfFrame;
				newPerf.avgPercentage = percentOfFrame;
				newPerf.sampleCount = 1;
				sysPerformance.push_back(newPerf);
			}
		}

		// Remove entries for systems that no longer exist
		sysPerformance.erase(
			std::remove_if(sysPerformance.begin(), sysPerformance.end(),
						   [&currentSystemNames](const SystemPerformance& perf) {
			return currentSystemNames.find(perf.name) == currentSystemNames.end();
		}),
			sysPerformance.end()
		);
	}

	// Adds line passed in to the debug log ImGui window
	void DebuggerApp::AddDebugLine(const std::string& txt) {
		debuglines.push_back(txt);
	}

	// Clears the Debug log infomation window
	void DebuggerApp::ClearDebugLog() {
		debuglines.clear();
	}

	// Opens up an ImGui window for debug log infomation to be outputted here
	void DebuggerApp::ShowDebugLog() {
		if (ImGui::GetCurrentContext() == nullptr) {
			return;
		}
		ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(),
								   ImGuiCond_FirstUseEver);
		ImGui::Begin("Console Log###ConsoleLog");

		// Clear logs if the button was pressed
		if (ImGui::Button("Clear Logs")) {
			if (coreEngine) {
				if (auto* audioMgr = coreEngine->GetSystem<AudioManager>()) {
					audioMgr->PlayUIClickSound();
				}
			}
			ClearDebugLog();
		}

		// For everyline stored, print it out
		for (const auto& line : debuglines) {
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

	void DebuggerApp::SetupDefaultLayout() {
		static bool layoutInitialized = false;
		if (layoutInitialized) return;

		ImGuiID dockspaceID = GraphicsEngine::Instance().GetMainDockspaceID();
		if (dockspaceID == 0) {
			return;  // Not ready yet, try next frame
		}

		std::cout << "Setting up default ImGui layout...\n";

		// Clear any existing layout
		ImGui::DockBuilderRemoveNode(dockspaceID);

		// Recreate the dockspace with proper sizing
		// ImGuiViewport* viewport = ImGui::GetMainViewport();	// suppressed unused variable warning
		ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceID, ImVec2(1184, 784));  // Your working size
		ImGui::DockBuilderSetNodePos(dockspaceID, ImVec2(8, 8));

		// Split the dockspace to match your working layout
		// Main horizontal split: top area (882 height) and bottom console (109 height)
		ImGuiID dock_top, dock_bottom;
		ImGui::DockBuilderSplitNode(dockspaceID, ImGuiDir_Down, 0.12f, &dock_bottom, &dock_top);

		// Split top area: left sidebar (323 width) and right area (861 width)
		ImGuiID dock_left, dock_right;
		ImGui::DockBuilderSplitNode(dock_top, ImGuiDir_Left, 0.27f, &dock_left, &dock_right);

		// Split right area: center scene (561 width) and right panel (296 width)
		ImGuiID dock_center, dock_right_panel;
		ImGui::DockBuilderSplitNode(dock_right, ImGuiDir_Right, 0.35f, &dock_right_panel, &dock_center);

		// Dock all windows to their positions
		ImGui::DockBuilderDockWindow("Debug Information###DebugInfo", dock_left);
		ImGui::DockBuilderDockWindow("Assets###LE_Assets", dock_left);
		ImGui::DockBuilderDockWindow("Prefabs###LE_Prefabs", dock_left);
		ImGui::DockBuilderDockWindow("Scene###SceneWindow", dock_center);
		ImGui::DockBuilderDockWindow("Console Log###ConsoleLog", dock_bottom);
		ImGui::DockBuilderDockWindow("Level###LE_Level", dock_right_panel);

		ImGui::DockBuilderFinish(dockspaceID);

		layoutInitialized = true;
		std::cout << "Default ImGui layout initialized successfully!\n";
	}

	void DebuggerApp::DrawTransitionPanel() {
#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
	// Use the correct member and explicit type to avoid deduction issues
	GraphicsEngine* gfx = coreEngine ? coreEngine->GetSystem<GraphicsEngine>() : nullptr;
	if (!gfx) {
		ImGui::Begin("Transition Preview");
		ImGui::TextColored(ImVec4(1,0.4f,0.4f,1), "GraphicsEngine system not found.");
		ImGui::End();
		return;
	}

	ImGui::Begin("Transition Preview");

	// Status
	const bool active = gfx->IsTransitionActive();
	ImGui::Text("Active: %s", active ? "Yes" : "No");
	ImGui::Text("At Blackout: %s", gfx->IsAtBlackout() ? "Yes" : "No");

	// Controls
	ImGui::Separator();
	ImGui::SliderFloat("Fade Out (s)", &mFadeOutSec, 0.0f, 2.0f);
	ImGui::SliderFloat("Fade In (s)",  &mFadeInSec,  0.0f, 2.0f);

	if (ImGui::Button("Start Transition")) {
		gfx->StartSceneTransition(mFadeOutSec, mFadeInSec);
	}
	ImGui::SameLine();
	if (ImGui::Button("Force Blackout")) {
		// Simulate blackout: start transition with zero fade-out then immediately continue
		gfx->StartSceneTransition(0.0f, mFadeInSec);
	}
	ImGui::SameLine();
	if (ImGui::Button("Continue Fade-In")) {
		gfx->ContinueTransitionFadeIn();
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel")) {
		// Simple cancel: start transition with zero durations to clear state
		gfx->StartSceneTransition(0.0f, 0.0f);
	}

	ImGui::End();
#endif
	}
}

#else // Release: define trivial gDebugger so other translation units can link

#include "DebugUI.hpp"

namespace Debug {
	DebuggerApp gDebugger;
}

#endif // defined(_DEBUG) || defined(ENABLE_DEBUG_UI)

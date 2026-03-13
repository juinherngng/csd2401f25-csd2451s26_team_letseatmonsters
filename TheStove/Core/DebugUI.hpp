/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			DebugUI.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Glenn Yeo Yi Heng, g.yeo@digipen.edu	(30%)
 CO-AUTHORS: 		Ng Juin Herng, juinherng.ng@digipen.edu (20%)
					Seah Wang Hua, wanghua.seah"digipen.edu (50%)

 DESCRIPTION:		The declarations of functions for the debugger window.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include "../Graphics/GraphicsEngine.hpp"

#include "AudioManager.hpp"
#include "FontSystem.hpp"
#include "Precompiled.hpp"

#include <fstream>
#include <iostream>
#include <ostream>
#include <streambuf>
#include <string>
#include <vector>

namespace CoreFramework {
	class CoreEngine;
}
class Scene;

// Forward declare GLFW window type so Release branch doesn't need GLFW headers
struct GLFWwindow;

struct SystemPerformance {
	std::string name;				// Name of the system
	float percentageOf = 0.0f;		// The %tage of the total system time (relative distribution)
	float percentageOfFrame = 0.0f;	// The %tage of the frame time (absolute usage)
	float peakPercentage = 0.0f;	// Peak percentage recorded
	float avgPercentage = 0.0f;		// Average percentage
	int sampleCount = 0;			// Number of samples for averaging
	float lastTimeMs = 0.0f;		// Last frame time in milliseconds
};

enum class FPSMode {
	Unlimited,
	VSYNC,
	Capped
};

namespace Debug {

#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)

	class DebuggerApp {
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

		bool IsActive() const {
			return openedDebugger;
		}

		void AddDebugLine(const std::string& txt);

		void ClearDebugLog();

		void ShowDebugLog();

		void SetRenderStats(int objects, int batches, int instanced, int draws);

		void SetScene(Scene* scenePtr) {
			scene_ = scenePtr;
		}

		// Font System integration
		void InitializeFontSystem();
		void RenderTextOverlays();

		void SetupDefaultLayout();

		// draws transition preview panel
		void DrawTransitionPanel();

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

		Scene* scene_ = nullptr;

		// Font System members
		bool fontSystemInitialized = false;
		FontSystem::Text text1;
		FontSystem::Text text2;

		// Transition panel state
		float mFadeOutSec = 0.35f;
		float mFadeInSec = 0.35f;
	};
	extern DebuggerApp gDebugger;

#else // Release branch: provide a small no-op implementation so callers still compile / link

	class DebuggerApp {
	public:
		DebuggerApp() noexcept : fps(0), msperFrame(0), fpsMode(FPSMode::VSYNC), openedDebugger(false) {
		}
		~DebuggerApp() noexcept = default;

		void Shutdown() noexcept {
		}
		bool InitializeDebuggerApp(GLFWwindow* /*externalWindow*/, CoreFramework::CoreEngine* /*coreEnginePtr*/) noexcept {
			return false;
		}
		void UpdateDebuggerApp() noexcept {
		}
		void RenderDebuggerApp() noexcept {
		}
		void RunDebuggerApp() noexcept {
		}
		void LogError(const std::string& /*errorMessage*/) noexcept {
		}
		void UpdateSystemTimes(float /*loopTime*/) noexcept {
		}
		bool IsActive() const noexcept {
			return false;
		}
		void AddDebugLine(const std::string& /*txt*/) noexcept {
		}
		void ClearDebugLog() noexcept {
			debuglines.clear();
		}
		void ShowDebugLog() noexcept {
		}
		void SetRenderStats(int /*objects*/, int /*batches*/, int /*instanced*/, int /*draws*/) noexcept {
		}
		void SetScene(Scene* /*scenePtr*/) noexcept {
		}
		void SetupDefaultLayout() noexcept {
		}

	public:
		float fps = 0; // FPS
		float msperFrame = 0; // MS/frame
		std::vector<SystemPerformance> sysPerformance;
		FPSMode fpsMode = FPSMode::VSYNC;
		bool openedDebugger = false;
		std::vector<std::string> debuglines;
	};

	extern DebuggerApp gDebugger;

#endif // defined(_DEBUG) || defined(ENABLE_DEBUG_UI)

} // namespace Debug

/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			DebugUI.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Glenn Yeo Yi Heng, g.yeo@digipen.edu	(30%)
 CO-AUTHORS: 		Ng Juin Herng, juinherng.ng@digipen.edu (10%)
					Seah Wang Hua, wanghua.seah"digipen.edu (25%)
					Yat Chun Wee, y.chunwee@digipen.edu		(35%)

 DESCRIPTION:		The declarations of functions for the debugger window.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <fstream>
#include <iostream>
#include <ostream>
#include <streambuf>
#include <string>
#include <vector>

#include "EngineCore/AudioManager.hpp"
#include "EngineCore/FontSystem.hpp"
#include "EngineCore/Precompiled.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"

namespace CoreFramework {
	class CoreEngine;
}
class Scene;

// Forward declare GLFW window type so Release branch doesn't need GLFW headers
struct GLFWwindow;

/**
 * @brief Stores timing statistics for a single engine system in the debug UI.
 */
struct SystemPerformance {
	std::string name;				// Name of the system
	float percentageOf = 0.0f;		// The %tage of the total system time (relative distribution)
	float percentageOfFrame = 0.0f;	// The %tage of the frame time (absolute usage)
	float peakPercentage = 0.0f;	// Peak percentage recorded
	float avgPercentage = 0.0f;		// Average percentage
	int sampleCount = 0;			// Number of samples for averaging
	float lastTimeMs = 0.0f;		// Last frame time in milliseconds
};

/**
 * @brief Enumerates the frame pacing modes exposed by the debug UI.
 */
enum class FPSMode {
	Unlimited,
	VSYNC,
	Capped
};

namespace Debug {

#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)

	class DebuggerApp {
	public:
		/**
		 * @brief Constructs the debugger application state.
		 */
		DebuggerApp();

		/**
		 * @brief Destroys the debugger application state.
		 */
		~DebuggerApp();

		/**
		 * @brief Shuts down debugger-owned runtime systems.
		 */
		void Shutdown();

		/**
		 * @brief Initializes the debugger against an existing host window and engine.
		 * @param externalWindow Host GLFW window to render into.
		 * @param coreEnginePtr Core engine instance used for inspection and control.
		 * @return True if initialization succeeded, otherwise false.
		 */
		bool InitializeDebuggerApp(GLFWwindow* externalWindow, CoreFramework::CoreEngine* coreEnginePtr);

		/**
		 * @brief Updates debugger hotkeys and lightweight state.
		 */
		void UpdateDebuggerApp();

		/**
		 * @brief Renders the debugger UI windows for the current frame.
		 */
		void RenderDebuggerApp();

		/**
		 * @brief Runs a full debugger frame update and render pass.
		 */
		void RunDebuggerApp();

		/**
		 * @brief Appends an error message to the debugger crash log.
		 * @param errorMessage Error text to record.
		 */
		void LogError(const std::string& errorMessage);

		/**
		 * @brief Recomputes per-system timing statistics for the debug UI.
		 * @param loopTime Frame delta time used for absolute frame-percentage calculations.
		 */
		void UpdateSystemTimes(float loopTime);

		/**
		 * @brief Returns whether any debugger-owned window is currently active.
		 * @return True if any debugger UI should remain visible.
		 */
		bool IsActive() const {
			// Any visible debugger panel counts as the debugger being active.
			return openedDebugger || showConsoleLogWindow || showTransitionPreviewWindow;
		}

		/**
		 * @brief Adds a line of text to the in-editor debug log window.
		 * @param txt Log line to append.
		 */
		void AddDebugLine(const std::string& txt);

		/**
		 * @brief Clears all lines from the debug log window.
		 */
		void ClearDebugLog();

		/**
		 * @brief Renders the console-log window.
		 */
		void ShowDebugLog();

		/**
		 * @brief Updates cached render statistics displayed by the debugger.
		 * @param objects Total renderable objects.
		 * @param batches Total render batches.
		 * @param instanced Total instanced objects.
		 * @param draws Total draw calls.
		 */
		void SetRenderStats(int objects, int batches, int instanced, int draws);

		/**
		 * @brief Connects the debugger to the active scene.
		 * @param scenePtr Scene pointer used by scene-editing debug controls.
		 */
		void SetScene(Scene* scenePtr) {
			// Store a non-owning pointer so the debugger can drive scene-level tools.
			scene_ = scenePtr;
		}

		/**
		 * @brief Initializes the font system used by text overlays.
		 */
		void InitializeFontSystem();

		/**
		 * @brief Renders the debugger's text overlays on top of the scene.
		 */
		void RenderTextOverlays();

		/**
		 * @brief Builds the default dock layout for the debugger windows.
		 */
		void SetupDefaultLayout();

		/**
		 * @brief Draws the transition preview panel.
		 */
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
		bool showConsoleLogWindow = true;
		bool showTransitionPreviewWindow = true;

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
		/**
		 * @brief Constructs the no-op release debugger.
		 */
		DebuggerApp() noexcept : fps(0), msperFrame(0), fpsMode(FPSMode::VSYNC), openedDebugger(false) {
			// Release builds keep a trivial debugger object so call sites still compile.
		}

		/**
		 * @brief Destroys the no-op release debugger.
		 */
		~DebuggerApp() noexcept = default;

		/**
		 * @brief No-op shutdown for release builds.
		 */
		void Shutdown() noexcept {
			// Release builds do not allocate debugger resources.
		}

		/**
		 * @brief No-op debugger initialization for release builds.
		 * @return Always returns false because the debugger is disabled.
		 */
		bool InitializeDebuggerApp(GLFWwindow* /*externalWindow*/, CoreFramework::CoreEngine* /*coreEnginePtr*/) noexcept {
			// Signal to callers that the debug UI is unavailable in release builds.
			return false;
		}

		/**
		 * @brief No-op debugger update for release builds.
		 */
		void UpdateDebuggerApp() noexcept {
			// Release builds do not update debugger state.
		}

		/**
		 * @brief No-op debugger render for release builds.
		 */
		void RenderDebuggerApp() noexcept {
			// Release builds do not render debugger windows.
		}

		/**
		 * @brief No-op debugger frame runner for release builds.
		 */
		void RunDebuggerApp() noexcept {
			// Release builds do not execute debugger frames.
		}

		/**
		 * @brief No-op crash logging for release builds.
		 * @param errorMessage Ignored error string.
		 */
		void LogError(const std::string& /*errorMessage*/) noexcept {
			// Release builds intentionally ignore debugger-only crash logging.
		}

		/**
		 * @brief No-op timing update for release builds.
		 * @param loopTime Ignored frame delta.
		 */
		void UpdateSystemTimes(float /*loopTime*/) noexcept {
			// Release builds do not gather debugger timing metrics.
		}

		/**
		 * @brief Returns whether any release-debugger flags are active.
		 * @return True if any debugger window flag is set.
		 */
		bool IsActive() const noexcept {
			// Preserve the same API shape as debug builds for shared call sites.
			return openedDebugger || showConsoleLogWindow || showTransitionPreviewWindow;
		}

		/**
		 * @brief No-op debug-line append for release builds.
		 * @param txt Ignored debug log line.
		 */
		void AddDebugLine(const std::string& /*txt*/) noexcept {
			// Release builds do not retain debugger log lines.
		}

		/**
		 * @brief Clears the stored debug lines in release builds.
		 */
		void ClearDebugLog() noexcept {
			// Keep the container empty so shared UI code can still call this safely.
			debuglines.clear();
		}

		/**
		 * @brief No-op console window render for release builds.
		 */
		void ShowDebugLog() noexcept {
			// Release builds do not render the console log window.
		}

		/**
		 * @brief No-op render-stat update for release builds.
		 * @param objects Ignored object count.
		 * @param batches Ignored batch count.
		 * @param instanced Ignored instanced-object count.
		 * @param draws Ignored draw-call count.
		 */
		void SetRenderStats(int /*objects*/, int /*batches*/, int /*instanced*/, int /*draws*/) noexcept {
			// Release builds do not display render stats.
		}

		/**
		 * @brief No-op scene binding for release builds.
		 * @param scenePtr Ignored scene pointer.
		 */
		void SetScene(Scene* /*scenePtr*/) noexcept {
			// Release builds do not expose scene tools through the debugger.
		}

		/**
		 * @brief No-op dock-layout setup for release builds.
		 */
		void SetupDefaultLayout() noexcept {
			// Release builds do not create debugger dock layouts.
		}

	public:
		float fps = 0; // FPS
		float msperFrame = 0; // MS/frame
		std::vector<SystemPerformance> sysPerformance;
		FPSMode fpsMode = FPSMode::VSYNC;
		bool openedDebugger = false;
		std::vector<std::string> debuglines;
		bool showConsoleLogWindow = false;
		bool showTransitionPreviewWindow = false;
	};

	extern DebuggerApp gDebugger;

#endif // defined(_DEBUG) || defined(ENABLE_DEBUG_UI)

} // namespace Debug

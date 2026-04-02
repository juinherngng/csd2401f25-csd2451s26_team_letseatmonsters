/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			ApplicationState.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (33.3%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(33.3%)
					Seah Wang Hua, wanghua.seah@digipen.edu (33.3%)

 DESCRIPTION:		Declares the shared runtime state used by the desktop
					application layer and static OS callbacks.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <memory>

struct GLFWwindow;

namespace CoreFramework {
	class CoreEngine;
}

class Scene;
class LevelEditor;

namespace Debug {
	class DebuggerApp;
}

/**
 * @brief Aggregates application-owned pointers and frame state shared across callbacks.
 */
struct ApplicationState {
	/** @brief Constructs the runtime state container. */
	ApplicationState();
	/** @brief Destroys the runtime state container after owned resources are released. */
	~ApplicationState();

	// Owns the engine systems and message bus.
	std::unique_ptr<CoreFramework::CoreEngine> coreEngine;
	// Owns the active scene for gameplay and editor rendering.
	std::unique_ptr<Scene> currentScene;
#ifdef _DEBUG
	// Owns the debug overlay in debug-enabled builds.
	std::unique_ptr<Debug::DebuggerApp> debugApp;
#endif

	// Raw GLFW window handle owned by GLFW itself.
	GLFWwindow* window = nullptr;
	// Timestamp of the previous frame used for delta time calculations.
	float lastFrame = 0.0f;
	// Smoothed delta time used by FPS readouts.
	float smoothedDt = 0.0f;
	// Flag requested by signal handlers or window callbacks to stop the app loop.
	volatile bool shouldExit = false;

	// Last known mouse x-coordinate for delta calculations.
	double lastMouseX = 0.0;
	// Last known mouse y-coordinate for delta calculations.
	double lastMouseY = 0.0;
	// Tracks whether the mouse history has been initialized yet.
	bool mouseInitialized = false;

	// Tracks whether the app is currently paused due to OS focus loss.
	bool pausedByOSFocus = false;
	// Stores the scene simulation state to restore after focus returns.
	bool simActiveBeforePause = false;
	// Indicates whether a native modal dialog is currently open.
	bool modalDialogOpen = false;

	// Tracks whether the window is currently fullscreen.
	bool isFullscreen = false;
	// Cached windowed-mode x-position.
	int windowedPosX = 100;
	// Cached windowed-mode y-position.
	int windowedPosY = 100;
	// Cached windowed-mode width.
	int windowedWidth = 1200;
	// Cached windowed-mode height.
	int windowedHeight = 800;
	// Tracks the previous F11 key state for edge-trigger fullscreen toggling.
	bool f11WasDown = false;

	// Owns the editor controller used to draw editor UI outside of runtime scene ownership.
	std::unique_ptr<LevelEditor> levelEditor;
};

// Global pointer used by callbacks that cannot capture the application instance directly.
extern ApplicationState* g_AppState;

/**
 * @brief Updates whether a native modal dialog is currently open.
 * @param open True when a modal dialog is active.
 */
void SetModalDialogOpen(bool open);

/**
 * @brief Returns whether the desktop window is currently fullscreen.
 * @return True when the runtime window is fullscreen.
 */
bool IsApplicationFullscreen();

/**
 * @brief Applies the requested fullscreen state immediately to the desktop window.
 * @param fullscreen True for fullscreen, false for windowed.
 * @return True when the request could be applied.
 */
bool SetApplicationFullscreen(bool fullscreen);

/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Application.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (33.3%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(33.3%)
					Seah Wang Hua, wanghua.seah@digipen.edu (33.3%)

 DESCRIPTION:		Declares the high-level desktop application wrapper that owns
					startup, per-frame execution, and shutdown.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>

#include "EngineCore/ApplicationFlowCoordinator.hpp"
#include "EngineCore/ApplicationState.hpp"

 /**
  * @brief Coordinates startup, per-frame execution, and shutdown for the desktop application.
  */
class Application {
public:
	/** @brief Constructs the application wrapper. */
	Application() = default;
	/** @brief Destroys the application wrapper after shutdown has completed. */
	~Application();
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	/**
	 * @brief Runs the full application lifecycle from startup to shutdown.
	 * @return Process exit code returned to the operating system.
	 */
	int Run();

private:
	/**
	 * @brief Initializes the window, engine systems, scene, and debug UI.
	 * @param width Requested starting window width in pixels.
	 * @param height Requested starting window height in pixels.
	 * @param title Window title text.
	 * @param fullscreen True to start the application in fullscreen mode.
	 * @return True when initialization succeeds.
	 */
	bool Initialize(int width, int height, const std::string& title, bool fullscreen);
	/** @brief Advances one application frame of input, gameplay, and system updates. */
	void Update();
	/** @brief Renders one application frame. */
	void Draw();
	/** @brief Releases application resources in a safe shutdown order. */
	void Cleanup();

	// Stores the mutable runtime state owned by the application.
	ApplicationState state_;
	// Owns app-level game-flow transitions between scene requests and state switches.
	ApplicationFlowCoordinator flowCoordinator_;
};

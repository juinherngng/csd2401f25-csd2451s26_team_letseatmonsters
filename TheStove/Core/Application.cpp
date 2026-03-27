/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Application.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (33.3%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(33.3%)
					Seah Wang Hua, wanghua.seah@digipen.edu (33.3%)

 DESCRIPTION:		Owns the desktop application lifecycle. Handles startup, window creation,
					engine wiring, per-frame update/draw, and shutdown.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "Application.hpp"

#include "../Graphics/GraphicsEngine.hpp"
#include "../Graphics/ResourceManager.hpp"
#include "../Graphics/SceneManager.hpp"
#include "AudioLoading.hpp"
#include "AudioManager.hpp"
#include "ConfigManager.hpp"
#include "Core.hpp"
#include "DebugUI.hpp"
#include "FileDropHandler.hpp"
#include "FilePaths.hpp"
#include "GameBootstrap.hpp"
#include "GameStateManager.hpp"
#include "LevelEditorFileIO.hpp"
#include "Logger.hpp"
#include "MovementManager.hpp"
#include "Precompiled.hpp"
#include "TileMap.hpp"

#include <csignal>
#include <filesystem>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

ApplicationState* g_AppState = nullptr;

/**
 * @brief Constructs the runtime state container with default-initialized members.
 */
ApplicationState::ApplicationState() {
	// In-class member initializers establish the default runtime state.
}
/**
 * @brief Destroys the runtime state container after its owned members have been released.
 */
ApplicationState::~ApplicationState() {
	// Owned resources are released by their smart pointers during teardown.
}

namespace {
	/**
	 * @brief Handles standard process signals by requesting a graceful shutdown.
	 * @param signal Signal number received by the process.
	 */
	void SignalHandler(int signal) {
		// Record the shutdown request and ask GLFW to exit its event loop.
		TS_LOG_WARN("Received signal " << signal << ", cleaning up...");

		if (g_AppState) {
			g_AppState->shouldExit = true;
			if (g_AppState->window) {
				glfwSetWindowShouldClose(g_AppState->window, GLFW_TRUE);
			}
		}
	}

	/**
	 * @brief Synchronizes the OpenGL viewport and graphics system after framebuffer size changes.
	 * @param window GLFW window associated with the callback.
	 * @param width New framebuffer width in pixels.
	 * @param height New framebuffer height in pixels.
	 */
	void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
		(void)window;

		// Ignore transient zero-sized callbacks that can happen during minimization.
		if (width <= 0 || height <= 0) {
			return;
		}

		// Update both the GL viewport and the engine-side cached framebuffer size.
		glViewport(0, 0, width, height);

		if (g_AppState && g_AppState->coreEngine) {
			if (auto* gfxEngine = g_AppState->coreEngine->GetSystem<GraphicsEngine>()) {
				gfxEngine->Resize(width, height);
			}
		}
	}

	/**
	 * @brief Applies pause or resume behavior when the app loses or regains OS focus.
	 * @param pause True to pause simulation and audio, false to restore them.
	 */
	void HandlePauseResume(bool pause) {
		if (!g_AppState || !g_AppState->coreEngine) {
			return;
		}

		auto* audioMgr = g_AppState->coreEngine->GetSystem<AudioManager>();
		auto* inputMgr = g_AppState->coreEngine->GetSystem<InputManager>();
		auto* animMgr = g_AppState->coreEngine->GetSystem<AnimationManager>();
		Scene* scene = g_AppState->currentScene.get();

		if (pause) {
			// Avoid reapplying the pause path when multiple focus-loss callbacks fire.
			if (g_AppState->pausedByOSFocus) {
				return;
			}

			g_AppState->pausedByOSFocus = true;
			g_AppState->mouseInitialized = false;

			// Remember the previous simulation state so it can be restored later.
			if (scene) {
				g_AppState->simActiveBeforePause = scene->IsSimulationActive();
				scene->SetSimulationActive(false);
			}

			if (audioMgr) {
				audioMgr->PauseAll();
			}

			if (inputMgr) {
				inputMgr->ClearState();
			}
		}
		else {
			// Ignore resume requests if the app was not paused by OS focus handling.
			if (!g_AppState->pausedByOSFocus) {
				return;
			}

			g_AppState->pausedByOSFocus = false;
			g_AppState->mouseInitialized = false;
			// Reset the frame timer so the next delta time does not spike.
			g_AppState->lastFrame = static_cast<float>(glfwGetTime());

			if (scene) {
				scene->SetSimulationActive(g_AppState->simActiveBeforePause);
			}

			if (audioMgr) {
				audioMgr->ResumeAll();
			}

			if (inputMgr) {
				inputMgr->ClearState();
			}

			if (animMgr) {
				animMgr->Play();
			}
		}
	}

	/**
	 * @brief Switches the window between fullscreen and windowed presentation modes.
	 * @param app Mutable application state containing cached window placement data.
	 */
	void ToggleFullscreen(ApplicationState& app) {
		if (!app.window) {
			return;
		}

		if (!app.isFullscreen) {
			// Cache the windowed placement before entering fullscreen.
			glfwGetWindowPos(app.window, &app.windowedPosX, &app.windowedPosY);
			glfwGetWindowSize(app.window, &app.windowedWidth, &app.windowedHeight);

			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);

			glfwSetWindowMonitor(
				app.window,
				monitor,
				0, 0,
				mode->width,
				mode->height,
				mode->refreshRate
			);

			app.isFullscreen = true;
		}
		else {
			glfwSetWindowMonitor(
				app.window,
				nullptr,
				app.windowedPosX,
				app.windowedPosY,
				app.windowedWidth,
				app.windowedHeight,
				0
			);

			app.isFullscreen = false;
		}

		int fbw = 0;
		int fbh = 0;
		glfwGetFramebufferSize(app.window, &fbw, &fbh);
		// Refresh the viewport after the monitor switch has completed.
		glViewport(0, 0, fbw, fbh);

		if (app.coreEngine) {
			if (auto* gfx = app.coreEngine->GetSystem<GraphicsEngine>()) {
				gfx->Resize(fbw, fbh);
			}
		}
	}

#ifdef _WIN32
	/**
	 * @brief Handles Windows console control events by requesting a graceful shutdown.
	 * @param signal Console event code received from the OS.
	 * @return TRUE when the event was handled by the application.
	 */
	BOOL WINAPI ConsoleHandler(DWORD signal) {
		switch (signal) {
		case CTRL_C_EVENT:
		case CTRL_BREAK_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			TS_LOG_WARN("Console event detected, cleaning up...");
			if (g_AppState) {
				g_AppState->shouldExit = true;
				if (g_AppState->window) {
					glfwSetWindowShouldClose(g_AppState->window, GLFW_TRUE);
				}
			}

			return TRUE;
		default:
			return FALSE;
		}
	}

	/**
	 * @brief Changes the current working directory to the executable directory on Windows.
	 */
	void SetWorkingDirectoryToExecutable() {
		// Resolve the executable path so relative asset paths work from IDE launches.
		char exePath[MAX_PATH];
		GetModuleFileNameA(NULL, exePath, MAX_PATH);

		std::string exePathStr(exePath);
		size_t lastSlash = exePathStr.find_last_of("\\/");
		if (lastSlash != std::string::npos) {
			std::string exeDir = exePathStr.substr(0, lastSlash);
			SetCurrentDirectoryA(exeDir.c_str());
			TS_LOG_DEBUG("[Application] Set working directory to: " << exeDir);
		}
	}
#else
	/**
	 * @brief Changes the current working directory to the executable directory on non-Windows platforms.
	 */
	void SetWorkingDirectoryToExecutable() {
		// Mirror the Windows behavior so relative runtime paths resolve consistently.
		auto exePath = std::filesystem::read_symlink("/proc/self/exe");
		auto exeDir = exePath.parent_path();
		std::filesystem::current_path(exeDir);
		TS_LOG_DEBUG("[Application] Set working directory to: " << exeDir);
	}
#endif
}

/**
 * @brief Marks whether a native modal dialog is currently open.
 * @param open True when a modal dialog is active.
 */
void SetModalDialogOpen(bool open) {
	// Expose this state to callbacks that need to avoid aggressive focus handling.
	if (g_AppState) {
		g_AppState->modalDialogOpen = open;
	}
}

/**
 * @brief Destroys the application wrapper.
 */
Application::~Application() {
	// Cleanup is handled explicitly by Run(); nothing extra is needed here.
}

/**
 * @brief Runs startup, the main loop, and shutdown for the desktop application.
 * @return Process exit code reported back to the operating system.
 */
int Application::Run() {
	// Publish the active state so static callbacks can reach the current application instance.
	g_AppState = &state_;

	try {
		// Normalize the working directory before loading any config or asset files.
		SetWorkingDirectoryToExecutable();

		// Install process-level shutdown hooks before engine startup begins.
		std::signal(SIGINT, SignalHandler);
		std::signal(SIGTERM, SignalHandler);
		std::signal(SIGABRT, SignalHandler);

#ifdef _WIN32
		if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
			TS_LOG_ERROR("Failed to set console control handler");
		}
#endif

		auto settings = ConfigManager::LoadFromAssetsOrDefaults();
		ConfigManager::Validate(settings);

		// Fail early if the application bootstrap cannot create its window or systems.
		if (!Initialize(settings.resolution.width, settings.resolution.height, "TheStove", settings.fullscreen)) {
			Cleanup();
			g_AppState = nullptr;
			return -1;
		}

		// Apply persisted audio settings after the audio system has been created.
		if (auto* audioMgr = state_.coreEngine->GetSystem<AudioManager>()) {
			audioMgr->ApplySettings(settings);
			TS_LOG_DEBUG(
				"AudioManager system found in CoreEngine - BGM Volume: " << audioMgr->GetBgmVolume()
				<< ", VFX Volume: " << audioMgr->GetVfxVolume()
			);
		}
		else {
			TS_LOG_WARN("AudioManager system not found in CoreEngine");
		}

		// Seed the frame timer before entering the main loop.
		state_.lastFrame = static_cast<float>(glfwGetTime());

		while (!glfwWindowShouldClose(state_.window) && !state_.shouldExit) {
			Update();
			Draw();
		}

		Cleanup();
		g_AppState = nullptr;
		return 0;
	}
	catch (const std::exception& e) {
#ifdef _DEBUG
		if (state_.debugApp) {
			state_.debugApp->LogError(std::string("Unhandled exception: ") + e.what());
		}
#endif
		TS_LOG_ERROR("Fatal error: " << e.what());
		Cleanup();
		g_AppState = nullptr;
		return -1;
	}
	catch (...) {
#ifdef _DEBUG
		if (state_.debugApp) {
			state_.debugApp->LogError("Unknown crash occurred");
		}
#endif
		TS_LOG_ERROR("Fatal error: Unknown exception");
		Cleanup();
		g_AppState = nullptr;
		return -1;
	}
}

/**
 * @brief Creates the window, registers callbacks, wires engine systems, and loads the initial scene.
 * @param width Requested starting window width in pixels.
 * @param height Requested starting window height in pixels.
 * @param title Window title text.
 * @param fullscreen True to start in fullscreen mode.
 * @return True when initialization succeeds completely.
 */
bool Application::Initialize(int width, int height, const std::string& title, bool fullscreen) {
	// Store the preferred windowed dimensions for later fullscreen toggles.
	state_.windowedWidth = width;
	state_.windowedHeight = height;
	state_.windowedPosX = 100;
	state_.windowedPosY = 100;
	state_.isFullscreen = fullscreen;

	glfwSetErrorCallback([](int error, const char* description) {
		TS_LOG_ERROR("GLFW Error " << error << ": " << description);
		});

	if (!glfwInit()) {
		TS_LOG_ERROR("Failed to init GLFW");
		return false;
	}

	if (fullscreen) {
		// Use the monitor's native video mode when starting fullscreen.
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		width = mode->width;
		height = mode->height;
		state_.window = glfwCreateWindow(width, height, title.c_str(), monitor, nullptr);
	}
	else {
		state_.window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	}

	if (!state_.window) {
		TS_LOG_ERROR("Failed to create window");
		glfwTerminate();
		return false;
	}

	if (!state_.isFullscreen) {
		// Cache the actual window placement returned by the platform.
		glfwGetWindowPos(state_.window, &state_.windowedPosX, &state_.windowedPosY);
		glfwGetWindowSize(state_.window, &state_.windowedWidth, &state_.windowedHeight);
	}

	// Make the context current before GLAD initialization or any OpenGL calls.
	glfwMakeContextCurrent(state_.window);

	glfwSetWindowCloseCallback(state_.window, [](GLFWwindow* win) {
		(void)win;
		// Defer the actual teardown to the main loop so cleanup order stays centralized.
		TS_LOG_INFO("Window close requested, cleaning up...");
		if (g_AppState) {
			g_AppState->shouldExit = true;
		}
		});

	glfwSetCharCallback(state_.window, [](GLFWwindow* win, unsigned int c) {
		(void)win;
		// Forward text input into the engine message bus for gameplay and editor consumers.
		if (g_AppState && g_AppState->coreEngine) {
			g_AppState->coreEngine->GetMessageBus().Post<CoreFramework::CharacterKeyMessage>(static_cast<char>(c), true);
		}
		});

	glfwSetMouseButtonCallback(state_.window, [](GLFWwindow* win, int button, int action, int mods) {
		(void)mods;
		(void)win;
		// Sample the cursor position at click time so receivers get button and location together.
		if (g_AppState && g_AppState->coreEngine) {
			double x = 0.0;
			double y = 0.0;
			glfwGetCursorPos(g_AppState->window, &x, &y);
			g_AppState->coreEngine->GetMessageBus().Post<CoreFramework::MouseButtonMessage>(button, action == GLFW_PRESS, x, y);
		}
		});

	glfwSetCursorPosCallback(state_.window, [](GLFWwindow* win, double xpos, double ypos) {
		(void)win;

		if (g_AppState && g_AppState->coreEngine) {
			// Calculate deltas lazily so the first cursor event does not produce a bogus jump.
			double dx = 0.0;
			double dy = 0.0;

			if (g_AppState->mouseInitialized) {
				dx = xpos - g_AppState->lastMouseX;
				dy = ypos - g_AppState->lastMouseY;
			}
			else {
				g_AppState->mouseInitialized = true;
			}

			g_AppState->lastMouseX = xpos;
			g_AppState->lastMouseY = ypos;

			g_AppState->coreEngine->GetMessageBus().Post<CoreFramework::MouseMoveMessage>(xpos, ypos, dx, dy);
		}
		});

	glfwSetKeyCallback(state_.window, nullptr);
	glfwSetScrollCallback(state_.window, nullptr);

	glfwSetDropCallback(state_.window, [](GLFWwindow* win, int count, const char** paths) {
		(void)win;
		// Route file drops through the engine-managed file drop handler.
		if (g_AppState && g_AppState->coreEngine) {
			if (auto* dropHandler = g_AppState->coreEngine->GetSystem<FileDropHandler>()) {
				dropHandler->HandleGLFWDrop(count, paths);
			}
		}
		});

#ifndef _DEBUG
	glfwSetWindowFocusCallback(state_.window, [](GLFWwindow* win, int focused) {
		if (focused == GLFW_FALSE) {
			// Native modal dialogs should pause systems without forcing a minimize.
			if (g_AppState && g_AppState->modalDialogOpen) {
				HandlePauseResume(true);
				return;
			}

			// In release builds, minimize on focus loss and keep pause state in sync.
			glfwIconifyWindow(win);
			HandlePauseResume(true);
		}
		else {
			HandlePauseResume(false);
		}
		});
#endif

	glfwSetWindowIconifyCallback(state_.window, [](GLFWwindow* win, int iconified) {
		(void)win;
		// Keep pause state aligned with iconify/minimize events from the OS.
		HandlePauseResume(iconified == GLFW_TRUE);
		});

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		TS_LOG_ERROR("Failed to initialize GLAD");
		return false;
	}

	glfwSetFramebufferSizeCallback(state_.window, FramebufferSizeCallback);

	// Register engine systems in the order they should initialize and update.
	state_.coreEngine = std::make_unique<CoreFramework::CoreEngine>();
	state_.coreEngine->AddSystem(std::make_unique<InputManager>());
	state_.coreEngine->AddSystem(std::make_unique<GraphicsEngine>());
	state_.coreEngine->AddSystem(std::make_unique<AudioManager>(state_.coreEngine->GetMessageBus()));
	state_.coreEngine->AddSystem(std::make_unique<FileDropHandler>(state_.coreEngine->GetMessageBus()));
	state_.coreEngine->AddSystem(std::make_unique<Framework::GameStateManager>(state_.coreEngine->GetMessageBus()));
	state_.coreEngine->AddSystem(std::make_unique<AnimationManager>());
	state_.coreEngine->AddSystem(std::make_unique<MovementManager>());
	state_.coreEngine->AddSystem(std::make_unique<PhysicsManager>());
	state_.coreEngine->AddSystem(std::make_unique<CollisionManager>());

	state_.coreEngine->Initialize();

	// Resolve all required systems once after initialization for validation and wiring.
	auto* inputMgr = state_.coreEngine->GetSystem<InputManager>();
	auto* graphicsEngine = state_.coreEngine->GetSystem<GraphicsEngine>();
	auto* audioMgr = state_.coreEngine->GetSystem<AudioManager>();
	auto* animMgr = state_.coreEngine->GetSystem<AnimationManager>();
	auto* physicsMgr = state_.coreEngine->GetSystem<PhysicsManager>();
	auto* movementMgr = state_.coreEngine->GetSystem<MovementManager>();
	auto* collisionMgr = state_.coreEngine->GetSystem<CollisionManager>();

	if (!inputMgr || !graphicsEngine || !animMgr || !physicsMgr || !movementMgr || !collisionMgr) {
		TS_LOG_ERROR("Failed to acquire one or more required engine systems");
		return false;
	}

	// Give the input manager direct access to the live GLFW window.
	inputMgr->SetWindow(state_.window);

	if (audioMgr) {
		// Bind the audio manager before loading the catalog so ResourceManager can create sounds.
		ResourceManager::Instance().SetAudioManager(audioMgr);
#ifdef _DEBUG
		const std::string catalogPath = FilePaths::Audio::CATALOG_EDITOR;
#else
		const std::string catalogPath = FilePaths::Audio::CATALOG;
#endif
		if (!Audio::AudioCatalog::LoadCatalogFromFile(catalogPath)) {
			TS_LOG_WARN("Failed to load audio catalog from " << catalogPath);
		}
		Audio::AudioCatalog::LoadAllAudio();
	}

	// Initialize the graphics system with the current framebuffer dimensions.
	int fbw = 0;
	int fbh = 0;
	glfwGetFramebufferSize(state_.window, &fbw, &fbh);
	glViewport(0, 0, fbw, fbh);
	graphicsEngine->Resize(fbw, fbh);

	// Create the scene after all managers it depends on are available.
	state_.currentScene = std::make_unique<Scene>(*graphicsEngine, *inputMgr, *animMgr, *movementMgr, *physicsMgr, *collisionMgr);
	RegisterGameBindings(*state_.currentScene);

	if (audioMgr) {
		// Let the scene trigger UI and gameplay audio directly.
		state_.currentScene->SetAudioManager(audioMgr);
	}

	// Connect the scene to the message bus and seed its initial load state.
	state_.currentScene->SetMessageBus(&state_.coreEngine->GetMessageBus());
	state_.currentScene->LoadScene("LoadTest");

	// Wire the scene entity manager into each system that operates on scene objects.
	animMgr->SetEntityManager(&state_.currentScene->GetEntityManager());
	physicsMgr->SetEntityManager(&state_.currentScene->GetEntityManager());
	physicsMgr->SetInputManager(inputMgr);
	movementMgr->SetEntityManager(&state_.currentScene->GetEntityManager());
	movementMgr->SetInputManager(inputMgr);
	collisionMgr->SetEntityManager(&state_.currentScene->GetEntityManager());

	if (auto* gsm = state_.coreEngine->GetSystem<Framework::GameStateManager>()) {
		// Connect state management after the scene exists so level loads can target it.
		gsm->SetScene(state_.currentScene.get());
		if (audioMgr) {
			gsm->SetAudioManager(audioMgr);
		}

		ConfigureGameStates(*gsm);
		ConfigureGameStateAudioPolicy(*gsm);
		gsm->InitializeGameState(Framework::GameState::MainMenu, 0.0f);

		if (auto* animMgrForcePlay = state_.coreEngine->GetSystem<AnimationManager>()) {
			animMgrForcePlay->Play();
		}
	}

#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
	// Build the debug UI last so it can inspect the already-initialized engine state.
	state_.debugApp = std::make_unique<Debug::DebuggerApp>();
	if (!state_.debugApp->InitializeDebuggerApp(state_.window, state_.coreEngine.get())) {
		TS_LOG_ERROR("Failed to initialize DebuggerApp");
		return false;
	}

	state_.debugApp->AddDebugLine("DebuggerApp initialized successfully\n");
	state_.debugApp->SetScene(state_.currentScene.get());
#endif

	return true;
}

/**
 * @brief Processes one frame of input, scene logic, transitions, and engine updates.
 */
void Application::Update() {
	// Measure the raw frame time before any pause logic or replay overrides are applied.
	float currentFrame = static_cast<float>(glfwGetTime());
	float rawDeltaTime = currentFrame - state_.lastFrame;
	state_.lastFrame = currentFrame;

	float frameDt = rawDeltaTime;

	glfwPollEvents();

	// Toggle fullscreen only on the rising edge of F11.
	int f11State = glfwGetKey(state_.window, GLFW_KEY_F11);
	bool f11Down = (f11State == GLFW_PRESS || f11State == GLFW_REPEAT);
	if (f11Down && !state_.f11WasDown) {
		ToggleFullscreen(state_);
	}
	state_.f11WasDown = f11Down;

	if (state_.pausedByOSFocus) {
		// Keep timing statistics fresh even while simulation is paused by the OS.
		state_.smoothedDt = (state_.smoothedDt == 0.0f)
			? frameDt
			: (0.96f * state_.smoothedDt) + (0.04f * frameDt);

#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
		if (state_.debugApp) {
			state_.debugApp->fps = 0.0f;
			state_.debugApp->msperFrame = 0.0f;
		}
#endif
		return;
	}

	// Let the scene update first so it can publish pending transitions or replay timing.
	state_.currentScene->Update(frameDt, state_.window);

	const float replayDt = state_.currentScene->GetReplayFrameDt();
	const bool replayActive = state_.currentScene->IsReplayPlaybackActive();
	const bool hasReplayDt = state_.currentScene->HasReplayFrameDt();
	frameDt = (replayActive && hasReplayDt) ? replayDt : frameDt;

	auto* graphicsEngine = state_.coreEngine->GetSystem<GraphicsEngine>();

	if (state_.currentScene->HasPendingStateChange()) {
		// Queue scene changes behind a fade transition to avoid popping between states.
		Framework::GameState newState = state_.currentScene->GetPendingState();
		state_.currentScene->ClearPendingStateChange();

		if (graphicsEngine && !graphicsEngine->IsTransitionActive()) {
			graphicsEngine->StartSceneTransition(0.35f, 0.35f);
			state_.pendingStateAfterFade = newState;
			TS_LOG_DEBUG("[Application] Queued state change " << static_cast<int>(newState) << " to run at blackout");
		}
		else {
			state_.pendingStateAfterFade = newState;
		}
	}

	if (graphicsEngine && graphicsEngine->IsAtBlackout() && state_.pendingStateAfterFade.has_value()) {
		// Apply the queued state change only when the transition has fully blacked out.
		if (auto* gsm = state_.coreEngine->GetSystem<Framework::GameStateManager>()) {
			TS_LOG_INFO("[Application] Blackout reached; switching to state " << static_cast<int>(*state_.pendingStateAfterFade));
			gsm->UpdateGameState(*state_.pendingStateAfterFade, frameDt);
		}
		else {
			TS_LOG_ERROR("[Application] GameStateManager not found!");
		}

		graphicsEngine->ContinueTransitionFadeIn();
		state_.pendingStateAfterFade.reset();

		// Clear carry-over input so menu clicks do not leak into the next state.
		if (auto* inputMgr = state_.coreEngine->GetSystem<InputManager>()) {
			inputMgr->ClearState();
		}
	}

	// Smooth the displayed frame timing so the debug readout is less noisy.
	state_.smoothedDt = (state_.smoothedDt == 0.0f) ? frameDt : (0.96f * state_.smoothedDt) + (0.04f * frameDt);

#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
	if (state_.debugApp) {
		state_.debugApp->fps = (state_.smoothedDt > 0.f) ? (1.f / state_.smoothedDt + 0.5f) : 0.f;
		state_.debugApp->msperFrame = state_.smoothedDt * 1000.0f;
	}
#endif

	// Replay playback can override dt so systems advance deterministically during playback.
	if (replayActive && hasReplayDt) {
		state_.coreEngine->GameLoop(frameDt);
	}
	else {
		state_.coreEngine->GameLoop();
	}
}

/**
 * @brief Renders the current frame, including scene content, text, and debug overlays.
 */
void Application::Draw() {
	// Gather renderable objects into a transient draw list for batched rendering.
	std::vector<GameObject*> drawList;

	auto* graphicsEngine = state_.coreEngine->GetSystem<GraphicsEngine>();
	if (!graphicsEngine) {
		TS_LOG_ERROR("GraphicsEngine not found in CoreEngine during draw!");
		return;
	}

	// Begin the frame before any scene or debug UI rendering.
	graphicsEngine->BeginFrame();
	state_.currentScene->DrawUI();
	state_.currentScene->CollectRenderablePointers(drawList);

#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
	if (state_.debugApp && state_.debugApp->IsActive()) {
		state_.debugApp->RenderDebuggerApp();
	}
#endif

#ifndef _DEBUG
	const bool pauseOverlayActive = state_.currentScene->IsPauseOverlayActive();
	if (pauseOverlayActive) {
		// Keep pause text above the gameplay frame when the overlay is active.
		state_.currentScene->RenderLevelTextObjects();
	}
#endif

	// Suppress debug text during cutscenes so HUD labels do not overlap full-screen art.
	graphicsEngine->SetSuppressDebugTextRendering(state_.currentScene->IsAnyCutsceneActive());
	graphicsEngine->RenderBatched(drawList);
	state_.currentScene->RenderFPSText();

#ifndef _DEBUG
	if (!pauseOverlayActive) {
		state_.currentScene->RenderLevelTextObjects();
	}
#endif

#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
	if (state_.debugApp) {
		state_.debugApp->SetRenderStats(
			graphicsEngine->GetTotalObjects(),
			graphicsEngine->GetBatchCount(),
			graphicsEngine->GetInstancedObjectCount(),
			graphicsEngine->GetDrawCallCount()
		);
	}
#endif

	glfwSwapBuffers(state_.window);
}

/**
 * @brief Shuts down callbacks, systems, resources, and the GLFW window in a safe order.
 */
void Application::Cleanup() {
	static bool cleanupCalled = false;
	// Guard against duplicate shutdown paths from multiple error exits.
	if (cleanupCalled) {
		return;
	}
	cleanupCalled = true;

	TS_LOG_INFO("Starting cleanup...");

	if (state_.window) {
		// Clear callbacks first so no pending OS events call into torn-down state.
		glfwSetWindowCloseCallback(state_.window, nullptr);
		glfwSetCharCallback(state_.window, nullptr);
		glfwSetMouseButtonCallback(state_.window, nullptr);
		glfwSetCursorPosCallback(state_.window, nullptr);
		glfwSetKeyCallback(state_.window, nullptr);
		glfwSetScrollCallback(state_.window, nullptr);
		glfwSetDropCallback(state_.window, nullptr);
		glfwSetWindowFocusCallback(state_.window, nullptr);
		glfwSetWindowIconifyCallback(state_.window, nullptr);
		glfwSetErrorCallback(nullptr);
		glfwPollEvents();
	}

	// Prevent any later callback from reaching stale application state.
	g_AppState = nullptr;

	if (state_.coreEngine) {
		// Drop queued messages so no late dispatch happens during teardown.
		state_.coreEngine->GetMessageBus().ClearQueue();
	}

	if (state_.coreEngine) {
		if (auto* audioMgr = state_.coreEngine->GetSystem<AudioManager>()) {
			// Stop playback before destroying the audio backend.
			audioMgr->StopAllSounds();
			audioMgr->Shutdown();
		}
	}

	// Release catalog-owned sound resources before graphics and engine teardown.
	Audio::AudioCatalog::UnloadAllAudio();

#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
	if (state_.debugApp) {
		// Shutdown the debug UI while the GL context is still valid.
		state_.debugApp->Shutdown();
		state_.debugApp.reset();
	}
#endif

	// Destroy scene-owned objects before shutting down rendering systems.
	state_.currentScene.reset();

	if (state_.coreEngine) {
		if (auto* gfxEngine = state_.coreEngine->GetSystem<GraphicsEngine>()) {
			gfxEngine->Shutdown();
		}
	}

	// Clear cached resources and then destroy the engine itself.
	ResourceManager::Instance().Clear();
	state_.coreEngine.reset();

	if (state_.window) {
		// Release the GL context before destroying the GLFW window.
		glfwMakeContextCurrent(nullptr);
		glfwDestroyWindow(state_.window);
		state_.window = nullptr;
	}

	glfwPollEvents();
	glfwTerminate();
	TS_LOG_INFO("Cleanup complete.");
}

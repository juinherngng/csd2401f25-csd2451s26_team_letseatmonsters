/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Main.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu
					Seah Wang Hua, wanghua.seah@digipen.edu

 DESCRIPTION:		Entry point of the application. Initializes GLFW, creates the CoreEngine and all
					engine systems, loads the active Scene, and runs the main update/draw loop.
					Handles window creation, fullscreen toggling, OS-focus pause/resume behaviour,
					signal handling, and overall application shutdown and cleanup.

		All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <algorithm>
#include <cctype>
#include <csignal>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>

#include "Core/AudioLoading.hpp"
#include "Core/AudioManager.hpp"
#include "Core/ConfigManager.hpp"
#include "Core/Core.hpp"
#include "Core/DebugUI.hpp"
#include "Core/FileDropHandler.hpp"
#include "Core/GameStateManager.hpp"
#include "Core/LevelEditorFileIO.hpp"
#include "Core/MovementManager.hpp"
#include "Core/Precompiled.hpp"
#include "Core/TileMap.hpp"
#include "Graphics/GraphicsEngine.hpp"
#include "Graphics/ResourceManager.hpp"
#include "Graphics/SceneManager.hpp"

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

#define DBG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DBG_NEW
#endif

// Application state structure - eliminates static variables
struct ApplicationState {
	std::unique_ptr<CoreFramework::CoreEngine> coreEngine;
	std::unique_ptr<Scene> currentScene;
#ifdef _DEBUG
	std::unique_ptr<Debug::DebuggerApp> debugApp;
#endif

	GLFWwindow* window = nullptr; // GLFW owns this, we just reference it
	float lastFrame = 0.0f;
	float smoothedDt = 0.0f;
	volatile bool shouldExit = false;

	// Mouse tracking for delta calculations
	double lastMouseX = 0.0;
	double lastMouseY = 0.0;
	bool mouseInitialized = false;

	// Pause / OS focus handling
	bool pausedByOSFocus = false;
	bool simActiveBeforePause = false;
	bool modalDialogOpen = false;

	// Fullscreen toggling (F11)
	bool isFullscreen = false;
	int windowedPosX = 100;
	int windowedPosY = 100;
	int windowedWidth = 1200;
	int windowedHeight = 800;
	bool f11WasDown = false;
};

// Global app state pointer for signal handlers and callbacks
ApplicationState* g_AppState = nullptr;

// Forward Declarations
static void draw(ApplicationState& app);
static void update(ApplicationState& app);
static bool init(ApplicationState& app, GLint width, GLint height, std::string title, bool fullscreen);
static void cleanup(ApplicationState& app);
static void signalHandler(int signal);
static void HandlePauseResume(bool pause);
static void ToggleFullscreen(ApplicationState& app);

// Exposed for file-dialog code
void SetModalDialogOpen(bool open) {
	if (g_AppState) {
		g_AppState->modalDialogOpen = open;
	}
}

// Signal handler for Ctrl+C, Ctrl+Break, and console close
static void signalHandler(int signal) {
	std::cout << "\nReceived signal " << signal << ", cleaning up..." << std::endl;

	if (g_AppState) {
		g_AppState->shouldExit = true;

		// Set the window to close if it exists
		if (g_AppState->window) {
			glfwSetWindowShouldClose(g_AppState->window, GLFW_TRUE);
		}
	}
}

static void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
	(void)window;

	// Guard against spurious 0x0 events
	if (width <= 0 || height <= 0) {
		return;
	}

	// Safe to call now that GLAD is initialized
	glViewport(0, 0, width, height);

	// Keep both engines in sync - get GraphicsEngine from CoreEngine
	GraphicsEngine::Instance().Resize(width, height);
	if (g_AppState && g_AppState->coreEngine) {
		if (auto* gfxEngine = g_AppState->coreEngine->GetSystem<GraphicsEngine>()) {
			gfxEngine->Resize(width, height);
		}
	}
}

static void HandlePauseResume(bool pause) {
	if (!g_AppState || !g_AppState->coreEngine) {
		return;
	}

	auto* audioMgr = g_AppState->coreEngine->GetSystem<AudioManager>();
	auto* inputMgr = g_AppState->coreEngine->GetSystem<InputManager>();
	Scene* scene = g_AppState->currentScene.get();

	if (pause) {
		if (g_AppState->pausedByOSFocus) {
			return; // already paused
		}

		g_AppState->pausedByOSFocus = true;

		// Remember and stop simulation (gameplay/physics)
		if (scene) {
			g_AppState->simActiveBeforePause = scene->IsSimulationActive();
			scene->SetSimulationActive(false);
		}

		// Pause all audio
		if (audioMgr) {
			audioMgr->PauseAll();
		}

		// Clear input so keys/mouse don't get stuck
		if (inputMgr) {
			inputMgr->ClearState();
		}
	}
	else {
		if (!g_AppState->pausedByOSFocus) {
			return; // not paused by OS
		}

		g_AppState->pausedByOSFocus = false;

		// Avoid a huge dt spike when we come back
		g_AppState->lastFrame = static_cast<float>(glfwGetTime());

		// Restore simulation to whatever it was before pause
		if (scene) {
			scene->SetSimulationActive(g_AppState->simActiveBeforePause);
		}

		// Resume audio
		if (audioMgr) {
			audioMgr->ResumeAll();
		}

		// Clear any weird lingering input states
		if (inputMgr) {
			inputMgr->ClearState();
		}
	}
}

static void ToggleFullscreen(ApplicationState& app) {
	if (!app.window) {
		return;
	}

	// If we�re going from windowed to fullscreen
	if (!app.isFullscreen) {
		// Save current windowed position and size
		glfwGetWindowPos(app.window, &app.windowedPosX, &app.windowedPosY);
		glfwGetWindowSize(app.window, &app.windowedWidth, &app.windowedHeight);

		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);

		// Switch to fullscreen on the primary monitor
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
	else { // fullscreen to windowed
		glfwSetWindowMonitor(
			app.window,
			nullptr,
			app.windowedPosX,
			app.windowedPosY,
			app.windowedWidth,
			app.windowedHeight,
			0 // refresh rate ignored for windowed
		);

		app.isFullscreen = false;
	}

	// Update viewport and graphics engine after the switch
	int fbw = 0, fbh = 0;
	glfwGetFramebufferSize(app.window, &fbw, &fbh);
	glViewport(0, 0, fbw, fbh);

	GraphicsEngine::Instance().Resize(fbw, fbh);
	if (app.coreEngine) {
		if (auto* gfx = app.coreEngine->GetSystem<GraphicsEngine>()) {
			gfx->Resize(fbw, fbh);
		}
	}
}

#ifdef _WIN32
#include <windows.h>

// Windows console event handler
BOOL WINAPI ConsoleHandler(DWORD signal) {
	switch (signal) {
		case CTRL_C_EVENT:
		case CTRL_BREAK_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
		std::cout << "Console event detected, cleaning up..." << std::endl;
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
#endif

// Main
int main() {

#ifdef _DEBUG
	// Enable full automatic memory leak detection
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// Output to stderr
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);

	std::cout << "=== Memory leak detection enabled ===" << std::endl;
#endif

#ifdef _WIN32
	// Get the executable path and set working directory to its location
	char exePath[MAX_PATH];
	GetModuleFileNameA(NULL, exePath, MAX_PATH);
	
	// Extract directory from full path
	std::string exePathStr(exePath);
	size_t lastSlash = exePathStr.find_last_of("\\/");
	if (lastSlash != std::string::npos) {
		std::string exeDir = exePathStr.substr(0, lastSlash);
		SetCurrentDirectoryA(exeDir.c_str());
		std::cout << "[Main] Set working directory to: " << exeDir << std::endl;
	}
#else
	// For non-Windows platforms, use std::filesystem
	auto exePath = std::filesystem::read_symlink("/proc/self/exe");
	auto exeDir = exePath.parent_path();
	std::filesystem::current_path(exeDir);
	std::cout << "[Main] Set working directory to: " << exeDir << std::endl;
#endif

	// Create application state on the stack
	ApplicationState app;
	g_AppState = &app; // Set global pointer for signal handlers

	// Install signal handlers
	std::signal(SIGINT, signalHandler);  // Ctrl+C
	std::signal(SIGTERM, signalHandler); // Termination request
	std::signal(SIGABRT, signalHandler); // Abort

#ifdef _WIN32
	// Windows-specific console event handler
	if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
		std::cerr << "Failed to set console control handler" << std::endl;
	}
#endif

	auto settings = ConfigManager::LoadFromAssetsOrDefaults();
	ConfigManager::Validate(settings);

	bool startFullscreen = settings.fullscreen;

	if (!init(app, settings.resolution.width, settings.resolution.height, "TheStove", false)) {
		cleanup(app);
		return -1;
	}

	if (startFullscreen) {
		ToggleFullscreen(app);
	}

	if (auto* audioMgr = app.coreEngine->GetSystem<AudioManager>()) {
		audioMgr->ApplySettings(settings);
		float bgm = audioMgr->GetBgmVolume();
		float vfx = audioMgr->GetVfxVolume();
		std::cout << "AudioManager system found in CoreEngine - BGM Volume: " << bgm << ", VFX Volume: " << vfx << "\n";
	}
	else {
		std::cerr << "AudioManager system not found in CoreEngine\n";
	}

	// test tile map
	std::cout << "Testing TileMap class" << std::endl;
	MapData testMap(6, 6);

	for (int i = 0; i < testMap.getHeight(); i++) {
		testMap.setTile(i, i, 1);
	}

	testMap.printMap();

	std::cout << "There are " << testMap.SweepFor(ENTITY) << " Entities on the Map" << std::endl;

	app.lastFrame = static_cast<float>(glfwGetTime());

	while (!glfwWindowShouldClose(app.window) && !app.shouldExit) {

		try {
			// ---- TEST CASES FOR PRINTING TO CRASH_LOG.TXT ----
			// Uncomment one at a time to test
			// throw std::runtime_error("Test crash_log");
			// throw 42; // unknown exception

			/*std::string filename = "fake_file.txt";
			std::ifstream file(filename);

			if (!file.is_open())
			{
				app.debugApp->LogError("Test Case : could not open file : " + filename);
			 }
			throw std::runtime_error("Unknown file could not be opened.");*/

			//app.debugApp->RunDebuggerApp();
		}
		catch (const std::exception& e) {
#ifdef _DEBUG
			if (app.debugApp) app.debugApp->LogError(std::string("Unhandled exception: ") + e.what());
#endif
			std::cerr << "Error: " << e.what() << std::endl;
			cleanup(app);
			return -1;
		}
		catch (...) {
#ifdef _DEBUG
			if (app.debugApp) app.debugApp->LogError("Unknown crash occurred");
#endif
			std::cerr << "Crash: Unknown exception\n";				
			cleanup(app);
			return -1;
		}

		update(app);
		draw(app);
	}

	cleanup(app);

	g_AppState = nullptr; // Clear global pointer

#ifdef _DEBUG
	std::cout << "\n=== Memory Leak Report ===" << std::endl;
	std::cout << "Checking for memory leaks..." << std::endl;
	std::cout << "If no leaks are detected, no additional output will appear below." << std::endl;
	std::cout << "=== End of Memory Leak Report ===" << std::endl;
#endif

	return 0;
}

// Initialization / Shutdown
static bool init(ApplicationState& app, GLint width, GLint height, std::string title, bool fullscreen) {
	// Save desired windowed size from config (used when toggling out of fullscreen)
	app.windowedWidth = width;
	app.windowedHeight = height;
	app.windowedPosX = 100;
	app.windowedPosY = 100;

	// Start fullscreen state according to config
	app.isFullscreen = fullscreen;

	// Set GLFW error callback
	glfwSetErrorCallback([](int error, const char* description) {
		std::cerr << "GLFW Error " << error << ": " << description << std::endl;
	});

	// Initialize GLFW
	if (!glfwInit()) {
		std::cerr << "Failed to init GLFW" << std::endl;
		return false;
	}

	// Make the window non-resizable
	// glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	// Decide between windowed and fullscreen
	if (fullscreen) {
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);

		// Use the monitor's native resolution for true fullscreen
		width = mode->width;
		height = mode->height;

		app.window = glfwCreateWindow(width, height, title.c_str(), monitor, nullptr);
	}
	else {
		// Normal windowed mode
		app.window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	}

	if (!app.window) {
		std::cerr << "Failed to create window" << std::endl;
		glfwTerminate();
		app.window = nullptr;
		return false;
	}

	// If we started in windowed mode, remember its actual pos/size
	if (!app.isFullscreen) {
		glfwGetWindowPos(app.window, &app.windowedPosX, &app.windowedPosY);
		glfwGetWindowSize(app.window, &app.windowedWidth, &app.windowedHeight);
	}

	glfwMakeContextCurrent(app.window);

	// Add window close callback to trigger cleanup
	glfwSetWindowCloseCallback(app.window, [](GLFWwindow* win) {
		(void)win; // suppress unused parameter warning
		std::cout << "Window close requested, cleaning up..." << std::endl;
		if (g_AppState) {
			g_AppState->shouldExit = true;
		}
	});

	// Message callbacks to post input events to CoreEngine
	glfwSetCharCallback(app.window, [](GLFWwindow* win, unsigned int c) {
		(void)win;   // suppress unused parameter warning
		if (g_AppState && g_AppState->coreEngine)
			g_AppState->coreEngine->GetMessageBus().Post<CoreFramework::CharacterKeyMessage>(static_cast<char>(c), true);
	});

	// Mouse button to message bus
	glfwSetMouseButtonCallback(app.window, [](GLFWwindow* win, int button, int action, int mods) {
		(void)mods, (void)win;    // suppress unused parameter warning
		if (g_AppState && g_AppState->coreEngine) {
			double x, y;
			glfwGetCursorPos(g_AppState->window, &x, &y);
			g_AppState->coreEngine->GetMessageBus().Post<CoreFramework::MouseButtonMessage>(button, action == GLFW_PRESS, x, y);
		}
	});

	// Mouse move to message bus (with delta)
	glfwSetCursorPosCallback(app.window, [](GLFWwindow* win, double xpos, double ypos) {
		(void)win; // suppress unused parameter warning

		if (g_AppState && g_AppState->coreEngine) {
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

	// Ensure we don't have any other callbacks set
	glfwSetKeyCallback(app.window, nullptr);
	glfwSetScrollCallback(app.window, nullptr);

	// External file drop callback - forwards to FileDropHandler system
	glfwSetDropCallback(app.window, [](GLFWwindow* win, int count, const char** paths) {
		(void)win;
		if (g_AppState && g_AppState->coreEngine) {
			if (auto* dropHandler = g_AppState->coreEngine->GetSystem<FileDropHandler>()) {
				dropHandler->HandleGLFWDrop(count, paths);
			}
		}
	});

	// When we lose focus (ALT-TAB, CTRL-ALT-DEL, clicking another window),
	// pause the game but don't force minimize
	glfwSetWindowFocusCallback(app.window, [](GLFWwindow* win, int focused) {
		(void)win; // suppress unused parameter warning

		if (focused == GLFW_FALSE) {
			// Don't minimize if a modal dialog (file picker) is open
			if (g_AppState && g_AppState->modalDialogOpen) {
				// Just pause audio/input, but don't force minimize
				HandlePauseResume(true);
				return;
			}

			// Pause gameplay, physics, audio, and clear input
			HandlePauseResume(true);
		}
		else {
			// We regained focus (coming back from taskbar / ALT-TAB)
			HandlePauseResume(false);
		}
	});

	// Iconify callback is kept just to keep pause/resume in sync
	glfwSetWindowIconifyCallback(app.window, [](GLFWwindow* win, int iconified) {
		(void)win;
		HandlePauseResume(iconified == GLFW_TRUE);
	});


	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD\n";
		return false;
	}

	// Automatically update viewport and projection when window is resized/maximized
	glfwSetFramebufferSizeCallback(app.window, FramebufferSizeCallback);

	// Create CoreEngine with smart pointer
	app.coreEngine = std::make_unique<CoreFramework::CoreEngine>();

	// Add systems using unique_ptr with MessageBus reference
	app.coreEngine->AddSystem(std::make_unique<InputManager>());
	app.coreEngine->AddSystem(std::make_unique<GraphicsEngine>());		// Register GraphicsEngine as a system - CoreEngine takes ownership
	app.coreEngine->AddSystem(std::make_unique<AudioManager>(app.coreEngine->GetMessageBus()));
	app.coreEngine->AddSystem(std::make_unique<FileDropHandler>(app.coreEngine->GetMessageBus()));
	app.coreEngine->AddSystem(std::make_unique<Framework::GameStateManager>(app.coreEngine->GetMessageBus()));
	app.coreEngine->AddSystem(std::make_unique<AnimationManager>());
	app.coreEngine->AddSystem(std::make_unique<MovementManager>());
	app.coreEngine->AddSystem(std::make_unique<PhysicsManager>());
	app.coreEngine->AddSystem(std::make_unique<CollisionManager>());

	app.coreEngine->Initialize();

	// Set window for InputManager system
	if (auto* inputMgr = app.coreEngine->GetSystem<InputManager>()) {
		inputMgr->SetWindow(app.window);
		std::cout << "InputManager system initialized.\n";
	}
	else {
		std::cerr << "Warning: InputManager not found in CoreEngine!\n";
	}

	// Initialize ResourceManager with AudioManager
	if (auto* audioMgr = app.coreEngine->GetSystem<AudioManager>()) {
		ResourceManager::Instance().SetAudioManager(audioMgr);
		std::cout << "ResourceManager initialized with AudioManager." << std::endl;

		// Load audio catalog from SOURCE directory (../../assets from build/Release)
		const std::string catalogPath = "../../assets/Audio/AudioCatalog.json";
		if (!Audio::AudioCatalog::LoadCatalogFromFile(catalogPath))
		{
			std::cerr << "Warning: Failed to load audio catalog from " << catalogPath << std::endl;
			std::cerr << "Creating default catalog..." << std::endl;
			// If catalog doesn't exist, it will be empty but won't crash
		}

		// Load all audio assets from the catalog
		Audio::AudioCatalog::LoadAllAudio();
	}
	else {
		std::cerr << "Warning: AudioManager not found in CoreEngine for ResourceManager!" << std::endl;
	}

	// Get GraphicsEngine from CoreEngine
	GraphicsEngine* graphicsEngine = app.coreEngine->GetSystem<GraphicsEngine>();
	if (!graphicsEngine) {
		std::cerr << "Failed to get GraphicsEngine from CoreEngine\n";
		return false;
	}

	// Initialize once with the current framebuffer size (handles DPI scaling)
	int fbw = 0, fbh = 0;
	glfwGetFramebufferSize(app.window, &fbw, &fbh);

	// Update OpenGL viewport immediately
	glViewport(0, 0, fbw, fbh);

	// Update BOTH the singleton (used by InputManager) and the instance (used by renderer)
	GraphicsEngine::Instance().Resize(fbw, fbh);
	graphicsEngine->Resize(fbw, fbh);

	// Get InputManager system for Scene
	InputManager* inputMgr = app.coreEngine->GetSystem<InputManager>();
	if (!inputMgr) {
		std::cerr << "Failed to get InputManager from CoreEngine\n";
		return false;
	}

	// Get AnimationManager system for Scene
	AnimationManager* animMgr = app.coreEngine->GetSystem<AnimationManager>();
	if (!animMgr) {
		std::cerr << "Failed to get AnimationManager from CoreEngine\n";
		return false;
	}

	// Get PhysicsManager system
	PhysicsManager* physicsMgr = app.coreEngine->GetSystem<PhysicsManager>();
	if (!physicsMgr) {
		std::cerr << "Failed to get PhysicsManager from CoreEngine\n";
		return false;
	}

	// Get MovementManager system
	MovementManager* movementMgr = app.coreEngine->GetSystem<MovementManager>();
	if (!movementMgr) {
		std::cerr << "Failed to get MovementManager from CoreEngine\n";
		return false;
	}

	// Get CollisionManager system
	CollisionManager* collisionMgr = app.coreEngine->GetSystem<CollisionManager>();
	if (!collisionMgr) {
		std::cerr << "Failed to get CollisionManager from CoreEngine\n";
		return false;
	}

	// Create Scene with smart pointer, passing all manager references
	app.currentScene = std::make_unique<Scene>(*graphicsEngine, *inputMgr, *animMgr,
											   *movementMgr, *physicsMgr, *collisionMgr);

	// Get AudioManager and set it on Scene for UI sounds
	AudioManager* audioMgr = app.coreEngine->GetSystem<AudioManager>();
	if (audioMgr) {
		app.currentScene->SetAudioManager(audioMgr);
		std::cout << "AudioManager connected to Scene for UI sounds.\n";
	}

	app.currentScene->LoadScene("LoadTest");

	// Set the EntityManager reference in AnimationManager
	animMgr->SetEntityManager(&app.currentScene->GetEntityManager());

	std::cout << "AnimationManager system connected to Scene and EntityManager.\n";

	// Set the EntityManager and InputManager references in PhysicsManager
	physicsMgr->SetEntityManager(&app.currentScene->GetEntityManager());
	physicsMgr->SetInputManager(inputMgr);

	std::cout << "PhysicsManager system connected to EntityManager and InputManager.\n";

	// Set the EntityManager and InputManager references in MovementManager
	movementMgr->SetEntityManager(&app.currentScene->GetEntityManager());
	movementMgr->SetInputManager(inputMgr);

	std::cout << "MovementManager system connected to EntityManager and InputManager.\n";

	// Set the EntityManager reference in CollisionManager
	collisionMgr->SetEntityManager(&app.currentScene->GetEntityManager());

	std::cout << "CollisionManager system connected to EntityManager.\n";

	// Scene is now constructed with MovementManager reference - no need for SetMovementManager
	std::cout << "Scene connected to MovementManager system.\n";

	{
	auto* gsm = app.coreEngine->GetSystem<Framework::GameStateManager>();
	auto* audioMgrGsm = app.coreEngine->GetSystem<AudioManager>();

	if (gsm) {
		gsm->SetScene(app.currentScene.get());

		// Inject AudioManager for state-based audio control
		if (audioMgrGsm) {
			gsm->SetAudioManager(audioMgrGsm);
		}

		// Map states to JSON files
		gsm->RegisterJsonState(Framework::GS_Level1, "../levels/main_menu.json");	// state 0 = menu
		gsm->RegisterJsonState(Framework::GS_Level2, "../levels/kitchen01.json");	// state 1 = gameplay
		
		// Initialize to main menu state
		gsm->InitializeGameState(Framework::GS_Level1, 0.0f);
	}
}

#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
	// Create DebuggerApp with smart pointer (debug-only)
	app.debugApp = std::make_unique<Debug::DebuggerApp>();
	if (!app.debugApp->InitializeDebuggerApp(app.window, app.coreEngine.get())) {
		std::cerr << "Failed to initialize DebuggerApp\n";
		return false;
	}
	else {
		app.debugApp->AddDebugLine("DebuggerApp initialized successfully\n");
	}

	app.debugApp->SetScene(app.currentScene.get());
#endif

	return true;
}

// Update / Draw
static void update(ApplicationState& app) {
	// Calculate delta time
	float currentFrame = static_cast<float>(glfwGetTime());
	float deltaTime = currentFrame - app.lastFrame;
	app.lastFrame = currentFrame;

	glfwPollEvents();

	// Handle F11 for fullscreen toggle (global hotkey)
	int f11State = glfwGetKey(app.window, GLFW_KEY_F11);
	bool f11Down = (f11State == GLFW_PRESS || f11State == GLFW_REPEAT);

	// Edge detect: only toggle when key transitions from up to down
	if (f11Down && !app.f11WasDown) {
		ToggleFullscreen(app);
	}

	app.f11WasDown = f11Down;

	if (app.pausedByOSFocus) {
		// You can still keep FPS stats if you like, or set them to 0
		app.smoothedDt = (app.smoothedDt == 0.0f)
			?deltaTime
			:(0.96f * app.smoothedDt) + (0.04f * deltaTime);

#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
		if (app.debugApp) {
			app.debugApp->fps = 0.0f;
			app.debugApp->msperFrame = 0.0f;
		}
#endif

		// Do NOT update scene or core engine while paused
		return;
	}

	// engine.BeginImGuiFrame();

	// Update scene with delta time and window pointer
	app.currentScene->Update(deltaTime, app.window);

	// Check for pending state changes from menu buttons
	if (app.currentScene->HasPendingStateChange()) {
		int newState = app.currentScene->GetPendingState();
		app.currentScene->ClearPendingStateChange();

		std::cout << "[Main] Processing state change request to state: " << newState << std::endl;

		// Trigger the state change through GameStateManager
		if (auto* gsm = app.coreEngine->GetSystem<Framework::GameStateManager>()) {
			std::cout << "[Main] Calling GameStateManager::UpdateGameState(" << newState << ")" << std::endl;
			gsm->UpdateGameState(newState, deltaTime);
		}
		else {
			std::cerr << "[Main] ERROR: GameStateManager not found!" << std::endl;
		}
	}

	// Smoothing for deltatime (for the fps)
	// Account for division by 0 on the first frame where gDt = 0
	// This controls how fast the fps counter reacts to changes
	// (higher value = smoother fps) else 
	// (lower value = faster fps change response but more jittery)
	app.smoothedDt = (app.smoothedDt == 0.0f)?deltaTime:(0.96f * app.smoothedDt) + (0.04f * deltaTime);

	// Update FPS display variables for DebuggerApp
#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
	if (app.debugApp) {
		app.debugApp->fps = (app.smoothedDt > 0.f)?(1.f / app.smoothedDt + 0.5f):0.f;
		app.debugApp->msperFrame = (app.smoothedDt * 1000.0f);
	}
#endif

	app.coreEngine->GameLoop();

	// if (app.debugApp->IsActive())
	// {
	//     app.debugApp->UpdateDebuggerApp();
	// }
}

static void draw(ApplicationState& app) {
	std::vector<GameObject*> drawList;

	// Get GraphicsEngine from CoreEngine
	auto* graphicsEngine = app.coreEngine->GetSystem<GraphicsEngine>();
	if (!graphicsEngine) {
		std::cerr << "GraphicsEngine not found in CoreEngine during draw!\n";
		return;
	}

	graphicsEngine->BeginFrame();

	// UI first
	app.currentScene->DrawUI();

	drawList.clear();
	app.currentScene->CollectRenderablePointers(drawList);

#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
	if (app.debugApp && app.debugApp->IsActive()) {
		app.debugApp->RenderDebuggerApp();
	}
#endif

	//graphicsEngine->Render(drawList);
	graphicsEngine->RenderBatched(drawList);

	// Render menu button texts on top (in both debug and release)
	app.currentScene->RenderMenuButtonTexts();

#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
	if (app.debugApp) {
		app.debugApp->SetRenderStats(
			graphicsEngine->GetTotalObjects(),
			graphicsEngine->GetBatchCount(),
			graphicsEngine->GetInstancedObjectCount(),
			graphicsEngine->GetDrawCallCount()
		);
	}
#endif

	glfwSwapBuffers(app.window);
}

void cleanup(ApplicationState& app) {
	static bool cleanupCalled = false;

	// Prevent multiple cleanup calls
	if (cleanupCalled) {
		return;
	}

	cleanupCalled = true;

	std::cout << "Starting cleanup..." << std::endl;

	// Clear all GLFW callbacks FIRST to prevent dangling references
	if (app.window) {
		std::cout << "Clearing GLFW callbacks..." << std::endl;
		glfwSetWindowCloseCallback(app.window, nullptr);
		glfwSetCharCallback(app.window, nullptr);
		glfwSetMouseButtonCallback(app.window, nullptr);
		glfwSetCursorPosCallback(app.window, nullptr);
		glfwSetKeyCallback(app.window, nullptr);
		glfwSetScrollCallback(app.window, nullptr);
		glfwSetDropCallback(app.window, nullptr);
		glfwSetWindowFocusCallback(app.window, nullptr);
		glfwSetWindowIconifyCallback(app.window, nullptr);
		glfwSetErrorCallback(nullptr);

		// Poll events one last time to clear any pending callbacks
		glfwPollEvents();
	}

	// Clear global app state pointer to prevent callback access
	g_AppState = nullptr;

	// Flush CoreEngine messages to prevent orphaned messages
	if (app.coreEngine) {
		std::cout << "Flushing remaining messages..." << std::endl;
		app.coreEngine->GetMessageBus().ClearQueue();
	}

	// Stop and shutdown audio
	if (app.coreEngine) {
		if (auto* audioMgr = app.coreEngine->GetSystem<AudioManager>()) {
			std::cout << "Stopping all sounds..." << std::endl;
			audioMgr->StopAllSounds();
			std::cout << "Shutting down audio..." << std::endl;
			audioMgr->Shutdown();
		}
	}
				
	// Unload all audio assets
	std::cout << "Unloading audio assets..." << std::endl;
	Audio::AudioCatalog::UnloadAllAudio();

	// Shutdown ImGui (must happen while OpenGL context is valid)
#if defined(_DEBUG) || defined(ENABLE_DEBUG_UI)
	if (app.debugApp) {
		std::cout << "Shutting down debugger..." << std::endl;
		app.debugApp->Shutdown();
		app.debugApp.reset();
	}
#endif

	// Clean up scene objects
	if (app.currentScene) {
		std::cout << "Deleting scene..." << std::endl;
		app.currentScene.reset();
	}

	// Shutdown graphics engine (now managed by CoreEngine)
	if (app.coreEngine) {
		if (auto* gfxEngine = app.coreEngine->GetSystem<GraphicsEngine>()) {
			std::cout << "Shutting down graphics engine..." << std::endl;
			gfxEngine->Shutdown();
		}
	}

	// Clear resource manager (while context still valid)
	std::cout << "Clearing resource manager..." << std::endl;
	ResourceManager::Instance().Clear();

	// Destroy CoreEngine and all systems
	if (app.coreEngine) {
		std::cout << "Destroying core engine..." << std::endl;
		app.coreEngine.reset();
	}

	// Make the context non-current before destroying window
	if (app.window) {
		glfwMakeContextCurrent(nullptr);
	}

	// Destroy window
	if (app.window) {
		std::cout << "Destroying window..." << std::endl;
		glfwDestroyWindow(app.window);
		app.window = nullptr;
	}

	// Poll events one final time to process window destruction
	glfwPollEvents();

	// Terminate GLFW
	std::cout << "Terminating GLFW..." << std::endl;
	glfwTerminate();

	std::cout << "Cleanup complete." << std::endl;
}

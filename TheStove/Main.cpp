#include "Core/DebugUI.hpp"
#include "Core/Precompiled.hpp"

#include <algorithm>
#include <cctype>
#include <csignal>
#include <iostream>
#include <sstream>
#include <string>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

#define DBG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DBG_NEW

#endif

#include "Graphics/GraphicsEngine.hpp"
#include "Graphics/SceneManager.hpp"
#include "Graphics/ResourceManager.hpp"
#include "Core/Core.hpp"
#include "Core/ConfigManager.hpp"
#include "Core/AudioManager.hpp"
#include "Core/AudioLoading.hpp"
#include "Core/GameStateManager.hpp"
#include "Core/TileMap.hpp"
#include "Core/MovementManager.hpp"

// Application state structure - eliminates static variables
struct ApplicationState {
	std::unique_ptr<CoreFramework::CoreEngine> coreEngine;
	std::unique_ptr<Scene> currentScene;
	std::unique_ptr<Debug::DebuggerApp> debugApp;
	GLFWwindow* window = nullptr; // GLFW owns this, we just reference it
	float lastFrame = 0.0f;
	float smoothedDt = 0.0f;
	volatile bool shouldExit = false;

	// Mouse tracking for delta calculations
	double lastMouseX = 0.0;
	double lastMouseY = 0.0;
	bool mouseInitialized = false;

	bool pausedByOSFocus = false;       // true while we're paused due to focus/iconify
	bool simActiveBeforePause = false;  // remember if simulation was running
	
	// Track when native dialogs are open to prevent unwanted minimize
	bool modalDialogOpen = false;
};

// Global app state pointer for signal handlers and callbacks
static ApplicationState* g_AppState = nullptr;

// Function to set modal dialog state (called by file dialog code)
void SetModalDialogOpen(bool open) {
	if (g_AppState) {
		g_AppState->modalDialogOpen = open;
	}
}

static void draw(ApplicationState& app);
static void update(ApplicationState& app);
static bool init(ApplicationState& app, GLint width, GLint height, std::string title, bool fullscreen);
static void cleanup(ApplicationState& app);
static void signalHandler(int signal);

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

static void HandlePauseResume(bool pause);

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

int main() {

#ifdef _DEBUG
	// Enable full automatic memory leak detection
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// Output to stderr
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);

	std::cout << "=== Memory leak detection enabled ===" << std::endl;
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

	if (!init(app, settings.resolution.width, settings.resolution.height, "TheStove", settings.fullscreen)) {
		cleanup(app);
		return -1;
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
			app.debugApp->LogError(std::string("Unhandled exception: ") + e.what());
			std::cerr << "Error: " << e.what() << std::endl;
			cleanup(app);
			return -1;
		}
		catch (...) // Catches all other exceptions not caught by the first
		{
			app.debugApp->LogError("Unknown crash occurred");
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

static bool init(ApplicationState& app, GLint width, GLint height, std::string title, bool fullscreen) {
	// Set GLFW error callback
	glfwSetErrorCallback([](int error, const char* description) {
		std::cerr << "GLFW Error " << error << ": " << description << std::endl;
		});

	// Initialize GLFW
	if (!glfwInit()) {
		std::cerr << "Failed to init GLFW" << std::endl;
		return false;
	}

	GLFWmonitor* monitor = nullptr;
	if (fullscreen) {
		monitor = glfwGetPrimaryMonitor();
	}

	app.window = glfwCreateWindow(width, height, title.c_str(), monitor, nullptr);
	if (!app.window) {
		std::cerr << "Failed to create window" << std::endl;
		glfwTerminate();
		app.window = nullptr;
		return false;
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

	glfwSetMouseButtonCallback(app.window, [](GLFWwindow* win, int button, int action, int mods) {
		(void)mods, (void)win;    // suppress unused parameter warning
		if (g_AppState && g_AppState->coreEngine) {
			double x, y;
			glfwGetCursorPos(g_AppState->window, &x, &y);
			g_AppState->coreEngine->GetMessageBus().Post<CoreFramework::MouseButtonMessage>(button, action == GLFW_PRESS, x, y);
		}
		});

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

			// Force the game window to minimize for real Alt-Tab/focus loss
			//glfwIconifyWindow(win);
			
			// Pause gameplay, physics, audio, and clear input
			// but let the user/OS decide if they want to minimize
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

		// Load audio catalog from JSON file
		if (!Audio::AudioCatalog::LoadCatalogFromFile("../assets/Audio/AudioCatalog.json"))
		{
			std::cerr << "Warning: Failed to load audio catalog. Creating default catalog..." << std::endl;
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

	// Create DebuggerApp with smart pointer
	app.debugApp = std::make_unique<Debug::DebuggerApp>();
	if (!app.debugApp->InitializeDebuggerApp(app.window, app.coreEngine.get())) {
		std::cerr << "Failed to initialize DebuggerApp\n";
		return false;
	}
	else {
		app.debugApp->AddDebugLine("DebuggerApp initialized successfully\n");
	}

	app.debugApp->SetScene(app.currentScene.get());

	return true;
}

static void update(ApplicationState& app) {

	// Calculate delta time
	float currentFrame = static_cast<float>(glfwGetTime());
	float deltaTime = currentFrame - app.lastFrame;
	app.lastFrame = currentFrame;

	glfwPollEvents();

	if (app.pausedByOSFocus) {
		// You can still keep FPS stats if you like, or set them to 0
		app.smoothedDt = (app.smoothedDt == 0.0f) ? deltaTime
			: (0.96f * app.smoothedDt) + (0.04f * deltaTime);

		if (app.debugApp) {
			app.debugApp->fps = 0.0f;
			app.debugApp->msperFrame = 0.0f;
		}

		// Do NOT update scene or core engine while paused
		return;
	}

	// engine.BeginImGuiFrame();

	// Update scene with delta time and window pointer
	app.currentScene->Update(deltaTime, app.window);

	// Smoothing for deltatime (for the fps)
	// Account for division by 0 on the first frame where gDt = 0
	// This controls how fast the fps counter reacts to changes
	// (higher value = smoother fps) else 
	// (lower value = faster fps change response but more jittery)
	app.smoothedDt = (app.smoothedDt == 0.0f) ? deltaTime : (0.96f * app.smoothedDt) + (0.04f * deltaTime);

	// Update FPS display variables for DebuggerApp
	app.debugApp->fps = (app.smoothedDt > 0.f) ? (1.f / app.smoothedDt + 0.5f) : 0.f;
	app.debugApp->msperFrame = (app.smoothedDt * 1000.0f);

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
	app.currentScene->DrawUI();
	drawList.clear();
	app.currentScene->CollectRenderablePointers(drawList);

	if (app.debugApp->IsActive()) {
		app.debugApp->RenderDebuggerApp();
	}

	//graphicsEngine->Render(drawList);
	graphicsEngine->RenderBatched(drawList);

	app.debugApp->SetRenderStats(
		graphicsEngine->GetTotalObjects(),
		graphicsEngine->GetBatchCount(),
		graphicsEngine->GetInstancedObjectCount(),
		graphicsEngine->GetDrawCallCount()
	);

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

	// STEP 1: Clear all GLFW callbacks FIRST to prevent dangling references
	if (app.window) {
		std::cout << "Clearing GLFW callbacks..." << std::endl;
		glfwSetWindowCloseCallback(app.window, nullptr);
		glfwSetCharCallback(app.window, nullptr);
		glfwSetMouseButtonCallback(app.window, nullptr);
		glfwSetCursorPosCallback(app.window, nullptr);
		glfwSetKeyCallback(app.window, nullptr);
		glfwSetScrollCallback(app.window, nullptr);
		glfwSetWindowFocusCallback(app.window, nullptr);
		glfwSetWindowIconifyCallback(app.window, nullptr);
		glfwSetErrorCallback(nullptr);

		// Poll events one last time to clear any pending callbacks
		glfwPollEvents();
	}

	// STEP 2: Clear global app state pointer to prevent callback access
	g_AppState = nullptr;

	// STEP 3: Flush CoreEngine messages to prevent orphaned messages
	if (app.coreEngine) {
		std::cout << "Flushing remaining messages..." << std::endl;
		app.coreEngine->GetMessageBus().ClearQueue();
	}

	// STEP 6: Stop and shutdown audio
	if (app.coreEngine) {
		if (auto* audioMgr = app.coreEngine->GetSystem<AudioManager>()) {
			std::cout << "Stopping all sounds..." << std::endl;
			audioMgr->StopAllSounds();
			std::cout << "Shutting down audio..." << std::endl;
			audioMgr->Shutdown();
		}
	}

	// STEP 6.5: Unload all audio assets
	std::cout << "Unloading audio assets..." << std::endl;
	Audio::AudioCatalog::UnloadAllAudio();

	// STEP 4: Shutdown ImGui (must happen while OpenGL context is valid)
	if (app.debugApp) {
		std::cout << "Shutting down debugger..." << std::endl;
		app.debugApp->Shutdown();
		app.debugApp.reset();
	}

	// STEP 5: Clean up scene objects
	if (app.currentScene) {
		std::cout << "Deleting scene..." << std::endl;
		app.currentScene.reset();
	}

	// STEP 7: Shutdown graphics engine (now managed by CoreEngine)
	if (app.coreEngine) {
		if (auto* gfxEngine = app.coreEngine->GetSystem<GraphicsEngine>()) {
			std::cout << "Shutting down graphics engine..." << std::endl;
			gfxEngine->Shutdown();
		}
	}

	// STEP 8: Clear resource manager (while context still valid)
	std::cout << "Clearing resource manager..." << std::endl;
	ResourceManager::Instance().Clear();

	// STEP 9: Destroy CoreEngine and all systems
	if (app.coreEngine) {
		std::cout << "Destroying core engine..." << std::endl;
		app.coreEngine.reset();
	}

	// STEP 10: Make the context non-current before destroying window
	if (app.window) {
		glfwMakeContextCurrent(nullptr);
	}

	// STEP 11: Destroy window
	if (app.window) {
		std::cout << "Destroying window..." << std::endl;
		glfwDestroyWindow(app.window);
		app.window = nullptr;
	}

	// STEP 12: Poll events one final time to process window destruction
	glfwPollEvents();

	// STEP 13: Terminate GLFW
	std::cout << "Terminating GLFW..." << std::endl;
	glfwTerminate();

	std::cout << "Cleanup complete." << std::endl;
}

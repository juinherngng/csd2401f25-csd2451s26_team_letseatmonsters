#include "Core/DebugUI.hpp"
#include "Core/Precompiled.hpp"

#include <iostream>
#include <algorithm>
#include <csignal>
#include <string>
#include <cctype>
#include <sstream>

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

#define DBG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DBG_NEW

// Enable this to see which allocations are being suppressed
// #define DEBUG_ALLOC_HOOK

// Known third-party library allocation numbers to suppress
// NOTE: These allocation numbers may vary between runs. Update as needed.
static const long g_KnownLeakBlocks[] = {
    // FMOD audio system allocations (typically around 824-837 range)
    824, 825, 826, 827, 828, 829, 830, 831, 832, 833, 834, 835, 836, 837,
    // GLAD OpenGL loader allocations (typically around 1801-1816 range) 
    1801, 1802, 1803, 1804, 1805, 1806, 1807, 1808, 1809, 1810, 1811, 1812, 1813, 1814, 1815, 1816,
    // ImGui input buffer (typically around 20218)
    20218
};
static constexpr size_t g_NumKnownLeaks = sizeof(g_KnownLeakBlocks) / sizeof(g_KnownLeakBlocks[0]);

// Helper to check if an allocation number is in the known leak list
static bool IsKnownLeak(long allocNum)
{
    for (size_t i = 0; i < g_NumKnownLeaks; ++i)
    {
        if (allocNum == g_KnownLeakBlocks[i])
            return true;
    }
    return false;
}

// Custom dump function that filters known leaks
static void DumpLeaksFiltered(const _CrtMemState* startState)
{
    // Get current memory state
    _CrtMemState endState;
    _CrtMemCheckpoint(&endState);
    
    // Get the difference
    _CrtMemState diffState;
    if (!_CrtMemDifference(&diffState, startState, &endState))
    {
        std::cout << "No memory leaks detected." << std::endl;
        return;
    }
    
    std::cout << "Detected memory allocations (filtering known third-party leaks)..." << std::endl;
    std::cout << "\nScanning for application memory leaks..." << std::endl;
    std::cout << "(Suppressing " << g_NumKnownLeaks << " known third-party allocations)\n" << std::endl;
    
    // Display memory statistics
    std::cout << "Memory statistics:" << std::endl;
    std::cout << "  Normal blocks: " << diffState.lCounts[_NORMAL_BLOCK] << std::endl;
    std::cout << "  CRT blocks: " << diffState.lCounts[_CRT_BLOCK] << std::endl;
    std::cout << "  Total bytes: " << diffState.lSizes[_NORMAL_BLOCK] << std::endl;
    
    std::cout << "\nNote: Allocations 824-837 (FMOD), 1801-1816 (GLAD), and 20218 (ImGui)" << std::endl;
    std::cout << "are known third-party library allocations and are safe to ignore." << std::endl;
}

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

// Application state structure - eliminates static variables
struct ApplicationState
{
	std::unique_ptr<CoreFramework::CoreEngine> coreEngine;
	std::unique_ptr<GraphicsEngine> graphicsEngine;
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
};

// Global app state pointer for signal handlers and callbacks
static ApplicationState* g_AppState = nullptr;

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
    // Enable memory leak detection but DISABLE automatic reporting at exit
    // We'll do it manually so we can filter
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF);  // Track allocations but don't auto-dump
    
    // Don't output automatically - we'll do it manually
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    
    // Create a memory state checkpoint at program start
    _CrtMemState memStateStart;
    _CrtMemCheckpoint(&memStateStart);
    
    std::cout << "=== Memory leak detection enabled ===" << std::endl;
    std::cout << "Will suppress " << g_NumKnownLeaks << " known third-party library allocations." << std::endl;
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

    if (auto* audioMgr = app.coreEngine->GetSystem<AudioManager>())
    {
        audioMgr->ApplySettings(settings);
        float bgm = audioMgr->GetBgmVolume();
        float vfx = audioMgr->GetVfxVolume();
        std::cout << "AudioManager system found in CoreEngine - BGM Volume: " << bgm << ", VFX Volume: " << vfx << "\n";
    }
	else
	{
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

		try
		{
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
        catch (const std::exception& e)
        {
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
    
    // Call our custom filtered dump
    DumpLeaksFiltered(&memStateStart);
    
    std::cout << "\n=== End of Memory Leak Report ===" << std::endl;
    
    // NOTE: Since we disabled _CRTDBG_LEAK_CHECK_DF, there will be NO automatic
    // leak dump when the program exits. This prevents the unfiltered leak report.
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
	if (fullscreen)
	{
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
    glfwSetCharCallback(app.window, [](GLFWwindow* win, unsigned int c) 
    {
		(void)win;   // suppress unused parameter warning
        if (g_AppState && g_AppState->coreEngine)
            g_AppState->coreEngine->Post<CoreFramework::CharacterKeyMessage>(static_cast<char>(c), true);
	});

    glfwSetMouseButtonCallback(app.window, [](GLFWwindow* win, int button, int action, int mods)
    {
		(void)mods, (void)win;    // suppress unused parameter warning
        if (g_AppState && g_AppState->coreEngine)
        {
            double x, y;
            glfwGetCursorPos(g_AppState->window, &x, &y);
            g_AppState->coreEngine->Post<CoreFramework::MouseButtonMessage>(button, action == GLFW_PRESS, x, y);
        }
    });

    glfwSetCursorPosCallback(app.window, [](GLFWwindow* win, double xpos, double ypos)
    {
		(void)win; // suppress unused parameter warning
		
		if (g_AppState && g_AppState->coreEngine)
		{
			double dx = 0.0;
			double dy = 0.0;
			
			if (g_AppState->mouseInitialized)
			{
				dx = xpos - g_AppState->lastMouseX;
				dy = ypos - g_AppState->lastMouseY;
			}
			else
			{
				g_AppState->mouseInitialized = true;
			}
			
			g_AppState->lastMouseX = xpos;
			g_AppState->lastMouseY = ypos;
			
			g_AppState->coreEngine->Post<CoreFramework::MouseMoveMessage>(xpos, ypos, dx, dy);
		}
	});

    // Ensure we don't have any other callbacks set
    glfwSetKeyCallback(app.window, nullptr);
    glfwSetScrollCallback(app.window, nullptr);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD\n";
		return false;
	}

	// Create CoreEngine with smart pointer
	app.coreEngine = std::make_unique<CoreFramework::CoreEngine>();
	
	// Add systems using unique_ptr
    app.coreEngine->AddSystem(std::make_unique<AudioManager>());
    app.coreEngine->AddSystem(std::make_unique<Framework::GameStateManager>());

	app.coreEngine->Initialize();
	
	// Initialize ResourceManager with AudioManager
	if (auto* audioMgr = app.coreEngine->GetSystem<AudioManager>())
	{
		ResourceManager::Instance().SetAudioManager(audioMgr);
		std::cout << "ResourceManager initialized with AudioManager." << std::endl;
		
		// Load all audio assets centrally using AudioCatalog
		Audio::AudioCatalog::LoadAllAudio();
	}
	else
	{
		std::cerr << "Warning: AudioManager not found in CoreEngine for ResourceManager!" << std::endl;
	}
	
	// Create GraphicsEngine with smart pointer
	app.graphicsEngine = std::make_unique<GraphicsEngine>();
	app.graphicsEngine->Initialize();
	
	// Create Scene with smart pointer
	app.currentScene = std::make_unique<Scene>(*app.graphicsEngine);
	app.currentScene->LoadScene("LoadTest");

	// Create DebuggerApp with smart pointer
	app.debugApp = std::make_unique<Debug::DebuggerApp>();
    if (!app.debugApp->InitializeDebuggerApp(app.window, app.coreEngine.get()))
    {
        std::cerr << "Failed to initialize DebuggerApp\n";
        return false;
    }
    else
    {
        app.debugApp->AddDebugLine("DebuggerApp initialized successfully\n");
    }

	return true;
}

static void update(ApplicationState& app) {

	// Calculate delta time
	float currentFrame = static_cast<float>(glfwGetTime());
	float deltaTime = currentFrame - app.lastFrame;
	app.lastFrame = currentFrame;

	glfwPollEvents();

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

    app.graphicsEngine->BeginFrame();
    app.currentScene->DrawUI();
	drawList.clear();
	app.currentScene->CollectRenderablePointers(drawList);

    if (app.debugApp->IsActive())
    {
        app.debugApp->RenderDebuggerApp();
    }

    app.graphicsEngine->Render(drawList);

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
    if (app.window)
    {
        std::cout << "Clearing GLFW callbacks..." << std::endl;
        glfwSetWindowCloseCallback(app.window, nullptr);
        glfwSetCharCallback(app.window, nullptr);
        glfwSetMouseButtonCallback(app.window, nullptr);
        glfwSetCursorPosCallback(app.window, nullptr);
        glfwSetKeyCallback(app.window, nullptr);
        glfwSetErrorCallback(nullptr);
        
        // Poll events one last time to clear any pending callbacks
        glfwPollEvents();
    }

    // STEP 2: Clear global app state pointer to prevent callback access
    g_AppState = nullptr;

    // STEP 3: Flush CoreEngine messages to prevent orphaned messages
    if (app.coreEngine)
    {
        std::cout << "Flushing remaining messages..." << std::endl;
        app.coreEngine->FlushMessages();
    }

    // STEP 4: Shutdown ImGui (must happen while OpenGL context is valid)
    if (app.debugApp)
    {
        std::cout << "Shutting down debugger..." << std::endl;
        app.debugApp->Shutdown();
        app.debugApp.reset();
    }

    // STEP 5: Clean up scene objects
    if (app.currentScene)
    {
        std::cout << "Deleting scene..." << std::endl;
        app.currentScene.reset();
    }

    // STEP 6: Stop and shutdown audio
    if (app.coreEngine)
    {
        if (auto* audioMgr = app.coreEngine->GetSystem<AudioManager>())
        {
            std::cout << "Stopping all sounds..." << std::endl;
            audioMgr->StopAllSounds();
            std::cout << "Shutting down audio..." << std::endl;
            audioMgr->Shutdown();
        }
    }
    
    // STEP 6.5: Unload all audio assets
    std::cout << "Unloading audio assets..." << std::endl;
    Audio::AudioCatalog::UnloadAllAudio();

    // STEP 7: Shutdown graphics engine (clears background object)
    if (app.graphicsEngine)
    {
        std::cout << "Shutting down graphics engine..." << std::endl;
        app.graphicsEngine->Shutdown();
        app.graphicsEngine.reset();
    }

    // STEP 8: Clear resource manager (while context still valid)
    std::cout << "Clearing resource manager..." << std::endl;
    ResourceManager::Instance().Clear();

    // STEP 9: Destroy CoreEngine and all systems
    if (app.coreEngine)
    {
        std::cout << "Destroying core engine..." << std::endl;
        app.coreEngine.reset();
    }

    // STEP 10: Make the context non-current before destroying window
    if (app.window)
    {
        glfwMakeContextCurrent(nullptr);
    }

    // STEP 11: Destroy window
    if (app.window)
    {
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

#include "Core/ImGuiDebugger.hpp"
#include "Core/Precompiled.hpp"

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

// Custom allocator hook to suppress automatic leak reports
static _CRT_ALLOC_HOOK g_pfnOldCrtAllocHook = nullptr;

static int MemoryAllocHook(int nAllocType, void* pvData, size_t nSize, int nBlockUse,
                           long lRequest, const unsigned char* szFileName, int nLine)
{
    (void)nAllocType; (void)pvData; (void)nSize; (void)nBlockUse;
    (void)lRequest; (void)szFileName; (void)nLine; // suppress unused warnings
    
    // Allow all allocations/deallocations to proceed normally
    return 1; // TRUE
}

#define DBG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DBG_NEW
#endif

#include <iostream>
#include <algorithm>
#include <csignal>

#include "Graphics/GraphicsEngine.hpp"
#include "Graphics/SceneManager.hpp"
#include "Graphics/ResourceManager.hpp"
#include "Core/Core.hpp"
#include "Core/ConfigManager.hpp"
#include "Core/AudioManager.hpp"
#include "Core/GameStateManager.hpp"
#include "Core/TileMap.hpp"

static void draw();
static void update();
static bool init(GLint width, GLint height, std::string title, bool fullscreen);
static void cleanup();
static void signalHandler(int signal);

static AudioManager audioManager;
static Framework::GameStateManager GSM;
static GraphicsEngine engine;
static Scene* currentScene = nullptr;
static GLFWwindow* window = nullptr;
static float lastFrame = 0.0f;
static float smoothedDt = 0.0f; // smoothed delta time for fps calc
static volatile bool shouldExit = false; // flag for graceful shutdown

static CoreFramework::CoreEngine coreEngine;
CoreFramework::CoreEngine* CoreFramework::CORE = &coreEngine; // Set the global CORE pointer

static Debug::DebuggerApp debugapp;

// Signal handler for Ctrl+C, Ctrl+Break, and console close
static void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", cleaning up..." << std::endl;
    shouldExit = true;
    
    // Set the window to close if it exists
    if (window) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
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
            shouldExit = true;
            if (window) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            cleanup();
            return TRUE;
        default:
            return FALSE;
    }
}
#endif

int main() {

#ifdef _DEBUG
    // Install our allocation hook first to intercept CRT operations
    g_pfnOldCrtAllocHook = _CrtSetAllocHook(MemoryAllocHook);
    
    // Disable ALL automatic leak reporting
    int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmpFlag &= ~_CRTDBG_LEAK_CHECK_DF;  // Clear the leak check bit
    tmpFlag |= _CRTDBG_ALLOC_MEM_DF;    // Keep memory tracking
    _CrtSetDbgFlag(tmpFlag);
    
    // Disable all CRT report types
    _CrtSetReportMode(_CRT_WARN, 0);
    _CrtSetReportMode(_CRT_ERROR, 0);
    _CrtSetReportMode(_CRT_ASSERT, 0);
    
    // Create a memory state checkpoint AFTER static objects are initialized
    _CrtMemState memStateStart;
    _CrtMemCheckpoint(&memStateStart);
#endif

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

    if (!init(settings.resolution.width, settings.resolution.height, "TheStove", settings.fullscreen)) {
        cleanup();
        return -1;
    }

    if (auto* audioMgr = coreEngine.GetSystem<AudioManager>())
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

	lastFrame = static_cast<float>(glfwGetTime());

	while (!glfwWindowShouldClose(window) && !shouldExit) {

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
                debugapp.LogError("Test Case : could not open file : " + filename);
            }
            throw std::runtime_error("Unknown file could not be opened.");*/


            //debugapp.RunDebuggerApp();
        }
        catch (const std::exception& e)
        {
            //DebuggerApp tmpDebugger; // for logging crashes
            debugapp.LogError(std::string("Unhandled exception: ") + e.what());
            std::cerr << "Error: " << e.what() << std::endl;
            cleanup();
            return -1;
        }
        catch (...) // Catches all other exceptions not caught by the first
        {
            //DebuggerApp tmpDebugger; // for logging crashes
            debugapp.LogError("Unknown crash occurred");
            std::cerr << "Crash: Unknown exception\n";
            cleanup();
            return -1;
        }

        update();

		draw();
	}

	cleanup();

#ifdef _DEBUG
    // Create a memory state checkpoint AFTER cleanup
    _CrtMemState memStateEnd, memStateDiff;
    _CrtMemCheckpoint(&memStateEnd);
    
    std::cout << "\n=== Memory Leak Report ===" << std::endl;
    // Compare the two memory states to find only the leaks between checkpoints
    if (_CrtMemDifference(&memStateDiff, &memStateStart, &memStateEnd)) {
        std::cout << "Memory leaks detected between checkpoints!" << std::endl;
        std::cout << "Dumping leak statistics:" << std::endl;
        _CrtMemDumpStatistics(&memStateDiff);
    } else {
        std::cout << "No memory leaks detected." << std::endl;
    }
    
    // Restore the original allocation hook
    _CrtSetAllocHook(g_pfnOldCrtAllocHook);
#endif

	return 0;
}

static bool init(GLint width, GLint height, std::string title, bool fullscreen) {
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

	window = glfwCreateWindow(width, height, title.c_str(), monitor, nullptr);
	if (!window) {
		std::cerr << "Failed to create window" << std::endl;
		glfwTerminate();
		window = nullptr;
		return false;
	}
	glfwMakeContextCurrent(window);

    // Add window close callback to trigger cleanup
    glfwSetWindowCloseCallback(window, [](GLFWwindow* win) {
		(void)win; // suppress unused parameter warning
        std::cout << "Window close requested, cleaning up..." << std::endl;
        shouldExit = true;
    });

	// Message callbacks to post input events to CoreEngine
    glfwSetCharCallback(window, [](GLFWwindow* win, unsigned int c) 
    {
		(void)window, (void)win;   // suppress unused parameter warning
        if (CoreFramework::CORE)
            CoreFramework::CORE->Post<CoreFramework::CharacterKeyMessage>(static_cast<char>(c), true);
	});

    glfwSetMouseButtonCallback(window, [](GLFWwindow* win, int button, int action, int mods)
    {
		(void)mods, (void)win;    // suppress unused parameter warning
        if (CoreFramework::CORE)
        {
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            CoreFramework::CORE->Post<CoreFramework::MouseButtonMessage>(button, action == GLFW_PRESS, x, y);
        }
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow* win, double xpos, double ypos)
    {
		(void)win; // suppress unused parameter warning
		static double lastX = xpos;
		static double lastY = ypos;
		double dx = xpos - lastX;
		double dy = ypos - lastY;
		lastX = xpos;
        lastY = ypos;

			if (CoreFramework::CORE)
			{
				CoreFramework::CORE->Post<CoreFramework::MouseMoveMessage>(xpos, ypos, dx, dy);
			}
		});

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD\n";
		return false;
	}

    coreEngine.AddSystem(&audioManager);
    coreEngine.AddSystem(&GSM);

	coreEngine.Initialize();
	engine.Initialize();
	currentScene = new Scene(engine);
	currentScene->LoadScene("LoadTest");

    if (!debugapp.InitializeDebuggerApp(window))
    {
        std::cerr << "Failed to initialize DebuggerApp\n";
        return false;
    }
    else
    {
        debugapp.AddDebugLine("DebuggerApp initialized successfully\n");
    }

	return true;
}

static void update() {

	// Calculate delta time
	float currentFrame = static_cast<float>(glfwGetTime());
	float deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	glfwPollEvents();

	// Update scene with delta time and window pointer
	currentScene->Update(deltaTime, window);


    // Smoothing for deltatime (for the fps)
    // Account for division by 0 on the first frame where gDt = 0
    // This controls how fast the fps counter reacts to changes
    // (higher value = smoother fps) else 
    // (lower value = faster fps change response but more jittery)
    smoothedDt = (smoothedDt == 0.0f) ? deltaTime : (0.96f * smoothedDt) + (0.04f * deltaTime);

	// Update FPS display variables for DebuggerApp
	debugapp.fps = (smoothedDt > 0.f) ? (1.f / smoothedDt + 0.5f) : 0.f;
	debugapp.msperFrame = (smoothedDt * 1000.0f);

    coreEngine.GameLoop();

    if (debugapp.IsActive())
    {
        debugapp.UpdateDebuggerApp();
    }
}

static void draw() {
	std::vector<GameObject*> drawList;

    engine.BeginFrame();
	drawList.clear();
	currentScene->CollectRenderablePointers(drawList);
    engine.Render(drawList);

    if (debugapp.IsActive())
    {
        debugapp.RenderDebuggerApp();
    }

    glfwSwapBuffers(window);
    
}

void cleanup() {
    static bool cleanupCalled = false;
    
    // Prevent multiple cleanup calls
    if (cleanupCalled) {
        return;
    }
    cleanupCalled = true;

    std::cout << "Starting cleanup..." << std::endl;

    // Clean up scene first
    if (currentScene)
    {
        std::cout << "Deleting scene..." << std::endl;
        delete currentScene;
		currentScene = nullptr;
    }

    // Stop and shutdown audio
    if (auto* audioMgr = coreEngine.GetSystem<AudioManager>())
    {
        std::cout << "Stopping all sounds..." << std::endl;
		audioMgr->StopAllSounds();
        std::cout << "Shutting down audio..." << std::endl;
        audioMgr->Shutdown();
	}

    // Shutdown graphics engine
    std::cout << "Shutting down graphics engine..." << std::endl;
    engine.Shutdown();

    // Clear resource manager
    std::cout << "Clearing resource manager..." << std::endl;
	ResourceManager::Instance().Clear();

    // Shutdown debugger
    std::cout << "Shutting down debugger..." << std::endl;
	debugapp.Shutdown();

    // Destroy window
    if (window)
    {
        std::cout << "Destroying window..." << std::endl;
        glfwDestroyWindow(window);
        window = nullptr;
	}

    // Terminate GLFW
    std::cout << "Terminating GLFW..." << std::endl;
	glfwTerminate();

    std::cout << "Cleanup complete." << std::endl;
}

#include "Core/ImGuiDebugger.hpp"
#include "Core/Precompiled.hpp"

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#define DBG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DBG_NEW
#endif

#include <iostream>
#include <algorithm>

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

static AudioManager audioManager;
static Framework::GameStateManager GSM;
static GraphicsEngine engine;
static Scene* currentScene = nullptr;
static GLFWwindow* window = nullptr;
static float lastFrame = 0.0f;
static float smoothedDt = 0.0f; // smoothed delta time for fps calc

static CoreFramework::CoreEngine coreEngine;
CoreFramework::CoreEngine* CoreFramework::CORE = &coreEngine; // Set the global CORE pointer

static Debug::DebuggerApp debugapp;

int main() {

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
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

	while (!glfwWindowShouldClose(window)) {

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
			return -1;
		}
		catch (...) // Catches all other exceptions not caught by the first
		{
			//DebuggerApp tmpDebugger; // for logging crashes
			debugapp.LogError("Unknown crash occurred");
			std::cerr << "Crash: Unknown exception\n";
			return -1;
		}

		update();

		draw();
	}

	cleanup();

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

	// engine.BeginImGuiFrame();

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

	/*if (debugapp.IsActive()) {
		debugapp.UpdateDebuggerApp();
	}*/
}

static void draw() {
	std::vector<GameObject*> drawList;

	engine.BeginFrame();

	currentScene->DrawUI();

	if (debugapp.IsActive()) {
		debugapp.RenderDebuggerApp();
	}

	drawList.clear();
	currentScene->CollectRenderablePointers(drawList);
	engine.Render(drawList);

	glfwSwapBuffers(window);
}

void cleanup() {
	if (currentScene)
	{
		delete currentScene;
		currentScene = nullptr;
	}

	if (auto* audioMgr = coreEngine.GetSystem<AudioManager>())
	{
		audioMgr->StopAllSounds();
		audioMgr->Shutdown();
	}

	engine.Shutdown();
	ResourceManager::Instance().Clear();
	debugapp.Shutdown();

	if (window)
	{
		glfwDestroyWindow(window);
		window = nullptr;
	}

	glfwTerminate();

}

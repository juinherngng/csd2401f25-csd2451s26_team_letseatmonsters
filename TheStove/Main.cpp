#include <iostream>
#include <crtdbg.h>
#include <algorithm>

#include "Graphics/GraphicsEngine.h"
#include "Graphics/SceneManager.h"
#include "Graphics/ResourceManager.h"
#include "Core/ImGuiDebugger.hpp"
#include "Core/Precompiled.hpp"
#include "Core/Core.hpp"
#include "Core/ConfigManager.hpp"
#include "Core/AudioManager.hpp"
#include "Core/GameStateManager.hpp"
#include "Core/TileMap.hpp"

#define _CRTDBG_MAP_ALLOC
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)

static void draw();
static void update();
static bool init(GLint width, GLint height, std::string title, bool fullscreen);
static void cleanup();

static GraphicsEngine engine;
static Scene* currentScene = nullptr;
static GLFWwindow* window = nullptr;
static float lastFrame = 0.0f;
static float smoothedDt = 0.0f; // smoothed delta time for fps calc

static CoreFramework::CoreEngine coreEngine;
CoreFramework::CoreEngine* CoreFramework::CORE = &coreEngine; // Set the global CORE pointer

static DebuggerApp debugapp;

static void CheckMemoryLeaks()
{
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);

    _CrtDumpMemoryLeaks();  // check for mem leaks
}

int main() {

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    auto settings = ConfigManager::LoadFromAssetsOrDefaults();
	ConfigManager::Validate(settings);

    if (!init(settings.resolution.width, settings.resolution.height, "TheStove", settings.fullscreen)) {
        cleanup();
        CheckMemoryLeaks();
        return -1;
    }
    
    if (auto* audioMgr = coreEngine.GetSystem<AudioManager>())
    {
        audioMgr->ApplySettings(settings);
        float bgm = audioMgr->GetBgmVolume();
        float vfx = audioMgr->GetVfxVolume();
        std::cout << "AudioManager system found in CoreEngine - BGM Volume: " << bgm << ", VFX Volume: " << vfx << "\n";

        audioMgr->PlaySound("boiling sound", bgm, false);
        std::cout << "Playing 'boiling sound'\n";
    }
    else
    {
        std::cerr << "AudioManager system not found in CoreEngine\n";
    }

    // test tile map
    MapData testMap(6, 6);

    for (int i = 0; i < testMap.getHeight(); i++) {
        testMap.setTile(i, i, 1);
    }

    testMap.printMap();

    std::cout << "There are " << testMap.SweepFor(ENTITY) << " Entities on the Map" << std::endl;

	lastFrame = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window)) {

        update();

        draw();
    }

    cleanup();

	CheckMemoryLeaks();

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

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }

    coreEngine.AddSystem(new AudioManager());
    coreEngine.AddSystem(new Framework::GameStateManager());

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
		std::cout << "DebuggerApp initialized successfully\n";
    }

	// Test loading an audio file

	auto& rm = ResourceManager::Instance();
	auto* snd = rm.LoadAudio("boiling sound", "../assets/Audio/Boiling7.wav", true, true);
    if (!snd)
    {
		std::cerr << "Failed to load audio 'boiling sound'\n";
    }
    else
    {
		unsigned int lenMs = 0;
        int ch = 0, bits = 0;
        float freq = 0;
        if (rm.GetAudioInfo("boiling sound", lenMs, ch, bits, freq))
        {
            std::cout << "Audio 'boiling sound' info - Length: " << lenMs << " ms, Channels: " << ch << ", Bits: " << bits << ", Frequency: " << freq << " Hz\n";
        }
        else
        {
            std::cerr << "Failed to get audio info for 'boiling sound'\n";
        }
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


    // Smoothing for gDt (for the fps)
    // Account for division by 0 on the first frame where gDt = 0
    // This controls how fast the fps counter reacts to changes
    // (higher value = smoother fps) else 
    // (lower value = faster fps change response but more jittery)
    smoothedDt = (smoothedDt == 0.0f) ? CoreFramework::gDt : (0.96f * smoothedDt) + (0.04f * CoreFramework::gDt);

    // Update FPS display variables for DebuggerApp
    debugapp.fps = (smoothedDt > 0.f) ? (1.f / smoothedDt + 0.5f) : 0.f;
    debugapp.msperFrame = (smoothedDt * 1000.0f);

    coreEngine.GameLoop(debugapp);
    debugapp.UpdateDebuggerApp();
}

static void draw() {
    engine.BeginFrame();
    engine.Render();

    debugapp.RenderDebuggerApp();
    glfwSwapBuffers(window);
    
}

void cleanup() {
    engine.Shutdown();
    coreEngine.DestroySystems();
    if (currentScene)
    {
        delete currentScene;
		currentScene = nullptr;
    }

    if (window)
    {
        glfwDestroyWindow(window);
        window = nullptr;
    }

    glfwTerminate();

}

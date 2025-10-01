#include <iostream>
#include <crtdbg.h>

#include "Graphics/GraphicsEngine.h"
#include "Graphics/SceneManager.h"
#include "Core/ImGuiDebugger.hpp"
#include "Core/Precompiled.hpp"
#include "Core/Core.hpp"
#include "Core/ConfigManager.hpp"
#include "Core/AudioManager.hpp"
#include "Core/GameStateManager.hpp"
#include "Core/TileMap.hpp"

#define _CRTDBG_MAP_ALLOC

static void draw();
static void update();
static void init(GLint width, GLint height, std::string title);
static void cleanup();

static GraphicsEngine engine;
static Scene* currentScene;
static GLFWwindow* window;
static float lastFrame = 0.0f;

static CoreFramework::CoreEngine coreEngine;
static CoreFramework::CoreEngine* CoreFramework::CORE = &coreEngine; // Set the global CORE pointer

static DebuggerApp debugapp;

void CheckMemoryLeaks()
{
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);

    _CrtDumpMemoryLeaks();  // check for mem leaks
}

int main() {

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    coreEngine.AddSystem(new AudioManager());
    coreEngine.AddSystem(new Framework::GameStateManager());
    
    init(1200, 800, "TheStove");

    // test tile map
    MapData testMap(6, 6);

    for (int i = 0; i < testMap.getHeight(); i++) {
        testMap.setTile(i, i, 1);
    }

    testMap.printMap();

    std::cout << "There are " << testMap.SweepFor(ENTITY) << " Entities on the Map" << std::endl;

    while (!glfwWindowShouldClose(window)) {

        update();

        draw();
    }

    cleanup();

	CheckMemoryLeaks();

    return 0;
}

static void init(GLint width, GLint height, std::string title) {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW" << std::endl;
        exit(-1);
    }

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        exit(-1);
    }

    coreEngine.Initialize();
    engine.Initialize();
    currentScene = new Scene(engine);
    currentScene->LoadScene("LoadTest");

    if (!debugapp.InitializeDebuggerApp(window))
    {
        std::cerr << "Failed to initialize DebuggerApp\n";
        exit(-1);
    }
}

static void update() {

    // Calculate delta time
    float currentFrame = static_cast<float>(glfwGetTime());
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    glfwPollEvents();

    // Update scene with delta time and window pointer
    currentScene->Update(deltaTime, window);

    // Update FPS display variables for DebuggerApp
    debugapp.msperFrame = deltaTime * 1000.0f;
    debugapp.fps = 1.0f / deltaTime;

    // coreEngine.GameLoop(debugapp);
    debugapp.UpdateDebuggerApp();
}

static void draw() {
    engine.BeginFrame();
    engine.Render();

    debugapp.RenderDebuggerApp();
    glfwSwapBuffers(window);
    
}

void cleanup() {

    debugapp.~DebuggerApp();

    engine.Shutdown();
    coreEngine.DestroySystems();
    delete currentScene;

    glfwDestroyWindow(window);
    glfwTerminate();

}

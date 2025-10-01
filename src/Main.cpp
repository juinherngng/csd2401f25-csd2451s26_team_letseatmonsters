#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>


#include "Graphics/GraphicsEngine.h"
#include "Graphics/SceneManager.h"
#include "ImGuiDebugger.hpp"
#include "Precompiled.hpp"
#include "Core.hpp"
#include "ConfigManager.hpp"

static void draw();
static void update();
static void init(GLint width, GLint height, std::string title);
static void cleanup();

static GraphicsEngine engine;
static Scene* currentScene;
static GLFWwindow* window;
static float lastFrame = 0.0f;
static MockSystem* mockSystem = nullptr;

class MockSystem : public CoreFramework::SystemInterface
{
public:
    void Initialize() override {
        std::cout << "MockSystem initialized." << std::endl;
    }

    void Update(float timeSlice) override {
        std::cout << "System updated with dt = " << timeSlice << std::endl;

        static int count = 0;
        if (++count > 30) {
            auto quitMsg = new CoreFramework::Message(CoreFramework::MsgId::QUIT);
            std::cout << "MockSystem sent QUIT message." << std::endl;
            CoreFramework::CORE->BroadcastMessage(quitMsg);
            delete quitMsg;
        }
    }
    void SendMessage(CoreFramework::Message*) override {}
    std::string GetName() override { return "MockSystem"; }
};

int main() {

	CoreFramework::CoreEngine engine;
	CoreFramework::CORE = &engine; // Set the global CORE pointer
	DebuggerApp debugapp; // Watashi no debugger

	/ add test system
	//engine.AddSystem(new MockSystem());
    
    init(1200, 800, "TheStove");

    while (!glfwWindowShouldClose(window)) {

        update();

        draw();
    }

    cleanup();

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

    engine.Initialize();
    currentScene = new Scene(engine);
    currentScene->LoadScene("LoadTest");

	mockSystem = new MockSystem();
    mockSystem->Initialize();
}

static void update() {

    // Calculate delta time
    float currentFrame = static_cast<float>(glfwGetTime());
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    glfwPollEvents();

    // Update scene with delta time and window pointer
    currentScene->Update(deltaTime, window);

	if (mockSystem) {
        mockSystem->Update(deltaTime);
    }
}

static void draw() {
    engine.BeginFrame();
    engine.Render();
    glfwSwapBuffers(window);
}

void cleanup() {
    engine.Shutdown();
    delete currentScene;
	delete mockSystem;
    glfwDestroyWindow(window);
    glfwTerminate();

}

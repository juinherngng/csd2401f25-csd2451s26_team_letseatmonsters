#include "Graphics/GraphicsEngine.h"
#include "Graphics/SceneManager.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

static void draw();
static void update();
static void init(GLint width, GLint height, std::string title);
static void cleanup();

static GraphicsEngine engine;
static Scene* currentScene;
static GLFWwindow* window;
static float lastFrame = 0.0f;

int main() {
    
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
}

static void update() {

    // Calculate delta time
    float currentFrame = static_cast<float>(glfwGetTime());
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    glfwPollEvents();

    // Update scene with delta time and window pointer
    currentScene->Update(deltaTime, window);
}

static void draw() {
    engine.BeginFrame();
    engine.Render();
    glfwSwapBuffers(window);
}

void cleanup() {
    engine.Shutdown();
    delete currentScene;
    glfwDestroyWindow(window);
    glfwTerminate();

}
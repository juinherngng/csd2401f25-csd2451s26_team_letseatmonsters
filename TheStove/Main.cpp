#include "Graphics/GraphicsEngine.h"
#include "Graphics/SceneManager.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

int main() {
    // Initialize GLFW
    if (!glfwInit())
        return -1;
    
    GLFWwindow* window = glfwCreateWindow(1200, 800, "TheStove", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    GraphicsEngine engine;
    engine.Initialize();
    
    Scene currentScene(engine);
	currentScene.LoadScene("LoadTest");

    float lastFrame = 0.0f;
    float currentFrame = 0.0f;
    float deltaTime = 0.0f;

    while (!glfwWindowShouldClose(window)) {

        // Calculate delta time
        currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

       // currentScene.Update(deltaTime);

        engine.BeginFrame();  
        engine.Render();  

        glfwSwapBuffers(window);
    }

    engine.Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

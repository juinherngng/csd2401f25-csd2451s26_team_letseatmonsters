#include "Graphics/Renderer.h"
#include "Graphics/Shader.h"
#include "Graphics/Mesh.h"
#include <Graphics/GameObject.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

int main() {
    // Initialize GLFW
    if (!glfwInit())
        return -1;
    GLFWwindow* window = glfwCreateWindow(800, 600, "TheStove", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    Renderer renderer;

    // Triangle vertex data: positions and colors
    float vertices[] = {
        // positions        // colors
         0.0f,  0.5f, 0.0f,  1, 0, 0,
         0.5f, -0.5f, 0.0f,  0, 1, 0,
        -0.5f, -0.5f, 0.0f,  0, 0, 1
    };

    Mesh triangleMesh(vertices, 3, 6 * sizeof(float));
    Shader shader("../TheStove/Graphics/shaders/shader.vert", "../TheStove/Graphics/shaders/shader.frag");

    GameObject obj(&triangleMesh, &shader);

    // Orthographic view projection setup for 2D screen coordinates
    glm::mat4 projection = glm::ortho(0.0f, 800.f, 0.0f, 600.f);
    glm::mat4 view = glm::mat4(1.0f);

    // ImGui-controlled transform variables
    static float position[3] = { 0.f, 0.f, 0.f };
    static float scale[3] = { 1.f, 1.f, 1.f };
    static float rotationDegrees = 0.f;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui control panel
        ImGui::Begin("Transform Controls");
        ImGui::SliderFloat3("Position", position, 0.f, 100.f);
        ImGui::SliderFloat3("Scale", scale, 0.1f, 5.f);
        ImGui::SliderFloat("Rotation (degrees)", &rotationDegrees, 0.f, 360.f);
        ImGui::End();

        // Update GameObject transforms
        obj.SetPosition(glm::vec3(position[0], position[1], position[2]));
        obj.SetScale(glm::vec3(scale[0], scale[1], scale[2]));
        obj.SetRotation(glm::radians(rotationDegrees), glm::vec3(0, 0, 1));

        renderer.Clear();

        // Draw the object
        obj.Draw(view, projection);

        // Render ImGui UI
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup ImGui and GLFW
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

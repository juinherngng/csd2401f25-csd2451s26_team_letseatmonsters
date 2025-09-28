/**
 * @file   Main.cpp
 * @author
 * @date   19 Sep 2025
 * @brief  Entry point for the OpenGL application with ImGui overlay.
 *
 * This file demonstrates:
 * - Creating an OpenGL 3.3 context using GLFW
 * - Loading shaders and rendering a colored quad (Cube class)
 * - Integrating ImGui for UI overlays
 * - Reading settings from ConfigManager to configure resolution/fullscreen
 */

#include <shader.h>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

#include "Precompiled.hpp"
#include "Core.hpp"
#include "ConfigManager.hpp"

 /**
  * @class Cube
  * @brief Simple drawable 2D square made of two triangles with per-vertex colors.
  */
class Cube {
public:
	Cube() : m_programId(0), m_vertexBuffer(0), m_vertexArrayId(0) {}
	~Cube() {
		if (m_vertexBuffer)  glDeleteBuffers(1, &m_vertexBuffer);
		if (m_vertexArrayId) glDeleteVertexArrays(1, &m_vertexArrayId);
		if (m_programId)     glDeleteProgram(m_programId);
	}

	void initialize() {
		static auto vs = R"(
            #version 330 core

            layout(location = 0) in vec3 aPosition;
            layout(location = 1) in vec3 aColor;

            uniform mat4 vertexTransform;

            out vec3 vColor;

            void main() {
                gl_Position = vertexTransform * vec4(aPosition, 1.0);
                vColor = aColor;
            }
        )";

		static auto fs = R"(
            #version 330 core

            in vec3 vColor;
            out vec4 FragColor;

            void main() {
                FragColor = vec4(vColor, 1.0);
            }
        )";

		m_programId = LoadShaders(vs, fs, !true);
		m_geometryBuffer = {
			//  x      y     z      r     g     b
			-0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 1.0f,   // Bottom-left, cyan
			 0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 1.0f,   // Bottom-right, magenta
			 0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   // Top-right, yellow

			 0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   // Top-right (repeat), red
			-0.5f,  0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   // Top-left, green
			-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f    // Bottom-left (repeat), blue
		};

		glGenVertexArrays(1, &m_vertexArrayId);
		glBindVertexArray(m_vertexArrayId);

		glGenBuffers(1, &m_vertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, m_geometryBuffer.size() * sizeof(GLfloat), m_geometryBuffer.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
	}

	void render(const glm::mat4& p_modelMat) {
		glUseProgram(m_programId);
		glBindVertexArray(m_vertexArrayId);
		GLint vTransformLoc = glGetUniformLocation(m_programId, "vertexTransform");
		glUniformMatrix4fv(vTransformLoc, 1, GL_FALSE, &p_modelMat[0][0]);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

private:
	GLuint m_programId, m_vertexBuffer, m_vertexArrayId;
	std::vector<GLfloat> m_geometryBuffer;
};

/**
 * @class GLApp
 * @brief Manages the application lifecycle: window, rendering, input, and ImGui.
 */
class GLApp {
public:
	GLApp(int p_width, int p_height, const char* p_title, bool fullscreen = false)
		: m_angleOfRotation(0.0f),
		m_rotationSpeed(0.01f),
		m_window(nullptr),
		m_cube(std::make_shared<Cube>())
	{
		if (!glfwInit())
			throw std::runtime_error("GLFW initialization failed");

		glfwSetErrorCallback([](int error, const char* description) {
			fprintf(stderr, "GLFW Error %d: %s\n", error, description);
			});

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

		GLFWmonitor* monitor = nullptr;
		int width = p_width;
		int height = p_height;

		// m_window = glfwCreateWindow(width, height, p_title, monitor, nullptr);

		if (fullscreen) {
			monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);

			width = mode->width;
			height = mode->height;

			glfwWindowHint(GLFW_RED_BITS, mode->redBits);
			glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

			m_window = glfwCreateWindow(width, height, p_title, monitor, nullptr);
		}
		else {
			// Normal windowed
			m_window = glfwCreateWindow(width, height, p_title, nullptr, nullptr);
			if (m_window) {
				// center on primary monitor
				GLFWmonitor* prim = glfwGetPrimaryMonitor();
				const GLFWvidmode* mode = glfwGetVideoMode(prim);
				int x = (mode->width - width) / 2;
				int y = (mode->height - height) / 2;
				glfwSetWindowPos(m_window, x, y);
			}
		}

		if (!m_window) {
			glfwTerminate();
			throw std::runtime_error("Window creation failed");
		}

		glfwMakeContextCurrent(m_window);
		glfwSwapInterval(1);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			glfwDestroyWindow(m_window);
			glfwTerminate();
			throw std::runtime_error("Failed to initialize GLAD");
		}

		// Now it's safe to use OpenGL functions
		int fbw = 0, fbh = 0;
		glfwGetFramebufferSize(m_window, &fbw, &fbh);
		if (fbw <= 0 || fbh <= 0) { fbw = width; fbh = height; }  // fallback for minimized/0 dpi
		glViewport(0, 0, fbw, fbh);

		// Keep viewport in sync on resize/DPI changes
		glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow*, int w, int h) {
			if (w <= 0 || h <= 0) return;
			glViewport(0, 0, w, h);
			});

		// ---- GLAD initialization instead of GLEW ----
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			glfwTerminate();
			throw std::runtime_error("Failed to initialize GLAD");
		}
		// ---------------------------------------------

		glEnable(GL_DEPTH_TEST);
		glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GLFW_TRUE);
		m_cube->initialize();

		// Setup ImGui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForOpenGL(m_window, true);
		ImGui_ImplOpenGL3_Init("#version 330");
	}

	~GLApp() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		// Ensure a context is current while freeing GL resources
		if (m_window) glfwMakeContextCurrent(m_window);

		// Destroy GL resources BEFORE terminating GLFW
		m_cube.reset();                 // runs ~Cube() now, while context is valid

		if (m_window) {
			glfwDestroyWindow(m_window);
			m_window = nullptr;
		}

		glfwTerminate();
	}

	void update() {
		m_angleOfRotation += m_rotationSpeed;
	}

	void render() {
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 modelMat = glm::mat4(1.0f);
		modelMat = glm::rotate(modelMat, m_angleOfRotation, glm::vec3(0.0f, 0.0f, 1.0f));
		m_cube->render(modelMat);

		// Start ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Create ImGui window
		ImGui::Begin("OpenGL 3.3 Demo");
		ImGui::Text("Hello, ImGui!");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::SliderFloat("Rotation Speed", &m_rotationSpeed, 0.0f, 0.1f);
		if (ImGui::Button("Reset Rotation")) {
			m_angleOfRotation = 0.0f;
		}
		ImGui::End();

		// Render ImGui
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(m_window);
	}

	void run() {
		while (glfwGetKey(m_window, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(m_window)) {
			update();
			render();
			glfwPollEvents();
		}
	}

public:
	float m_angleOfRotation;
	float m_rotationSpeed;

private:
	GLFWwindow* m_window;
	std::shared_ptr<Cube> m_cube;
};

// test system (do not add your systems here)
// see reference in System.hpp
class MockSystem : public CoreFramework::SystemInterface
{
public:
	void Initialize() override {
		std::cout << "MockSystem initialized." << std::endl;
	}

    void Update(float timeSlice) override {
        std::cout << "System updated with dt = " << timeSlice << std::endl;

        static int count = 0;
		// After 30 updates, send a QUIT message to stop the engine
        if (++count > 30) {
            // Create a quit message and broadcast it
            auto quitMsg = new CoreFramework::Message(CoreFramework::MsgId::QUIT);
            std::cout << "MockSystem sent QUIT message." << std::endl;
            CoreFramework::CORE->BroadcastMessage(quitMsg);
            delete quitMsg;
        }
    }
    void SendMessage(CoreFramework::Message*) override {}
    std::string GetName() override { return "MockSystem"; }
};

/**
 * @brief Program entry point.
 * Loads configuration, creates GLApp, and runs the render loop.
 */
int main() {

    CoreFramework::CoreEngine engine;
	CoreFramework::CORE = &engine;

    // add test system
	engine.AddSystem(new MockSystem());

	engine.Initialize();
	engine.GameLoop();
	engine.DestroySystems();

    try {
		auto settings = ConfigManager::LoadFromAssetsOrDefaults();
		ConfigManager::Validate(settings);
        GLApp app(settings.resolution.width,
			settings.resolution.height,
			"Render Cube",
			settings.fullscreen);

		std::cout << "Loaded settings:\n";
		std::cout << "  Resolution: " << settings.resolution.width
			<< "x" << settings.resolution.height << "\n";
		std::cout << "  Fullscreen: " << (settings.fullscreen ? "true" : "false") << "\n";
		std::cout << "  BGM Volume: " << settings.bgmVolume << "\n";
		std::cout << "  VFX Volume: " << settings.vfxVolume << "\n";

        app.run();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}

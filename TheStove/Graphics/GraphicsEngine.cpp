/*
----------------------------------------------------------------------------------------------------
FILE NAME:			GraphicsEngine.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Seah Wang Hua, wanghua.seah@digipen.edu

DESCRIPTION:		Implements initialization, default resource loading, background handling,
					and batched rendering of GameObjects with error checks.

		All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <iostream>
#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "GraphicsEngine.hpp"
#include "MeshLoader.hpp"

static bool s_imguiInitialized = false;

GraphicsEngine& GraphicsEngine::Instance() {
	static GraphicsEngine instance;
	return instance;
}

// Constructor: Initializes references and identity matrices for view/projection.
GraphicsEngine::GraphicsEngine()
	: resourceManager(ResourceManager::Instance()),
	projection(1.0f),
	view(1.0f)
{
}

// Initialize renderer state and default camera/projection.
void GraphicsEngine::Initialize() {
	renderer.Initialize();
	renderer.SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	CreateSceneFBO(1200, 800);
	// Resize(kRefW, kRefH);
	view = glm::mat4(1.0f);

	// Load default resources
	LoadDefaultResources();

	DebugRenderer::Init();
	DebugRenderer::SetEnabled(false);

	if (!s_imguiInitialized) {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGui::StyleColorsDark();
		ImGui_ImplGlfw_InitForOpenGL(glfwGetCurrentContext(), true);
		ImGui_ImplOpenGL3_Init("#version 330 core");
		s_imguiInitialized = true;
	}
}

void GraphicsEngine::DestroySceneFBO() {
	if (mSceneDepth) { glDeleteRenderbuffers(1, &mSceneDepth);  mSceneDepth = 0; }
	if (mSceneColor) { glDeleteTextures(1, &mSceneColor);      mSceneColor = 0; }
	if (mSceneFBO) { glDeleteFramebuffers(1, &mSceneFBO);    mSceneFBO = 0; }
}

void GraphicsEngine::CreateSceneFBO(int w, int h) {
	DestroySceneFBO();

	glGenFramebuffers(1, &mSceneFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, mSceneFBO);

	glGenTextures(1, &mSceneColor);
	glBindTexture(GL_TEXTURE_2D, mSceneColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mSceneColor, 0);

	glGenRenderbuffers(1, &mSceneDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, mSceneDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mSceneDepth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		// handle/log error as you prefer
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	mSceneWidth = w; mSceneHeight = h;
}

void GraphicsEngine::ResizeSceneFBO(int w, int h) {
	if (w <= 0 || h <= 0) return;
	CreateSceneFBO(w, h);
}

void GraphicsEngine::BeginSceneRender() {
	glBindFramebuffer(GL_FRAMEBUFFER, mSceneFBO);
	glViewport(0, 0, mSceneWidth, mSceneHeight);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void GraphicsEngine::EndSceneRender() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


const glm::mat4& GraphicsEngine::GetProjection() const {
	return projection;
}

const glm::mat4& GraphicsEngine::GetView() const {
	return view;
}
void GraphicsEngine::Resize(int width, int height) {
	if (width <= 0 || height <= 0) {
		return;
	}

	screenWidth = width;
	screenHeight = height;

	// Keep the *projection* fixed to reference pixels so sprites don't scale
	// (0,0) top-left, (kRefW,kRefH) bottom-right
	projection = glm::ortho(
		0.0f, static_cast<float>(kRefW),
		static_cast<float>(kRefH), 0.0f,
		-1.0f, 1.0f
	);

	// Compute letterboxed viewport centered in the window
	const float sx = static_cast<float>(width) / static_cast<float>(kRefW);
	const float sy = static_cast<float>(height) / static_cast<float>(kRefH);
	viewportScale_ = std::min(sx, sy);

	viewportW_ = static_cast<int>(kRefW * viewportScale_);
	viewportH_ = static_cast<int>(kRefH * viewportScale_);
	viewportX_ = (width - viewportW_) / 2;
	viewportY_ = (height - viewportH_) / 2;

	// Apply the viewport now
	glViewport(viewportX_, viewportY_, viewportW_, viewportH_);

	// Background should match the reference canvas (it renders in world pixels)
	if (backgroundObject) {
		backgroundObject->SetPosition(glm::vec3(kRefW * 0.5f, kRefH * 0.5f, 0.0f));
		backgroundObject->SetScale(glm::vec3(static_cast<float>(kRefW),
			static_cast<float>(kRefH), 1.0f));
	}
}

void GraphicsEngine::ApplyViewport() const {
	// Centered letterbox area where the game actually renders
	glViewport(viewportX_, viewportY_, viewportW_, viewportH_);
}


// Internal helper to preload common shaders and meshes.
void GraphicsEngine::LoadDefaultResources() {
	// Load default shader
	resourceManager.LoadShader("basic",
		"../TheStove/Graphics/shaders/shader.vert",
		"../TheStove/Graphics/shaders/shader.frag");

	// Load texture shader
	resourceManager.LoadShader("texture",
		"../TheStove/Graphics/shaders/texture.vert",
		"../TheStove/Graphics/shaders/texture.frag");

	// Load sprite shader
	resourceManager.LoadShader("sprite",
		"../TheStove/Graphics/shaders/sprite.vert",
		"../TheStove/Graphics/shaders/sprite.frag");

	// Load static sprite shader
	resourceManager.LoadShader("staticsprite",
		"../TheStove/Graphics/shaders/staticsprite.vert",
		"../TheStove/Graphics/shaders/staticsprite.frag");

	// Load animated sprite shader
	resourceManager.LoadShader("animatedsprite",
		"../TheStove/Graphics/shaders/animatedsprite.vert",
		"../TheStove/Graphics/shaders/animatedsprite.frag");

	// Load triangle mesh
	std::vector<float> vertices;
	GLsizei vertexCount, vertexSize;
	MeshLoader::LoadSimpleTriangle(vertices, vertexCount, vertexSize);
	resourceManager.LoadMesh("triangle", vertices, vertexCount, vertexSize);

	// Load sprite mesh
	MeshLoader::LoadSprite(vertices, vertexCount, vertexSize);
	resourceManager.LoadMesh("sprite", vertices, vertexCount, vertexSize);


	// Load fullscreen quad mesh
	MeshLoader::LoadFullscreenQuad(vertices, vertexCount, vertexSize);
	resourceManager.LoadMesh("fullscreen_quad", vertices, vertexCount, vertexSize);
}

//Sets and update a fullscreen background texture and ensure a background quad exists.
void GraphicsEngine::SetBackground(const std::string& texturePath) {
	// Load background texture
	Texture* bgTexture = resourceManager.LoadTexture("background", texturePath);
	if (!bgTexture) {
		std::cerr << "Failed to load background texture: " << texturePath << std::endl;
		return;
	}

	// Create background object if it doesn't exist
	if (!backgroundObject) {
		Mesh* quadMesh = resourceManager.GetMesh("fullscreen_quad");
		Shader* textureShader = resourceManager.GetShader("texture");

		if (quadMesh && textureShader) {
			backgroundObject = std::make_unique<GameObject>(quadMesh, textureShader);
			// Position background to fill screen
			backgroundObject->SetPosition(glm::vec3(screenWidth * 0.5f, screenHeight * 0.5f, 0.0f));
			backgroundObject->SetScale(glm::vec3(static_cast<float>(screenWidth), static_cast<float>(screenHeight), 1.0f));
		}
	}

	if (backgroundObject) {
		backgroundObject->SetTexture(bgTexture);
	}
}

// Remove the background object (if present).
void GraphicsEngine::ClearBackground() {
	backgroundObject.reset();
}

void GraphicsEngine::BeginImGuiFrame() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGuiViewport* vp = ImGui::GetMainViewport();

	ImGui::DockSpaceOverViewport(
		vp->ID,
		vp,
		ImGuiDockNodeFlags_PassthruCentralNode |
		ImGuiDockNodeFlags_NoDockingInCentralNode
	);

	// DockSpace host (lets all editor windows dock/undock)
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags hostFlags =
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	if (ImGui::Begin("###DockSpaceHost", nullptr, hostFlags)) {
		ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
		ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
	}

	ImGui::End();
	ImGui::PopStyleVar(2);
}


void GraphicsEngine::EndImGuiFrame() {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GraphicsEngine::BeginFrame() {
	// Clear the entire backbuffer first (black bars included)
	glViewport(0, 0, screenWidth, screenHeight);
	renderer.Clear();

	// Then restrict rendering to the centered game area
	ApplyViewport();

	BeginImGuiFrame();
}

// Render the background and then all provided GameObjects
void GraphicsEngine::Render(const std::vector<GameObject*>& objects) {
	// Background first
	if (backgroundObject) {
		glDisable(GL_DEPTH_TEST);
		backgroundObject->Draw(view, projection);
		glEnable(GL_DEPTH_TEST);
	}

	// Scene objects
	for (const auto* obj : objects) {
		if (!obj) { continue; }

		obj->Draw(view, projection);

		// Draw bounding boxes only when debug is enabled (R)
		if (DebugRenderer::IsEnabled()) {
			glDisable(GL_DEPTH_TEST);
			obj->DrawBoundingBox(view, projection, { 1.0f, 0.0f, 0.0f });
			glEnable(GL_DEPTH_TEST);
		}
	}

	// Debug lines/points flush (R/T)
	if (DebugRenderer::IsEnabled()) {
		glDisable(GL_DEPTH_TEST);
		DebugRenderer::Flush(view, projection);
		glEnable(GL_DEPTH_TEST);
	}

	// ImGui on top
	EndImGuiFrame();

	// GL error check
	GLenum error;
	while ((error = glGetError()) != GL_NO_ERROR) {
		std::cerr << "OpenGL error after draw call: " << error << std::endl;
	}
}

// Destroy background and clear ResourceManager caches
void GraphicsEngine::Shutdown() {
	backgroundObject.reset();
	DebugRenderer::Shutdown();
	resourceManager.Clear();

	// ImGui cleanup
	if (s_imguiInitialized) {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		s_imguiInitialized = false;
	}
}

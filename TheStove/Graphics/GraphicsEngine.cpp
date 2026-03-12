/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GraphicsEngine.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (50%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(30%)
					Ng Juin Herng, juinherng.ng@digipen.edu (20%)

 DESCRIPTION:		Implements initialization, default resource loading, background handling, draw calls
					and batched instanced rendering of GameObjects.

		All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "../Core/FontSystem.hpp"
#include "../Core/InputManager.hpp"
#include "../Core/LevelEditorPanelFonts.hpp"

#include "GraphicsEngine.hpp"
#include "MeshLoader.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

// File-scoped state
static bool _imguiInitialized = false;

// Helper function to resolve shader paths across different build configurations
namespace {
	constexpr const char* kOpenGLErrorPrefixDefault = "[GraphicsEngine] OpenGL error";
	constexpr int kInvalidLayerValue = 1000000;

	struct RenderPassConfig {
		bool depthTestEnabled = true;
		bool depthWriteEnabled = true;
		bool blendingEnabled = false;
		GLenum blendSrcRgb = GL_SRC_ALPHA;
		GLenum blendDstRgb = GL_ONE_MINUS_SRC_ALPHA;
		GLenum blendSrcAlpha = GL_SRC_ALPHA;
		GLenum blendDstAlpha = GL_ONE_MINUS_SRC_ALPHA;
	};

	class ScopedRenderPassState {
	public:
		explicit ScopedRenderPassState(const RenderPassConfig& config)
			: depthTestWasEnabled_(glIsEnabled(GL_DEPTH_TEST) == GL_TRUE),
			blendWasEnabled_(glIsEnabled(GL_BLEND) == GL_TRUE) {
			GLboolean depthMaskState = GL_TRUE;
			glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskState);
			depthWriteWasEnabled_ = (depthMaskState == GL_TRUE);

			glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcRgb_);
			glGetIntegerv(GL_BLEND_DST_RGB, &blendDstRgb_);
			glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcAlpha_);
			glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstAlpha_);

			SetEnabled(GL_DEPTH_TEST, config.depthTestEnabled);
			glDepthMask(config.depthWriteEnabled ? GL_TRUE : GL_FALSE);
			SetEnabled(GL_BLEND, config.blendingEnabled);
			if (config.blendingEnabled) {
				glBlendFuncSeparate(config.blendSrcRgb, config.blendDstRgb, config.blendSrcAlpha, config.blendDstAlpha);
			}
		}

		~ScopedRenderPassState() {
			SetEnabled(GL_DEPTH_TEST, depthTestWasEnabled_);
			glDepthMask(depthWriteWasEnabled_ ? GL_TRUE : GL_FALSE);
			SetEnabled(GL_BLEND, blendWasEnabled_);
			glBlendFuncSeparate(
				static_cast<GLenum>(blendSrcRgb_),
				static_cast<GLenum>(blendDstRgb_),
				static_cast<GLenum>(blendSrcAlpha_),
				static_cast<GLenum>(blendDstAlpha_)
			);
		}

		ScopedRenderPassState(const ScopedRenderPassState&) = delete;
		ScopedRenderPassState& operator=(const ScopedRenderPassState&) = delete;

	private:
		static void SetEnabled(GLenum capability, bool enabled) {
			if (enabled) {
				glEnable(capability);
			}
			else {
				glDisable(capability);
			}
		}

		bool depthTestWasEnabled_ = true;
		bool depthWriteWasEnabled_ = true;
		bool blendWasEnabled_ = false;
		GLint blendSrcRgb_ = GL_SRC_ALPHA;
		GLint blendDstRgb_ = GL_ONE_MINUS_SRC_ALPHA;
		GLint blendSrcAlpha_ = GL_SRC_ALPHA;
		GLint blendDstAlpha_ = GL_ONE_MINUS_SRC_ALPHA;
	};

	std::string ResolveShaderPath(const std::string& relativePathFromProjectRoot) {
		// Print working directory only once
		static bool printedCwd = false;
		if (!printedCwd) {
			std::cout << "[ShaderPath] Current working directory: " << std::filesystem::current_path() << std::endl;
			printedCwd = true;
		}

		// Extract just the filename from the path
		std::string filename = std::filesystem::path(relativePathFromProjectRoot).filename().string();

		// Try multiple possible locations, prioritizing build/shaders since it exists
		std::vector<std::string> possiblePaths = {
			"shaders/" + filename,                                  // From build directory (build/shaders/)
			"../shaders/" + filename,                               // From build/Debug or build/Release
			relativePathFromProjectRoot,                            // Original path (e.g., "../shaders/shader.vert")
			"../../TheStove/Graphics/shaders/" + filename,          // From build/Release to source
			"../TheStove/Graphics/shaders/" + filename,             // From build to source
			"Graphics/shaders/" + filename                          // Alternative structure
		};

		for (const auto& path : possiblePaths) {
			if (std::filesystem::exists(path)) {
				std::cout << "[ShaderPath] Found '" << filename << "' at: " << path << std::endl;
				return path;
			}
		}

		// If not found, return original path and let error handling catch it
		std::cerr << "[ShaderPath] ERROR: Shader '" << filename << "' not found in any expected location!" << std::endl;
		std::cerr << "[ShaderPath] Tried paths relative to: " << std::filesystem::current_path() << std::endl;
		return relativePathFromProjectRoot;
	}

	void LogOpenGLErrors(const char* prefix = kOpenGLErrorPrefixDefault) {
		GLenum error;
		while ((error = glGetError()) != GL_NO_ERROR) {
			std::cerr << prefix << ": " << error << std::endl;
		}
	}

	bool NeedsPerInstanceTintFallback(const std::vector<Mesh::InstanceData>& batch) {
		return std::any_of(batch.begin(), batch.end(), [](const Mesh::InstanceData& inst) {
			return inst.colorTint.x != 1.0f || inst.colorTint.y != 1.0f ||
				inst.colorTint.z != 1.0f || inst.colorTint.w < 0.999f;
			});
	}
}

// Singleton access
GraphicsEngine& GraphicsEngine::Instance() {
	static GraphicsEngine instance;
	return instance;
}

// Constructor: Initializes references and identity matrices for view/projection
GraphicsEngine::GraphicsEngine()
	: resourceManager(ResourceManager::Instance()),
	projection(1.0f),
	view(1.0f) {
}

// Initialize core renderer, FBO, default resources, and ImGui
void GraphicsEngine::Initialize() {
	// Ensure there's a current GLFW OpenGL context before calling any GL functions.
	GLFWwindow* ctx = glfwGetCurrentContext();
	if (ctx == nullptr) {
		std::cerr << "[GraphicsEngine] ERROR: No current OpenGL context. Initialize must be called after creating/making context current.\n";
		return;
	}

	// Load GL function pointers as early as possible (must succeed before any gl* calls).
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "[GraphicsEngine] ERROR: gladLoadGLLoader failed - no GL functions available\n";
		return;
	}

	// Now safe to call GL / renderer initialization
	renderer.Initialize();
	renderer.SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);

	CreateSceneFBO(kRefW, kRefH);
	view = glm::mat4(1.0f);

	LoadDefaultResources();

	// Initialize FontSystem
	if (!FontSystem::FontManager::Instance().Initialize()) {
		std::cerr << "[GraphicsEngine] ERROR: Failed to initialize FontManager\n";
	}

	if (!FontSystem::TextRenderer::Instance().Initialize()) {
		std::cerr << "[GraphicsEngine] ERROR: Failed to initialize TextRenderer\n";
	}

	DebugRenderer::Init();
	DebugRenderer::SetEnabled(false);

#ifdef _DEBUG
	// Initialize ImGui only after GL loader succeeded and we have a valid context.
	if (!_imguiInitialized) {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		// Set ImGui INI file path to a persistent location outside build directory
		// This survives clean builds
		io.IniFilename = "../../imgui.ini";

		// Slightly larger UI for readability
		io.FontGlobalScale = 1.0f;
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(1.2f);
		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForOpenGL(ctx, true);
		ImGui_ImplOpenGL3_Init("#version 330 core");
		_imguiInitialized = true;
	}
#endif
}

// SystemInterface Update - currently just tracks deltaTime for performance monitoring
void GraphicsEngine::Update(float dt) {
	// Store dt for performance tracking
	lastDt = dt;

	// Update transition state machine
	UpdateTransition(dt);

	// Note: Actual rendering is still called from main loop via BeginFrame/Render/EndFrame
	// This Update is just for system integration and performance monitoring
	(void)dt; // Suppress unused parameter warning if no other logic needed
}

// Destroy the current scene FBO (safe to call repeatedly)
void GraphicsEngine::DestroySceneFBO() {
	if (mSceneDepth) {
		glDeleteRenderbuffers(1, &mSceneDepth);
		mSceneDepth = 0;
	}

	if (mSceneColor) {
		glDeleteTextures(1, &mSceneColor);
		mSceneColor = 0;
	}

	if (mSceneFBO) {
		glDeleteFramebuffers(1, &mSceneFBO);
		mSceneFBO = 0;
	}
}

// Create a color and depth FBO for the scene at the given size
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
		// std::cerr << "[GraphicsEngine] Scene FBO incomplete\n";
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	mSceneWidth = w;
	mSceneHeight = h;
}

// Resize (recreate) the scene FBO if size is valid
void GraphicsEngine::ResizeSceneFBO(int w, int h) {
	if (w <= 0 || h <= 0) {
		return;
	}

	CreateSceneFBO(w, h);
}

// Bind scene FBO and clear
void GraphicsEngine::BeginSceneRender() {
	glBindFramebuffer(GL_FRAMEBUFFER, mSceneFBO);
	glViewport(0, 0, mSceneWidth, mSceneHeight);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

// Unbind scene FBO
void GraphicsEngine::EndSceneRender() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Getters
const glm::mat4& GraphicsEngine::GetProjection() const {
	return projection;
}
const glm::mat4& GraphicsEngine::GetView() const {
	return view;
}
ImGuiID GraphicsEngine::GetMainDockspaceID() const {
	return mMainDockspaceId;
}

// Handle window resize: update letterboxed viewport and background placement
void GraphicsEngine::Resize(int width, int height) {
	if (width <= 0 || height <= 0) {
		return;
	}

	screenWidth = width;
	screenHeight = height;

	// Fixed pixel-space ortho (origin top-left)
	projection = glm::ortho(
		0.0f, static_cast<float>(kRefW),
		static_cast<float>(kRefH), 0.0f,
		-1.0f, 1.0f
	);

	// Fullscreen viewport without letterboxing
	viewportW_ = width;
	viewportH_ = height;
	viewportX_ = 0;
	viewportY_ = 0;
	viewportScale_ = std::min(
		static_cast<float>(width) / static_cast<float>(kRefW),
		static_cast<float>(height) / static_cast<float>(kRefH)
	);

	// Apply the viewport now
	glViewport(viewportX_, viewportY_, viewportW_, viewportH_);

	// Keep background quad aligned to the reference canvas
	if (backgroundObject) {
		backgroundObject->SetPosition(glm::vec3(kRefW * 0.5f, kRefH * 0.5f, 0.0f));
		backgroundObject->SetScale(glm::vec3(static_cast<float>(kRefW),
			static_cast<float>(kRefH), 1.0f));
	}
	if (backgroundOverlayObject) {
		backgroundOverlayObject->SetPosition(glm::vec3(kRefW * 0.5f, kRefH * 0.5f, 0.0f));
		backgroundOverlayObject->SetScale(glm::vec3(static_cast<float>(kRefW),
			static_cast<float>(kRefH), 1.0f));
	}
}

// Apply the current letterboxed viewport (use before world rendering)
void GraphicsEngine::ApplyViewport() const {
	glViewport(viewportX_, viewportY_, viewportW_, viewportH_);
}

// Load core shaders and meshes used by engine/editor
void GraphicsEngine::LoadDefaultResources() {
	// Load default shader
	resourceManager.LoadShader("basic",
		ResolveShaderPath("../shaders/shader.vert"),
		ResolveShaderPath("../shaders/shader.frag"));

	// Load texture shader
	resourceManager.LoadShader("texture",
		ResolveShaderPath("../shaders/texture.vert"),
		ResolveShaderPath("../shaders/texture.frag"));

	// Load sprite shader
	resourceManager.LoadShader("sprite",
		ResolveShaderPath("../shaders/sprite.vert"),
		ResolveShaderPath("../shaders/sprite.frag"));

	// Load static sprite shader
	resourceManager.LoadShader("staticsprite",
		ResolveShaderPath("../shaders/staticsprite.vert"),
		ResolveShaderPath("../shaders/staticsprite.frag"));

	// Load animated sprite shader
	resourceManager.LoadShader("animatedsprite",
		ResolveShaderPath("../shaders/animatedsprite.vert"),
		ResolveShaderPath("../shaders/animatedsprite.frag"));

	// Load instanced static sprite shader
	resourceManager.LoadShader("staticsprite_instanced",
		ResolveShaderPath("../shaders/staticsprite_instanced.vert"),
		ResolveShaderPath("../shaders/staticsprite_instanced.frag"));

	// Load instanced animated sprite shader
	resourceManager.LoadShader("animatedsprite_instanced",
		ResolveShaderPath("../shaders/animatedsprite_instanced.vert"),
		ResolveShaderPath("../shaders/animatedsprite.frag"));

	// Shadow blob shader (no texture required)
	resourceManager.LoadShader("shadow",
		ResolveShaderPath("../shaders/shadow.vert"),
		ResolveShaderPath("../shaders/shadow.frag"));

	// Solid color fullscreen overlay shader for transitions
	resourceManager.LoadShader("screenfade",
		ResolveShaderPath("../shaders/screenfade.vert"),
		ResolveShaderPath("../shaders/screenfade.frag"));

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

// Set/ensure a fullscreen background quad using the given texture path
void GraphicsEngine::SetBackground(const std::string& texturePath) {
	// Derive a unique key per path to avoid returning a cached texture
	std::string key = "background_" + std::filesystem::path(texturePath).filename().string();

	Texture* bgTexture = resourceManager.LoadTexture(key, texturePath);
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
			// Position background to fill reference canvas (not screen size)
			backgroundObject->SetPosition(glm::vec3(kRefW * 0.5f, kRefH * 0.5f, 0.0f));
			backgroundObject->SetScale(glm::vec3(static_cast<float>(kRefW), static_cast<float>(kRefH), 1.0f));
		}
	}

	if (backgroundObject) {
		backgroundObject->SetTexture(bgTexture);
	}
}

void GraphicsEngine::SetBackgroundOverlay(const std::string& texturePath) {
	std::string key = "background_overlay_" + std::filesystem::path(texturePath).filename().string();

	Texture* overlayTexture = resourceManager.LoadTexture(key, texturePath);
	if (!overlayTexture) {
		std::cerr << "Failed to load background overlay texture: " << texturePath << std::endl;
		return;
	}

	if (!backgroundOverlayObject) {
		Mesh* quadMesh = resourceManager.GetMesh("fullscreen_quad");
		Shader* textureShader = resourceManager.GetShader("texture");

		if (quadMesh && textureShader) {
			backgroundOverlayObject = std::make_unique<GameObject>(quadMesh, textureShader);
			backgroundOverlayObject->SetPosition(glm::vec3(kRefW * 0.5f, kRefH * 0.5f, 0.0f));
			backgroundOverlayObject->SetScale(glm::vec3(static_cast<float>(kRefW), static_cast<float>(kRefH), 1.0f));
		}
	}

	if (backgroundOverlayObject) {
		backgroundOverlayObject->SetTexture(overlayTexture);
	}
}

// Remove the background object (if present).
void GraphicsEngine::ClearBackground() {
	backgroundObject.reset();
}

void GraphicsEngine::ClearBackgroundOverlay() {
	backgroundOverlayObject.reset();
}

// Start a new ImGui frame and host a global DockSpace
void GraphicsEngine::BeginImGuiFrame() {
#ifdef _DEBUG
	// Guard: only call backend frame functions if ImGui was initialized
	if (!_imguiInitialized || ImGui::GetCurrentContext() == nullptr) {
		return;
	}

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	sceneViewportPresenter_.BeginDockspaceFrame(mMainDockspaceId);
#endif
}

// Parse a layer number from a string, returning a default of 1 for empty and an error code for invalid input
int GraphicsEngine::ParseLayerNumber(const std::string& layerName) {
	if (layerName.empty()) {
		return 1;
	}

	int result = 0;
	for (char c : layerName) {
		if (!std::isdigit(static_cast<unsigned char>(c))) {
			return kInvalidLayerValue;
		}

		result = result * 10 + (c - '0');
	}

	return result;
}

// Compute the screen-space rect of the Scene image based on the current viewport and reference size, for mouse picking and UI alignment
void GraphicsEngine::ComputeSceneImageRect(ImVec2& outPos, ImVec2& outSize) const {
	sceneViewportPresenter_.ComputeSceneImageRect(
		sceneImagePos_,
		sceneImageSize_,
		kRefW,
		kRefH,
		outPos,
		outSize
	);
}

// Render the background quad (if set) with appropriate shader and texture bindings, ignoring depth and blending
void GraphicsEngine::RenderBackground(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
	if (!backgroundObject) {
		return;
	}

	const ScopedRenderPassState passState({
		.depthTestEnabled = false,
		.depthWriteEnabled = true,
		.blendingEnabled = false
		});

	Shader* shader = backgroundObject->GetShader();
	if (shader) {
		shader->Use();
		shader->SetModelMatrix(backgroundObject->GetModelMatrix());
		shader->SetViewMatrix(viewMatrix);
		shader->SetProjectionMatrix(projectionMatrix);
	}

	Texture* tex = backgroundObject->GetTexture();
	if (tex && shader) {
		tex->Bind(0);
		shader->SetTexture("u_Texture", 0);
	}

	Mesh* mesh = backgroundObject->GetMesh();
	if (mesh) {
		mesh->Draw();
	}
}

void GraphicsEngine::RenderBackgroundOverlay(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
	if (!backgroundOverlayObject) {
		return;
	}

	const ScopedRenderPassState passState({
		.depthTestEnabled = false,
		.depthWriteEnabled = true,
		.blendingEnabled = true
		});

	Shader* shader = backgroundOverlayObject->GetShader();
	if (shader) {
		shader->Use();
		shader->SetModelMatrix(backgroundOverlayObject->GetModelMatrix());
		shader->SetViewMatrix(viewMatrix);
		shader->SetProjectionMatrix(projectionMatrix);
	}

	Texture* tex = backgroundOverlayObject->GetTexture();
	if (tex && shader) {
		tex->Bind(0);
		shader->SetTexture("u_Texture", 0);
	}

	Mesh* mesh = backgroundOverlayObject->GetMesh();
	if (mesh) {
		mesh->Draw();
	}
}

// Blit the rendered scene from the FBO to the default framebuffer, applying letterboxing as needed
void GraphicsEngine::PresentSceneToDefaultFramebuffer() {
	if (mSceneFBO == 0 || mSceneColor == 0 || screenWidth <= 0 || screenHeight <= 0) {
		return;
	}

	glBindFramebuffer(GL_READ_FRAMEBUFFER, mSceneFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	const int dstX0 = viewportX_;
	const int dstY0 = viewportY_;
	const int dstX1 = viewportX_ + viewportW_;
	const int dstY1 = viewportY_ + viewportH_;

	glViewport(0, 0, screenWidth, screenHeight);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	glBlitFramebuffer(
		0, 0, mSceneWidth, mSceneHeight,
		dstX0, dstY0, dstX1, dstY1,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR
	);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Draw the Scene window and present the scene FBO texture inside it
void GraphicsEngine::DrawSceneDockWindow() {
#ifdef _DEBUG
	// Guard: bail if ImGui not initialized or no context
	if (!_imguiInitialized || ImGui::GetCurrentContext() == nullptr) {
		return;
	}

	sceneViewportPresenter_.DrawSceneWindow(
		mSceneColor,
		kRefW,
		kRefH,
		mMainDockspaceId,
		sceneImagePos_,
		sceneImageSize_
	);
#endif
}

// Unbind FBO, draw ImGui (including Scene window with FBO texture), and present to default framebuffer if not in debug mode
void GraphicsEngine::EndSceneAndPresent() {
	EndSceneRender();
#ifdef _DEBUG
	DrawSceneDockWindow();
	EndImGuiFrame();
#else
	PresentSceneToDefaultFramebuffer();
#endif
}

// Finish the current ImGui frame and render it
void GraphicsEngine::EndImGuiFrame() {
#ifdef _DEBUG
	// Guard: only render if initialized & valid ImGui context
	if (!_imguiInitialized || ImGui::GetCurrentContext() == nullptr) {
		return;
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
}

// Frame begin: clear backbuffer, set viewport, bind scene FBO, begin ImGui
void GraphicsEngine::BeginFrame() {
	glViewport(0, 0, screenWidth, screenHeight);
	renderer.Clear();

	ApplyViewport();
	BeginSceneRender();
	BeginImGuiFrame();
}

// Convert current mouse (screen) into scene world coords if within image
bool GraphicsEngine::GetMouseWorldInScene(glm::vec2& outWorld, const glm::dvec2* mousePosOverride) const {

	if (mousePosOverride == nullptr) {
		ImVec2 localPos{}, sceneSize{};
		if (!TryGetMousePositionInScene(localPos, sceneSize)) {
			return false;
		}
		outWorld = ScenePixelToWorld(localPos, sceneSize);
		return true;
	}

	const glm::dvec2 mousePos = mousePosOverride ? *mousePosOverride : InputManager::Get().GetMousePosition();

#ifdef _DEBUG
	if (_imguiInitialized && ImGui::GetCurrentContext() != nullptr) {
		ImVec2 scenePos{}, sceneSize{};
		ComputeSceneImageRect(scenePos, sceneSize);

		if (sceneSize.x <= 0.0f || sceneSize.y <= 0.0f) {
			return false;
		}

		const float localX = static_cast<float>(mousePos.x) - scenePos.x;
		const float localY = static_cast<float>(mousePos.y) - scenePos.y;

		if (localX < 0.0f || localY < 0.0f || localX > sceneSize.x || localY > sceneSize.y) {
			return false;
		}

		outWorld = ScenePixelToWorld(ImVec2(localX, localY), sceneSize);
		return true;
	}
#endif

	// Release / no-ImGui-safe path (viewport space)
	const float vx = static_cast<float>(viewportX_);
	const float vy = static_cast<float>(viewportY_);
	const float vw = static_cast<float>(viewportW_);
	const float vh = static_cast<float>(viewportH_);
	if (vw <= 0.0f || vh <= 0.0f) {
		return false;
	}

	const float localX = static_cast<float>(mousePos.x) - vx;
	const float localY = static_cast<float>(mousePos.y) - vy;
	if (localX < 0.0f || localY < 0.0f || localX > vw || localY > vh) {
		return false;
	}

	outWorld = ScenePixelToWorld(ImVec2(localX, localY), ImVec2(vw, vh));
	return true;
}

// Get the screen-space rect of the Scene image for mouse picking and UI alignment
void GraphicsEngine::GetSceneImageRect(ImVec2& outPos, ImVec2& outSize) const {
#ifdef _DEBUG
	ComputeSceneImageRect(outPos, outSize);
#else
	// In release we render directly to the GLFW window. Keep it simple:
	outPos = ImVec2(0.0f, 0.0f);
	outSize = ImVec2(static_cast<float>(viewportW_),
		static_cast<float>(viewportH_));
#endif
}

// Try to get the mouse position in scene local pixel coordinates and scene size. Returns false if not over the scene image.
bool GraphicsEngine::TryGetMousePositionInScene(ImVec2& outLocalPos, ImVec2& outSceneSize) const {
#ifdef _DEBUG
	if (!_imguiInitialized || ImGui::GetCurrentContext() == nullptr) {
		return false;
	}

	ImVec2 scenePos;
	ComputeSceneImageRect(scenePos, outSceneSize);

	// First try ImGui's mouse position (which accounts for multiple viewports and should be more robust), but fall back to InputManager if it's not finite (can happen during docking/layout changes).
	ImVec2 mouse = ImGui::GetMousePos();
	if (!std::isfinite(mouse.x) || !std::isfinite(mouse.y)) {
		const glm::dvec2 mousePos = InputManager::Get().GetMousePosition();
		mouse = ImVec2(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
	}

	if (outSceneSize.x <= 0.0f || outSceneSize.y <= 0.0f) {
		// Fallback to the runtime viewport bounds when the scene tab has not produced a
		// valid image rect yet (e.g. first frame after docking/layout changes).
		const float vx = static_cast<float>(viewportX_);
		const float vy = static_cast<float>(viewportY_);
		const float vw = static_cast<float>(viewportW_);
		const float vh = static_cast<float>(viewportH_);
		if (vw <= 0.0f || vh <= 0.0f || mouse.x < vx || mouse.y < vy ||
			mouse.x >(vx + vw) || mouse.y >(vy + vh)) {
			return false;
		}

		outSceneSize = ImVec2(vw, vh);
		outLocalPos = ImVec2(mouse.x - vx, mouse.y - vy);
		return true;
	}

	if (mouse.x < scenePos.x || mouse.y < scenePos.y ||
		mouse.x > scenePos.x + outSceneSize.x || mouse.y > scenePos.y + outSceneSize.y) {
		return false;
	}

	outLocalPos = ImVec2(mouse.x - scenePos.x, mouse.y - scenePos.y);
	return true;
#else
	GLFWwindow* win = glfwGetCurrentContext();
	if (!win) {
		return false;
	}

	double mouseX = 0.0;
	double mouseY = 0.0;
	if (InputManager::Get().IsReplayOverride()) {
		const glm::dvec2 replayMouse = InputManager::Get().GetMousePosition();
		mouseX = replayMouse.x;
		mouseY = replayMouse.y;
	}
	else {
		glfwGetCursorPos(win, &mouseX, &mouseY);
	}

	const float vx = static_cast<float>(viewportX_);
	const float vy = static_cast<float>(viewportY_);
	const float vw = static_cast<float>(viewportW_);
	const float vh = static_cast<float>(viewportH_);
	if (vw <= 0.0f || vh <= 0.0f || mouseX < vx || mouseY < vy ||
		mouseX >(vx + vw) || mouseY >(vy + vh)) {
		return false;
	}

	outSceneSize = ImVec2(vw, vh);
	outLocalPos = ImVec2(static_cast<float>(mouseX) - vx, static_cast<float>(mouseY) - vy);
	return true;
#endif
}

// Convert pixel coordinates relative to the Scene image into world coordinates using inverse view-projection
glm::vec2 GraphicsEngine::ScenePixelToWorld(const ImVec2& localPixel, const ImVec2& sceneSize) const {
	const float u = localPixel.x / sceneSize.x;
	const float v = localPixel.y / sceneSize.y;

	const float px = u * static_cast<float>(kRefW);
	const float py = v * static_cast<float>(kRefH);

	glm::vec4 clip;
	clip.x = (px / static_cast<float>(kRefW)) * 2.0f - 1.0f;
	clip.y = 1.0f - (py / static_cast<float>(kRefH)) * 2.0f;
	clip.z = 0.0f;
	clip.w = 1.0f;

	const glm::mat4 invVP = glm::inverse(projection * view);
	const glm::vec4 world4 = invVP * clip;
	return glm::vec2(world4.x, world4.y);
}

// Convert world position to pixel coordinates relative to the Scene image (for editor gizmos, etc.)
ImVec2 GraphicsEngine::WorldToScenePixel(const glm::vec2& world, const ImVec2& scenePos, const ImVec2& sceneSize) const {
	glm::vec4 world4(world.x, world.y, 0.0f, 1.0f);
	const glm::vec4 clip = projection * view * world4;
	if (clip.w == 0.0f) {
		return ImVec2(-10000.0f, -10000.0f);
	}

	const glm::vec3 ndc = glm::vec3(clip) / clip.w;
	const float u = (ndc.x * 0.5f) + 0.5f;
	const float v = (-ndc.y * 0.5f) + 0.5f;

	return ImVec2(
		scenePos.x + (u * sceneSize.x),
		scenePos.y + (v * sceneSize.y)
	);
}

// Convert world position to screen coordinates relative to the Scene image (for editor gizmos, etc.)
ImVec2 GraphicsEngine::WorldToSceneImage(const glm::vec2& world) const {
#ifdef _DEBUG
	ImVec2 imgPos;
	ImVec2 imgSize;
	ComputeSceneImageRect(imgPos, imgSize);
	return WorldToScenePixel(world, imgPos, imgSize);
#else
	// In release builds the editor UI is disabled; this is only used in Debug.
	(void)world;
	return ImVec2(0.0f, 0.0f);
#endif
}

// Default render path
void GraphicsEngine::Render(const std::vector<GameObject*>& objects, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
	// Keep internal matrices in sync for editor picking + gizmos
	view = viewMatrix;
	projection = projectionMatrix;

	// Draw background first
	RenderBackground(viewMatrix, projectionMatrix);

	// Draw shadows before sprites
	DrawSpriteShadows(objects, viewMatrix, projectionMatrix);

	// Render all scene objects using a pass config tuned for alpha-blended sprites.
	const ScopedRenderPassState spritePassState({
		.depthTestEnabled = false,
		.depthWriteEnabled = true,
		.blendingEnabled = true
		});

	for (const auto* obj : objects) {
		if (!obj) {
			continue;
		}

		Shader* shader = obj->GetShader();
		if (!shader) {
			continue;
		}

		shader->Use();
		shader->SetModelMatrix(obj->GetModelMatrix());
		shader->SetViewMatrix(viewMatrix);
		shader->SetProjectionMatrix(projectionMatrix);

		// set per-object tint (u_Color)
		shader->SetColorTint(obj->GetColorTint());

		Texture* tex = obj->GetTexture();
		if (tex) {
			tex->Bind(0);
			shader->SetTexture("u_Texture", 0);
		}

		Mesh* mesh = obj->GetMesh();
		if (mesh) {
			mesh->Draw();
		}

		if (DebugRenderer::IsEnabled()) {
			const ScopedRenderPassState debugPassState({
				.depthTestEnabled = false,
				.depthWriteEnabled = true,
				.blendingEnabled = true
				});
			obj->DrawBoundingBox(viewMatrix, projectionMatrix, glm::vec3{ 1.0f, 0.0f, 0.0f });
		}
	}

	// Draw background overlay after scene objects so foreground overlays
	// (e.g. countertop tops/walls) can sit above gameplay sprites.
	RenderBackgroundOverlay(viewMatrix, projectionMatrix);

	// Draw transition overlay into the scene FBO before unbinding
	DrawTransitionOverlay();

	EndSceneAndPresent();
	LogOpenGLErrors("[GraphicsEngine] OpenGL error after draw call");
}

// Batched/instanced render path 
void GraphicsEngine::RenderBatched(const std::vector<GameObject*>& objects) {
	// Reset stats
	renderStats = RenderStats();
	renderStats.totalObjects = static_cast<int>(objects.size());

	// Draw background first
	RenderBackground(view, projection);

	// Draw shadows before sprites 
	DrawSpriteShadows(objects, view, projection);

	// Keep sprite batching in a pass with alpha blending and no depth testing.
	const ScopedRenderPassState spriteBatchPassState({
		.depthTestEnabled = false,
		.depthWriteEnabled = true,
		.blendingEnabled = true
		});
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef _DEBUG
	// Get text objects and sort by layer for interleaved rendering
	const auto& textObjects = LEPANELFONTS::GetTextObjects();
	struct SortedTextEntry {
		const LEPANELFONTS::TextObjectData* data;
		int layer;
	};
	std::vector<SortedTextEntry> sortedTextObjects;
	sortedTextObjects.reserve(textObjects.size());
	for (const auto& data : textObjects) {
		sortedTextObjects.emplace_back(SortedTextEntry{ &data, ParseLayerNumber(data.layer) });
	}

	std::sort(sortedTextObjects.begin(), sortedTextObjects.end(),
		[](const SortedTextEntry& a, const SortedTextEntry& b) {
			// Lower layer number = rendered first (behind)
			// Higher layer number = rendered later (on top)
			if (a.layer != b.layer) {
				return a.layer < b.layer;
			}

			// Same layer: use depth first, then Y position for sorting
			if (a.data->depth != b.data->depth) {
				return a.data->depth < b.data->depth;
			}

			return a.data->y < b.data->y;
		});

	size_t textIndex = 0; // Track which text objects have been rendered
	int lastProcessedLayer = 0; // Track the last layer we finished processing

	// Lambda to render text objects up to and including a certain layer
	auto renderTextUpToLayer = [&](int maxLayerNumber) {
		while (textIndex < sortedTextObjects.size()) {
			if (sortedTextObjects[textIndex].layer <= maxLayerNumber) {
				RenderSingleTextObject(*sortedTextObjects[textIndex].data);
				++textIndex;
			}
			else {
				break; // Text belongs to a higher layer, stop
			}
		}
		};
#endif

	// Early out if no objects (but still render text)
	if (objects.empty()) {
#ifdef _DEBUG
		// Render all text objects
		for (size_t i = 0; i < sortedTextObjects.size(); ++i) {
			RenderSingleTextObject(*sortedTextObjects[i].data);
		}
#endif

		// Keep overlay above any scene/text content.
		RenderBackgroundOverlay(view, projection);

		// Draw transition overlay even if empty scene
		DrawTransitionOverlay();
		EndSceneAndPresent();
		return;
	}

	// Prefetch possible instanced shaders + animated shader pointer
	Shader* staticsInstShader = resourceManager.GetShader("staticsprite_instanced");
	Shader* animatedInstShader = resourceManager.GetShader("animatedsprite_instanced");
	Shader* staticShader = resourceManager.GetShader("staticsprite");
	Shader* animShader = resourceManager.GetShader("animatedsprite");

	// Build contiguous runs keyed by (mesh, shader, texture) - preserves layering order
	std::vector<Mesh::InstanceData> instanceBatch;
	instanceBatch.reserve(objects.size());
	RenderKey currentKey{ nullptr, nullptr, nullptr };

	auto flushBatch = [&](const std::vector<Mesh::InstanceData>& batch, const RenderKey& key) {
		if (batch.empty() || !key.mesh || !key.shader) {
			return;
		}

		bool wantsInstancing = batch.size() >= INSTANCING_THRESHOLD;

		// Instanced shaders do not carry per-instance tint yet -> fall back when needed
		const bool needsPerInstanceTint = NeedsPerInstanceTintFallback(batch);

		// Map the original shader -> preferred instanced shader 
		Shader* preferredInstanced = nullptr;
		if (key.shader == staticShader) {
			preferredInstanced = staticsInstShader;
		}
		else if (key.shader == animShader) {
			preferredInstanced = animatedInstShader;
		}

		const bool useInstanced = wantsInstancing && preferredInstanced && !needsPerInstanceTint;

		if (useInstanced) {
			// existing instanced path (unchanged)
			key.mesh->SetupInstanceBuffer(batch);
			preferredInstanced->Use();
			preferredInstanced->SetViewMatrix(view);
			preferredInstanced->SetProjectionMatrix(projection);
			if (key.texture) {
				key.texture->Bind(0); preferredInstanced->SetTexture("u_Texture", 0);
			}

			key.mesh->DrawInstanced(key.texture, static_cast<GLsizei>(batch.size()));
			renderStats.drawCalls++;
			renderStats.totalBatches++;
			renderStats.instancedObjects += static_cast<int>(batch.size());
		}
		else {
			// existing non-instanced fallback, but keep tint+blending enabled
			key.shader->Use();
			key.shader->SetViewMatrix(view);
			key.shader->SetProjectionMatrix(projection);

			if (key.texture) {
				key.texture->Bind(0);
				key.shader->SetTexture("u_Texture", 0);
			}

			for (const auto& inst : batch) {
				key.shader->SetModelMatrix(inst.modelMatrix);

				if (key.shader == animShader) {
					key.shader->SetUVOffset(glm::vec2(inst.uvOffsetScale.x, inst.uvOffsetScale.y));
					key.shader->SetUVScale(glm::vec2(inst.uvOffsetScale.z, inst.uvOffsetScale.w));
				}

				key.shader->SetColorTint(inst.colorTint);

				key.mesh->Draw();
				renderStats.drawCalls++;
			}

			renderStats.totalBatches++;
		}
		};

	// Build runs in order, interleaving text objects at layer boundaries
	for (auto* obj : objects) {
		if (!obj) {
			continue;
		}

		Mesh* mesh = obj->GetMesh();
		Shader* shader = obj->GetShader();
		if (!mesh || !shader) {
			continue;
		}

		Texture* texture = obj->GetTexture();
		int objLayer = obj->GetRenderLayer();
#ifndef _DEBUG
		(void)objLayer;
#endif

#ifdef _DEBUG
		// When we move to a new (higher) layer, first render text objects
		// from the previous layers that haven't been rendered yet
		if (objLayer > lastProcessedLayer) {
			// Flush current batch before rendering text
			if (!instanceBatch.empty()) {
				flushBatch(instanceBatch, currentKey);
				instanceBatch.clear();
			}

			// Render text objects up to and including the previous layer
			renderTextUpToLayer(objLayer - 1);
			lastProcessedLayer = objLayer;
		}
#endif

		RenderKey key{ mesh, shader, texture };

		// Flush when render key changes
		if (key != currentKey && !instanceBatch.empty()) {
			flushBatch(instanceBatch, currentKey);
			instanceBatch.clear();
		}

		currentKey = key;

		// Always store per-object UV rect in instance data (instanced shader will use it, fallback uses uniforms)
		instanceBatch.emplace_back();
		auto& inst = instanceBatch.back();
		inst.modelMatrix = obj->GetModelMatrix();
		inst.uvOffsetScale = obj->GetUVRect();
		inst.colorTint = obj->GetColorTint();
	}

	// Flush remaining batch
	if (!instanceBatch.empty()) {
		flushBatch(instanceBatch, currentKey);
		instanceBatch.clear();
	}

#ifdef _DEBUG
	// Render remaining text objects (those in layers >= the last game object layer)
	while (textIndex < sortedTextObjects.size()) {
		RenderSingleTextObject(*sortedTextObjects[textIndex].data);
		++textIndex;
	}
#endif

	// Debug bounding boxes render
	if (DebugRenderer::IsEnabled()) {
		const ScopedRenderPassState debugPassState({
			.depthTestEnabled = false,
			.depthWriteEnabled = true,
			.blendingEnabled = true
			});
		for (const auto* obj : objects) {
			if (obj) {
				obj->DrawBoundingBox(view, projection, glm::vec3{ 1.0f, 0.0f, 0.0f });
			}
		}

		DebugRenderer::Flush(view, projection);
	}

	// Draw background overlay after scene/text/debug so it appears as the
	// foreground layer while still staying below transition effects.
	RenderBackgroundOverlay(view, projection);

	// Draw transition overlay on top of everything in the scene FBO
	DrawTransitionOverlay();

	// End-of-frame UI and finalization
	EndSceneAndPresent();
	LogOpenGLErrors("[GraphicsEngine] OpenGL error in batched rendering");
}

// Render a single text object (for layered rendering)
void GraphicsEngine::RenderSingleTextObject(const LEPANELFONTS::TextObjectData& data) {
	// Get the font from ResourceManager
	FontSystem::Font* font = ResourceManager::Instance().GetFont(data.fontName);
	if (!font) {
		return;
	}

	const ScopedRenderPassState textPassState({
		.depthTestEnabled = false,
		.depthWriteEnabled = true,
		.blendingEnabled = true
		});

	// Create a temporary Text object for rendering
	FontSystem::Text textRenderer;
	textRenderer.SetFont(font);
	textRenderer.SetText(data.text);
	textRenderer.SetPosition(glm::vec2(data.x, data.y));
	textRenderer.SetScale(data.scale);
	textRenderer.SetRotation(data.rotation);
	textRenderer.SetRotationMode(data.useBlockRotation ?
		FontSystem::Text::RotationMode::Block :
		FontSystem::Text::RotationMode::PerCharacter);
	textRenderer.SetColor(glm::vec4(data.colorR, data.colorG, data.colorB, data.colorA));

	// Render using TextRenderer singleton
	FontSystem::TextRenderer::Instance().RenderText(textRenderer, projection);
}

// Render text objects
void GraphicsEngine::RenderTextObjects() {
#ifdef _DEBUG
	// In debug builds, get text from the editor panel
	const auto& textObjects = LEPANELFONTS::GetTextObjects();

	if (textObjects.empty()) {
		return;
	}

	// Sort text objects by layer (lower layer numbers render first/behind)
	struct SortedTextEntry {
		const LEPANELFONTS::TextObjectData* data;
		int layer;
	};
	std::vector<SortedTextEntry> sortedTextObjects;
	sortedTextObjects.reserve(textObjects.size());
	for (const auto& data : textObjects) {
		sortedTextObjects.emplace_back(SortedTextEntry{ &data, ParseLayerNumber(data.layer) });
	}

	std::sort(sortedTextObjects.begin(), sortedTextObjects.end(),
		[](const SortedTextEntry& a, const SortedTextEntry& b) {
			// Higher layer number = rendered on top (later in draw order)
			if (a.layer != b.layer) {
				return a.layer < b.layer;
			}

			// Same layer: use Y position for depth sorting
			return a.data->y < b.data->y;
		});

	// Create Text renderers on demand and render
	for (const auto& entry : sortedTextObjects) {
		RenderSingleTextObject(*entry.data);
	}
#else
	// In release build, text will be rendered from game state
#endif
}

// Simple blob shadow pass (draw before sprites)
void GraphicsEngine::DrawSpriteShadows(const std::vector<GameObject*>& objects, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
	Shader* shadowShader = resourceManager.GetShader("shadow");
	Mesh* quad = resourceManager.GetMesh("sprite");

	if (!shadowShader || !quad) {
		return;
	}

	const ScopedRenderPassState passState({
		.depthTestEnabled = false,
		.depthWriteEnabled = false,
		.blendingEnabled = true
		});

	GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
	if (!blendWasEnabled) {
		glEnable(GL_BLEND);
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	shadowShader->Use();
	shadowShader->SetViewMatrix(viewMatrix);
	shadowShader->SetProjectionMatrix(projectionMatrix);

	for (const GameObject* obj : objects) {
		if (!obj || !obj->HasShadow()) {
			continue;
		}

		const glm::vec3 pos = obj->GetPositionGLM();
		const glm::vec2 size = obj->GetShadowSize();
		const glm::vec2 off = obj->GetShadowOffset();
		const float opacity = obj->GetShadowOpacity();

		// Model: place on object's XY with optional offset; keep Z slightly behind if needed
		glm::mat4 model(1.0f);
		model = glm::translate(model, glm::vec3(pos.x + off.x, pos.y + off.y, pos.z));
		model = glm::scale(model, glm::vec3(size.x, size.y, 1.0f));

		// Ellipse axis compensation so the gradient remains elliptical after non-uniform scale
		const float axisYOverX = (size.x != 0.0f) ? (size.y / size.x) : 1.0f;
		shadowShader->SetModelMatrix(model);
		shadowShader->SetColorTint(glm::vec4(0.0f, 0.0f, 0.0f, opacity));
		shadowShader->SetUVScale(glm::vec2(1.0f, axisYOverX)); // reuse uniform setter

		quad->Draw();
	}
}

// Clean up resources and ImGui context
void GraphicsEngine::Shutdown() {
	backgroundObject.reset();
	backgroundOverlayObject.reset();
	DebugRenderer::Shutdown();

	// Shutdown FontSystem
	FontSystem::TextRenderer::Instance().Shutdown();
	FontSystem::FontManager::Instance().Shutdown();

	resourceManager.Clear();

#ifdef _DEBUG
	// ImGui cleanup
	if (_imguiInitialized) {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		_imguiInitialized = false;
	}
#endif
}

// Transition implementation
void GraphicsEngine::StartSceneTransition(float fadeOutSeconds, float fadeInSeconds) {
	if (transitionPhase_ != TransitionPhase::None) {
		return; // already running
	}

	fadeOutTime_ = (fadeOutSeconds <= 0.0f) ? 0.001f : fadeOutSeconds;
	fadeInTime_ = (fadeInSeconds <= 0.0f) ? 0.001f : fadeInSeconds;
	transitionPhase_ = TransitionPhase::FadeOut;
	transitionTimer_ = 0.0f;
	transitionAlpha_ = 0.0f;
}

// Returns true if a transition is in progress (either fading out, hold/blackout, or fading in)
bool GraphicsEngine::IsTransitionActive() const {
	return transitionPhase_ != TransitionPhase::None;
}

// Returns true if currently in the hold/blackout phase (after fade-out completed, before fade-in starts)
bool GraphicsEngine::IsAtBlackout() const {
	return transitionPhase_ == TransitionPhase::Hold;
}

// Called by external code (e.g. SceneManager) when ready to start fade-in after scene switch
void GraphicsEngine::ContinueTransitionFadeIn() {
	if (transitionPhase_ == TransitionPhase::Hold) {
		transitionPhase_ = TransitionPhase::FadeIn;
		transitionTimer_ = 0.0f;
		transitionAlpha_ = 1.0f;
	}
}

// Update transition state; should be called every frame with delta time
void GraphicsEngine::UpdateTransition(float dt) {
	switch (transitionPhase_) {
	case TransitionPhase::None:
		transitionAlpha_ = 0.0f;
		break;
	case TransitionPhase::FadeOut:
	{
		transitionTimer_ += dt;
		float t = (fadeOutTime_ > 0.0f) ? (transitionTimer_ / fadeOutTime_) : 1.0f;
		if (t >= 1.0f) {
			t = 1.0f;
			transitionPhase_ = TransitionPhase::Hold; // wait for external scene switch
			transitionTimer_ = 0.0f;
		}

		transitionAlpha_ = t; // 0 -> 1
		break;
	}
	case TransitionPhase::Hold:
		transitionAlpha_ = 1.0f;
		break;
	case TransitionPhase::FadeIn:
	{
		transitionTimer_ += dt;
		float t = (fadeInTime_ > 0.0f) ? (transitionTimer_ / fadeInTime_) : 1.0f;
		if (t >= 1.0f) {
			t = 1.0f;
			transitionPhase_ = TransitionPhase::None;
		}

		transitionAlpha_ = 1.0f - t; // 1 -> 0
		break;
	}
	}
}

// Draw a fullscreen black quad with alpha based on current transition state, on top of the scene FBO
void GraphicsEngine::DrawTransitionOverlay() {
	if (transitionPhase_ == TransitionPhase::None || transitionAlpha_ <= 0.0f) {
		return;
	}

	Shader* fadeShader = resourceManager.GetShader("screenfade");
	Mesh* fsq = resourceManager.GetMesh("fullscreen_quad");
	if (!fadeShader || !fsq) {
		return;
	}

	// Build a model that fills the reference canvas (same as background quad)
	glm::mat4 model(1.0f);
	model = glm::translate(model, glm::vec3(kRefW * 0.5f, kRefH * 0.5f, 0.0f));
	model = glm::scale(model, glm::vec3(static_cast<float>(kRefW), static_cast<float>(kRefH), 1.0f));

	const ScopedRenderPassState passState({
		.depthTestEnabled = false,
		.depthWriteEnabled = true,
		.blendingEnabled = true
		});

	fadeShader->Use();
	fadeShader->SetModelMatrix(model);
	fadeShader->SetViewMatrix(view);
	fadeShader->SetProjectionMatrix(projection);
	fadeShader->SetColorTint(glm::vec4(0.0f, 0.0f, 0.0f, transitionAlpha_));

	fsq->Draw();
}

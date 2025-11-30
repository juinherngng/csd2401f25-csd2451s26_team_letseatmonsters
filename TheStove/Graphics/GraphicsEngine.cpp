/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GraphicsEngine.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu
					Ng Juin Herng, juinherng.ng@digipen.edu

 DESCRIPTION:		Implements initialization, default resource loading, background handling, draw calls
					and batched instanced rendering of GameObjects.

		All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <filesystem>
#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "GraphicsEngine.hpp"
#include "MeshLoader.hpp"
#include "../Core/FontSystem.hpp"
#include "../Core/LevelEditorPanelFonts.hpp"

// File-scoped state
static bool _imguiInitialized = false;

// Helper function to resolve shader paths across different build configurations
namespace {
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
}

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

	// Compute centered letterboxed viewport
	const float sx = static_cast<float>(width) / static_cast<float>(kRefW);
	const float sy = static_cast<float>(height) / static_cast<float>(kRefH);
	viewportScale_ = std::min(sx, sy);

	viewportW_ = static_cast<int>(kRefW * viewportScale_);
	viewportH_ = static_cast<int>(kRefH * viewportScale_);
	viewportX_ = (width - viewportW_) / 2;
	viewportY_ = (height - viewportH_) / 2;

	// Apply the viewport now
	glViewport(viewportX_, viewportY_, viewportW_, viewportH_);

	// Keep background quad aligned to the reference canvas
	if (backgroundObject) {
		backgroundObject->SetPosition(glm::vec3(kRefW * 0.5f, kRefH * 0.5f, 0.0f));
		backgroundObject->SetScale(glm::vec3(static_cast<float>(kRefW),
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

// Remove the background object (if present).
void GraphicsEngine::ClearBackground() {
	backgroundObject.reset();
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

	// Dockspace host window (full work area)
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags hostFlags =
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	if (ImGui::Begin("###DockSpaceHost", nullptr, hostFlags)) {
		ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
		ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), 0);
		mMainDockspaceId = dockspaceId;
	}

	ImGui::End();
	ImGui::PopStyleVar(2);
#endif
}

// Draw the Scene window and present the scene FBO texture inside it
void GraphicsEngine::DrawSceneDockWindow() {
#ifdef _DEBUG
	// Guard: bail if ImGui not initialized or no context
	if (!_imguiInitialized || ImGui::GetCurrentContext() == nullptr) {
		return;
	}

	ImGui::SetNextWindowDockID(GraphicsEngine::Instance().GetMainDockspaceID(),
							   ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Scene###SceneWindow")) {
		ImVec2 avail = ImGui::GetContentRegionAvail();
		const float targetAspect = float(kRefW) / float(kRefH);
		float w = avail.x, h = avail.y;
		float r = w / h;
		if (r > targetAspect) {
			w = h * targetAspect;
		}
		else {
			h = w / targetAspect;
		}

		// Center the image in the window
		ImVec2 cursor = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(cursor.x + (avail.x - w) * 0.5f,
								   cursor.y + (avail.y - h) * 0.5f));

		// Absolute rect for picking
		sceneImagePos_ = ImGui::GetCursorScreenPos();
		sceneImageSize_ = ImVec2(w, h);

		// Draw the FBO texture (v-flipped)
		ImGui::Image(
			(ImTextureID)(intptr_t)mSceneColor,
			ImVec2(w, h),
			ImVec2(0, 1), // uv0
			ImVec2(1, 0)  // uv1
		);

		// Invisible proxy for hover/click that exactly matches the scene image
		if (sceneImageSize_.x > 1.0f && sceneImageSize_.y > 1.0f) {
			ImGui::SetCursorScreenPos(sceneImagePos_);
			const bool pressed = ImGui::ImageButton(
				"##SceneImageBtn",
				(ImTextureID)(intptr_t)mSceneColor,
				ImVec2(sceneImageSize_.x, sceneImageSize_.y),
				ImVec2(0, 1),
				ImVec2(1, 0)
			);

			if (pressed || ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
				std::cout << "[Scene] LMB click inside Scene image\n";
			}
		}
	}

	ImGui::End();
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
bool GraphicsEngine::GetMouseWorldInScene(glm::vec2& outWorld) const {
#ifdef _DEBUG
	// Guard: If ImGui not ready, return early
	if (!_imguiInitialized || ImGui::GetCurrentContext() == nullptr) {
		(void)outWorld;
		return false;
	}

	ImVec2 imgPos = sceneImagePos_;
	ImVec2 imgSize = sceneImageSize_;

	// If the Scene window hasn't drawn this frame, reconstruct its rect
	if (imgSize.x <= 1.0f || imgSize.y <= 1.0f) {
		ImGuiWindow* sceneWin = ImGui::FindWindowByName("Scene###SceneWindow");
		if (sceneWin) {
			const ImRect c = sceneWin->InnerRect;  // screen-space
			const float availW = c.GetWidth();
			const float availH = c.GetHeight();

			const float targetAspect = float(kRefW) / float(kRefH);
			float w = availW, h = availH, r = w / h;
			if (r > targetAspect) {
				w = h * targetAspect;
			}
			else {
				h = w / targetAspect;
			}

			imgPos = ImVec2(c.Min.x + (availW - w) * 0.5f, c.Min.y + (availH - h) * 0.5f);
			imgSize = ImVec2(w, h);
		}
		else {
			ImGuiViewport* vp = ImGui::GetMainViewport();
			const float availW = vp->WorkSize.x;
			const float availH = vp->WorkSize.y;

			const float targetAspect = float(kRefW) / float(kRefH);
			float w = availW, h = availH, r = w / h;
			if (r > targetAspect) {
				w = h * targetAspect;
			}
			else {
				h = w / targetAspect;
			}

			imgPos = ImVec2(vp->WorkPos.x + (availW - w) * 0.5f, vp->WorkPos.y + (availH - h) * 0.5f);
			imgSize = ImVec2(w, h);
		}
	}

	// Mouse (absolute)
	const ImVec2 mouse = ImGui::GetMousePos();

	// Early-out if outside image rect
	if (mouse.x < imgPos.x || mouse.y < imgPos.y ||
		mouse.x > imgPos.x + imgSize.x || mouse.y > imgPos.y + imgSize.y) {
		return false;
	}

	// Local coordinates (0..size)
	const float localX = mouse.x - imgPos.x;
	const float localY = mouse.y - imgPos.y;

	// UV (0..1)
	const float u = localX / imgSize.x;
	const float v = localY / imgSize.y;

	// Pixel in reference space
	const float px = u * float(kRefW);
	const float py = v * float(kRefH);

	// Transform through inverse(V*P)
	glm::vec4 clip;
	clip.x = (px / float(kRefW)) * 2.0f - 1.0f;
	clip.y = 1.0f - (py / float(kRefH)) * 2.0f;
	clip.z = 0.0f;
	clip.w = 1.0f;

	const glm::mat4 invVP = glm::inverse(projection * view);
	const glm::vec4 world4 = invVP * clip;

	outWorld = glm::vec2(world4.x, world4.y);
	return true;
#else
	// Release: compute from GLFW mouse and letterboxed viewport
	GLFWwindow* win = glfwGetCurrentContext();
	if (!win) { return false; }

	// Mouse in window space
	double mx, my;
	glfwGetCursorPos(win, &mx, &my);

	// Check inside letterboxed viewport
	const float vx = static_cast<float>(viewportX_);
	const float vy = static_cast<float>(viewportY_);
	const float vw = static_cast<float>(viewportW_);
	const float vh = static_cast<float>(viewportH_);
	if (mx < vx || my < vy || mx > (vx + vw) || my > (vy + vh)) {
		return false;
	}

	// Local coords in viewport [0..vw],[0..vh]
	const float localX = static_cast<float>(mx) - vx;
	const float localY = static_cast<float>(my) - vy;

	// UV [0..1]
	const float u = localX / vw;
	const float v = localY / vh;

	// Pixel in reference canvas
	const float px = u * float(kRefW);
	const float py = v * float(kRefH);

	// Clip -> world using inverse(V*P) of reference canvas
	glm::vec4 clip;
	clip.x = (px / float(kRefW)) * 2.0f - 1.0f;
	clip.y = 1.0f - (py / float(kRefH)) * 2.0f;
	clip.z = 0.0f;
	clip.w = 1.0f;

	const glm::mat4 invVP = glm::inverse(projection * view);
	const glm::vec4 world4 = invVP * clip;
	outWorld = glm::vec2(world4.x, world4.y);
	return true;
#endif
}

// Default render path
void GraphicsEngine::Render(const std::vector<GameObject*>& objects, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
	// Draw background first
	if (backgroundObject) {
		glDisable(GL_DEPTH_TEST);
		Shader* shader = backgroundObject->GetShader();
		if (shader) {
			shader->Use();
			shader->SetModelMatrix(backgroundObject->GetModelMatrix());
			shader->SetViewMatrix(viewMatrix);
			shader->SetProjectionMatrix(projectionMatrix);
		}
		Texture* tex = backgroundObject->GetTexture();
		if (tex) {
			tex->Bind(0);
			shader->SetTexture("u_Texture", 0);
		}
		Mesh* mesh = backgroundObject->GetMesh();
		if (mesh) {
			mesh->Draw();
		}
		glEnable(GL_DEPTH_TEST);
	}

	// Render all scene objects
	for (const auto* obj : objects) {
		if (!obj) continue;

		Shader* shader = obj->GetShader();
		if (!shader) continue;

		shader->Use();
		shader->SetModelMatrix(obj->GetModelMatrix());
		shader->SetViewMatrix(viewMatrix);
		shader->SetProjectionMatrix(projectionMatrix);

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
			glDisable(GL_DEPTH_TEST);
			obj->DrawBoundingBox(viewMatrix, projectionMatrix, glm::vec3{ 1.0f, 0.0f, 0.0f });
			glEnable(GL_DEPTH_TEST);
		}
	}

	// Unbind scene FBO so default framebuffer can be used for final presentation
	EndSceneRender();

#ifdef _DEBUG
	// Debug: render ImGui dockspace + scene image into ImGui window
	DrawSceneDockWindow();
	EndImGuiFrame();
#else
	// Release: present the scene FBO to the default framebuffer (GLFW window)
	if (mSceneFBO != 0 && mSceneColor != 0 && screenWidth > 0 && screenHeight > 0) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, mSceneFBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		// Source rect: the whole scene FBO
		const int srcW = mSceneWidth;
		const int srcH = mSceneHeight;

		// Destination rect: SAME letterboxed region as Resize() + picking
		const int dstX0 = viewportX_;
		const int dstY0 = viewportY_;
		const int dstX1 = viewportX_ + viewportW_;
		const int dstY1 = viewportY_ + viewportH_;

		// Optional: clear full window to black, so bars look nice
		glViewport(0, 0, screenWidth, screenHeight);
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Blit the FBO into the letterboxed area
		glBlitFramebuffer(
			0, 0, srcW, srcH,
			dstX0, dstY0, dstX1, dstY1,
			GL_COLOR_BUFFER_BIT,
			GL_LINEAR
		);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
#endif


	GLenum error;
	while ((error = glGetError()) != GL_NO_ERROR) {
		std::cerr << "OpenGL error after draw call: " << error << std::endl;
	}
}

// Batched/instanced render path 
void GraphicsEngine::RenderBatched(const std::vector<GameObject*>& objects) {
	// Reset stats
	renderStats = RenderStats();
	renderStats.totalObjects = static_cast<int>(objects.size());

	// Draw background first
	if (backgroundObject) {
		glDisable(GL_DEPTH_TEST);
		Shader* shader = backgroundObject->GetShader();
		if (shader) {
			shader->Use();
			shader->SetModelMatrix(backgroundObject->GetModelMatrix());
			shader->SetViewMatrix(view);
			shader->SetProjectionMatrix(projection);
		}
		Texture* tex = backgroundObject->GetTexture();
		if (tex) {
			tex->Bind(0);
			shader->SetTexture("u_Texture", 0);
		}
		Mesh* mesh = backgroundObject->GetMesh();
		if (mesh) {
			mesh->Draw();
		}
		glEnable(GL_DEPTH_TEST);
	}

	// Early out if no objects
	if (objects.empty()) {
		// Render text objects even if no game objects
		RenderTextObjects();
		
		EndSceneRender();
#ifdef _DEBUG
		DrawSceneDockWindow();
		EndImGuiFrame();
#else
		// Blit scene FBO to default framebuffer in Release (guarded)
		if (mSceneFBO != 0 && mSceneColor != 0 && screenWidth > 0 && screenHeight > 0) {
			glBindFramebuffer(GL_READ_FRAMEBUFFER, mSceneFBO);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			const int srcW = mSceneWidth;
			const int srcH = mSceneHeight;

			const int dstX0 = viewportX_;
			const int dstY0 = viewportY_;
			const int dstX1 = viewportX_ + viewportW_;
			const int dstY1 = viewportY_ + viewportH_;

			glViewport(0, 0, screenWidth, screenHeight);
			glClearColor(0.f, 0.f, 0.f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT);

			glBlitFramebuffer(
				0, 0, srcW, srcH,
				dstX0, dstY0, dstX1, dstY1,
				GL_COLOR_BUFFER_BIT,
				GL_LINEAR
			);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
#endif

		return;
	}

	// Prefetch possible instanced shaders + animated shader pointer
	Shader* staticsInstShader = resourceManager.GetShader("staticsprite_instanced");
	Shader* animatedInstShader = resourceManager.GetShader("animatedsprite_instanced");
	Shader* animShader = resourceManager.GetShader("animatedsprite");

	// Build contiguous runs keyed by (mesh, shader, texture) - preserves layering order
	std::vector<Mesh::InstanceData> instanceBatch;
	RenderKey currentKey{ nullptr, nullptr, nullptr };

	auto flushBatch = [&](const std::vector<Mesh::InstanceData>& batch, const RenderKey& key) {
		if (batch.empty() || !key.mesh || !key.shader) return;

		const bool wantsInstancing = batch.size() >= INSTANCING_THRESHOLD;

		// Map the original shader -> preferred instanced shader 
		Shader* preferredInstanced = nullptr;
		if (key.shader == resourceManager.GetShader("staticsprite")) {
			preferredInstanced = staticsInstShader;
		}
		else if (key.shader == animShader) {
			preferredInstanced = animatedInstShader;
		}

		// If we should and can instance, use instanced path
		if (wantsInstancing && preferredInstanced) {
			key.mesh->SetupInstanceBuffer(batch);

			preferredInstanced->Use();
			preferredInstanced->SetViewMatrix(view);
			preferredInstanced->SetProjectionMatrix(projection);

			if (key.texture) {
				key.texture->Bind(0);
				preferredInstanced->SetTexture("u_Texture", 0);
			}

			key.mesh->DrawInstanced(key.texture, static_cast<GLsizei>(batch.size()));

			renderStats.drawCalls++;
			renderStats.totalBatches++;
			renderStats.instancedObjects += static_cast<int>(batch.size());
		}
		else {
			// Non-instanced fallback: draw each element with original shader so per-object uniforms work
			key.shader->Use();
			key.shader->SetViewMatrix(view);
			key.shader->SetProjectionMatrix(projection);

			if (key.texture) {
				key.texture->Bind(0);
				key.shader->SetTexture("u_Texture", 0);
			}

			for (const auto& inst : batch) {
				// per-object model matrix
				key.shader->SetModelMatrix(inst.modelMatrix);

				// If shader is animated (non-instanced), supply UV via uniforms
				if (key.shader == animShader) {
					key.shader->SetUVOffset(glm::vec2(inst.uvOffsetScale.x, inst.uvOffsetScale.y));
					key.shader->SetUVScale(glm::vec2(inst.uvOffsetScale.z, inst.uvOffsetScale.w));
				}

				key.mesh->Draw();
				renderStats.drawCalls++;
			}

			renderStats.totalBatches++;
		}
	};

	// Build runs in order
	for (auto* obj : objects) {
		if (!obj || !obj->GetMesh() || !obj->GetShader()) continue;

		RenderKey key{ obj->GetMesh(), obj->GetShader(), obj->GetTexture() };

		// Flush when render key changes
		if (key != currentKey && !instanceBatch.empty()) {
			flushBatch(instanceBatch, currentKey);
			instanceBatch.clear();
		}
		currentKey = key;

		Mesh::InstanceData inst;
		inst.modelMatrix = obj->GetModelMatrix();
		// Always store per-object UV rect in instance data (instanced shader will use it, fallback uses uniforms)
		inst.uvOffsetScale = obj->GetUVRect();
		instanceBatch.push_back(inst);
	}

	// Flush remaining batch
	if (!instanceBatch.empty()) {
		flushBatch(instanceBatch, currentKey);
		instanceBatch.clear();
	}

	// Debug bounding boxes render
	if (DebugRenderer::IsEnabled()) {
		glDisable(GL_DEPTH_TEST);
		for (const auto* obj : objects) {
			if (obj) {
				obj->DrawBoundingBox(view, projection, glm::vec3{ 1.0f, 0.0f, 0.0f });
			}
		}
		DebugRenderer::Flush(view, projection);
		glEnable(GL_DEPTH_TEST);
	}

	// Render text objects on top of scene
	RenderTextObjects();

	// End-of-frame UI and finalization
	EndSceneRender();
#ifdef _DEBUG
	DrawSceneDockWindow();
	EndImGuiFrame();
#else
	// Blit to default framebuffer (guarded) in Release
	if (mSceneFBO != 0 && mSceneColor != 0 && screenWidth > 0 && screenHeight > 0) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, mSceneFBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		const int srcW = mSceneWidth;
		const int srcH = mSceneHeight;

		const int dstX0 = viewportX_;
		const int dstY0 = viewportY_;
		const int dstX1 = viewportX_ + viewportW_;
		const int dstY1 = viewportY_ + viewportH_;

		glViewport(0, 0, screenWidth, screenHeight);
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		glBlitFramebuffer(
			0, 0, srcW, srcH,
			dstX0, dstY0, dstX1, dstY1,
			GL_COLOR_BUFFER_BIT,
			GL_LINEAR
		);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
#endif


	// OpenGL error check loop
	GLenum error;
	while ((error = glGetError()) != GL_NO_ERROR) {
		std::cerr << "[GraphicsEngine] OpenGL error in batched rendering: " << error << std::endl;
	}
}

// Render text objects
void GraphicsEngine::RenderTextObjects() {
#ifdef _DEBUG
	// In debug builds, get text from the editor panel
	const auto& textObjects = LEPANELFONTS::GetTextObjects();
	
	if (textObjects.empty()) {
		return;
	}
	
	// Create Text renderers on demand and render
	for (const auto& data : textObjects) {
		// Get the font from ResourceManager
		FontSystem::Font* font = ResourceManager::Instance().GetFont(data.fontName);
		if (!font) {
			continue;
		}
		
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
#else
	// In release build, text will be rendered from game state
#endif
}

// Free resources and shutdown ImGui
void GraphicsEngine::Shutdown() {
	backgroundObject.reset();
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


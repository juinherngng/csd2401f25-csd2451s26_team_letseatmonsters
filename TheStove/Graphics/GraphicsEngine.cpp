/*
----------------------------------------------------------------------------------------------------
FILE NAME:			GraphicsEngine.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Seah Wang Hua, wanghua.seah@digipen.edu
CO-AUTHORS:			Yat Chun Wee, y.chunwee@digipen.edu

DESCRIPTION:		Implements initialization, default resource loading, background handling, draw calls
					and batched instanced rendering of GameObjects.

		All content @ 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <iostream>
#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "GraphicsEngine.hpp"
#include "MeshLoader.hpp"

// File-scoped state
static bool _imguiInitialized = false;

GraphicsEngine& GraphicsEngine::Instance() {
	static GraphicsEngine instance;
	return instance;
}

// Constructor: Initializes references and identity matrices for view/projection
GraphicsEngine::GraphicsEngine()
	: resourceManager(ResourceManager::Instance()),
	projection(1.0f),
	view(1.0f)
{
}

// Initialize core renderer, FBO, default resources, and ImGui
void GraphicsEngine::Initialize() {
	renderer.Initialize();
	renderer.SetClearColor(0.2f, 0.3f, 0.3f, 1.0f);

	CreateSceneFBO(kRefW, kRefH);
	view = glm::mat4(1.0f);

	LoadDefaultResources();

	DebugRenderer::Init();
	DebugRenderer::SetEnabled(false);

	if (!_imguiInitialized) {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		// Slightly larger UI for readability
		io.FontGlobalScale = 1.0f;
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(1.2f);
		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForOpenGL(glfwGetCurrentContext(), true);
		ImGui_ImplOpenGL3_Init("#version 330 core");
		_imguiInitialized = true;
	}
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
const glm::mat4& GraphicsEngine::GetProjection() const { return projection; }
const glm::mat4& GraphicsEngine::GetView() const { return view; }
ImGuiID GraphicsEngine::GetMainDockspaceID() const { return mMainDockspaceId; }

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

	// Load instanced static sprite shader
	resourceManager.LoadShader("staticsprite_instanced",
		"../TheStove/Graphics/shaders/staticsprite_instanced.vert",
		"../TheStove/Graphics/shaders/staticsprite_instanced.frag");

	// Load instanced animated sprite shader
	resourceManager.LoadShader("animatedsprite_instanced",
		"../TheStove/Graphics/shaders/animatedsprite_instanced.vert",
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

// Set/ensure a fullscreen background quad using the given texture path
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

// Start a new ImGui frame and host a global DockSpace
void GraphicsEngine::BeginImGuiFrame() {
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
}

// Draw the Scene window and present the scene FBO texture inside it
void GraphicsEngine::DrawSceneDockWindow() {
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
}

// Finish the current ImGui frame and render it
void GraphicsEngine::EndImGuiFrame() {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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
}

void GraphicsEngine::Render(const std::vector<GameObject*>& objects, const glm::mat4& view, const glm::mat4& projection) {
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

	// Render all scene objects
	for (const auto* obj : objects) {
		if (!obj) continue;

		Shader* shader = obj->GetShader();
		if (!shader) continue;

		shader->Use();
		shader->SetModelMatrix(obj->GetModelMatrix());
		shader->SetViewMatrix(view);
		shader->SetProjectionMatrix(projection);

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
			obj->DrawBoundingBox(view, projection, glm::vec3{ 1.0f, 0.0f, 0.0f });
			glEnable(GL_DEPTH_TEST);
		}
	}

	EndSceneRender();
	DrawSceneDockWindow();
	EndImGuiFrame();

	GLenum error;
	while ((error = glGetError()) != GL_NO_ERROR) {
		std::cerr << "OpenGL error after draw call: " << error << std::endl;
	}
}

// Batched/instanced render path for static sprites; direct draw for animated (instanced animated not implemented yet)
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


	if (objects.empty()) {
		EndSceneRender();
		DrawSceneDockWindow();
		EndImGuiFrame();
		return;
	}

	// Separate by shader type
	std::vector<GameObject*> staticSprites;
	std::vector<GameObject*> animatedSprites;

	// Get the animatedsprite shader once for comparison
	Shader* animShader = resourceManager.GetShader("animatedsprite");

	for (auto* obj : objects) {
		if (!obj || !obj->GetMesh() || !obj->GetShader()) {
			continue;
		}

		if (obj->GetShader() == animShader) {
			animatedSprites.push_back(obj); // animated
		}
		else {
			staticSprites.push_back(obj); // static
		}
	}

	// Render static sprites with batching and instancing
	if (!staticSprites.empty()) {
		std::map<RenderKey, std::vector<GameObject*>> batches;

		for (auto* obj : staticSprites) {
			RenderKey key{ obj->GetMesh(), obj->GetShader(), obj->GetTexture() };
			batches[key].push_back(obj);
		}

		renderStats.totalBatches += static_cast<int>(batches.size());

		for (auto& [key, batch] : batches) {
			if (batch.size() >= INSTANCING_THRESHOLD) {
				// Instanced path
				renderStats.instancedObjects += static_cast<int>(batch.size());

				std::vector<glm::mat4> modelMatrices;
				modelMatrices.reserve(batch.size());
				for (const auto* obj : batch) {
					modelMatrices.push_back(obj->GetModelMatrix());
				}

				key.mesh->SetupInstanceBuffer(modelMatrices);

				Shader* instancedShader = resourceManager.GetShader("staticsprite_instanced");
				if (!instancedShader) {
					instancedShader = key.shader;
				}

				instancedShader->Use();
				instancedShader->SetViewMatrix(view);
				instancedShader->SetProjectionMatrix(projection);
				if (key.texture) {
					instancedShader->SetTexture("u_Texture", 0);
				}

				key.mesh->DrawInstanced(key.texture, batch.size());
				renderStats.drawCalls++;
			}
			else {
				// Non-instanced path
				key.shader->Use();
				key.shader->SetViewMatrix(view);
				key.shader->SetProjectionMatrix(projection);
				if (key.texture) {
					key.texture->Bind(0);
					key.shader->SetTexture("u_Texture", 0);
				}

				for (const auto* obj : batch) {
					key.shader->SetModelMatrix(obj->GetModelMatrix());
					key.mesh->Draw();
					renderStats.drawCalls++;
				}
			}
		}
	}

	// Render animated sprites (no batching/instancing, not implemented yet)
	for (const auto* obj : animatedSprites) {
		Shader* shader = obj->GetShader();
		Mesh* mesh = obj->GetMesh();
		Texture* texture = obj->GetTexture();

		shader->Use();
		shader->SetModelMatrix(obj->GetModelMatrix());
		shader->SetViewMatrix(view);
		shader->SetProjectionMatrix(projection);

		// Set UV coordinates 
		const glm::vec4 uv = obj->GetUVRect();
		shader->SetUVOffset(glm::vec2(uv.x, uv.y));
		shader->SetUVScale(glm::vec2(uv.z, uv.w));

		if (texture) {
			texture->Bind(0);
			shader->SetTexture("u_Texture", 0);
		}

		mesh->Draw();
		renderStats.drawCalls++;
	}

	// Debug bounding boxes render
	if (DebugRenderer::IsEnabled()) {
		glDisable(GL_DEPTH_TEST);
		for (const auto* obj : objects) {
			if (obj) {
				obj->DrawBoundingBox(view, projection, glm::vec3(1.0f, 0.0f, 0.0f));
			}
		}
		DebugRenderer::Flush(view, projection);
		glEnable(GL_DEPTH_TEST);
	}

	// End-of-frame UI
	EndSceneRender();
	DrawSceneDockWindow();
	EndImGuiFrame();

	// GL error check
	GLenum error;
	while ((error = glGetError()) != GL_NO_ERROR) {
		std::cerr << "[GraphicsEngine] OpenGL error in batched rendering: " << error << std::endl;
	}
}

// Free resources and shutdown ImGui
void GraphicsEngine::Shutdown() {
	backgroundObject.reset();
	DebugRenderer::Shutdown();
	resourceManager.Clear();

	// ImGui cleanup
	if (_imguiInitialized) {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		_imguiInitialized = false;
	}
}



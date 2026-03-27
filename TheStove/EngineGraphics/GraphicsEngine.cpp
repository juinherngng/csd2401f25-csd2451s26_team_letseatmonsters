/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GraphicsEngine.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (30%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(50%)
					Ng Juin Herng, juinherng.ng@digipen.edu (20%)

 DESCRIPTION:		Implements GraphicsEngine initialization, resource bootstrap, and background
					rendering helpers that remain in the core engine translation unit.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <filesystem>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "EngineCore/FontSystem.hpp"
#include "EngineCore/Logger.hpp"
#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/Mesh.hpp"
#include "EngineGraphics/MeshLoader.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/Shader.hpp"
#include "EngineGraphics/Texture.hpp"

// Shared debug-editor initialization state for the ImGui backends.
bool g_GraphicsEngineImGuiInitialized = false;

GraphicsEngine* GraphicsEngine::activeInstance_ = nullptr;

// Helper utilities used by the GraphicsEngine core bootstrap and background code.
namespace {
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

		/**
		 * @brief Destroys the `ScopedRenderPassState` instance and releases owned resources.
		 */
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

		/**
		 * @brief Constructs a `ScopedRenderPassState` instance.
		 */
		ScopedRenderPassState(const ScopedRenderPassState&) = delete;
		ScopedRenderPassState& operator=(const ScopedRenderPassState&) = delete;

	private:

		/**
		 * @brief Sets enabled.
		 * @param capability Parameter for capability.
		 * @param enabled Parameter for enabled.
		 * @return Result produced by this operation.
		 */
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

	/**
	 * @brief Resolves shader path.
	 * @param relativePathFromProjectRoot Parameter for relative path from project root.
	 * @return Result produced by this operation.
	 */
	std::string ResolveShaderPath(const std::string& relativePathFromProjectRoot) {
		// Print working directory only once
		static bool printedCwd = false;
		if (!printedCwd) {
			TS_LOG_DEBUG("[ShaderPath] Current working directory: " << std::filesystem::current_path());
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
				TS_LOG_DEBUG("[ShaderPath] Found '" << filename << "' at: " << path);
				return path;
			}
		}

		// If not found, return original path and let error handling catch it
		TS_LOG_ERROR("[ShaderPath] Shader '" << filename << "' not found in any expected location.");
		TS_LOG_ERROR("[ShaderPath] Tried paths relative to: " << std::filesystem::current_path());
		return relativePathFromProjectRoot;
	}

}

/**
 * @brief Performs instance.
 * @return Result produced by this operation.
 */
GraphicsEngine& GraphicsEngine::Instance() {
	if (activeInstance_ == nullptr) {
		throw std::runtime_error("GraphicsEngine::Instance() called before engine system registration");
	}

	return *activeInstance_;
}

// Constructor: Initializes references and identity matrices for view/projection
GraphicsEngine::GraphicsEngine()
	: resourceManager(ResourceManager::Instance()),
	projection(1.0f),
	view(1.0f) {
	activeInstance_ = this;
}

/**
 * @brief Destroys the `GraphicsEngine` instance.
 */
GraphicsEngine::~GraphicsEngine() = default;

/**
 * @brief Initializes this object.
 * @return Result produced by this operation.
 */
void GraphicsEngine::Initialize() {
	// Ensure there's a current GLFW OpenGL context before calling any GL functions.
	GLFWwindow* ctx = glfwGetCurrentContext();
	if (ctx == nullptr) {
		TS_LOG_ERROR("[GraphicsEngine] No current OpenGL context. Initialize must be called after creating/making context current.");
		return;
	}

	// Load GL function pointers as early as possible (must succeed before any gl* calls).
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		TS_LOG_ERROR("[GraphicsEngine] gladLoadGLLoader failed; no GL functions available.");
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
		TS_LOG_ERROR("[GraphicsEngine] Failed to initialize FontManager.");
	}

	if (!FontSystem::TextRenderer::Instance().Initialize()) {
		TS_LOG_ERROR("[GraphicsEngine] Failed to initialize TextRenderer.");
	}

	DebugRenderer::Init();
	DebugRenderer::SetEnabled(false);

#ifdef _DEBUG
	// Initialize ImGui only after GL loader succeeded and we have a valid context.
	if (!g_GraphicsEngineImGuiInitialized) {
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
		g_GraphicsEngineImGuiInitialized = true;
	}
#endif
}

/**
 * @brief Updates this object.
 * @param dt Frame delta time in seconds.
 * @return Result produced by this operation.
 */
void GraphicsEngine::Update(float dt) {
	// Store dt for performance tracking
	lastDt = dt;

	// Update transition state machine
	UpdateTransition(dt);

	// Note: Actual rendering is still called from main loop via BeginFrame/Render/EndFrame
	// This Update is just for system integration and performance monitoring
	(void)dt; // Suppress unused parameter warning if no other logic needed
}


/**
 * @brief Loads default resources.
 * @return Result produced by this operation.
 */
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

	// Outline-only shader (uses sprite alpha, ignores sprite RGB)
	resourceManager.LoadShader("hover_outline",
		ResolveShaderPath("../shaders/staticsprite.vert"),
		ResolveShaderPath("../shaders/hover_outline.frag"));

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

/**
 * @brief Sets background.
 * @param texturePath Parameter for texture path.
 * @return Result produced by this operation.
 */
void GraphicsEngine::SetBackground(const std::string& texturePath) {
	// Derive a unique key per path to avoid returning a cached texture
	std::string key = "background_" + std::filesystem::path(texturePath).filename().string();

	Texture* bgTexture = resourceManager.LoadTexture(key, texturePath);
	if (!bgTexture) {
		TS_LOG_ERROR("[GraphicsEngine] Failed to load background texture: " << texturePath);
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

/**
 * @brief Sets background overlay.
 * @param texturePath Parameter for texture path.
 * @return Result produced by this operation.
 */
void GraphicsEngine::SetBackgroundOverlay(const std::string& texturePath) {
	std::string key = "background_overlay_" + std::filesystem::path(texturePath).filename().string();

	Texture* overlayTexture = resourceManager.LoadTexture(key, texturePath);
	if (!overlayTexture) {
		TS_LOG_ERROR("[GraphicsEngine] Failed to load background overlay texture: " << texturePath);
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

/**
 * @brief Clears background.
 * @return Result produced by this operation.
 */
void GraphicsEngine::ClearBackground() {
	backgroundObject.reset();
}

/**
 * @brief Clears background overlay.
 * @return Result produced by this operation.
 */
void GraphicsEngine::ClearBackgroundOverlay() {
	backgroundOverlayObject.reset();
}


/**
 * @brief Renders background.
 * @param viewMatrix Parameter for view matrix.
 * @param projectionMatrix Parameter for projection matrix.
 * @return Result produced by this operation.
 */
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

/**
 * @brief Renders background overlay.
 * @param viewMatrix Parameter for view matrix.
 * @param projectionMatrix Parameter for projection matrix.
 * @return Result produced by this operation.
 */
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



/**
 * @brief Performs shutdown.
 * @return Result produced by this operation.
 */
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
	if (g_GraphicsEngineImGuiInitialized) {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		g_GraphicsEngineImGuiInitialized = false;
	}
#endif

	if (activeInstance_ == this) {
		activeInstance_ = nullptr;
	}
}

/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GraphicsEngine.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (40%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(50%)
					Ng Juin Herng, juinherng.ng@digipen.edu (10%)

 DESCRIPTION:		Declares the GraphicsEngine responsible for initialization, off-screen scene FBO,
					ImGui dockspace, background handling, and batched rendering.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include "../Core/System.hpp"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "DebugRenderer.hpp"
#include "GameObject.hpp"
#include "imgui.h"
#include "Renderer.hpp"
#include "ResourceManager.hpp"
#include "SceneViewportPresenter.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

// Forward declare text object data struct
namespace LEPANELFONTS {
	struct TextObjectData;
}

// Forward declare Scene to avoid circular dependency
class GraphicsEngine : public CoreFramework::SystemInterface {
public:
	GraphicsEngine();
	void Initialize() override;
	void Update(float dt) override;
	std::string GetName() override {
		return "GraphicsEngine";
	}

	// Singleton access (safe since CoreEngine initializes all systems before the main loop)
	static GraphicsEngine& Instance();
	void Shutdown();   // Free GPU resources and shutdown ImGui

	// Frame management
	void BeginFrame();       // Clear, bind scene FBO, begin ImGui
	void BeginImGuiFrame();  // Start ImGui frame
	void EndImGuiFrame();    // Render ImGui

	// Background
	void SetBackground(const std::string& texturePath); // Create/update fullscreen background quad
	void ClearBackground();                             // Remove background

	// Viewport and resizing
	void Resize(int width, int height);  // Recompute letterboxed viewport, keep background aligned
	int GetWidth() const {
		return screenWidth;
	}
	int GetHeight() const {
		return screenHeight;
	}
	int GetViewportX() const {
		return viewportX_;
	}
	int GetViewportY() const {
		return viewportY_;
	}
	int GetViewportW() const {
		return viewportW_;
	}
	int GetViewportH() const {
		return viewportH_;
	}
	float GetViewportScale() const {
		return viewportScale_;
	}
	void ApplyViewport() const;

	// Scene FBO management
	void BeginSceneRender();
	void EndSceneRender();
	unsigned int GetSceneColorTexture() const {
		return mSceneColor;
	} // for ImGui::Image
	int GetSceneWidth() const {
		return mSceneWidth;
	}
	int GetSceneHeight() const {
		return mSceneHeight;
	}

	// ImGui and picking
	void DrawSceneDockWindow();                           // Draws Scene window with FBO image
	bool GetMouseWorldInScene(glm::vec2& outWorld) const; // Screen->world if within Scene image
	ImGuiID GetMainDockspaceID() const;

	// Camera accessors for external use (e.g. text rendering)
	const glm::mat4& GetProjection() const;
	const glm::mat4& GetView() const;

	// Rendering
	void Render(const std::vector<GameObject*>& objects, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
	void RenderBatched(const std::vector<GameObject*>& objects);

	// Render statistics getters
	int GetTotalObjects() const {
		return renderStats.totalObjects;
	}
	int GetDrawCallCount() const {
		return renderStats.drawCalls;
	}
	int GetBatchCount() const {
		return renderStats.totalBatches;
	}
	int GetInstancedObjectCount() const {
		return renderStats.instancedObjects;
	}

	// Reference render size
	static constexpr int kRefW = 1600;
	static constexpr int kRefH = 900;

	// Returns the screen-space rect of the Scene image
	void GetSceneImageRect(ImVec2& outPos, ImVec2& outSize) const;

	// Convert world-space (editor) coordinates to screen-space inside the Scene image
	ImVec2 WorldToSceneImage(const glm::vec2& world) const;

	// Scene Transition
	void StartSceneTransition(float fadeOutSeconds = 0.35f, float fadeInSeconds = 0.35f);
	bool IsTransitionActive() const;
	bool IsAtBlackout() const;       // true when fade-out finished and overlay is fully opaque
	void ContinueTransitionFadeIn(); // call once you switched scenes to start fade-in

private:
	// Internal helper types
	struct RenderKey {
		Mesh* mesh;
		Shader* shader;
		Texture* texture;

		bool operator<(const RenderKey& other) const {
			if (mesh != other.mesh) return mesh < other.mesh;
			if (shader != other.shader) return shader < other.shader;
			return texture < other.texture;
		}

		bool operator==(const RenderKey& other) const {
			return mesh == other.mesh && shader == other.shader && texture == other.texture;
		}

		bool operator!=(const RenderKey& other) const {
			return !(*this == other);
		}
	};

	// Render statistics for the current frame, updated by RenderBatched
	struct RenderStats {
		int totalObjects = 0;
		int totalBatches = 0;
		int instancedObjects = 0;
		int drawCalls = 0;
	};

	// Scene transition phases for fade-out, hold, and fade-in
	enum class TransitionPhase {
		None,
		FadeOut,
		Hold, // fully black while the caller switches scenes
		FadeIn
	};

	// Core systems and world state
	Renderer renderer;
	ResourceManager& resourceManager;
	std::unique_ptr<GameObject> backgroundObject;
	glm::mat4 view{ 1.0f };
	glm::mat4 projection{ 1.0f };

	// Screen and viewport dimensions
	int screenWidth = kRefW;
	int screenHeight = kRefH;
	float lastDt = 0.0f;

	// Viewport / render target
	int viewportX_ = 0;
	int viewportY_ = 0;
	int viewportW_ = 0;
	int viewportH_ = 0;
	float viewportScale_ = 1.0f;

	// Off-screen framebuffer for scene rendering
	unsigned int mSceneFBO = 0;
	unsigned int mSceneColor = 0; // GL_RGBA8 color texture
	unsigned int mSceneDepth = 0; // GL_DEPTH24_STENCIL8 renderbuffer
	int mSceneWidth = 1200;
	int mSceneHeight = 800;

	// Editor scene view
	SceneViewportPresenter sceneViewportPresenter_;
	ImVec2 sceneImagePos_{ 0.0f, 0.0f };
	ImVec2 sceneImageSize_{ 0.0f, 0.0f };
	ImGuiID mMainDockspaceId = 0;

	// Rendering stats/config
	RenderStats renderStats;
	static constexpr int INSTANCING_THRESHOLD = 10;

	// Transition state
	TransitionPhase transitionPhase_ = TransitionPhase::None;
	float fadeOutTime_ = 0.0f;
	float fadeInTime_ = 0.0f;
	float transitionTimer_ = 0.0f;
	float transitionAlpha_ = 0.0f;
	float dbgFadeOutSeconds_ = 0.35f;
	float dbgFadeInSeconds_ = 0.35f;

	// Lifecycle helpers
	void LoadDefaultResources();
	void CreateSceneFBO(int w, int h);
	void DestroySceneFBO();
	void ResizeSceneFBO(int w, int h);

	// Render helpers
	void RenderTextObjects();
	void RenderSingleTextObject(const LEPANELFONTS::TextObjectData& textData);
	void EndSceneAndPresent();
	void DrawSpriteShadows(const std::vector<GameObject*>& objects, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
	void RenderBackground(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
	void PresentSceneToDefaultFramebuffer();

	// Coordinate conversion helpers
	void ComputeSceneImageRect(ImVec2& outPos, ImVec2& outSize) const;
	bool TryGetMousePositionInScene(ImVec2& outLocalPos, ImVec2& outSceneSize) const;
	glm::vec2 ScenePixelToWorld(const ImVec2& localPixel, const ImVec2& sceneSize) const;
	ImVec2 WorldToScenePixel(const glm::vec2& world, const ImVec2& scenePos, const ImVec2& sceneSize) const;

	// Misc helpers
	void UpdateTransition(float dt);
	void DrawTransitionOverlay();
	static int ParseLayerNumber(const std::string& layerName);
};

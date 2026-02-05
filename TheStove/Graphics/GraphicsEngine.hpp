/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GraphicsEngine.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu
					Ng Juin Herng, juinherng.ng@digipen.edu (10%)

 DESCRIPTION:		Declares the GraphicsEngine responsible for initialization, off-screen scene FBO,
					ImGui dockspace, background handling, and batched rendering.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "../Core/System.hpp"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "DebugRenderer.hpp"
#include "GameObject.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "Renderer.hpp"
#include "ResourceManager.hpp"

// Forward declare text object data struct
namespace LEPANELFONTS {
	struct TextObjectData;
}

class GraphicsEngine : public CoreFramework::SystemInterface {
public:
	// ----- Lifecycle -----
	GraphicsEngine();
	// SystemInterface implementation
	void Initialize() override;			// Init renderer, FBO, default resources, ImGui
	void Update(float dt) override;
	std::string GetName() override {
		return "GraphicsEngine";
	}

	static GraphicsEngine& Instance();

	void Shutdown();   // Free GPU resources and shutdown ImGui

	// ----- Per-frame workflow -----
	void BeginFrame();       // Clear, bind scene FBO, begin ImGui
	void BeginImGuiFrame();  // Start ImGui frame
	void EndImGuiFrame();    // Render ImGui

	// ----- Background management -----
	void SetBackground(const std::string& texturePath); // Create/update fullscreen background quad
	void ClearBackground();                             // Remove background

	// ----- Window / Viewport -----
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

	// ----- Scene FBO (off-screen Scene window target) -----
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

	// ----- ImGui Windows / Picking -----
	void DrawSceneDockWindow();                           // Draws Scene window with FBO image
	bool GetMouseWorldInScene(glm::vec2& outWorld) const; // Screen->world if within Scene image
	ImGuiID GetMainDockspaceID() const;

	// ----- Camera matrices -----
	const glm::mat4& GetProjection() const;
	const glm::mat4& GetView() const;

	// ----- Rendering paths -----
	void Render(const std::vector<GameObject*>& objects, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
	void RenderBatched(const std::vector<GameObject*>& objects);

	// ----- Render statistics -----
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
	bool IsAtBlackout() const;           // true when fade-out finished and overlay is fully opaque
	void ContinueTransitionFadeIn();     // call once you switched scenes to start fade-in

private:
	// Core state
	Renderer renderer;
	ResourceManager& resourceManager;

	int screenWidth = 1600;
	int screenHeight = 900;

	// Background rendering
	std::unique_ptr<GameObject> backgroundObject;

	// Camera
	glm::mat4 view{ 1.0f };
	glm::mat4 projection{ 1.0f };

	// Resources
	void LoadDefaultResources();

	// Letterboxed viewport (centered)
	int viewportX_ = 0;
	int viewportY_ = 0;
	int viewportW_ = 0;
	int viewportH_ = 0;
	float viewportScale_ = 1.0f;

	// Off-screen scene FBO
	unsigned int mSceneFBO = 0;
	unsigned int mSceneColor = 0; // GL_RGBA8 color texture
	unsigned int mSceneDepth = 0; // GL_DEPTH24_STENCIL8 renderbuffer

	int mSceneWidth = 1200;
	int mSceneHeight = 800;

	void CreateSceneFBO(int w, int h);
	void DestroySceneFBO();
	void ResizeSceneFBO(int w, int h);

	// Scene window rect (for picking)
	ImVec2 sceneImagePos_{ 0.0f, 0.0f };
	ImVec2 sceneImageSize_{ 0.0f, 0.0f };
	ImGuiID mMainDockspaceId = 0;

	// Batching helpers
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

	// Render statistics
	struct RenderStats {
		int totalObjects = 0;
		int totalBatches = 0;
		int instancedObjects = 0;
		int drawCalls = 0;
	} renderStats;

	// Instancing threshold
	static constexpr int INSTANCING_THRESHOLD = 10;

	// Text rendering
	void RenderTextObjects();

	// Render a single text object (used for layered rendering)
	void RenderSingleTextObject(const LEPANELFONTS::TextObjectData& textData);

	// Shadows
	void DrawSpriteShadows(const std::vector<GameObject*>& objects, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

	// Transition 
	enum class TransitionPhase {
		None,
		FadeOut,
		Hold,     // fully black while the caller switches scenes
		FadeIn
	};

	TransitionPhase transitionPhase_ = TransitionPhase::None;
	float fadeOutTime_ = 0.0f;
	float fadeInTime_ = 0.0f;
	float transitionTimer_ = 0.0f;
	float transitionAlpha_ = 0.0f;

	// Cached last used fade times for preview UI (debug only)
	float dbgFadeOutSeconds_ = 0.35f;
	float dbgFadeInSeconds_ = 0.35f;

	void UpdateTransition(float dt);
	void DrawTransitionOverlay();
};

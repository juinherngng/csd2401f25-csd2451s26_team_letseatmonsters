/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GraphicsEngine.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (25%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(65%)
					Ng Juin Herng, juinherng.ng@digipen.edu (10%)

 DESCRIPTION:		Declares the GraphicsEngine responsible for initialization, off-screen scene FBO,
					ImGui dockspace, background handling, and batched rendering.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "EngineCore/RuntimeTextData.hpp"
#include "EngineCore/System.hpp"
#include "EngineGraphics/Renderer.hpp"
#include "EngineGraphics/SceneViewportPresenter.hpp"
#include "imgui.h"

class GameObject;
class ResourceManager;
class Mesh;
class Shader;
class Texture;

class GraphicsEngine : public CoreFramework::SystemInterface {
public:

	/**
	 * @brief Constructs a `GraphicsEngine` instance.
	 */
	GraphicsEngine();

	/**
	 * @brief Destroys the `GraphicsEngine` instance.
	 */
	~GraphicsEngine() override;

	/**
	 * @brief Initializes this object.
	 */
	void Initialize() override;

	/**
	 * @brief Updates this object.
	 * @param dt Frame delta time in seconds.
	 */
	void Update(float dt) override;

	/**
	 * @brief Returns the stable name for this object.
	 * @return Requested value.
	 */
	std::string GetName() override {
		return "GraphicsEngine";
	}

	/**
	 * @brief Performs instance.
	 * @return Result produced by this operation.
	 */
	static GraphicsEngine& Instance();

	/**
	 * @brief Performs shutdown.
	 */
	void Shutdown();   // Free GPU resources and shutdown ImGui

	/**
	 * @brief Begins frame.
	 */
	void BeginFrame();       // Clear, bind scene FBO, begin ImGui

	/**
	 * @brief Begins im gui frame.
	 */
	void BeginImGuiFrame();  // Start ImGui frame

	/**
	 * @brief Ends im gui frame.
	 */
	void EndImGuiFrame();    // Render ImGui

	/**
	 * @brief Sets background overlay.
	 * @param texturePath Parameter for texture path.
	 */
	void SetBackgroundOverlay(const std::string& texturePath); // Create/update fullscreen overlay quad (drawn above background)

	/**
	 * @brief Sets background.
	 * @param texturePath Parameter for texture path.
	 */
	void SetBackground(const std::string& texturePath); // Create/update fullscreen background quad

	/**
	 * @brief Clears background overlay.
	 */
	void ClearBackgroundOverlay();                      // Remove overlay background

	/**
	 * @brief Clears background.
	 */
	void ClearBackground();                             // Remove background

	/**
	 * @brief Performs resize.
	 * @param width Width value in pixels.
	 * @param height Height value in pixels.
	 */
	void Resize(int width, int height);  // Recompute letterboxed viewport, keep background aligned

	/**
	 * @brief Returns width.
	 * @return Requested value.
	 */
	int GetWidth() const {
		return screenWidth;
	}

	/**
	 * @brief Returns height.
	 * @return Requested value.
	 */
	int GetHeight() const {
		return screenHeight;
	}

	/**
	 * @brief Returns viewport x.
	 * @return Requested value.
	 */
	int GetViewportX() const {
		return viewportX_;
	}

	/**
	 * @brief Returns viewport y.
	 * @return Requested value.
	 */
	int GetViewportY() const {
		return viewportY_;
	}

	/**
	 * @brief Returns viewport w.
	 * @return Requested value.
	 */
	int GetViewportW() const {
		return viewportW_;
	}

	/**
	 * @brief Returns viewport h.
	 * @return Requested value.
	 */
	int GetViewportH() const {
		return viewportH_;
	}

	/**
	 * @brief Returns viewport scale.
	 * @return Requested value.
	 */
	float GetViewportScale() const {
		return viewportScale_;
	}

	/**
	 * @brief Applies viewport.
	 */
	void ApplyViewport() const;

	/**
	 * @brief Begins scene render.
	 */
	void BeginSceneRender();

	/**
	 * @brief Ends scene render.
	 */
	void EndSceneRender();

	/**
	 * @brief Returns scene color texture.
	 * @return Requested value.
	 */
	unsigned int GetSceneColorTexture() const {
		return mSceneColor;
	} // for ImGui::Image

	/**
	 * @brief Returns scene width.
	 * @return Requested value.
	 */
	int GetSceneWidth() const {
		return mSceneWidth;
	}

	/**
	 * @brief Returns scene height.
	 * @return Requested value.
	 */
	int GetSceneHeight() const {
		return mSceneHeight;
	}

	/**
	 * @brief Draws scene dock window.
	 */
	void DrawSceneDockWindow();                           // Draws Scene window with FBO image

	/**
	 * @brief Presents the completed scene framebuffer to the final runtime/editor target.
	 */
	void PresentFrame();

	/**
	 * @brief Returns mouse world in scene.
	 * @param outWorld Output value for out world.
	 * @param mousePosOverride Parameter for mouse pos override.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool GetMouseWorldInScene(glm::vec2& outWorld, const glm::dvec2* mousePosOverride = nullptr) const; // Screen->world if within Scene image

	/**
	 * @brief Returns main dockspace id.
	 * @return Requested value.
	 */
	ImGuiID GetMainDockspaceID() const;

	/**
	 * @brief Returns projection.
	 * @return Requested value.
	 */
	const glm::mat4& GetProjection() const;

	/**
	 * @brief Returns view.
	 * @return Requested value.
	 */
	const glm::mat4& GetView() const;

	/**
	 * @brief Renders this object.
	 * @param objects Parameter for objects.
	 * @param viewMatrix Parameter for view matrix.
	 * @param projectionMatrix Parameter for projection matrix.
	 */
	void Render(const std::vector<GameObject*>& objects, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

	/**
	 * @brief Renders batched.
	 * @param objects Parameter for objects.
	 */
	void RenderBatched(const std::vector<GameObject*>& objects);

	/**
	 * @brief Returns total objects.
	 * @return Requested value.
	 */
	int GetTotalObjects() const {
		return renderStats.totalObjects;
	}

	/**
	 * @brief Returns draw call count.
	 * @return Requested value.
	 */
	int GetDrawCallCount() const {
		return renderStats.drawCalls;
	}

	/**
	 * @brief Returns batch count.
	 * @return Requested value.
	 */
	int GetBatchCount() const {
		return renderStats.totalBatches;
	}

	/**
	 * @brief Returns instanced object count.
	 * @return Requested value.
	 */
	int GetInstancedObjectCount() const {
		return renderStats.instancedObjects;
	}

	// Reference render size
	static constexpr int kRefW = 1600;
	static constexpr int kRefH = 900;

	/**
	 * @brief Returns scene image rect.
	 * @param outPos Output value for out pos.
	 * @param outSize Output value for out size.
	 */
	void GetSceneImageRect(ImVec2& outPos, ImVec2& outSize) const;

	/**
	 * @brief Performs world to scene image.
	 * @param world Parameter for world.
	 * @return Result produced by this operation.
	 */
	ImVec2 WorldToSceneImage(const glm::vec2& world) const;

	/**
	 * @brief Performs start scene transition.
	 * @param fadeOutSeconds Parameter for fade out seconds.
	 * @param fadeInSeconds Parameter for fade in seconds.
	 */
	void StartSceneTransition(float fadeOutSeconds = 0.35f, float fadeInSeconds = 0.35f);

	/**
	 * @brief Returns whether transition active.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsTransitionActive() const;

	/**
	 * @brief Returns whether the fade overlay is currently visible on screen.
	 * @return True when the transition tint alpha is greater than zero.
	 */
	bool IsTransitionOverlayVisible() const;

	/**
	 * @brief Returns whether at blackout.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsAtBlackout() const;       // true when fade-out finished and overlay is fully opaque

	/**
	 * @brief Performs continue transition fade in.
	 */
	void ContinueTransitionFadeIn(); // call once you switched scenes to start fade-in

	/**
	 * @brief Performs cancel scene transition.
	 */
	void CancelSceneTransition();    // immediately clear any active transition state

	/**
	 * @brief Sets suppress debug text rendering.
	 * @param suppress Parameter for suppress.
	 */
	void SetSuppressDebugTextRendering(bool suppress) {
		suppressDebugTextRendering_ = suppress;
	}

private:
	static GraphicsEngine* activeInstance_;

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
	std::unique_ptr<GameObject> backgroundOverlayObject;
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
	bool suppressDebugTextRendering_ = false;

	/**
	 * @brief Loads default resources.
	 */
	void LoadDefaultResources();

	/**
	 * @brief Creates scene fbo.
	 * @param w Parameter for w.
	 * @param h Parameter for h.
	 */
	void CreateSceneFBO(int w, int h);

	/**
	 * @brief Performs destroy scene fbo.
	 */
	void DestroySceneFBO();

	/**
	 * @brief Performs resize scene fbo.
	 * @param w Parameter for w.
	 * @param h Parameter for h.
	 */
	void ResizeSceneFBO(int w, int h);

	/**
	 * @brief Renders text objects.
	 */
	void RenderTextObjects();

	/**
	 * @brief Renders single text object.
	 * @param textData Parameter for text data.
	 */
	void RenderSingleTextObject(const RuntimeTextData& textData);

	/**
	 * @brief Ends scene and present.
	 */
	void EndSceneAndPresent();

	/**
	 * @brief Draws sprite shadows.
	 * @param objects Parameter for objects.
	 * @param viewMatrix Parameter for view matrix.
	 * @param projectionMatrix Parameter for projection matrix.
	 */
	void DrawSpriteShadows(const std::vector<GameObject*>& objects, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

	/**
	 * @brief Renders background.
	 * @param viewMatrix Parameter for view matrix.
	 * @param projectionMatrix Parameter for projection matrix.
	 */
	void RenderBackground(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

	/**
	 * @brief Renders background overlay.
	 * @param viewMatrix Parameter for view matrix.
	 * @param projectionMatrix Parameter for projection matrix.
	 */
	void RenderBackgroundOverlay(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

	/**
	 * @brief Performs present scene to default framebuffer.
	 */
	void PresentSceneToDefaultFramebuffer();

	/**
	 * @brief Computes scene image rect.
	 * @param outPos Output value for out pos.
	 * @param outSize Output value for out size.
	 */
	void ComputeSceneImageRect(ImVec2& outPos, ImVec2& outSize) const;

	/**
	 * @brief Attempts to get mouse position in scene.
	 * @param outLocalPos Output value for out local pos.
	 * @param outSceneSize Output value for out scene size.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool TryGetMousePositionInScene(ImVec2& outLocalPos, ImVec2& outSceneSize) const;

	/**
	 * @brief Performs scene pixel to world.
	 * @param localPixel Parameter for local pixel.
	 * @param sceneSize Parameter for scene size.
	 * @return Result produced by this operation.
	 */
	glm::vec2 ScenePixelToWorld(const ImVec2& localPixel, const ImVec2& sceneSize) const;

	/**
	 * @brief Performs world to scene pixel.
	 * @param world Parameter for world.
	 * @param scenePos Parameter for scene pos.
	 * @param sceneSize Parameter for scene size.
	 * @return Result produced by this operation.
	 */
	ImVec2 WorldToScenePixel(const glm::vec2& world, const ImVec2& scenePos, const ImVec2& sceneSize) const;

	/**
	 * @brief Updates transition.
	 * @param dt Frame delta time in seconds.
	 */
	void UpdateTransition(float dt);

	/**
	 * @brief Draws transition overlay.
	 */
	void DrawTransitionOverlay();

	/**
	 * @brief Performs parse layer number.
	 * @param layerName Parameter for layer name.
	 * @return Result produced by this operation.
	 */
	static int ParseLayerNumber(const std::string& layerName);
};

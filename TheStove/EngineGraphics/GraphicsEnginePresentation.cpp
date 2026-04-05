/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GraphicsEnginePresentation.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements GraphicsEngine ImGui/presentation flow and coordinate conversion
					helpers used by the editor scene viewport and mouse picking.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <cmath>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "EngineCore/InputManager.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"

extern bool g_GraphicsEngineImGuiInitialized;

namespace {
	constexpr int kInvalidLayerValue = 1000000;
}

/**
 * @brief Begins a new ImGui frame when the editor UI is active.
 */
void GraphicsEngine::BeginImGuiFrame() {
#ifdef _DEBUG
	// Only drive the backend when ImGui has been initialized successfully.
	if (!g_GraphicsEngineImGuiInitialized || ImGui::GetCurrentContext() == nullptr) {
		return;
	}

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Let the presenter own the dockspace layout before scene windows are drawn.
	sceneViewportPresenter_.BeginDockspaceFrame(mMainDockspaceId);
#endif
}

/**
 * @brief Parses a string layer name into a numeric sort key.
 * @param layerName Layer name to parse.
 * @return Numeric layer value, or a large fallback when parsing fails.
 */
int GraphicsEngine::ParseLayerNumber(const std::string& layerName) {
	// Treat empty names as the base gameplay layer.
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

/**
 * @brief Computes the on-screen rectangle occupied by the scene image.
 * @param outPos Output position of the rendered scene image.
 * @param outSize Output size of the rendered scene image.
 */
void GraphicsEngine::ComputeSceneImageRect(ImVec2& outPos, ImVec2& outSize) const {
	// Defer the exact placement rules to the shared viewport presenter.
	sceneViewportPresenter_.ComputeSceneImageRect(
		sceneImagePos_,
		sceneImageSize_,
		kRefW,
		kRefH,
		outPos,
		outSize
	);
}

/**
 * @brief Presents the off-screen scene framebuffer into the default window framebuffer.
 */
void GraphicsEngine::PresentSceneToDefaultFramebuffer() {
	// Bail out when the scene render target or destination window is not ready yet.
	if (mSceneFBO == 0 || mSceneColor == 0 || screenWidth <= 0 || screenHeight <= 0) {
		return;
	}

	glBindFramebuffer(GL_READ_FRAMEBUFFER, mSceneFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	// Blit the scene texture into the letterboxed runtime viewport inside the window framebuffer.
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

/**
 * @brief Draws the editor's scene viewport window.
 */
void GraphicsEngine::DrawSceneDockWindow() {
#ifdef _DEBUG
	// Guard against early calls before the editor UI has been initialized.
	if (!g_GraphicsEngineImGuiInitialized || ImGui::GetCurrentContext() == nullptr) {
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

/**
 * @brief Presents the already-rendered scene framebuffer to the editor or runtime window.
 */
void GraphicsEngine::PresentFrame() {
	EndSceneAndPresent();
}

/**
 * @brief Finalizes scene rendering and presents the frame to its final target.
 */
void GraphicsEngine::EndSceneAndPresent() {
	// Unbind the off-screen scene target before presenting to the editor or runtime window.
	EndSceneRender();
#ifdef _DEBUG
	DrawSceneDockWindow();
	EndImGuiFrame();
#else
	PresentSceneToDefaultFramebuffer();
#endif
}

/**
 * @brief Ends the current ImGui frame and submits its draw data.
 */
void GraphicsEngine::EndImGuiFrame() {
#ifdef _DEBUG
	// Skip rendering when the editor backends were not initialized.
	if (!g_GraphicsEngineImGuiInitialized || ImGui::GetCurrentContext() == nullptr) {
		return;
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
}

/**
 * @brief Begins a new frame by clearing the backbuffer and preparing scene/editor targets.
 */
void GraphicsEngine::BeginFrame() {
	// Clear the runtime backbuffer first, then set up scene rendering for the new frame.
	glViewport(0, 0, screenWidth, screenHeight);
	renderer.Clear();

	ApplyViewport();
	BeginSceneRender();
	BeginImGuiFrame();
}

/**
 * @brief Converts a mouse position into world coordinates inside the scene viewport.
 * @param outWorld Output world-space mouse position.
 * @param mousePosOverride Optional mouse position override.
 * @return `true` when the mouse lies inside the active scene image.
 */
bool GraphicsEngine::GetMouseWorldInScene(glm::vec2& outWorld, const glm::dvec2* mousePosOverride) const {
	// Use the high-level scene-space helper when the caller wants the current cursor position.
	if (mousePosOverride == nullptr) {
		ImVec2 localPos{}, sceneSize{};
		if (!TryGetMousePositionInScene(localPos, sceneSize)) {
			return false;
		}

		outWorld = ScenePixelToWorld(localPos, sceneSize);
		return true;
	}

	const glm::dvec2 mousePos = *mousePosOverride;

#ifdef _DEBUG
	// In editor builds, test against the docked scene image first.
	if (g_GraphicsEngineImGuiInitialized && ImGui::GetCurrentContext() != nullptr) {
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

	// Fall back to raw runtime viewport coordinates outside the editor path.
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

/**
 * @brief Returns the active scene image rectangle.
 * @param outPos Output position of the scene image.
 * @param outSize Output size of the scene image.
 */
void GraphicsEngine::GetSceneImageRect(ImVec2& outPos, ImVec2& outSize) const {
#ifdef _DEBUG
	// In editor builds, use the docked scene viewport rectangle.
	ComputeSceneImageRect(outPos, outSize);
#else
	// In runtime builds, the scene occupies the cached presentation viewport directly.
	outPos = ImVec2(0.0f, 0.0f);
	outSize = ImVec2(static_cast<float>(viewportW_), static_cast<float>(viewportH_));
#endif
}

/**
 * @brief Attempts to get the mouse position relative to the current scene image.
 * @param outLocalPos Output local mouse position in scene pixels.
 * @param outSceneSize Output scene image size in pixels.
 * @return `true` when the cursor lies inside the active scene image.
 */
bool GraphicsEngine::TryGetMousePositionInScene(ImVec2& outLocalPos, ImVec2& outSceneSize) const {
#ifdef _DEBUG
	if (!g_GraphicsEngineImGuiInitialized || ImGui::GetCurrentContext() == nullptr) {
		return false;
	}

	ImVec2 scenePos;
	ComputeSceneImageRect(scenePos, outSceneSize);

	// Prefer ImGui's mouse position, but fall back to InputManager during docking glitches.
	ImVec2 mouse = ImGui::GetMousePos();
	if (!std::isfinite(mouse.x) || !std::isfinite(mouse.y)) {
		const glm::dvec2 mousePos = InputManager::Get().GetMousePosition();
		mouse = ImVec2(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
	}

	if (outSceneSize.x <= 0.0f || outSceneSize.y <= 0.0f) {
		// Fall back to the runtime viewport before the docked scene image has stabilized.
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

/**
 * @brief Converts scene-local pixel coordinates into world coordinates.
 * @param localPixel Mouse position inside the scene image.
 * @param sceneSize Current scene image size.
 * @return World-space position corresponding to the pixel.
 */
glm::vec2 GraphicsEngine::ScenePixelToWorld(const ImVec2& localPixel, const ImVec2& sceneSize) const {
	// Convert the local scene pixel into reference-canvas coordinates first.
	const float u = localPixel.x / sceneSize.x;
	const float v = localPixel.y / sceneSize.y;

	const float px = u * static_cast<float>(kRefW);
	const float py = v * static_cast<float>(kRefH);

	glm::vec4 clip;
	clip.x = (px / static_cast<float>(kRefW)) * 2.0f - 1.0f;
	clip.y = 1.0f - (py / static_cast<float>(kRefH)) * 2.0f;
	clip.z = 0.0f;
	clip.w = 1.0f;

	// Unproject through the current view/projection pair to recover world space.
	const glm::mat4 invVP = glm::inverse(projection * view);
	const glm::vec4 world4 = invVP * clip;
	return glm::vec2(world4.x, world4.y);
}

/**
 * @brief Converts a world position into scene-image pixel coordinates.
 * @param world World-space position to project.
 * @param scenePos Top-left corner of the scene image.
 * @param sceneSize Size of the scene image.
 * @return Scene-image pixel position.
 */
ImVec2 GraphicsEngine::WorldToScenePixel(const glm::vec2& world, const ImVec2& scenePos, const ImVec2& sceneSize) const {
	// Project the world point into clip space using the current view/projection state.
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

/**
 * @brief Converts a world position directly into scene-image coordinates.
 * @param world World-space position to project.
 * @return Position inside the scene image in editor builds.
 */
ImVec2 GraphicsEngine::WorldToSceneImage(const glm::vec2& world) const {
#ifdef _DEBUG
	// Reuse the current scene image rectangle so gizmos align with the docked viewport.
	ImVec2 imgPos;
	ImVec2 imgSize;
	ComputeSceneImageRect(imgPos, imgSize);
	return WorldToScenePixel(world, imgPos, imgSize);
#else
	// The helper is only meaningful in editor builds where a scene image exists.
	(void)world;
	return ImVec2(0.0f, 0.0f);
#endif
}

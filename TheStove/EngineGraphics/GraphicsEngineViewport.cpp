/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GraphicsEngineViewport.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements GraphicsEngine framebuffer, viewport, and resize helpers that
					manage the off-screen scene target and presentation dimensions.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "EngineCore/Logger.hpp"
#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"

/**
 * @brief Releases the scene framebuffer and its attached GPU resources.
 */
void GraphicsEngine::DestroySceneFBO() {
	// Release the depth/stencil renderbuffer first so the framebuffer can be torn down cleanly.
	if (mSceneDepth) {
		glDeleteRenderbuffers(1, &mSceneDepth);
		mSceneDepth = 0;
	}

	// Release the color attachment texture used for off-screen scene rendering.
	if (mSceneColor) {
		glDeleteTextures(1, &mSceneColor);
		mSceneColor = 0;
	}

	// Finally destroy the framebuffer object itself.
	if (mSceneFBO) {
		glDeleteFramebuffers(1, &mSceneFBO);
		mSceneFBO = 0;
	}
}

/**
 * @brief Creates the off-screen framebuffer used for scene rendering.
 * @param w Width of the scene render target in pixels.
 * @param h Height of the scene render target in pixels.
 */
void GraphicsEngine::CreateSceneFBO(int w, int h) {
	// Rebuild the render target from scratch so color and depth attachments always match.
	DestroySceneFBO();

	glGenFramebuffers(1, &mSceneFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, mSceneFBO);

	// Allocate the color texture that will later be presented to the scene viewport or window.
	glGenTextures(1, &mSceneColor);
	glBindTexture(GL_TEXTURE_2D, mSceneColor);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mSceneColor, 0);

	// Allocate a depth/stencil buffer so world rendering keeps its existing depth assumptions.
	glGenRenderbuffers(1, &mSceneDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, mSceneDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mSceneDepth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		TS_LOG_ERROR("[GraphicsEngine] Scene framebuffer is incomplete after creation.");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	mSceneWidth = w;
	mSceneHeight = h;
}

/**
 * @brief Recreates the scene framebuffer at a new size.
 * @param w New framebuffer width in pixels.
 * @param h New framebuffer height in pixels.
 */
void GraphicsEngine::ResizeSceneFBO(int w, int h) {
	// Ignore invalid sizes that can appear transiently during minimization or docking changes.
	if (w <= 0 || h <= 0) {
		return;
	}

	CreateSceneFBO(w, h);
}

/**
 * @brief Begins rendering into the off-screen scene framebuffer.
 */
void GraphicsEngine::BeginSceneRender() {
	// Bind the scene FBO and clear all attachments so the new frame starts from a known state.
	glBindFramebuffer(GL_FRAMEBUFFER, mSceneFBO);
	glViewport(0, 0, mSceneWidth, mSceneHeight);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

/**
 * @brief Ends rendering into the off-screen scene framebuffer.
 */
void GraphicsEngine::EndSceneRender() {
	// Restore the default framebuffer so later presentation work can choose the final target.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/**
 * @brief Returns the current projection matrix used by the renderer.
 * @return Immutable projection matrix reference.
 */
const glm::mat4& GraphicsEngine::GetProjection() const {
	return projection;
}

/**
 * @brief Returns the current view matrix used by the renderer.
 * @return Immutable view matrix reference.
 */
const glm::mat4& GraphicsEngine::GetView() const {
	return view;
}

/**
 * @brief Returns the active ImGui dockspace id for the main editor layout.
 * @return Dockspace identifier tracked by the scene viewport presenter.
 */
ImGuiID GraphicsEngine::GetMainDockspaceID() const {
	return mMainDockspaceId;
}

/**
 * @brief Recomputes viewport state and projection matrices after a window resize.
 * @param width New window width in pixels.
 * @param height New window height in pixels.
 */
void GraphicsEngine::Resize(int width, int height) {
	// Ignore invalid dimensions that can happen while the window is minimized.
	if (width <= 0 || height <= 0) {
		return;
	}

	screenWidth = width;
	screenHeight = height;

	// Keep rendering in a fixed reference-space orthographic projection.
	projection = glm::ortho(
		0.0f, static_cast<float>(kRefW),
		static_cast<float>(kRefH), 0.0f,
		-1.0f, 1.0f
	);

	// Track the runtime viewport dimensions used for scene presentation and picking.
	viewportW_ = width;
	viewportH_ = height;
	viewportX_ = 0;
	viewportY_ = 0;
	viewportScale_ = std::min(
		static_cast<float>(width) / static_cast<float>(kRefW),
		static_cast<float>(height) / static_cast<float>(kRefH)
	);

	glViewport(viewportX_, viewportY_, viewportW_, viewportH_);

	// Keep background quads pinned to the authored reference canvas after the resize.
	if (backgroundObject) {
		backgroundObject->SetPosition(glm::vec3(kRefW * 0.5f, kRefH * 0.5f, 0.0f));
		backgroundObject->SetScale(glm::vec3(static_cast<float>(kRefW), static_cast<float>(kRefH), 1.0f));
	}

	if (backgroundOverlayObject) {
		backgroundOverlayObject->SetPosition(glm::vec3(kRefW * 0.5f, kRefH * 0.5f, 0.0f));
		backgroundOverlayObject->SetScale(glm::vec3(static_cast<float>(kRefW), static_cast<float>(kRefH), 1.0f));
	}
}

/**
 * @brief Applies the cached runtime viewport to the current OpenGL context.
 */
void GraphicsEngine::ApplyViewport() const {
	// Reapply the last computed viewport whenever presentation code temporarily changes it.
	glViewport(viewportX_, viewportY_, viewportW_, viewportH_);
}

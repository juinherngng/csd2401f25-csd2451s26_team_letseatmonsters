/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GraphicsEngineEffects.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements GraphicsEngine fade transition state and fullscreen effects
					draws that run after the main scene render path.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "GraphicsEngine.hpp"
#include "Mesh.hpp"
#include "ResourceManager.hpp"
#include "Shader.hpp"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

namespace {
	/**
	 * @brief Describes the GL state required for one effect pass.
	 */
	struct RenderPassConfig {
		bool depthTestEnabled = true;
		bool depthWriteEnabled = true;
		bool blendingEnabled = false;
		GLenum blendSrcRgb = GL_SRC_ALPHA;
		GLenum blendDstRgb = GL_ONE_MINUS_SRC_ALPHA;
		GLenum blendSrcAlpha = GL_SRC_ALPHA;
		GLenum blendDstAlpha = GL_ONE_MINUS_SRC_ALPHA;
	};

	/**
	 * @brief Temporarily applies an effect pass GL state and restores the previous state on scope exit.
	 */
	class ScopedRenderPassState {
	public:
		/**
		 * @brief Captures the current GL state and applies the requested effect-pass state.
		 * @param config Desired effect-pass configuration for this scope.
		 */
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
		 * @brief Restores the GL state that was active before the effect pass began.
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

		ScopedRenderPassState(const ScopedRenderPassState&) = delete;
		ScopedRenderPassState& operator=(const ScopedRenderPassState&) = delete;

	private:
		/**
		 * @brief Enables or disables a GL capability for the lifetime of the scoped pass.
		 * @param capability OpenGL capability enum to toggle.
		 * @param enabled `true` to enable the capability, `false` to disable it.
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
}

/**
 * @brief Starts a fade-out/fade-in scene transition when no transition is currently active.
 * @param fadeOutSeconds Requested fade-out duration in seconds.
 * @param fadeInSeconds Requested fade-in duration in seconds.
 */
void GraphicsEngine::StartSceneTransition(float fadeOutSeconds, float fadeInSeconds) {
	// Do not restart a transition that is already in progress.
	if (transitionPhase_ != TransitionPhase::None) {
		return;
	}

	// Clamp zero-length requests to a tiny duration so division stays well-defined.
	fadeOutTime_ = (fadeOutSeconds <= 0.0f) ? 0.001f : fadeOutSeconds;
	fadeInTime_ = (fadeInSeconds <= 0.0f) ? 0.001f : fadeInSeconds;
	transitionPhase_ = TransitionPhase::FadeOut;
	transitionTimer_ = 0.0f;
	transitionAlpha_ = 0.0f;
}

/**
 * @brief Returns whether a scene transition is currently active.
 * @return `true` when the transition state machine is in any non-idle phase.
 */
bool GraphicsEngine::IsTransitionActive() const {
	return transitionPhase_ != TransitionPhase::None;
}

/**
 * @brief Returns whether the transition has reached full blackout and is waiting for a scene swap.
 * @return `true` when the fade-out completed and the engine is holding on a fully opaque overlay.
 */
bool GraphicsEngine::IsAtBlackout() const {
	return transitionPhase_ == TransitionPhase::Hold;
}

/**
 * @brief Continues the transition into the fade-in phase after the scene swap has completed.
 */
void GraphicsEngine::ContinueTransitionFadeIn() {
	if (transitionPhase_ == TransitionPhase::Hold) {
		transitionPhase_ = TransitionPhase::FadeIn;
		transitionTimer_ = 0.0f;
		transitionAlpha_ = 1.0f;
	}
}

/**
 * @brief Immediately clears any active scene transition state.
 */
void GraphicsEngine::CancelSceneTransition() {
	transitionPhase_ = TransitionPhase::None;
	transitionTimer_ = 0.0f;
	transitionAlpha_ = 0.0f;
}

/**
 * @brief Advances the fade transition state machine for the current frame.
 * @param dt Frame delta time in seconds.
 */
void GraphicsEngine::UpdateTransition(float dt) {
	switch (transitionPhase_) {
	case TransitionPhase::None:
		transitionAlpha_ = 0.0f;
		break;

	case TransitionPhase::FadeOut:
	{
		// Increase the overlay opacity until the handoff blackout is reached.
		transitionTimer_ += dt;
		float t = (fadeOutTime_ > 0.0f) ? (transitionTimer_ / fadeOutTime_) : 1.0f;
		if (t >= 1.0f) {
			t = 1.0f;
			transitionPhase_ = TransitionPhase::Hold;
			transitionTimer_ = 0.0f;
		}

		transitionAlpha_ = t;
		break;
	}

	case TransitionPhase::Hold:
		// Stay fully black until the caller signals that the next scene is ready.
		transitionAlpha_ = 1.0f;
		break;

	case TransitionPhase::FadeIn:
	{
		// Decrease the overlay opacity until normal rendering is fully visible again.
		transitionTimer_ += dt;
		float t = (fadeInTime_ > 0.0f) ? (transitionTimer_ / fadeInTime_) : 1.0f;
		if (t >= 1.0f) {
			t = 1.0f;
			transitionPhase_ = TransitionPhase::None;
		}

		transitionAlpha_ = 1.0f - t;
		break;
	}
	}
}

/**
 * @brief Draws the fullscreen fade overlay for the active transition state.
 */
void GraphicsEngine::DrawTransitionOverlay() {
	if (transitionPhase_ == TransitionPhase::None || transitionAlpha_ <= 0.0f) {
		return;
	}

	Shader* fadeShader = resourceManager.GetShader("screenfade");
	Mesh* fullscreenQuad = resourceManager.GetMesh("fullscreen_quad");
	if (fadeShader == nullptr || fullscreenQuad == nullptr) {
		return;
	}

	// Match the authored reference-space fullscreen quad used by the scene background.
	glm::mat4 model(1.0f);
	model = glm::translate(model, glm::vec3(kRefW * 0.5f, kRefH * 0.5f, 0.0f));
	model = glm::scale(model, glm::vec3(static_cast<float>(kRefW), static_cast<float>(kRefH), 1.0f));

	const ScopedRenderPassState passState({
		.depthTestEnabled = false,
		.depthWriteEnabled = true,
		.blendingEnabled = true
		});

	// Draw the fade quad on top of the composed scene using the transition alpha as tint opacity.
	fadeShader->Use();
	fadeShader->SetModelMatrix(model);
	fadeShader->SetViewMatrix(view);
	fadeShader->SetProjectionMatrix(projection);
	fadeShader->SetColorTint(glm::vec4(0.0f, 0.0f, 0.0f, transitionAlpha_));

	fullscreenQuad->Draw();
}

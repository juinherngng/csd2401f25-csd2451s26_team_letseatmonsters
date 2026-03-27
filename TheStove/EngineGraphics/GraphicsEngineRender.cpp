/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GraphicsEngineRender.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements GraphicsEngine sprite, text, and shadow rendering paths after
					the render-stage split from the main GraphicsEngine translation unit.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <algorithm>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "EngineCore/FontSystem.hpp"
#include "EngineCore/Logger.hpp"
#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/Mesh.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/Shader.hpp"
#include "EngineGraphics/Texture.hpp"

namespace {
	constexpr int kForegroundUiLayerThreshold = 99;

	/**
	 * @brief Describes the GL state required for one render pass.
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
	 * @brief Temporarily applies a render-pass state and restores the previous one on scope exit.
	 */
	class ScopedRenderPassState {
	public:
		/**
		 * @brief Captures the current GL state and applies the requested pass configuration.
		 * @param config Desired render-pass state for the current scope.
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

			// Apply the requested state before issuing any draw calls in this pass.
			SetEnabled(GL_DEPTH_TEST, config.depthTestEnabled);
			glDepthMask(config.depthWriteEnabled ? GL_TRUE : GL_FALSE);
			SetEnabled(GL_BLEND, config.blendingEnabled);
			if (config.blendingEnabled) {
				glBlendFuncSeparate(config.blendSrcRgb, config.blendDstRgb, config.blendSrcAlpha, config.blendDstAlpha);
			}
		}

		/**
		 * @brief Restores the GL state that was active before the scoped render pass began.
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
		 * @brief Enables or disables a GL capability as part of the pass setup.
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

	/**
	 * @brief Logs and drains any pending OpenGL errors after a rendering phase.
	 * @param prefix Message prefix used for each emitted error.
	 */
	void LogOpenGLErrors(const char* prefix) {
		GLenum error = GL_NO_ERROR;
		while ((error = glGetError()) != GL_NO_ERROR) {
			TS_LOG_ERROR(prefix << ": " << error);
		}
	}

	/**
	 * @brief Returns whether a batch needs the non-instanced path because tint varies per object.
	 * @param batch Candidate instance data batch.
	 * @return `true` when per-instance color data would be lost by the instanced shader path.
	 */
	bool NeedsPerInstanceTintFallback(const std::vector<Mesh::InstanceData>& batch) {
		return std::any_of(batch.begin(), batch.end(), [](const Mesh::InstanceData& inst) {
			return inst.colorTint.x != 1.0f || inst.colorTint.y != 1.0f ||
				inst.colorTint.z != 1.0f || inst.colorTint.w < 0.999f;
			});
	}
}

/**
 * @brief Renders a frame using the per-object sprite path.
 * @param objects Scene objects to render.
 * @param viewMatrix View matrix for the current frame.
 * @param projectionMatrix Projection matrix for the current frame.
 */
void GraphicsEngine::Render(const std::vector<GameObject*>& objects, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
	// Keep internal matrices in sync so editor picking and overlays use the same camera.
	view = viewMatrix;
	projection = projectionMatrix;

	// Draw the authored background before any gameplay sprites.
	RenderBackground(viewMatrix, projectionMatrix);

	// Place soft shadows beneath the sprite layer.
	DrawSpriteShadows(objects, viewMatrix, projectionMatrix);

	const ScopedRenderPassState spritePassState({
		.depthTestEnabled = false,
		.depthWriteEnabled = true,
		.blendingEnabled = true
		});

	Shader* animatedSpriteShader = resourceManager.GetShader("animatedsprite");

	for (const auto* obj : objects) {
		// Skip invalid or incomplete renderables without disturbing frame flow.
		if (obj == nullptr) {
			continue;
		}

		Shader* shader = obj->GetShader();
		if (shader == nullptr) {
			continue;
		}

		// Bind object transforms before any sprite-specific uniforms.
		shader->Use();
		shader->SetModelMatrix(obj->GetModelMatrix());
		shader->SetViewMatrix(viewMatrix);
		shader->SetProjectionMatrix(projectionMatrix);

		// Animated sprites still rely on per-object UV window uniforms.
		if (shader == animatedSpriteShader) {
			const glm::vec4 uv = obj->GetUVRect();
			shader->SetUVOffset(glm::vec2(uv.x, uv.y));
			shader->SetUVScale(glm::vec2(uv.z, uv.w));
		}

		// Apply color tint before binding the texture so the shader sees the full sprite state.
		shader->SetColorTint(obj->GetColorTint());

		Texture* texture = obj->GetTexture();
		if (texture != nullptr) {
			texture->Bind(0);
			shader->SetTexture("u_Texture", 0);
		}

		Mesh* mesh = obj->GetMesh();
		if (mesh != nullptr) {
			mesh->Draw();
		}

		// Draw debug bounds in a separate pass so they stay visible above sprites.
		if (DebugRenderer::IsEnabled()) {
			const ScopedRenderPassState debugPassState({
				.depthTestEnabled = false,
				.depthWriteEnabled = true,
				.blendingEnabled = true
				});
			obj->DrawBoundingBox(viewMatrix, projectionMatrix, glm::vec3{ 1.0f, 0.0f, 0.0f });
		}
	}

	// Keep authored foreground overlays on top of the world layer.
	RenderBackgroundOverlay(viewMatrix, projectionMatrix);

	// Composite the fade overlay into the scene target before presenting it.
	DrawTransitionOverlay();

	EndSceneAndPresent();
	LogOpenGLErrors("[GraphicsEngine] OpenGL error after draw call");
}

/**
 * @brief Renders a frame using the batched sprite path when possible.
 * @param objects Scene objects to render.
 */
void GraphicsEngine::RenderBatched(const std::vector<GameObject*>& objects) {
	// Reset frame statistics before gathering this frame's render work.
	renderStats = RenderStats();
	renderStats.totalObjects = static_cast<int>(objects.size());

	std::vector<GameObject*> worldObjects;
	std::vector<GameObject*> foregroundUiObjects;
	worldObjects.reserve(objects.size());
	foregroundUiObjects.reserve(objects.size());

	// Keep high-layer HUD sprites above the authored foreground overlay.
	for (GameObject* obj : objects) {
		if (obj == nullptr) {
			continue;
		}

		if (obj->GetRenderLayer() >= kForegroundUiLayerThreshold) {
			foregroundUiObjects.push_back(obj);
		}
		else {
			worldObjects.push_back(obj);
		}
	}

	RenderBackground(view, projection);
	DrawSpriteShadows(worldObjects, view, projection);

	const ScopedRenderPassState spriteBatchPassState({
		.depthTestEnabled = false,
		.depthWriteEnabled = true,
		.blendingEnabled = true
		});
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Even empty scenes still need overlays to render correctly.
	if (objects.empty()) {
		RenderBackgroundOverlay(view, projection);
		DrawTransitionOverlay();
		return;
	}

	// Resolve shader variants up front so the batch flush path stays compact.
	Shader* staticInstancedShader = resourceManager.GetShader("staticsprite_instanced");
	Shader* animatedInstancedShader = resourceManager.GetShader("animatedsprite_instanced");
	Shader* staticShader = resourceManager.GetShader("staticsprite");
	Shader* animatedShader = resourceManager.GetShader("animatedsprite");

	std::vector<Mesh::InstanceData> instanceBatch;
	instanceBatch.reserve(objects.size());
	RenderKey currentKey{ nullptr, nullptr, nullptr };

	const auto flushBatch = [&](const std::vector<Mesh::InstanceData>& batch, const RenderKey& key) {
		// Ignore incomplete batch keys so partially-authored objects cannot corrupt draw state.
		if (batch.empty() || key.mesh == nullptr || key.shader == nullptr) {
			return;
		}

		const bool wantsInstancing = batch.size() >= INSTANCING_THRESHOLD;
		const bool needsPerInstanceTint = NeedsPerInstanceTintFallback(batch);

		// Promote supported sprite shaders to their instanced variants when the batch is large enough.
		Shader* preferredInstanced = nullptr;
		if (key.shader == staticShader) {
			preferredInstanced = staticInstancedShader;
		}
		else if (key.shader == animatedShader) {
			preferredInstanced = animatedInstancedShader;
		}

		const bool useInstanced = wantsInstancing && preferredInstanced != nullptr && !needsPerInstanceTint;
		if (useInstanced) {
			key.mesh->SetupInstanceBuffer(batch);
			preferredInstanced->Use();
			preferredInstanced->SetViewMatrix(view);
			preferredInstanced->SetProjectionMatrix(projection);
			if (key.texture != nullptr) {
				key.texture->Bind(0);
				preferredInstanced->SetTexture("u_Texture", 0);
			}

			key.mesh->DrawInstanced(key.texture, static_cast<GLsizei>(batch.size()));
			renderStats.drawCalls++;
			renderStats.totalBatches++;
			renderStats.instancedObjects += static_cast<int>(batch.size());
			return;
		}

		// Fall back to the per-object path when the shader feature set does not match instancing support.
		key.shader->Use();
		key.shader->SetViewMatrix(view);
		key.shader->SetProjectionMatrix(projection);
		if (key.texture != nullptr) {
			key.texture->Bind(0);
			key.shader->SetTexture("u_Texture", 0);
		}

		for (const Mesh::InstanceData& inst : batch) {
			key.shader->SetModelMatrix(inst.modelMatrix);
			if (key.shader == animatedShader) {
				key.shader->SetUVOffset(glm::vec2(inst.uvOffsetScale.x, inst.uvOffsetScale.y));
				key.shader->SetUVScale(glm::vec2(inst.uvOffsetScale.z, inst.uvOffsetScale.w));
			}

			key.shader->SetColorTint(inst.colorTint);
			key.mesh->Draw();
			renderStats.drawCalls++;
		}

		renderStats.totalBatches++;
		};

	const auto renderSubset = [&](const std::vector<GameObject*>& subset) {
		instanceBatch.clear();
		currentKey = RenderKey{ nullptr, nullptr, nullptr };

		// Preserve scene layering by only batching contiguous runs with the same render key.
		for (GameObject* obj : subset) {
			if (obj == nullptr) {
				continue;
			}

			Mesh* mesh = obj->GetMesh();
			Shader* shader = obj->GetShader();
			if (mesh == nullptr || shader == nullptr) {
				continue;
			}

			RenderKey key{ mesh, shader, obj->GetTexture() };
			if (key != currentKey && !instanceBatch.empty()) {
				flushBatch(instanceBatch, currentKey);
				instanceBatch.clear();
			}

			currentKey = key;

			// Capture every object's transform, UV window, and tint so the flush path can choose how to draw it.
			instanceBatch.emplace_back();
			Mesh::InstanceData& inst = instanceBatch.back();
			inst.modelMatrix = obj->GetModelMatrix();
			inst.uvOffsetScale = obj->GetUVRect();
			inst.colorTint = obj->GetColorTint();
		}

		if (!instanceBatch.empty()) {
			flushBatch(instanceBatch, currentKey);
		}
	};

	renderSubset(worldObjects);

	// Foreground art should still sit above the batched world content.
	RenderBackgroundOverlay(view, projection);
	renderSubset(foregroundUiObjects);

	if (DebugRenderer::IsEnabled()) {
		const ScopedRenderPassState debugPassState({
			.depthTestEnabled = false,
			.depthWriteEnabled = true,
			.blendingEnabled = true
			});
		for (const GameObject* obj : objects) {
			if (obj != nullptr) {
				obj->DrawBoundingBox(view, projection, glm::vec3{ 1.0f, 0.0f, 0.0f });
			}
		}

		// Flush any shared debug primitives after the object-local boxes are queued.
		DebugRenderer::Flush(view, projection);
	}

	DrawTransitionOverlay();
	LogOpenGLErrors("[GraphicsEngine] OpenGL error in batched rendering");
}

/**
 * @brief Renders one debug/editor text object into the current scene target.
 * @param data Text object description sourced from scene/runtime authored text state.
 */
void GraphicsEngine::RenderSingleTextObject(const RuntimeTextData& data) {
	// Ignore hidden or empty entries so the editor can leave placeholder rows in the list.
	if (!data.visible || data.text.empty()) {
		return;
	}

	FontSystem::Font* font = ResourceManager::Instance().GetFont(data.fontName);
	if (font == nullptr) {
		return;
	}

	const ScopedRenderPassState textPassState({
		.depthTestEnabled = false,
		.depthWriteEnabled = true,
		.blendingEnabled = true
		});

	// Build a transient text descriptor from the serialized panel data for this frame only.
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

	FontSystem::TextRenderer::Instance().RenderText(textRenderer, projection);
}

/**
 * @brief Renders all editor-managed text objects in sorted layer order.
 */
void GraphicsEngine::RenderTextObjects() {
	// Runtime/authored text rendering is scene-owned; GraphicsEngine no longer pulls it from editor globals.
}

/**
 * @brief Draws blob-style sprite shadows beneath objects that opt into shadow rendering.
 * @param objects Scene objects to inspect for shadow data.
 * @param viewMatrix View matrix for the current frame.
 * @param projectionMatrix Projection matrix for the current frame.
 */
void GraphicsEngine::DrawSpriteShadows(const std::vector<GameObject*>& objects, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
	Shader* shadowShader = resourceManager.GetShader("shadow");
	Mesh* quad = resourceManager.GetMesh("sprite");
	if (shadowShader == nullptr || quad == nullptr) {
		return;
	}

	const ScopedRenderPassState passState({
		.depthTestEnabled = false,
		.depthWriteEnabled = false,
		.blendingEnabled = true
		});
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Bind shared shadow shader state once before iterating through the shadow casters.
	shadowShader->Use();
	shadowShader->SetViewMatrix(viewMatrix);
	shadowShader->SetProjectionMatrix(projectionMatrix);

	for (const GameObject* obj : objects) {
		if (obj == nullptr || !obj->HasShadow()) {
			continue;
		}

		const glm::vec3 position = obj->GetPositionGLM();
		const glm::vec2 size = obj->GetShadowSize();
		const glm::vec2 offset = obj->GetShadowOffset();
		const float opacity = obj->GetShadowOpacity();

		// Position a scaled quad under the sprite using the object's authored shadow settings.
		glm::mat4 model(1.0f);
		model = glm::translate(model, glm::vec3(position.x + offset.x, position.y + offset.y, position.z));
		model = glm::scale(model, glm::vec3(size.x, size.y, 1.0f));

		const float axisYOverX = (size.x != 0.0f) ? (size.y / size.x) : 1.0f;
		shadowShader->SetModelMatrix(model);
		shadowShader->SetColorTint(glm::vec4(0.0f, 0.0f, 0.0f, opacity));
		shadowShader->SetUVScale(glm::vec2(1.0f, axisYOverX));

		quad->Draw();
	}
}

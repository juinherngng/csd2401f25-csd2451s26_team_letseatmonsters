/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManagerEffects.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements Scene runtime visual feedback helpers such as customer payment
					effects, animated temporary sprites, and floating world text rendering.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>

#include "EngineCore/FilePaths.hpp"
#include "EngineCore/FontSystem.hpp"
#include "EngineGraphics/AnimationManager.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"

namespace {
	// -------------------------------------------------------------------------------------------------
	// Local Effect Helpers
	// -------------------------------------------------------------------------------------------------

	constexpr char kUiButtonStarSheetPath[] = "../assets/staranim-Sheet2.png";
	constexpr int kUiButtonStarSheetCols = 11;
	constexpr int kUiButtonStarSheetRows = 2;
	constexpr float kUiButtonStarFrameAspectFallback =
		(4000.0f * static_cast<float>(kUiButtonStarSheetRows)) /
		(227.0f * static_cast<float>(kUiButtonStarSheetCols));

	/**
	 * @brief Builds the sprite-sheet UVs used by the customer payment star burst.
	 * @return Ordered frame rectangles for the temporary payment animation.
	 */
	std::vector<glm::vec4> CreateCustomerPaymentStarFrames() {
		constexpr int totalCols = kUiButtonStarSheetCols;
		constexpr int totalRows = kUiButtonStarSheetRows;
		const float frameW = 1.0f / static_cast<float>(totalCols);
		const float frameH = 1.0f / static_cast<float>(totalRows);

		std::vector<glm::vec4> frames;
		frames.reserve(16);

		for (int col = 0; col < 11; ++col) {
			frames.emplace_back(col * frameW, 1.0f * frameH, frameW, frameH);
		}

		for (int col = 0; col < 4; ++col) {
			frames.emplace_back(col * frameW, 0.0f, frameW, frameH);
		}

		return frames;
	}

	/**
	 * @brief Builds the sprite-sheet UVs used by the blue-button hover burst.
	 * @return Ordered frame rectangles for the temporary UI star animation.
	 */
	std::vector<glm::vec4> CreateUiButtonStarFrames() {
		return CreateCustomerPaymentStarFrames();
	}

	/**
	 * @brief Returns the authored aspect ratio of a single hover-burst frame.
	 * @return Frame width divided by frame height.
	 */
	float GetUiButtonStarFrameAspect() {
		Texture* texture = ResourceManager::Instance().LoadTexture(
			"animatedsprite_" + std::string(kUiButtonStarSheetPath),
			kUiButtonStarSheetPath);
		if (!texture || texture->GetWidth() <= 0 || texture->GetHeight() <= 0) {
			return kUiButtonStarFrameAspectFallback;
		}

		return (static_cast<float>(texture->GetWidth()) * static_cast<float>(kUiButtonStarSheetRows)) /
			(static_cast<float>(texture->GetHeight()) * static_cast<float>(kUiButtonStarSheetCols));
	}

	/**
	 * @brief Computes a hover-burst size that preserves the animation frame aspect.
	 * @param buttonScale Current button render scale.
	 * @return Width/height for the spawned hover effect.
	 */
	glm::vec2 ComputeUiButtonHoverEffectSize(const glm::vec3& buttonScale) {
		const float frameAspect = std::max(GetUiButtonStarFrameAspect(), 0.0001f);
		const float minWidth = buttonScale.x * 1.6f;
		const float minHeight = buttonScale.y * 1.4f;
		const float effectHeight = std::max(minHeight, minWidth / frameAspect);
		return glm::vec2(effectHeight * frameAspect, effectHeight);
	}

	/**
	 * @brief Estimates rendered text width using glyph advance metrics when available.
	 * @param text Text to measure.
	 * @param font Font used for rendering.
	 * @param scale Render scale to apply.
	 * @return Estimated width in scene-space units.
	 */
	float EstimateTextWidth(const std::string& text, FontSystem::Font* font, float scale) {
		if (!font) {
			return static_cast<float>(text.size()) * 18.0f * scale;
		}

		float width = 0.0f;
		for (char c : text) {
			const FontSystem::Character* ch = font->GetCharacter(c);
			if (ch) {
				width += static_cast<float>(ch->advance >> 6) * scale;
			}
		}

		return width;
	}
} // namespace

// -------------------------------------------------------------------------------------------------
// Runtime Effect Creation
// -------------------------------------------------------------------------------------------------

/**
 * @brief Spawns animated and text feedback for customer payment events.
 * @param tableObjectID Identifier of the table where the payment occurred.
 * @param amount Currency amount earned from the payment.
 */
void Scene::TriggerCustomerPaymentFeedback(int tableObjectID, int amount) {
	GameObject* tableObj = GetGameObjectByID(tableObjectID);
	if (!tableObj) {
		return;
	}

	FontSystem::Font* font = ResourceManager::Instance().GetFont("payment_popup_font");
	if (!font) {
		font = FontSystem::FontManager::Instance().LoadFont(
			"payment_popup_font",
			FilePaths::Fonts::TO_THE_POINT,
			72);
	}

	static const std::vector<glm::vec4> kStarFrames = CreateCustomerPaymentStarFrames();
	constexpr float kFrameDuration = 0.045f;
	const float kLifetime = static_cast<float>(kStarFrames.size()) * kFrameDuration + 0.05f;

	const glm::vec3 tablePos = tableObj->GetPositionGLM();
	const std::string layer = GetObjectLayer(tableObjectID).empty() ? "6" : GetObjectLayer(tableObjectID);

	if (amount > 0) {
		GameObject* fx = SpawnAnimatedSprite(
			"../assets/staranim-Sheet.png",
			glm::vec3(tablePos.x, tablePos.y - 12.0f, tablePos.z),
			glm::vec2(240.0f, 240.0f),
			kStarFrames,
			kFrameDuration,
			false,
			layer);

		if (fx) {
			// These feedback sprites are purely visual and should never participate in gameplay systems.
			fx->SetColliderSize(Math::Vector2D(0.0f, 0.0f));
			fx->SetMovableByPhysics(false);
			fx->EnableShadow(false);
			fx->SetRenderSortOrder(350);

			runtimeAnimatedFx_.push_back(RuntimeAnimatedFx{
				fx->GetID(),
				fx->GetScaleGLM(),
				0.0f,
				kLifetime,
				true });
		}
	}

	const std::string amountText = (amount > 0)
		? ("+" + std::to_string(amount))
		: "0";

	floatingWorldTextFx_.push_back(FloatingWorldTextFx{
		amountText,
		glm::vec2(tablePos.x, tablePos.y - 28.0f),
		glm::vec2(0.0f, -60.0f),
		amount > 0 ? glm::vec4(1.0f, 0.92f, 0.30f, 1.0f)
				   : glm::vec4(0.85f, 0.85f, 0.85f, 1.0f),
		1.0f,
		0.0f,
		0.9f,
		layer });
}

/**
 * @brief Spawns a temporary star burst on top of a hovered blue UI button.
 * @param buttonObjectID Identifier of the hovered button.
 */
void Scene::TriggerUiButtonHoverFeedback(int buttonObjectID) {
	GameObject* buttonObj = GetGameObjectByID(buttonObjectID);
	if (!buttonObj) {
		return;
	}

	animationManager.Play();

	static const std::vector<glm::vec4> kUiStarFrames = CreateUiButtonStarFrames();
	constexpr float kFrameDuration = 0.04f;
	const float kLifetime = static_cast<float>(kUiStarFrames.size()) * kFrameDuration + 0.05f;

	const glm::vec3 buttonPos = buttonObj->GetPositionGLM();
	const glm::vec3 buttonScale = buttonObj->GetScaleGLM();
	const std::string layer = GetObjectLayer(buttonObjectID).empty() ? "10" : GetObjectLayer(buttonObjectID);
	const glm::vec3 effectPos(
		buttonPos.x,
		buttonPos.y - buttonScale.y * 0.52f,
		buttonPos.z);
	const glm::vec2 effectSize = ComputeUiButtonHoverEffectSize(buttonScale);

	GameObject* fx = SpawnAnimatedSprite(
		kUiButtonStarSheetPath,
		effectPos,
		effectSize,
		kUiStarFrames,
		kFrameDuration,
		false,
		layer);

	if (!fx) {
		return;
	}

	fx->SetColliderSize(Math::Vector2D(0.0f, 0.0f));
	fx->SetMovableByPhysics(false);
	fx->EnableShadow(false);
	fx->SetRenderSortOrder(std::max(buttonObj->GetRenderSortOrder() + 1, 500));
	fx->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, 0.95f));

	runtimeAnimatedFx_.push_back(RuntimeAnimatedFx{
		fx->GetID(),
		fx->GetScaleGLM(),
		0.0f,
		kLifetime,
		false });
}

// -------------------------------------------------------------------------------------------------
// Runtime Effect Updates And Rendering
// -------------------------------------------------------------------------------------------------

/**
 * @brief Advances and cleans up temporary animated visual effects.
 * @param dt Frame delta time in seconds.
 */
void Scene::UpdateRuntimeAnimatedFx(float dt) {
	runtimeAnimatedFx_.erase(
		std::remove_if(runtimeAnimatedFx_.begin(), runtimeAnimatedFx_.end(),
			[&](RuntimeAnimatedFx& fx) {
				GameObject* obj = GetGameObjectByID(fx.objectId);
				if (!obj) {
					return true;
				}

				fx.elapsed += dt;
				const float t = std::clamp(fx.elapsed / std::max(fx.lifetime, 0.0001f), 0.0f, 1.0f);

				if (fx.animateScale) {
					const float scaleMul = 0.85f + 0.30f * t;
					obj->SetScale(glm::vec3(
						fx.baseScale.x * scaleMul,
						fx.baseScale.y * scaleMul,
						fx.baseScale.z));
				}
				else {
					obj->SetScale(fx.baseScale);
				}

				float alpha = 1.0f;
				if (t > 0.70f) {
					alpha = 1.0f - ((t - 0.70f) / 0.30f);
				}
				obj->SetColorTint(glm::vec4(1.0f, 1.0f, 1.0f, std::clamp(alpha, 0.0f, 1.0f)));

				if (fx.elapsed >= fx.lifetime) {
					if (GetGameObjectByID(fx.objectId)) {
						DespawnByID(fx.objectId);
					}
					return true;
				}
				return false;
			}),
		runtimeAnimatedFx_.end());
}

/**
 * @brief Advances transient floating text feedback and removes expired entries.
 * @param dt Frame delta time in seconds.
 */
void Scene::UpdateFloatingWorldTextFx(float dt) {
	for (auto& fx : floatingWorldTextFx_) {
		fx.elapsed += dt;
		fx.pos += fx.velocity * dt;
	}

	floatingWorldTextFx_.erase(
		std::remove_if(floatingWorldTextFx_.begin(), floatingWorldTextFx_.end(),
			[](const FloatingWorldTextFx& fx) {
				return fx.elapsed >= fx.lifetime;
			}),
		floatingWorldTextFx_.end());
}

/**
 * @brief Renders the active floating world-text feedback for the current frame.
 * @param projection Projection matrix used by the text renderer.
 * @param pauseActive Whether the pause overlay is currently active.
 * @param cutsceneActive Whether any cutscene is currently active.
 */
void Scene::RenderFloatingWorldTextFx(const glm::mat4& projection, bool pauseActive, bool cutsceneActive) {
	if (pauseActive || cutsceneActive) {
		return;
	}

	FontSystem::Font* font = ResourceManager::Instance().GetFont("payment_popup_font");
	if (!font) {
		return;
	}

	for (const auto& fx : floatingWorldTextFx_) {
		if (!fx.layer.empty()) {
			Layer* layer = GetLayer(fx.layer);
			if (layer && (!layer->IsEnabled() || !layer->IsVisible())) {
				continue;
			}
		}

		const float t = std::clamp(fx.elapsed / std::max(fx.lifetime, 0.0001f), 0.0f, 1.0f);
		const float alpha = 1.0f - t;
		const float scale = fx.baseScale * (1.0f + 0.10f * t);

		FontSystem::Text text;
		text.SetFont(font);
		text.SetText(fx.text);
		text.SetColor(glm::vec4(fx.color.r, fx.color.g, fx.color.b, fx.color.a * alpha));
		text.SetScale(scale);
		text.SetRotationMode(FontSystem::Text::RotationMode::Block);

		const float textWidth = EstimateTextWidth(fx.text, font, scale);
		text.SetPosition(glm::vec2(fx.pos.x - textWidth * 0.5f, fx.pos.y));

		FontSystem::TextRenderer::Instance().RenderText(text, projection);
	}
}

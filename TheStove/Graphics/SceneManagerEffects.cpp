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

#include "../Core/FilePaths.hpp"
#include "../Core/FontSystem.hpp"

#include "ResourceManager.hpp"
#include "SceneManager.hpp"

#include <algorithm>

namespace {
	std::vector<glm::vec4> CreateCustomerPaymentStarFrames() {
		constexpr int totalCols = 11;
		constexpr int totalRows = 2;
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
}

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
			72
		);
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
			glm::vec2(160.0f, 160.0f),
			kStarFrames,
			kFrameDuration,
			false,
			layer
		);

		if (fx) {
			fx->SetColliderSize(Math::Vector2D(0.0f, 0.0f));
			fx->SetMovableByPhysics(false);
			fx->EnableShadow(false);
			fx->SetRenderSortOrder(350);

			runtimeAnimatedFx_.push_back(RuntimeAnimatedFx{
				fx->GetID(),
				fx->GetScaleGLM(),
				0.0f,
				kLifetime
				});
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
		layer
		});
}

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

				const float scaleMul = 0.85f + 0.30f * t;
				obj->SetScale(glm::vec3(
					fx.baseScale.x * scaleMul,
					fx.baseScale.y * scaleMul,
					fx.baseScale.z
				));

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
		runtimeAnimatedFx_.end()
	);
}

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
		floatingWorldTextFx_.end()
	);
}

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


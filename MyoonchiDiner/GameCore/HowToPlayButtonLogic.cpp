/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         HowToPlayButtonLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (50%)
 CO-AUTHOR:         Seah Wang Hua, wanghua.seah@digipen.edu (50%)

 DESCRIPTION:       Handles hover and click behaviour for the How To Play
					button in the main menu. On click, it spawns a fullscreen
					HowToPlay overlay image with a top-right Next button.
					Clicking Next advances through 3 pages, then closes the overlay.
					Esc also closes the overlay.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <array>

#include "EngineCore/FilePaths.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/HowToPlayButtonLogic.hpp"
#include "MyoonchiDiner/GamePaths.hpp"

#ifndef _DEBUG
namespace {
	static constexpr std::array<const char*, 3> kHowToPlayPages{
		"../assets/HowToPlay/howtoplay_1.png",
		"../assets/HowToPlay/howtoplay_2.png",
		"../assets/HowToPlay/howtoplay_3.png"
	};

	static constexpr const char* kNextButtonTexNormal = "../assets/HowToPlay/next_s.png";
	static constexpr const char* kNextButtonTexHover = "../assets/HowToPlay/next_h.png";

	static const glm::vec2 kNextButtonSize{ 219.75f, 105.75f };
	static const glm::vec2 kNextButtonMargin{ 200.0f, 140.0f }; // from bottom-right corner

	static std::string MakeHoverPath(const std::string& path) {
		if (path.empty()) return path;

		const size_t dot = path.find_last_of('.');
		const std::string ext = (dot != std::string::npos) ? path.substr(dot) : std::string();
		const std::string base = (dot != std::string::npos) ? path.substr(0, dot) : path;

		if (base.size() >= 2 && base.substr(base.size() - 2) == "_h") {
			return dot != std::string::npos ? path : (base + ext);
		}

		if (base.size() >= 2 && base.substr(base.size() - 2) == "_s") {
			return base.substr(0, base.size() - 2) + "_h" + ext;
		}

		return base + "_h" + ext;
	}

	static void TrySetTexture(GameObject* owner, const std::string& texPath) {
		if (!owner || texPath.empty()) return;

		std::string cacheName = "staticsprite_" + texPath;
		if (Texture* tex = ResourceManager::Instance().LoadTexture(cacheName, texPath)) {
			owner->SetTexture(tex);
		}
	}

	static bool IsPointInRect(const glm::vec2& p, const glm::vec2& min, const glm::vec2& max) {
		return p.x >= min.x && p.x <= max.x &&
			p.y >= min.y && p.y <= max.y;
	}

	static bool IsPointInObject(Scene& scene, int objectID, const glm::vec2& p) {
		GameObject* obj = scene.GetGameObjectByID(objectID);
		if (!obj) {
			return false;
		}

		const glm::vec3 pos = obj->GetPositionGLM();
		const glm::vec3 sz = obj->GetScaleGLM();
		const glm::vec2 min(pos.x - sz.x * 0.5f, pos.y - sz.y * 0.5f);
		const glm::vec2 max(pos.x + sz.x * 0.5f, pos.y + sz.y * 0.5f);

		return IsPointInRect(p, min, max);
	}

	static glm::vec2 GetMouseWorld(InputManager& input) {
		glm::vec2 mouseWorld{};
		if (!GraphicsEngine::Instance().GetMouseWorldInScene(mouseWorld)) {
			const glm::vec3 w = input.ScreenToWorld(
				static_cast<float>(input.GetMousePosition().x),
				static_cast<float>(input.GetMousePosition().y));
			mouseWorld = glm::vec2(w.x, w.y);
		}
		return mouseWorld;
	}
}
#endif // _DEBUG

void HowToPlayButtonLogic::Update(float /*dt*/, Scene& scene, InputManager& input) {
#ifdef _DEBUG
	(void)scene;
	(void)input;
#else
	GameObject* owner = GetOwner(scene);
	if (!owner) {
		return;
	}

	auto closeOverlay = [&]() {
		if (overlayId_ >= 0) {
			scene.DespawnByID(overlayId_);
			overlayId_ = -1;
		}
		if (nextButtonId_ >= 0) {
			scene.DespawnByID(nextButtonId_);
			nextButtonId_ = -1;
		}

		currentPage_ = 0;
		nextButtonHovered_ = false;
		scene.SetHowToPlayOverlayActive(false);
		};

	const bool overlayActive = scene.IsHowToPlayOverlayActive();

	if (!overlayActive && scene.IsMenuModalActive()) {
		if (hovered_) {
			hovered_ = false;
			TrySetTexture(owner, normalTexturePath_);
		}
		return;
	}

	// If overlay is active but this instance did not spawn it, ignore input.
	if (overlayActive && overlayId_ < 0) {
		return;
	}

	// --- CASE 1: overlay active -> Next to advance, Esc to close ---
	if (overlayActive) {
		const glm::vec2 mouseWorld = GetMouseWorld(input);
		const bool overNextButton = (nextButtonId_ >= 0) && IsPointInObject(scene, nextButtonId_, mouseWorld);

		if (overNextButton != nextButtonHovered_) {
			nextButtonHovered_ = overNextButton;

			if (GameObject* nextBtn = scene.GetGameObjectByID(nextButtonId_)) {
				TrySetTexture(nextBtn, nextButtonHovered_ ? kNextButtonTexHover : kNextButtonTexNormal);
			}

			if (nextButtonHovered_ && audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_HOVER)) {
				scene.TriggerUiButtonHoverFeedback(nextButtonId_);
				audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_HOVER, audioManager_->GetVfxVolume(), false);
			}
		}

		const bool click = input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);
		const bool escClose = input.IsKeyJustPressed(GLFW_KEY_ESCAPE);

		if (escClose) {
			if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_BACK)) {
				audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_BACK, audioManager_->GetVfxVolume(), false);
			}
			closeOverlay();
			input.ConsumeNextKeyPress(GLFW_KEY_ESCAPE);
			return;
		}

		if (!click) {
			return;
		}

		// Consume all clicks while overlay is open.
		input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);

		if (!overNextButton) {
			return;
		}

		if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON)) {
			audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON, audioManager_->GetVfxVolume(), false);
		}

		// Advance page or close on last page
		if (currentPage_ + 1 < static_cast<int>(kHowToPlayPages.size())) {
			++currentPage_;
			if (GameObject* overlay = scene.GetGameObjectByID(overlayId_)) {
				TrySetTexture(overlay, kHowToPlayPages[static_cast<size_t>(currentPage_)]);
			}
		}
		else {
			closeOverlay();
		}
		return;
	}

	// --- CASE 2: overlay not active -> regular button hover + click to show page 1 ---

	if (!initialized_) {
		normalTexturePath_ = scene.GetObjectTexturePath(GetOwnerID());
		hoverTexturePath_ = MakeHoverPath(normalTexturePath_);
		initialized_ = true;
	}

	const glm::vec2 mouseWorld = GetMouseWorld(input);

	const glm::vec3 pos = owner->GetPositionGLM();
	const glm::vec3 sz = owner->GetScaleGLM();
	const float halfW = sz.x * 0.5f;
	const float halfH = sz.y * 0.5f;

	const bool over =
		mouseWorld.x >= (pos.x - halfW) && mouseWorld.x <= (pos.x + halfW) &&
		mouseWorld.y >= (pos.y - halfH) && mouseWorld.y <= (pos.y + halfH);

	if (over && !hovered_) {
		hovered_ = true;
		TrySetTexture(owner, hoverTexturePath_);
		scene.TriggerUiButtonHoverFeedback(GetOwnerID());
		if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_HOVER)) {
			audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_HOVER, audioManager_->GetVfxVolume(), false);
		}
	}
	else if (!over && hovered_) {
		hovered_ = false;
		TrySetTexture(owner, normalTexturePath_);
	}

	if (!over || !input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		return;
	}

	if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON)) {
		audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON, audioManager_->GetVfxVolume(), false);
	}

	input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);

	const std::string overlayLayer = "9999998";
	const std::string nextButtonLayer = "9999999";
	const float w = static_cast<float>(GraphicsEngine::kRefW);
	const float h = static_cast<float>(GraphicsEngine::kRefH);

	GameObject* overlay = scene.SpawnStaticSprite(
		kHowToPlayPages[0],
		{ w * 0.5f, h * 0.5f, 0.0f },
		{ w, h },
		overlayLayer);

	if (!overlay) {
		return;
	}

	const glm::vec3 nextPos{
	w - kNextButtonMargin.x - (kNextButtonSize.x * 0.5f),
	kNextButtonMargin.y + (kNextButtonSize.y * 0.5f),
	0.0f
	};

	GameObject* nextBtn = scene.SpawnStaticSprite(
		kNextButtonTexNormal,
		nextPos,
		kNextButtonSize,
		nextButtonLayer);

	if (!nextBtn) {
		scene.DespawnByID(overlay->GetID());
		return;
	}
	overlayId_ = overlay->GetID();
	nextButtonId_ = nextBtn->GetID();
	currentPage_ = 0;
	nextButtonHovered_ = false;

	overlay->SetMovableByPhysics(false);
	nextBtn->SetMovableByPhysics(false);

	scene.SetHowToPlayOverlayActive(true);
#endif // _DEBUG
}

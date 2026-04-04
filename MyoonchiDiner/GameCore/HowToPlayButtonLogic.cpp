/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         HowToPlayButtonLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu	(40%)
 CO-AUTHORS:        Seah Wang Hua, wanghua.seah@digipen.edu (40%)
					Yat Chun Wee, y.chunwee@digipen.edu		(15%)
					Ng Juin Herng, juinherng.ng@digipen.edu (5%)

 DESCRIPTION:       Handles hover and click behaviour for the How To Play
					button in the main menu. On click, it spawns a fullscreen
					HowToPlay overlay image with a top-right Next button.
					Clicking Next advances through the authored how-to-play pages,
					then closes the overlay. Esc also closes the overlay.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <array>
#include <vector>

#include "EngineCore/FilePaths.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/HowToPlayButtonLogic.hpp"
#include "GameCore/MenuKeyboardNavigation.hpp"
#include "MyoonchiDiner/GamePaths.hpp"

namespace {
	static constexpr std::array<const char*, 4> kHowToPlayPages{
		"../assets/HowToPlay/howtoplay_1.png",
		"../assets/HowToPlay/howtoplay_2.png",
		"../assets/HowToPlay/howtoplay_3.png",
		"../assets/HowToPlay/howtoplay_4.png"
	};

	static constexpr const char* kNextButtonTexNormal = "../assets/HowToPlay/next_s.png";
	static constexpr const char* kNextButtonTexHover = "../assets/HowToPlay/next_h.png";

	static const glm::vec2 kNextButtonSize{ 219.75f, 105.75f };
	static const glm::vec2 kNextButtonMargin{ 200.0f, 140.0f }; // from bottom-right corner

	/**
	 * @brief Builds the hover-state texture path for a menu sprite.
	 *
	 * @param path The authored normal-state texture path.
	 *
	 * @return The matching hover texture path, preserving the original suffix rules.
	 */
	static std::string MakeHoverPath(const std::string& path) {
		if (path.empty()) return path;

		// Split the authored path so the hover suffix can be injected before the extension.
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

	/**
	 * @brief Replaces a sprite's texture if the requested asset can be loaded.
	 *
	 * @param owner The object whose texture should change.
	 * @param texPath The texture path to resolve through the resource manager.
	 */
	static void TrySetTexture(GameObject* owner, const std::string& texPath) {
		if (!owner || texPath.empty()) return;

		// Cache static UI sprites by path so hover swaps reuse the same loaded texture.
		std::string cacheName = "staticsprite_" + texPath;
		if (Texture* tex = ResourceManager::Instance().LoadTexture(cacheName, texPath)) {
			owner->SetTexture(tex);
		}
	}

	/**
	 * @brief Tests whether a point lies inside an axis-aligned rectangle.
	 *
	 * @param p The point to test.
	 * @param min The rectangle minimum corner.
	 * @param max The rectangle maximum corner.
	 *
	 * @return True when the point is inside or on the rectangle bounds.
	 */
	static bool IsPointInRect(const glm::vec2& p, const glm::vec2& min, const glm::vec2& max) {
		return p.x >= min.x && p.x <= max.x &&
			p.y >= min.y && p.y <= max.y;
	}

	/**
	 * @brief Tests whether a world-space point overlaps a scene object's bounds.
	 *
	 * @param scene The active scene that owns the object.
	 * @param objectID The object to test against.
	 * @param p The world-space point.
	 *
	 * @return True when the point lies inside the object's visual bounds.
	 */
	static bool IsPointInObject(Scene& scene, int objectID, const glm::vec2& p) {
		GameObject* obj = scene.GetGameObjectByID(objectID);
		if (!obj) {
			return false;
		}

		// Use the object's current transform as the clickable rectangle for overlay buttons.
		const glm::vec3 pos = obj->GetPositionGLM();
		const glm::vec3 sz = obj->GetScaleGLM();
		const glm::vec2 min(pos.x - sz.x * 0.5f, pos.y - sz.y * 0.5f);
		const glm::vec2 max(pos.x + sz.x * 0.5f, pos.y + sz.y * 0.5f);

		return IsPointInRect(p, min, max);
	}

	/**
	 * @brief Resolves the current mouse position into scene world space.
	 *
	 * @param input The input manager used as a fallback projection source.
	 *
	 * @return The mouse position in world coordinates.
	 */
	static glm::vec2 GetMouseWorld(InputManager& input) {
		glm::vec2 mouseWorld{};
		if (!GraphicsEngine::Instance().GetMouseWorldInScene(mouseWorld)) {
			// Fall back to InputManager projection when the graphics helper has no active scene camera.
			const glm::vec3 w = input.ScreenToWorld(
				static_cast<float>(input.GetMousePosition().x),
				static_cast<float>(input.GetMousePosition().y));
			mouseWorld = glm::vec2(w.x, w.y);
		}
		return mouseWorld;
	}
}

/**
 * @brief Updates hover, click, and overlay paging behavior for the How To Play button.
 *
 * @param dt Unused frame delta time.
 * @param scene The active scene.
 * @param input The input manager for mouse and keyboard navigation.
 */
void HowToPlayButtonLogic::Update(float /*dt*/, Scene& scene, InputManager& input) {
	GameObject* owner = GetOwner(scene);
	if (!owner) {
		return;
	}

	auto closeOverlay = [&]() {
		// Tear down both overlay objects so the menu can resume normal input routing.
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
		MenuKeyboardNavigation::ClearFocus(MenuKeyboardNavigation::BuildScopeKey(scene, "how_to_play_overlay"));
		};

	const bool overlayActive = scene.IsHowToPlayOverlayActive();

	if (!overlayActive && !scene.ShouldUseRuntimeParityMode()) {
		// In editor parity-off mode, the menu button should reset to its default visual state.
		if (hovered_) {
			hovered_ = false;
			TrySetTexture(owner, normalTexturePath_);
		}
		return;
	}

	if (scene.IsMenuInteractionSuppressed()) {
		// Suppressed menu interaction means another transition or cutscene owns the input.
		if (!overlayActive && hovered_) {
			hovered_ = false;
			TrySetTexture(owner, normalTexturePath_);
		}
		return;
	}

	if (!overlayActive && scene.IsMenuModalActive()) {
		// Do not let the button compete with other top-level menu modals.
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
		// While the overlay is open, keyboard focus is scoped to the single Next button.
		const int focusedButtonId = MenuKeyboardNavigation::UpdateFocus(
			scene,
			input,
			MenuKeyboardNavigation::BuildScopeKey(scene, "how_to_play_overlay"),
			nextButtonId_ >= 0 ? std::vector<int>{ nextButtonId_ } : std::vector<int>{},
			overNextButton ? nextButtonId_ : -1);
		const bool keyboardFocused = (focusedButtonId == nextButtonId_);
		const bool nextButtonHot = overNextButton || keyboardFocused;

		if (nextButtonHot != nextButtonHovered_) {
			nextButtonHovered_ = nextButtonHot;

			if (GameObject* nextBtn = scene.GetGameObjectByID(nextButtonId_)) {
				// Mirror hover focus visually whether it came from mouse or keyboard navigation.
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

		const bool keyboardSubmit = keyboardFocused && MenuKeyboardNavigation::ConsumeSubmitPress(input);
		if (!click && !keyboardSubmit) {
			return;
		}

		if (click) {
			// Consume all clicks while overlay is open.
			input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);
		}

		if (!(overNextButton || keyboardFocused)) {
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
		// Capture the authored sprite path once so later hover swaps remain deterministic.
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

	const std::vector<int> buttonIds = scene.IsPauseOverlayActive()
		? MenuKeyboardNavigation::CollectPauseOverlayButtons(scene)
		: MenuKeyboardNavigation::CollectCurrentSceneTopLevelButtons(scene);
	const std::string scopeKey = scene.IsPauseOverlayActive()
		? MenuKeyboardNavigation::GetPauseOverlayScopeKey(scene)
		: MenuKeyboardNavigation::GetCurrentSceneTopLevelScopeKey(scene);
	// Reuse the shared menu-navigation system so mouse and keyboard stay in sync.
	const int focusedButtonId = MenuKeyboardNavigation::UpdateFocus(
		scene,
		input,
		scopeKey,
		buttonIds,
		over ? GetOwnerID() : -1);
	const bool keyboardFocused = (focusedButtonId == GetOwnerID());
	const bool hot = over || keyboardFocused;

	if (hot && !hovered_) {
		hovered_ = true;
		TrySetTexture(owner, hoverTexturePath_);
		scene.TriggerUiButtonHoverFeedback(GetOwnerID());
		if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_HOVER)) {
			audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_HOVER, audioManager_->GetVfxVolume(), false);
		}
	}
	else if (!hot && hovered_) {
		hovered_ = false;
		TrySetTexture(owner, normalTexturePath_);
	}

	const bool keyboardSubmit = keyboardFocused && MenuKeyboardNavigation::ConsumeSubmitPress(input);
	if ((!over || !input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) && !keyboardSubmit) {
		return;
	}

	if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON)) {
		audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON, audioManager_->GetVfxVolume(), false);
	}

	if (over) {
		input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);
	}

	// Spawn the full-screen page first so the navigation button can render above it.
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

	// Anchor the Next button to the bottom-right corner of the reference resolution.
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
	MenuKeyboardNavigation::ClearFocus(MenuKeyboardNavigation::BuildScopeKey(scene, "how_to_play_overlay"));

	overlay->SetMovableByPhysics(false);
	nextBtn->SetMovableByPhysics(false);

	// Tell the rest of the menu system that a blocking instructional overlay is active.
	scene.SetHowToPlayOverlayActive(true);
}

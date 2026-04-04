/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			StartGamePromptLogic.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (50%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(30%)
					Ng Juin Herng, juinherng.ng@digipen.edu (10%)
					Vu Phan Hung, phanhung.vu@digipen.edu   (10%)

 DESCRIPTION:		Implements the Play button tutorial prompt and intro-video routing flow.
					Handles main-menu Play-button hover state, the tutorial-choice modal,
					and routing into either the tutorial level or the intro-video path.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <string>
#include <vector>

#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/MenuKeyboardNavigation.hpp"
#include "GameCore/StartGamePromptLogic.hpp"
#include "MyoonchiDiner/GamePaths.hpp"

namespace {
	// Popup and decision-button textures used by the tutorial-choice modal.
	constexpr const char* kTutorialPopupTexture = "../assets/UI/tutorial_popup.png";
	constexpr const char* kTutorialYesNormalTexture = "../assets/UI/yes_s.png";
	constexpr const char* kTutorialYesHoverTexture = "../assets/UI/yes_h.png";
	constexpr const char* kTutorialNoNormalTexture = "../assets/UI/no_s.png";
	constexpr const char* kTutorialNoHoverTexture = "../assets/UI/no_h.png";
	constexpr int kTutorialPopupSortOrder = 1000000;
	constexpr int kTutorialButtonSortOrder = 1000001;

	// Legacy still-frame intro cutscene sequence retained as authored data for the "No" branch.
	const std::vector<std::string> kIntroCutsceneFrames = {
		"../assets/Cutscenes/Cutscene_starting_1.1.png",
		"../assets/Cutscenes/Cutscene_starting_2.1.png",
		"../assets/Cutscenes/Cutscene_starting_3.1.png",
		"../assets/Cutscenes/Cutscene_starting_4.1.png",
		"../assets/Cutscenes/Cutscene_starting_5.1.png",
		"../assets/Cutscenes/Cutscene_starting_6.1.png",
	};

	/**
	 * @brief Builds the hover-state texture path for a button texture.
	 * @param path Current texture path assigned to the button.
	 * @return Hover-state texture path derived from the `_s`/`_h` naming convention.
	 */
	static std::string MakeHoverPath(const std::string& path) {
		// Preserve empty inputs so callers can safely skip missing scene metadata.
		if (path.empty()) {
			return path;
		}

		// Split the file path so suffix replacement does not disturb the extension.
		const size_t dot = path.find_last_of('.');
		const std::string ext = (dot != std::string::npos) ? path.substr(dot) : std::string();
		const std::string base = (dot != std::string::npos) ? path.substr(0, dot) : path;

		// Keep already-hovered textures unchanged to avoid duplicating the suffix.
		if (base.size() >= 2 && base.substr(base.size() - 2) == "_h") {
			return base + ext;
		}

		// Promote the standard selected-state suffix into the hover-state suffix.
		if (base.size() >= 2 && base.substr(base.size() - 2) == "_s") {
			return base.substr(0, base.size() - 2) + "_h" + ext;
		}

		// Fall back to appending `_h` when the authored texture uses a custom naming style.
		return base + "_h" + ext;
	}

	/**
	 * @brief Applies a texture directly to a GameObject when the asset is available.
	 * @param owner Object whose sprite texture should be changed.
	 * @param texPath Texture path to load and assign.
	 */
	static void TrySetTexture(GameObject* owner, const std::string& texPath) {
		// Ignore invalid targets or empty paths so hover resets stay defensive.
		if (!owner || texPath.empty()) {
			return;
		}

		// Reuse the shared static-sprite cache for responsive button swaps.
		const std::string cacheName = "staticsprite_" + texPath;
		if (Texture* tex = ResourceManager::Instance().LoadTexture(cacheName, texPath)) {
			owner->SetTexture(tex);
		}
	}

	/**
	 * @brief Applies a texture to a scene object and keeps scene metadata in sync.
	 * @param scene Active scene containing the target object.
	 * @param objectID Scene object id whose texture should be updated.
	 * @param texPath Texture path to load and assign.
	 */
	static void TrySetObjectTexture(Scene& scene, int objectID, const std::string& texPath) {
		// Skip invalid object ids or missing paths so prompt teardown can call this safely.
		if (objectID < 0 || texPath.empty()) {
			return;
		}

		GameObject* obj = scene.GetGameObjectByID(objectID);
		if (!obj) {
			return;
		}

		// Update both the live sprite and the scene-side texture path cache together.
		const std::string cacheName = "staticsprite_" + texPath;
		if (Texture* tex = ResourceManager::Instance().LoadTexture(cacheName, texPath)) {
			obj->SetTexture(tex);
			scene.SetObjectTexturePath(objectID, texPath);
		}
	}
}

bool StartGamePromptLogic::GetMouseWorld(Scene& /*scene*/, InputManager& input, glm::vec2& outWorld) const {
	// Prefer the active scene viewport mapping so UI hit-tests respect letterboxing and scaling.
	if (GraphicsEngine::Instance().GetMouseWorldInScene(outWorld)) {
		return true;
	}

	// Fall back to the generic screen-to-world conversion when the viewport helper is unavailable.
	const glm::vec3 w = input.ScreenToWorld(
		static_cast<float>(input.GetMousePosition().x),
		static_cast<float>(input.GetMousePosition().y));
	outWorld = glm::vec2(w.x, w.y);
	return true;
}

bool StartGamePromptLogic::IsPointInRect(const glm::vec2& p, const glm::vec2& min, const glm::vec2& max) const {
	// Use inclusive bounds so edge clicks still count as valid button hits.
	return p.x >= min.x && p.x <= max.x &&
		p.y >= min.y && p.y <= max.y;
}

void StartGamePromptLogic::OpenPrompt(Scene& scene) {
	// Ignore duplicate open requests so repeated clicks do not spawn stacked popups.
	if (promptOpen_) {
		return;
	}

	// Warm the hover VFX dependency used elsewhere in the menu presentation path.
	ResourceManager::Instance().LoadTexture(
		"animatedsprite_../assets/VFX/staranim-Sheet2.png",
		"../assets/VFX/staranim-Sheet2.png");

	// Center the popup inside the authored reference-space canvas.
	popupCenter_ = glm::vec2(
		static_cast<float>(GraphicsEngine::kRefW) * 0.5f,
		static_cast<float>(GraphicsEngine::kRefH) * 0.5f);

	GameObject* popup = scene.SpawnStaticSprite(
		kTutorialPopupTexture,
		glm::vec3(popupCenter_.x, popupCenter_.y, 0.0f),
		popupSize_,
		"999999");
	if (!popup) {
		return;
	}

	// Record the modal root and tell the scene to block other menu interactions.
	popup->SetRenderSortOrder(kTutorialPopupSortOrder);
	popupId_ = popup->GetID();
	promptOpen_ = true;
	scene.SetMenuModalActive(true);

	// Convert normalized popup-local coordinates into world-space button centers.
	auto ToWorld = [&](float nx, float ny) -> glm::vec2 {
		return popupCenter_ + glm::vec2(
			(nx - 0.5f) * popupSize_.x,
			(ny - 0.5f) * popupSize_.y
		);
		};

	// Use the authored popup layout so the Yes and No buttons match the UI artwork.
	const glm::vec2 yesCenter = ToWorld(0.50f, 0.71f);
	const glm::vec2 noCenter = ToWorld(0.50f, 0.84f);
	const glm::vec2 buttonSize(300.0f, 80.0f);

	// Spawn the Yes button and keep its texture metadata in sync with the live sprite.
	if (GameObject* yesBtn = scene.SpawnStaticSprite(
		kTutorialYesNormalTexture,
		glm::vec3(yesCenter.x, yesCenter.y, 0.0f),
		buttonSize,
		"999999")) {
		yesBtn->SetRenderSortOrder(kTutorialButtonSortOrder);
		yesButtonId_ = yesBtn->GetID();
		scene.SetObjectTexturePath(yesButtonId_, kTutorialYesNormalTexture);
	}

	// Spawn the No button using the same modal layer and sort priority.
	if (GameObject* noBtn = scene.SpawnStaticSprite(
		kTutorialNoNormalTexture,
		glm::vec3(noCenter.x, noCenter.y, 0.0f),
		buttonSize,
		"999999")) {
		noBtn->SetRenderSortOrder(kTutorialButtonSortOrder);
		noButtonId_ = noBtn->GetID();
		scene.SetObjectTexturePath(noButtonId_, kTutorialNoNormalTexture);
	}

	// Reset transient hover state and keyboard focus for the fresh modal.
	yesHovered_ = false;
	noHovered_ = false;
	MenuKeyboardNavigation::ClearFocus(MenuKeyboardNavigation::BuildScopeKey(scene, "start_prompt"));
}

void StartGamePromptLogic::ClosePrompt(Scene& scene) {
	// Tear down every popup-owned sprite before returning control to the regular menu.
	if (popupId_ >= 0) {
		scene.DespawnByID(popupId_);
		popupId_ = -1;
	}
	if (yesButtonId_ >= 0) {
		scene.DespawnByID(yesButtonId_);
		yesButtonId_ = -1;
	}
	if (noButtonId_ >= 0) {
		scene.DespawnByID(noButtonId_);
		noButtonId_ = -1;
	}

	// Reset modal-local state and restore top-level menu interaction.
	yesHovered_ = false;
	noHovered_ = false;
	promptOpen_ = false;
	scene.SetMenuModalActive(false);
	MenuKeyboardNavigation::ClearFocus(MenuKeyboardNavigation::BuildScopeKey(scene, "start_prompt"));
}

void StartGamePromptLogic::Update(float /*dt*/, Scene& scene, InputManager& input) {
	// Abort immediately if the Play button object was removed this frame.
	GameObject* owner = GetOwner(scene);
	if (!owner) {
		return;
	}

	// The how-to-play overlay sits above the Play flow, so do not process prompt input underneath it.
	if (scene.IsHowToPlayOverlayActive()) {
		return;
	}

	// Cache the Play button texture paths on first use so repeated hover swaps stay cheap.
	if (!initialized_) {
		normalTexturePath_ = scene.GetObjectTexturePath(GetOwnerID());
		hoverTexturePath_ = MakeHoverPath(normalTexturePath_);
		initialized_ = true;
	}

	// Freeze both the Play button and any open prompt while a cutscene or scene transition owns the screen.
	if (scene.IsMenuInteractionSuppressed()) {
		if (hovered_) {
			hovered_ = false;
			TrySetTexture(owner, normalTexturePath_);
		}

		if (yesHovered_) {
			yesHovered_ = false;
			TrySetObjectTexture(scene, yesButtonId_, kTutorialYesNormalTexture);
		}

		if (noHovered_) {
			noHovered_ = false;
			TrySetObjectTexture(scene, noButtonId_, kTutorialNoNormalTexture);
		}
		return;
	}

	// Resolve the current cursor position once so both the Play button and popup buttons share it.
	glm::vec2 mouseWorld{};
	GetMouseWorld(scene, input, mouseWorld);

	// When the modal is closed, the Play button behaves like a normal top-level menu entry.
	if (!promptOpen_) {
		// Editor-only scenes do not run the release-style Play button interaction flow.
		if (!scene.ShouldUseRuntimeParityMode()) {
			if (hovered_) {
				hovered_ = false;
				TrySetTexture(owner, normalTexturePath_);
			}
			return;
		}

		// Another modal already owns the menu, so force this button back to its idle state.
		if (scene.IsMenuModalActive()) {
			if (hovered_) {
				hovered_ = false;
				TrySetTexture(owner, normalTexturePath_);
			}
			return;
		}

		// Build the Play button bounds from the authored sprite transform.
		const glm::vec3 pos = owner->GetPositionGLM();
		const glm::vec3 sz = owner->GetScaleGLM();
		const glm::vec2 min(pos.x - sz.x * 0.5f, pos.y - sz.y * 0.5f);
		const glm::vec2 max(pos.x + sz.x * 0.5f, pos.y + sz.y * 0.5f);

		// Merge pointer hover with keyboard focus so both inputs drive the same highlight state.
		const bool over = IsPointInRect(mouseWorld, min, max);
		const int focusedButtonId = MenuKeyboardNavigation::UpdateFocus(
			scene,
			input,
			MenuKeyboardNavigation::GetCurrentSceneTopLevelScopeKey(scene),
			MenuKeyboardNavigation::CollectCurrentSceneTopLevelButtons(scene),
			over ? GetOwnerID() : -1);
		const bool keyboardFocused = (focusedButtonId == GetOwnerID());
		const bool hot = over || keyboardFocused;

		// Promote the Play button to its hover state and emit hover feedback once on entry.
		if (hot && !hovered_) {
			hovered_ = true;
			TrySetTexture(owner, hoverTexturePath_);
			scene.TriggerUiButtonHoverFeedback(GetOwnerID());
			if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_HOVER)) {
				audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_HOVER, audioManager_->GetVfxVolume(), false);
			}
		}
		// Restore the default Play texture once the hover or focus leaves the button.
		else if (!hot && hovered_) {
			hovered_ = false;
			TrySetTexture(owner, normalTexturePath_);
		}

		// Open the tutorial-choice modal on mouse click or keyboard submit.
		const bool keyboardSubmit = keyboardFocused && MenuKeyboardNavigation::ConsumeSubmitPress(input);
		if ((input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT) && over) || keyboardSubmit) {
			if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON)) {
				audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON, audioManager_->GetVfxVolume(), false);
			}

			// Consume the click so other menu systems do not also react to the same press.
			if (over) {
				input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);
			}

			OpenPrompt(scene);
		}
		return;
	}

	// While the prompt is open, ESC closes it instead of triggering other menu behavior.
	if (input.IsKeyJustPressed(GLFW_KEY_ESCAPE)) {
		if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_BACK)) {
			audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_BACK, audioManager_->GetVfxVolume(), false);
		}
		ClosePrompt(scene);
		input.ConsumeNextKeyPress(GLFW_KEY_ESCAPE);
		return;
	}

	// Reuse the same point-in-rect test for popup-owned Yes and No buttons.
	auto IsPointInObject = [&](int id) -> bool {
		GameObject* obj = scene.GetGameObjectByID(id);
		if (!obj) {
			return false;
		}

		// Derive the clickable bounds directly from the button sprite transform.
		const glm::vec3 pos = obj->GetPositionGLM();
		const glm::vec3 sz = obj->GetScaleGLM();
		const glm::vec2 min(pos.x - sz.x * 0.5f, pos.y - sz.y * 0.5f);
		const glm::vec2 max(pos.x + sz.x * 0.5f, pos.y + sz.y * 0.5f);
		return IsPointInRect(mouseWorld, min, max);
		};

	// Resolve both buttons through the shared keyboard navigation scope for the prompt modal.
	const bool yesOver = IsPointInObject(yesButtonId_);
	const bool noOver = IsPointInObject(noButtonId_);
	const int focusedButtonId = MenuKeyboardNavigation::UpdateFocus(
		scene,
		input,
		MenuKeyboardNavigation::BuildScopeKey(scene, "start_prompt"),
		{ yesButtonId_, noButtonId_ },
		yesOver ? yesButtonId_ : (noOver ? noButtonId_ : -1));
	const bool yesHot = yesOver || (focusedButtonId == yesButtonId_);
	const bool noHot = noOver || (focusedButtonId == noButtonId_);

	// Apply hover visuals and optional hover audio to the Yes button.
	if (yesHot && !yesHovered_) {
		yesHovered_ = true;
		TrySetObjectTexture(scene, yesButtonId_, kTutorialYesHoverTexture);
		scene.TriggerUiButtonHoverFeedback(yesButtonId_);
		if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_HOVER)) {
			audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_HOVER, audioManager_->GetVfxVolume(), false);
		}
	}
	else if (!yesHot && yesHovered_) {
		yesHovered_ = false;
		TrySetObjectTexture(scene, yesButtonId_, kTutorialYesNormalTexture);
	}

	// Apply hover visuals and optional hover audio to the No button.
	if (noHot && !noHovered_) {
		noHovered_ = true;
		TrySetObjectTexture(scene, noButtonId_, kTutorialNoHoverTexture);
		scene.TriggerUiButtonHoverFeedback(noButtonId_);
		if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_HOVER)) {
			audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_HOVER, audioManager_->GetVfxVolume(), false);
		}
	}
	else if (!noHot && noHovered_) {
		noHovered_ = false;
		TrySetObjectTexture(scene, noButtonId_, kTutorialNoNormalTexture);
	}

	// Accept either a direct click or keyboard submit for the currently focused decision button.
	const bool click = input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT);
	const bool keyboardSubmit = MenuKeyboardNavigation::ConsumeSubmitPress(input);
	if (!click && !keyboardSubmit) {
		return;
	}
	if (click) {
		// Consume all prompt clicks so the underlying menu cannot react to the same press.
		input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);
	}

	if (yesHot && (yesOver || focusedButtonId == yesButtonId_)) {
		// Route the Yes decision directly into the tutorial level transition.
		ClosePrompt(scene);
		if (audioManager_) {
			if (audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON)) {
				audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON, audioManager_->GetVfxVolume(), false);
			}
			if (audioManager_->HasSound(MyoonchiPaths::Audio::SFX_START_BUTTON)) {
				audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_START_BUTTON, audioManager_->GetVfxVolume(), false);
			}
			audioManager_->FadeChannel(MyoonchiPaths::Audio::BGM_MAIN_MENU, 0.0f, 0.35f);
		}
		scene.StartLevelTransition(tutorialJson_, activateSimulation_);
	}
	else if (noHot && (noOver || focusedButtonId == noButtonId_)) {
		// Route the No decision through the intro-video path before loading gameplay.
		ClosePrompt(scene);
		if (audioManager_) {
			if (audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON)) {
				audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON, audioManager_->GetVfxVolume(), false);
			}
			if (audioManager_->HasSound(MyoonchiPaths::Audio::SFX_START_BUTTON)) {
				audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_START_BUTTON, audioManager_->GetVfxVolume(), false);
			}
			audioManager_->FadeChannel(MyoonchiPaths::Audio::BGM_MAIN_MENU, 0.0f, 0.35f);
		}

		// Keep the authored still-frame list referenced so the legacy asset chain remains discoverable.
		(void)kIntroCutsceneFrames;

		// Start the transitioned MP4 intro cutscene that hands off to the gameplay destination on completion.
		scene.StartVideoCutsceneTransitioned(
			MyoonchiPaths::Videos::INTRO_CUTSCENE,
			skipJson_,
			activateSimulation_,
			false,
			0.35f,
			0.35f);

		if (audioManager_) {
			// Layer the authored intro SFX and BGM once the cutscene handoff has started.
			const float cutsceneBgmFadeIn = 1.0f;
			if (audioManager_->HasSound(MyoonchiPaths::Audio::SFX_INTRO_CUTSCENE)) {
				audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_INTRO_CUTSCENE, audioManager_->GetVfxVolume(), false);
			}
			audioManager_->PlaySound(MyoonchiPaths::Audio::BGM_INTRO_CUTSCENE, 0.0f, false);
			audioManager_->FadeChannel(
				MyoonchiPaths::Audio::BGM_INTRO_CUTSCENE,
				audioManager_->GetBgmVolume() * 1.6f,
				cutsceneBgmFadeIn);
		}
	}
}

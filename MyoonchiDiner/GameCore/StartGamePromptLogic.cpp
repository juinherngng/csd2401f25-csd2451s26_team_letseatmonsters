/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			StartGamePromptLogic.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Implements the StartGamePromptLogic class, which manages the tutorial prompt that appears
					when the player clicks the "Play" button on the main menu. This logic handles mouse input
					to detect clicks on the button, opens a tutorial popup, and manages hover states for visual feedback.
 DESCRIPTION:		Implements hover/click interaction flow for the main menu Play button and the
					tutorial decision popup.

			Flow:
				  1) While popup is closed:
					 - hover swaps Play texture (_s <-> _h),
					 - click opens tutorial decision popup.
				  2) While popup is open:
					 - hover swaps Yes/No textures,
					 - click Yes starts tutorial level,
					 - click No starts intro cutscene path.


		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <string>
#include <vector>

#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/StartGamePromptLogic.hpp"
#include "MyoonchiDiner/GamePaths.hpp"

namespace {
	// Popup + decision button textures.
	constexpr const char* kTutorialPopupTexture = "../assets/UI/tutorial_popup.png";
	constexpr const char* kTutorialYesNormalTexture = "../assets/UI/yes_s.png";
	constexpr const char* kTutorialYesHoverTexture = "../assets/UI/yes_h.png";
	constexpr const char* kTutorialNoNormalTexture = "../assets/UI/no_s.png";
	constexpr const char* kTutorialNoHoverTexture = "../assets/UI/no_h.png";
	constexpr int kTutorialPopupSortOrder = 1000000;
	constexpr int kTutorialButtonSortOrder = 1000001;

	// Intro cutscene frames used by the "No" path.
	const std::vector<std::string> kIntroCutsceneFrames = {
		"../assets/Cutscenes/Cutscene_starting_1.1.png",
		"../assets/Cutscenes/Cutscene_starting_2.1.png",
		"../assets/Cutscenes/Cutscene_starting_3.1.png",
		"../assets/Cutscenes/Cutscene_starting_4.1.png",
		"../assets/Cutscenes/Cutscene_starting_5.1.png",
		"../assets/Cutscenes/Cutscene_starting_6.1.png",
	};

	// Converts texture name variants between normal/hover naming convention.
	static std::string MakeHoverPath(const std::string& path) {
		if (path.empty()) return path;
		const size_t dot = path.find_last_of('.');
		const std::string ext = (dot != std::string::npos) ? path.substr(dot) : std::string();
		const std::string base = (dot != std::string::npos) ? path.substr(0, dot) : path;

		if (base.size() >= 2 && base.substr(base.size() - 2) == "_h") return base + ext;
		if (base.size() >= 2 && base.substr(base.size() - 2) == "_s") return base.substr(0, base.size() - 2) + "_h" + ext;
		return base + "_h" + ext;
	}

	// Sets texture directly on an object if path is valid/loadable.
	static void TrySetTexture(GameObject* owner, const std::string& texPath) {
		if (!owner || texPath.empty()) return;
		std::string cacheName = "staticsprite_" + texPath;
		if (Texture* tex = ResourceManager::Instance().LoadTexture(cacheName, texPath)) {
			owner->SetTexture(tex);
		}
	}

	// Sets texture on a scene object and updates scene texture metadata.
	static void TrySetObjectTexture(Scene& scene, int objectID, const std::string& texPath) {
		if (objectID < 0 || texPath.empty()) return;
		GameObject* obj = scene.GetGameObjectByID(objectID);
		if (!obj) return;

		std::string cacheName = "staticsprite_" + texPath;
		if (Texture* tex = ResourceManager::Instance().LoadTexture(cacheName, texPath)) {
			obj->SetTexture(tex);
			scene.SetObjectTexturePath(objectID, texPath);
		}
	}
}

// Get the mouse world position, trying GraphicsEngine first and falling back to InputManager conversion if necessary. Returns true if a valid world position was obtained.
bool StartGamePromptLogic::GetMouseWorld(Scene& /*scene*/, InputManager& input, glm::vec2& outWorld) const {
	if (GraphicsEngine::Instance().GetMouseWorldInScene(outWorld)) {
		return true;
	}

	glm::vec3 w = input.ScreenToWorld(
		static_cast<float>(input.GetMousePosition().x),
		static_cast<float>(input.GetMousePosition().y));
	outWorld = glm::vec2(w.x, w.y);
	return true;
}

// Simple AABB point-in-rect test
bool StartGamePromptLogic::IsPointInRect(const glm::vec2& p, const glm::vec2& min, const glm::vec2& max) const {
	return p.x >= min.x && p.x <= max.x &&
		p.y >= min.y && p.y <= max.y;
}

// Opens the tutorial prompt by spawning a new GameObject with the tutorial popup texture at the center of the screen. Sets promptOpen_ to true and stores the popup's GameObject ID for later despawning.
void StartGamePromptLogic::OpenPrompt(Scene& scene) {
	if (promptOpen_) {
		return;
	}

	ResourceManager::Instance().LoadTexture(
		"animatedsprite_../assets/VFX/staranim-Sheet2.png",
		"../assets/VFX/staranim-Sheet2.png");

	// Center popup on reference canvas.
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

	popup->SetRenderSortOrder(kTutorialPopupSortOrder);
	popupId_ = popup->GetID();
	promptOpen_ = true;
	scene.SetMenuModalActive(true);

	// Convert popup-local normalized coordinates into world-space.
	auto ToWorld = [&](float nx, float ny) -> glm::vec2 {
		return popupCenter_ + glm::vec2(
			(nx - 0.5f) * popupSize_.x,
			(ny - 0.5f) * popupSize_.y
		);
		};

	// Authored top/bottom button positions inside popup.
	const glm::vec2 yesCenter = ToWorld(0.50f, 0.71f);
	const glm::vec2 noCenter = ToWorld(0.50f, 0.84f);

	// Button sizing
	const glm::vec2 buttonSize(300.0f, 80.0f);

	if (GameObject* yesBtn = scene.SpawnStaticSprite(
		kTutorialYesNormalTexture,
		glm::vec3(yesCenter.x, yesCenter.y, 0.0f),
		buttonSize,
		"999999")) {
		yesBtn->SetRenderSortOrder(kTutorialButtonSortOrder);
		yesButtonId_ = yesBtn->GetID();
		scene.SetObjectTexturePath(yesButtonId_, kTutorialYesNormalTexture);
	}

	if (GameObject* noBtn = scene.SpawnStaticSprite(
		kTutorialNoNormalTexture,
		glm::vec3(noCenter.x, noCenter.y, 0.0f),
		buttonSize,
		"999999")) {
		noBtn->SetRenderSortOrder(kTutorialButtonSortOrder);
		noButtonId_ = noBtn->GetID();
		scene.SetObjectTexturePath(noButtonId_, kTutorialNoNormalTexture);
	}

	yesHovered_ = false;
	noHovered_ = false;
}

// Closes the tutorial prompt by despawning the popup GameObject using its stored ID. Sets promptOpen_ to false and resets popupId_.
void StartGamePromptLogic::ClosePrompt(Scene& scene) {
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

	yesHovered_ = false;
	noHovered_ = false;
	promptOpen_ = false;
	scene.SetMenuModalActive(false);
}

// Update handles both the hover state for the "Play" button and the click interactions when the prompt is open. 
// When the prompt is closed, it checks if the mouse is over the button and updates the hover texture accordingly.
// If the button is clicked, it opens the prompt. When the prompt is open, it checks for clicks on the "Yes" and "Skip tutorial" zones and triggers the appropriate transitions and audio.
void StartGamePromptLogic::Update(float /*dt*/, Scene& scene, InputManager& input) {
	GameObject* owner = GetOwner(scene);
	if (!owner) {
		return;
	}

	if (scene.IsHowToPlayOverlayActive()) {
		return;
	}

	// Lazy-init normal/hover texture paths for main Play button.
	if (!initialized_) {
		normalTexturePath_ = scene.GetObjectTexturePath(GetOwnerID());
		hoverTexturePath_ = MakeHoverPath(normalTexturePath_);
		initialized_ = true;
	}

	glm::vec2 mouseWorld{};
	GetMouseWorld(scene, input, mouseWorld);


	// ---------------------------------------------------------------------
	// Stage 1: popup closed -> interact with main Play button.
	// ---------------------------------------------------------------------
	if (!promptOpen_) {
		if (!scene.ShouldUseRuntimeParityMode()) {
			if (hovered_) {
				hovered_ = false;
				TrySetTexture(owner, normalTexturePath_);
			}
			return;
		}

		if (scene.IsMenuModalActive()) {
			if (hovered_) {
				hovered_ = false;
				TrySetTexture(owner, normalTexturePath_);
			}
			return;
		}

		const glm::vec3 pos = owner->GetPositionGLM();
		const glm::vec3 sz = owner->GetScaleGLM();
		const glm::vec2 min(pos.x - sz.x * 0.5f, pos.y - sz.y * 0.5f);
		const glm::vec2 max(pos.x + sz.x * 0.5f, pos.y + sz.y * 0.5f);

		const bool over = IsPointInRect(mouseWorld, min, max);

		// hover swap
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

		// Click opens popup.
		if (input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT) && over) {
			if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON)) {
				audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_CLICK_BUTTON, audioManager_->GetVfxVolume(), false);
			}
			input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);
			OpenPrompt(scene);
		}
		return;
	}

	// ---------------------------------------------------------------------
	// Stage 2: popup open -> interact with Yes/No decision buttons.
	// ---------------------------------------------------------------------
	if (input.IsKeyJustPressed(GLFW_KEY_ESCAPE)) {
		if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_BACK)) {
			audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_BACK, audioManager_->GetVfxVolume(), false);
		}
		ClosePrompt(scene);
		input.ConsumeNextKeyPress(GLFW_KEY_ESCAPE);
		return;
	}

	auto IsPointInObject = [&](int id) -> bool {
		GameObject* obj = scene.GetGameObjectByID(id);
		if (!obj) return false;

		const glm::vec3 pos = obj->GetPositionGLM();
		const glm::vec3 sz = obj->GetScaleGLM();
		const glm::vec2 min(pos.x - sz.x * 0.5f, pos.y - sz.y * 0.5f);
		const glm::vec2 max(pos.x + sz.x * 0.5f, pos.y + sz.y * 0.5f);
		return IsPointInRect(mouseWorld, min, max);
		};

	const bool yesOver = IsPointInObject(yesButtonId_);
	const bool noOver = IsPointInObject(noButtonId_);

	// Hover swaps for decision buttons.
	if (yesOver && !yesHovered_) {
		yesHovered_ = true;
		TrySetObjectTexture(scene, yesButtonId_, kTutorialYesHoverTexture);
		scene.TriggerUiButtonHoverFeedback(yesButtonId_);
		if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_HOVER)) {
			audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_HOVER, audioManager_->GetVfxVolume(), false);
		}
	}
	else if (!yesOver && yesHovered_) {
		yesHovered_ = false;
		TrySetObjectTexture(scene, yesButtonId_, kTutorialYesNormalTexture);
	}

	if (noOver && !noHovered_) {
		noHovered_ = true;
		TrySetObjectTexture(scene, noButtonId_, kTutorialNoHoverTexture);
		scene.TriggerUiButtonHoverFeedback(noButtonId_);
		if (audioManager_ && audioManager_->HasSound(MyoonchiPaths::Audio::SFX_UI_HOVER)) {
			audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_UI_HOVER, audioManager_->GetVfxVolume(), false);
		}
	}
	else if (!noOver && noHovered_) {
		noHovered_ = false;
		TrySetObjectTexture(scene, noButtonId_, kTutorialNoNormalTexture);
	}

	// Click handling for Yes/No actions.
	if (!input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		return;
	}
	input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);

	if (yesOver) {
		// Yes -> enter tutorial directly.
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
	else if (noOver) {
		// No -> play intro cutscene flow, then continue.
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

		scene.StartVideoCutscene(
			MyoonchiPaths::Videos::INTRO_CUTSCENE,
			skipJson_,
			activateSimulation_,
			false);

		if (audioManager_) {
			const float cutsceneBgmFadeIn = 1.0f;
			if (audioManager_->HasSound(MyoonchiPaths::Audio::SFX_INTRO_CUTSCENE)) {
				audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_INTRO_CUTSCENE, audioManager_->GetVfxVolume(), false);
			}
			audioManager_->PlaySound(MyoonchiPaths::Audio::BGM_INTRO_CUTSCENE, 0.0f, false);
			audioManager_->FadeChannel(MyoonchiPaths::Audio::BGM_INTRO_CUTSCENE, audioManager_->GetBgmVolume() * 1.6f, cutsceneBgmFadeIn);
		}
	}
}

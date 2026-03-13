/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			StartGamePromptLogic.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Implements the StartGamePromptLogic class, which manages the tutorial prompt that appears
					when the player clicks the "Play" button on the main menu. This logic handles mouse input
					to detect clicks on the button, opens a tutorial popup, and manages hover states for visual feedback.


		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "Core/StartGamePromptLogic.hpp"

#include "../GamePaths.hpp"

#include "Graphics/GameObject.hpp"
#include "Graphics/GraphicsEngine.hpp"
#include "Graphics/ResourceManager.hpp"
#include "Graphics/SceneManager.hpp"

#include <string>
#include <vector>

namespace {
	constexpr const char* kTutorialPopupTexture = "../assets/tutorial_popup.png";

	const std::vector<std::string> kIntroCutsceneFrames = {
		"../assets/Cutscenes/Cutscene_starting_1.1.png",
		"../assets/Cutscenes/Cutscene_starting_2.1.png",
		"../assets/Cutscenes/Cutscene_starting_3.1.png",
		"../assets/Cutscenes/Cutscene_starting_4.1.png",
		"../assets/Cutscenes/Cutscene_starting_5.1.png",
		"../assets/Cutscenes/Cutscene_starting_6.1.png",
	};

	// Given a texture path, generate the corresponding hover texture path by applying the following rules:
	static std::string MakeHoverPath(const std::string& path) {
		if (path.empty()) return path;
		const size_t dot = path.find_last_of('.');
		const std::string ext = (dot != std::string::npos) ? path.substr(dot) : std::string();
		const std::string base = (dot != std::string::npos) ? path.substr(0, dot) : path;

		if (base.size() >= 2 && base.substr(base.size() - 2) == "_h") return base + ext;
		if (base.size() >= 2 && base.substr(base.size() - 2) == "_s") return base.substr(0, base.size() - 2) + "_h" + ext;
		return base + "_h" + ext;
	}

	// Helper to set the owner's texture from a file path, using ResourceManager caching. The cache name is prefixed with "staticsprite_" to match EntityManager conventions for static sprites.
	static void TrySetTexture(GameObject* owner, const std::string& texPath) {
		if (!owner || texPath.empty()) return;
		std::string cacheName = "staticsprite_" + texPath;
		if (Texture* tex = ResourceManager::Instance().LoadTexture(cacheName, texPath)) {
			owner->SetTexture(tex);
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

	popupId_ = popup->GetID();
	promptOpen_ = true;
}

// Closes the tutorial prompt by despawning the popup GameObject using its stored ID. Sets promptOpen_ to false and resets popupId_.
void StartGamePromptLogic::ClosePrompt(Scene& scene) {
	if (popupId_ >= 0) {
		scene.DespawnByID(popupId_);
		popupId_ = -1;
	}
	promptOpen_ = false;
}

// Update handles both the hover state for the "Play" button and the click interactions when the prompt is open. 
// When the prompt is closed, it checks if the mouse is over the button and updates the hover texture accordingly.
// If the button is clicked, it opens the prompt. When the prompt is open, it checks for clicks on the "Yes" and "Skip tutorial" zones and triggers the appropriate transitions and audio.
void StartGamePromptLogic::Update(float /*dt*/, Scene& scene, InputManager& input) {
	GameObject* owner = GetOwner(scene);
	if (!owner) {
		return;
	}

	// lazy init hover textures
	if (!initialized_) {
		normalTexturePath_ = scene.GetObjectTexturePath(GetOwnerID());
		hoverTexturePath_ = MakeHoverPath(normalTexturePath_);
		initialized_ = true;
	}

	glm::vec2 mouseWorld{};
	GetMouseWorld(scene, input, mouseWorld);

	// Step 1: normal Play button click opens prompt
	if (!promptOpen_) {
		const glm::vec3 pos = owner->GetPositionGLM();
		const glm::vec3 sz = owner->GetScaleGLM();
		const glm::vec2 min(pos.x - sz.x * 0.5f, pos.y - sz.y * 0.5f);
		const glm::vec2 max(pos.x + sz.x * 0.5f, pos.y + sz.y * 0.5f);

		const bool over = IsPointInRect(mouseWorld, min, max);

		// hover swap
		if (over && !hovered_) {
			hovered_ = true;
			TrySetTexture(owner, hoverTexturePath_);
		}
		else if (!over && hovered_) {
			hovered_ = false;
			TrySetTexture(owner, normalTexturePath_);
		}

		if (input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT) && over) {
			input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);
			OpenPrompt(scene);
		}
		return;
	}

	// Step 2: prompt open -> click YES / SKIP zones
	if (!input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		return;
	}
	input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);

	// Convert normalized UV-space (0..1 on popup image) to world-space
	auto ToWorld = [&](float nx, float ny) -> glm::vec2 {
		return popupCenter_ + glm::vec2(
			(nx - 0.5f) * popupSize_.x,
			(ny - 0.5f) * popupSize_.y
		);
		};

	// Button Hitboxes:
	// Top button ("Yes")
	const glm::vec2 yesMin = ToWorld(0.21f, 0.67f);
	const glm::vec2 yesMax = ToWorld(0.79f, 0.75f);

	// Bottom button ("Skip tutorial")
	const glm::vec2 skipMin = ToWorld(0.21f, 0.80f);
	const glm::vec2 skipMax = ToWorld(0.79f, 0.88f);

	if (IsPointInRect(mouseWorld, yesMin, yesMax)) {
		ClosePrompt(scene);
		if (audioManager_) {
			audioManager_->PlayUIClickSound();
			if (audioManager_->HasSound(MyoonchiPaths::Audio::SFX_START_BUTTON)) {
				audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_START_BUTTON, audioManager_->GetVfxVolume(), false);
			}
			audioManager_->FadeChannel(MyoonchiPaths::Audio::BGM_MAIN_MENU, 0.0f, 0.35f);
		}
		scene.StartLevelTransition(tutorialJson_, activateSimulation_);
	}
	else if (IsPointInRect(mouseWorld, skipMin, skipMax)) {
		ClosePrompt(scene);
		if (audioManager_) {
			audioManager_->PlayUIClickSound();
			if (audioManager_->HasSound(MyoonchiPaths::Audio::SFX_START_BUTTON)) {
				audioManager_->PlaySound(MyoonchiPaths::Audio::SFX_START_BUTTON, audioManager_->GetVfxVolume(), false);
			}
			audioManager_->FadeChannel(MyoonchiPaths::Audio::BGM_MAIN_MENU, 0.0f, 0.35f);
		}

		scene.StartCutsceneTransitionedBounded(
			kIntroCutsceneFrames,
			{
				true,  // 1.1
				true,  // 2.1
				true,  // 3.1
				true,  // 4.1
				true,  // 5.1
				true,  // 6.1
			},
			skipJson_,
			activateSimulation_,
			0.35f, // fade out at boundaries
			0.35f, // fade in at boundaries
			1.50f, // per-frame hold (animation cadence)
			5,     // crossfade to frame index 5 (6.1)
			1.5f
			);

		if (audioManager_) {
			const float cutsceneBgmFadeIn = 1.0f;
			audioManager_->PlaySound(MyoonchiPaths::Audio::BGM_INTRO_CUTSCENE, 0.0f, false);
			audioManager_->FadeChannel(MyoonchiPaths::Audio::BGM_INTRO_CUTSCENE, audioManager_->GetBgmVolume() * 1.6f, cutsceneBgmFadeIn);
		}
	}
}

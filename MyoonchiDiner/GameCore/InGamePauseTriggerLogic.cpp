/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         InGamePauseTriggerLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Seah Wang Hua, wanghua.seah@digipen.edu

 DESCRIPTION:		Implements interaction logic for the in‑game pause trigger. This source file defines
					the behavior for a pause trigger UI element, including hover highlighting, click detection, 
					audio feedback, and transitioning the game into a paused state with an overlay.
 

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <string>

#include "EngineCore/AudioManager.hpp"
#include "EngineGraphics/GraphicsEngine.hpp"
#include "EngineGraphics/ResourceManager.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/InGamePauseTriggerLogic.hpp"
#include "GameCore/PlayerLogic.hpp"
#include "MyoonchiDiner/GamePaths.hpp"

 /**
  * @brief Builds a corresponding hover texture path from a base texture path.
  *
  * This helper inspects the input file path and ensures the returned path points
  * to the hover variant of the texture, following naming conventions:
  * - If the base name already ends with "_h", the original path is preserved.
  * - If it ends with "_s", the suffix is replaced with "_h".
  * - Otherwise, "_h" is appended before the extension.
  *
  * Examples:
  * - "button.png"      -> "button_h.png"
  * - "button_s.png"    -> "button_h.png"
  * - "button_h.png"    -> "button_h.png"
  *
  * @param path Original texture file path.
  * @return Path pointing to the matching hover‑state texture; if the input
  *         is empty, the empty string is returned.
  */
namespace {
	static std::string MakeHoverPath(const std::string& path) {
		if (path.empty()) {
			return path;
		}

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
	 * @brief Attempts to set a texture on the owner GameObject from a file path.
	 *
	 * This function uses the resource manager to load (or retrieve from cache)
	 * a texture associated with the given path and assigns it to the owner.
	 * If the owner is null or the path is empty, the call is ignored.
	 *
	 * Textures are cached under a "staticsprite_" prefix combined with the path
	 * to avoid redundant loads and allow reuse.
	 *
	 * @param owner Pointer to the GameObject that should receive the texture.
	 * @param texPath File path to the texture resource to load.
	 */
	static void TrySetTexture(GameObject* owner, const std::string& texPath) {
		if (!owner || texPath.empty()) {
			return;
		}

		const std::string cacheName = "staticsprite_" + texPath;
		if (Texture* tex = ResourceManager::Instance().LoadTexture(cacheName, texPath)) {
			owner->SetTexture(tex);
		}
	}
}

/**
 * @brief Per‑frame update for the in‑game pause trigger.
 *
 * The update routine performs the following:
 * - Early‑out if the simulation is inactive or the pause overlay is already shown.
 * - Lazily initializes the trigger by caching its normal and hover texture paths.
 * - Computes the mouse position in world space (using the graphics engine if
 *   available, with a fallback via InputManager).
 * - Performs simple AABB hit‑testing against the owner’s position and scale to
 *   determine hover state.
 * - On hover enter: swaps to the hover texture and plays a UI hover sound.
 * - On hover exit: restores the normal texture.
 * - On left‑mouse click while hovered: consumes the click, asks the player logic
 *   to enter the pause state, and requests the scene to display the pause overlay.
 *
 * @param dt Delta time since the previous frame (unused here, but part of the
 *        standard update signature).
 * @param scene Active scene containing the owner object and managing pause state.
 * @param input Input manager used for mouse position queries and button state.
 */
void InGamePauseTriggerLogic::Update(float /*dt*/, Scene& scene, InputManager& input) {
    #ifdef _DEBUG
	(void)scene;
	(void)input;
	return;
	#else
	// Skip when the game is not actively simulating or the pause UI is already visible.
	if (!scene.IsSimulationActive() || scene.IsPauseOverlayActive()) {
		return;
	}

	// Resolve the owning GameObject; without it there is nothing to update.
	GameObject* owner = GetOwner(scene);
	if (!owner) {
		return;
	}

	// One‑time initialization: capture the current texture path and derive the hover variant.
	if (!initialized_) {
		normalTexturePath_ = scene.GetObjectTexturePath(GetOwnerID());
		hoverTexturePath_ = MakeHoverPath(normalTexturePath_);
		initialized_ = true;
	}

	// Determine mouse position in world space, preferring the graphics engine's scene‑aware method.
	glm::vec2 mouseWorld{};
	if (!GraphicsEngine::Instance().GetMouseWorldInScene(mouseWorld)) {
		const glm::vec3 w = input.ScreenToWorld(
			static_cast<float>(input.GetMousePosition().x),
			static_cast<float>(input.GetMousePosition().y));
		mouseWorld = glm::vec2(w.x, w.y);
	}

	// Construct a simple axis‑aligned bounding box around the owner using position and scale.
	const glm::vec3 pos = owner->GetPositionGLM();
	const glm::vec3 sz = owner->GetScaleGLM();
	const float halfW = sz.x * 0.5f;
	const float halfH = sz.y * 0.5f;

	const bool over =
		mouseWorld.x >= (pos.x - halfW) && mouseWorld.x <= (pos.x + halfW) &&
		mouseWorld.y >= (pos.y - halfH) && mouseWorld.y <= (pos.y + halfH);

	// Handle hover enter: update state, swap texture, and play hover sound if available.
	if (over && !hovered_) {
		hovered_ = true;
		TrySetTexture(owner, hoverTexturePath_);
		if (AudioManager* audioManager = scene.GetAudioManager()) {
			if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_UI_HOVER)) {
				audioManager->PlaySound(MyoonchiPaths::Audio::SFX_UI_HOVER, audioManager->GetVfxVolume(), false);
			}
		}
	}
	// Handle hover exit: reset state and restore original texture.
	else if (!over && hovered_) {
		hovered_ = false;
		TrySetTexture(owner, normalTexturePath_);
	}

	// Only react to clicks when the cursor is over the trigger and the left button was just pressed.
	if (!over || !input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
		return;
	}

	if (AudioManager* audioManager = scene.GetAudioManager()) {
		if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_UI_BACK)) {
			audioManager->PlaySound(MyoonchiPaths::Audio::SFX_UI_BACK, audioManager->GetVfxVolume(), false);
		}
       if (audioManager->HasSound(MyoonchiPaths::Audio::SFX_UI_HOVER)) {
			audioManager->PlaySound(MyoonchiPaths::Audio::SFX_UI_HOVER, audioManager->GetVfxVolume(), false);
		}
	}

	// Consume the click so it is not processed by other UI or game elements.
	input.ConsumeNextMousePress(GLFW_MOUSE_BUTTON_LEFT);

	// Delegate entering the pause state to the player logic, if present.
	if (PlayerLogic* playerLogic = scene.GetLogicManager().GetLogicForObject<PlayerLogic>(scene.GetPlayerID())) {
		playerLogic->EnterPauseState(scene);
	}

	// Ask the scene to display the pause overlay UI.
	scene.ShowPauseOverlay();
   #endif
}

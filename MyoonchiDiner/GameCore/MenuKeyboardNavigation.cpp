/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         MenuKeyboardNavigation.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Implements shared keyboard-navigation helpers for menu-style UI.
					The file manages per-scope focus state, resolves authored
					button collections for each menu context, and reconciles
					keyboard focus with mouse hover so menu screens and pause
					overlays behave consistently.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>
#include <array>
#include <unordered_map>

#include "EngineCore/FilePaths.hpp"
#include "EngineCore/InputManager.hpp"
#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/MenuKeyboardNavigation.hpp"

namespace {
	/**
	 * @brief Remembers per-scope keyboard focus and the latest mouse activity.
	 * @details
	 * Each menu scope tracks which button is keyboard-focused, which button was
	 * last hovered by the mouse, and whether the scope should auto-focus a
	 * default button the first time it becomes active.
	 */
	struct FocusState {
		int focusedId = -1;
		int lastMouseHoveredId = -1;
		bool initialAutoFocusPending = true;
		glm::dvec2 lastMousePos{ 0.0, 0.0 };
		bool hasMousePos = false;
	};

	std::unordered_map<std::string, FocusState> gFocusStates;

	/**
	 * @brief Returns whether a button list currently contains a specific object ID.
	 * @param buttonIds Ordered button IDs to search.
	 * @param objectId Object ID to look for.
	 * @return True if the requested object ID appears in the list.
	 */
	bool ContainsId(const std::vector<int>& buttonIds, int objectId) {
		// Use a simple linear search because these button lists are intentionally tiny.
		return std::find(buttonIds.begin(), buttonIds.end(), objectId) != buttonIds.end();
	}

	/**
	 * @brief Sorts button IDs into their on-screen display order.
	 * @param scene Active scene containing the buttons.
	 * @param buttonIds Candidate button IDs to sort.
	 * @return Cleaned and sorted button IDs.
	 */
	std::vector<int> SortButtonsByDisplayOrder(Scene& scene, std::vector<int> buttonIds) {
		// Drop dead or invalid objects before sorting so focus never lands on stale IDs.
		buttonIds.erase(
			std::remove_if(buttonIds.begin(), buttonIds.end(),
				[&scene](int objectId) {
					return objectId < 0 || scene.GetGameObjectByID(objectId) == nullptr;
				}),
			buttonIds.end());

		// Sort primarily by vertical position, then by horizontal position, to match menu reading order.
		std::sort(buttonIds.begin(), buttonIds.end(),
			[&scene](int lhs, int rhs) {
				GameObject* lhsObj = scene.GetGameObjectByID(lhs);
				GameObject* rhsObj = scene.GetGameObjectByID(rhs);

				if (!lhsObj || !rhsObj) {
					return lhs < rhs;
				}

				const glm::vec3 lhsPos = lhsObj->GetPositionGLM();
				const glm::vec3 rhsPos = rhsObj->GetPositionGLM();

				if (lhsPos.y != rhsPos.y) {
					return lhsPos.y < rhsPos.y;
				}

				if (lhsPos.x != rhsPos.x) {
					return lhsPos.x < rhsPos.x;
				}

				return lhs < rhs;
			});

		return buttonIds;
	}

	/**
	 * @brief Returns whether a texture path matches a button's normal or hover variant.
	 * @param texturePath Texture path currently assigned to the object.
	 * @param normalTexturePath Authored normal-state texture path for the button.
	 * @return True if the texture path matches the normal or derived hover texture.
	 */
	bool MatchesTextureVariant(const std::string& texturePath, const char* normalTexturePath) {
		// Reject missing authored texture paths so callers do not treat empty textures as buttons.
		if (normalTexturePath == nullptr || normalTexturePath[0] == '\0') {
			return false;
		}

		if (texturePath == normalTexturePath) {
			return true;
		}

		const std::string normalPath(normalTexturePath);
		const size_t dot = normalPath.find_last_of('.');
		const std::string ext = (dot != std::string::npos) ? normalPath.substr(dot) : std::string();
		const std::string base = (dot != std::string::npos) ? normalPath.substr(0, dot) : normalPath;
		std::string hoverPath;
		if (base.size() >= 2 && base.substr(base.size() - 2) == "_s") {
			hoverPath = base.substr(0, base.size() - 2) + "_h" + ext;
		}
		else {
			hoverPath = base + "_h" + ext;
		}
		return texturePath == hoverPath;
	}

	/**
	 * @brief Returns the authored tag for a scene object.
	 * @param scene Active scene containing the object.
	 * @param objectId Runtime ID of the object to inspect.
	 * @return Object tag from the live scene or serialized defaults.
	 */
	std::string GetSceneObjectTag(Scene& scene, int objectId) {
		// Prefer the live object tag first so runtime overrides take effect immediately.
		std::string tag = scene.GetObjectTag(objectId);
		if (!tag.empty()) {
			return tag;
		}

		const Scene::Defaults defaults = scene.GetDefaults(objectId);
		return defaults.tag;
	}

	/**
	 * @brief Collects the authored main-menu buttons for the current scene.
	 * @param scene Active scene to inspect.
	 * @return Ordered main-menu button IDs.
	 */
	std::vector<int> CollectMainMenuButtons(Scene& scene) {
		// Main-menu navigation follows the standard authored button-tag set.
		return MenuKeyboardNavigation::CollectObjectsByTags(
			scene,
			{ "btn_play", "btn_howtoplay", "btn_settings", "btn_credits", "btn_quit" });
	}

	/**
	 * @brief Collects the authored settings-menu buttons for the current scene.
	 * @param scene Active scene to inspect.
	 * @return Ordered settings-menu button IDs.
	 */
	std::vector<int> CollectSettingsMenuButtons(Scene& scene) {
		// Include both slider widgets and discrete menu buttons in navigation order.
		return MenuKeyboardNavigation::CollectObjectsByTags(
			scene,
			{
				"settings_master_bar_visual",
				"settings_bgm_bar_visual",
				"settings_sfx_bar_visual",
				"settings_fullscreen_visual",
				"settings_windowed_visual",
				"settings_how_visual",
				"btn_main_menu"
			});
	}

	/**
	 * @brief Collects the authored win-screen buttons for the current scene.
	 * @param scene Active scene to inspect.
	 * @return Ordered win-screen button IDs.
	 */
	std::vector<int> CollectWinMenuButtons(Scene& scene) {
		// Win screens use a small fixed button set authored directly in the scene.
		return MenuKeyboardNavigation::CollectObjectsByTags(
			scene,
			{ "btn_next_level", "btn_howtoplay", "btn_main_menu" });
	}

	/**
	 * @brief Collects the authored lose-screen buttons for the current scene.
	 * @param scene Active scene to inspect.
	 * @return Ordered lose-screen button IDs.
	 */
	std::vector<int> CollectLoseMenuButtons(Scene& scene) {
		// Lose screens reuse the standard retry/help/main-menu button trio.
		return MenuKeyboardNavigation::CollectObjectsByTags(
			scene,
			{ "btn_retry_level", "btn_howtoplay", "btn_main_menu" });
	}

	/**
	 * @brief Collects the authored credits-screen buttons for the current scene.
	 * @param scene Active scene to inspect.
	 * @return Ordered credits-screen button IDs.
	 */
	std::vector<int> CollectCreditsMenuButtons(Scene& scene) {
		// Credits screens currently expose only the authored return-to-menu button.
		return MenuKeyboardNavigation::CollectObjectsByTags(scene, { "btn_main_menu" });
	}

	/**
	 * @brief Returns whether a scope should auto-focus its first button.
	 * @param scene Active scene containing the menu.
	 * @param scopeKey Focus scope key being evaluated.
	 * @param orderedButtonIds Ordered button IDs available in the scope.
	 * @return True if keyboard navigation should begin with the first button selected.
	 */
	bool ShouldAutoFocusFirstButtonForScope(Scene& scene, const std::string& scopeKey, const std::vector<int>& orderedButtonIds) {
		// Single-button menus and known modal scopes benefit from immediate default focus.
		if (orderedButtonIds.size() == 1 ||
			scopeKey.find("quit_popup") != std::string::npos ||
			scopeKey.find("pause_overlay") != std::string::npos ||
			scopeKey.find("settings") != std::string::npos) {
			return true;
		}

		if (scopeKey.find("top_level_menu") == std::string::npos) {
			return false;
		}

		const std::string levelPath = scene.GetCurrentLevelPath();
		return levelPath.find("win") != std::string::npos ||
			levelPath.find("dayclear") != std::string::npos ||
			levelPath.find("day_clear") != std::string::npos ||
			levelPath.find("lose") != std::string::npos;
	}
}

namespace MenuKeyboardNavigation {
	// This namespace centralizes scene-aware menu focus helpers so individual button scripts
	// can share the same keyboard navigation behavior instead of each tracking their own focus state.
	/**
	 * @brief Builds a focus-scope key for the current scene and the supplied scope name.
	 * @param scene Active scene whose level path should prefix the scope.
	 * @param scopeName Logical scope name for the menu region.
	 * @return Stable scope key used to store keyboard-focus state.
	 */
	std::string BuildScopeKey(Scene& scene, const char* scopeName) {
		// Prefix scopes with the current level path so focus does not bleed across scenes.
		const std::string& levelPath = scene.GetCurrentLevelPath();
		return (levelPath.empty() ? std::string("__boot__") : levelPath) + "::" + (scopeName ? scopeName : "menu");
	}

	/**
	 * @brief Returns the focus scope key for the current scene's top-level menu.
	 * @param scene Active scene containing the menu.
	 * @return Scope key used for top-level menu navigation.
	 */
	std::string GetCurrentSceneTopLevelScopeKey(Scene& scene) {
		// Top-level menus all share the same per-scene scope name.
		return BuildScopeKey(scene, "top_level_menu");
	}

	/**
	 * @brief Returns the focus scope key for the pause overlay.
	 * @param scene Active scene containing the pause overlay.
	 * @return Scope key used for pause-overlay navigation.
	 */
	std::string GetPauseOverlayScopeKey(Scene& scene) {
		// Keep pause-overlay focus isolated from the scene's top-level menu focus.
		return BuildScopeKey(scene, "pause_overlay");
	}

	/**
	 * @brief Collects scene objects whose tags match any of the supplied authored tags.
	 * @param scene Active scene to scan.
	 * @param tags Authored object tags that should be included.
	 * @return Ordered object IDs for the matching scene objects.
	 */
	std::vector<int> CollectObjectsByTags(Scene& scene, std::initializer_list<const char*> tags) {
		std::vector<int> buttonIds;

		// Preserve tag order while scanning the scene so the final sort only refines display placement.
		for (const char* tag : tags) {
			if (tag == nullptr || tag[0] == '\0') {
				continue;
			}

			for (GameObject* obj : scene.GetAllObjectsRaw()) {
				if (!obj) {
					continue;
				}

				if (GetSceneObjectTag(scene, obj->GetID()) == tag) {
					buttonIds.push_back(obj->GetID());
				}
			}
		}

		return SortButtonsByDisplayOrder(scene, std::move(buttonIds));
	}

	/**
	 * @brief Collects the current scene's top-level menu buttons.
	 * @param scene Active scene to inspect.
	 * @return Ordered object IDs for the current scene's top-level buttons.
	 */
	std::vector<int> CollectCurrentSceneTopLevelButtons(Scene& scene) {
		const std::string levelPath = scene.GetCurrentLevelPath();

		// Choose the button set from the current level path whenever the scene has already promoted it.
		if (levelPath.find("main_menu") != std::string::npos) {
			return CollectMainMenuButtons(scene);
		}

		if (levelPath.find("settings") != std::string::npos) {
			return CollectSettingsMenuButtons(scene);
		}

		if (levelPath.find("win") != std::string::npos) {
			return CollectWinMenuButtons(scene);
		}

		if (levelPath.find("lose") != std::string::npos) {
			return CollectLoseMenuButtons(scene);
		}

		if (levelPath.find("credits") != std::string::npos) {
			return CollectCreditsMenuButtons(scene);
		}

		// Fall back to visible authored button tags during first-load bootstrapping.
		if (std::vector<int> buttonIds = CollectMainMenuButtons(scene); !buttonIds.empty()) {
			return buttonIds;
		}

		if (std::vector<int> buttonIds = CollectSettingsMenuButtons(scene); !buttonIds.empty()) {
			return buttonIds;
		}

		if (std::vector<int> buttonIds = CollectWinMenuButtons(scene); !buttonIds.empty()) {
			return buttonIds;
		}

		if (std::vector<int> buttonIds = CollectLoseMenuButtons(scene); !buttonIds.empty()) {
			return buttonIds;
		}

		if (std::vector<int> buttonIds = CollectCreditsMenuButtons(scene); !buttonIds.empty()) {
			return buttonIds;
		}

		return {};
	}

	/**
	 * @brief Collects the currently visible pause-overlay buttons.
	 * @param scene Active scene to inspect.
	 * @return Ordered object IDs for the pause-overlay buttons.
	 */
	std::vector<int> CollectPauseOverlayButtons(Scene& scene) {
		std::vector<int> buttonIds;
		static constexpr std::array<const char*, 3> kPauseButtonTextures{
			FilePaths::Textures::BTN_RESUME,
			FilePaths::Textures::BTN_HOW,
			FilePaths::Textures::BTN_QUIT
		};

		// Pause overlay buttons are identified by their overlay layer and known button textures.
		for (GameObject* obj : scene.GetAllObjectsRaw()) {
			if (!obj) {
				continue;
			}

			const int objectId = obj->GetID();
			if (scene.GetObjectLayer(objectId) != "999999") {
				continue;
			}

			const std::string texturePath = scene.GetObjectTexturePath(objectId);
			bool matchesPauseButton = false;
			for (const char* normalTexturePath : kPauseButtonTextures) {
				if (MatchesTextureVariant(texturePath, normalTexturePath)) {
					matchesPauseButton = true;
					break;
				}
			}

			if (matchesPauseButton) {
				buttonIds.push_back(objectId);
			}
		}

		return SortButtonsByDisplayOrder(scene, std::move(buttonIds));
	}

	/**
	 * @brief Updates keyboard focus for a menu scope and returns the focused button.
	 * @param scene Active scene containing the candidate buttons.
	 * @param input Input manager used to read and consume navigation keys.
	 * @param scopeKey Stable focus scope key for this menu region.
	 * @param buttonIds Candidate button IDs for the scope.
	 * @param mouseHoveredId Currently mouse-hovered button ID, or `-1` when none is hovered.
	 * @return Focused button ID after processing input, or `-1` when no focus remains.
	 */
	int UpdateFocus(Scene& scene, InputManager& input, const std::string& scopeKey, const std::vector<int>& buttonIds, int mouseHoveredId) {
		// Drop stale scope state immediately when the menu no longer exposes any buttons.
		if (buttonIds.empty()) {
			gFocusStates.erase(scopeKey);
			return -1;
		}

		const std::vector<int> orderedButtonIds = SortButtonsByDisplayOrder(scene, buttonIds);
		if (orderedButtonIds.empty()) {
			gFocusStates.erase(scopeKey);
			return -1;
		}

		// Retrieve or create the remembered focus state for this specific menu scope.
		FocusState& state = gFocusStates[scopeKey];

		const glm::dvec2 mousePos = input.GetMousePosition();
		const bool mouseMoved = state.hasMousePos &&
			(mousePos.x != state.lastMousePos.x || mousePos.y != state.lastMousePos.y);
		state.lastMousePos = mousePos;
		state.hasMousePos = true;

		// Clear stale remembered IDs whenever the underlying button set changes.
		if (!ContainsId(orderedButtonIds, state.focusedId)) {
			state.focusedId = -1;
		}

		if (!ContainsId(orderedButtonIds, state.lastMouseHoveredId)) {
			state.lastMouseHoveredId = -1;
		}

		if (ContainsId(orderedButtonIds, mouseHoveredId)) {
			state.lastMouseHoveredId = mouseHoveredId;
		}

		// Mouse activity clears keyboard focus so hover and keyboard selection never fight each other.
		if (mouseMoved || input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
			state.focusedId = -1;
			state.initialAutoFocusPending = false;
		}

		// Some modal scopes benefit from defaulting focus onto the first button automatically.
		const bool shouldAutoFocusFirstButton =
			state.initialAutoFocusPending &&
			(state.focusedId < 0) &&
			ShouldAutoFocusFirstButtonForScope(scene, scopeKey, orderedButtonIds);
		if (shouldAutoFocusFirstButton) {
			state.focusedId = orderedButtonIds.front();
			state.initialAutoFocusPending = false;
		}

		// Convert Up/Down presses into a focus movement delta and consume them immediately.
		int moveDelta = 0;
		if (input.IsKeyJustPressed(GLFW_KEY_DOWN)) {
			moveDelta = 1;
			input.ConsumeNextKeyPress(GLFW_KEY_DOWN);
		}
		else if (input.IsKeyJustPressed(GLFW_KEY_UP)) {
			moveDelta = -1;
			input.ConsumeNextKeyPress(GLFW_KEY_UP);
		}

		// Apply keyboard movement by stepping through the sorted button list.
		if (moveDelta != 0) {
			state.initialAutoFocusPending = false;
			if (state.focusedId < 0) {
				if (ContainsId(orderedButtonIds, state.lastMouseHoveredId)) {
					state.focusedId = state.lastMouseHoveredId;
				}
				else {
					state.focusedId = (moveDelta > 0) ? orderedButtonIds.front() : orderedButtonIds.back();
				}
			}
			else {
				const auto it = std::find(orderedButtonIds.begin(), orderedButtonIds.end(), state.focusedId);
				if (it == orderedButtonIds.end()) {
					state.focusedId = (moveDelta > 0) ? orderedButtonIds.front() : orderedButtonIds.back();
				}
				else {
					const int currentIndex = static_cast<int>(std::distance(orderedButtonIds.begin(), it));
					const int maxIndex = static_cast<int>(orderedButtonIds.size()) - 1;
					const int nextIndex = std::clamp(currentIndex + moveDelta, 0, maxIndex);
					state.focusedId = orderedButtonIds[static_cast<size_t>(nextIndex)];
				}
			}
		}

		return state.focusedId;
	}

	/**
	 * @brief Consumes an Enter-based submit press if one is pending.
	 * @param input Input manager used to inspect and consume submit keys.
	 * @return True if a submit key press was consumed.
	 */
	bool ConsumeSubmitPress(InputManager& input) {
		// Treat either main Enter key as a submit action for menu navigation.
		if (input.IsKeyJustPressed(GLFW_KEY_ENTER)) {
			input.ConsumeNextKeyPress(GLFW_KEY_ENTER);
			return true;
		}

		if (input.IsKeyJustPressed(GLFW_KEY_KP_ENTER)) {
			input.ConsumeNextKeyPress(GLFW_KEY_KP_ENTER);
			return true;
		}

		return false;
	}

	/**
	 * @brief Clears remembered keyboard focus for a specific menu scope.
	 * @param scopeKey Focus scope key whose state should be erased.
	 */
	void ClearFocus(const std::string& scopeKey) {
		// Remove the stored state entirely so the scope reinitializes from scratch next time.
		gFocusStates.erase(scopeKey);
	}
}

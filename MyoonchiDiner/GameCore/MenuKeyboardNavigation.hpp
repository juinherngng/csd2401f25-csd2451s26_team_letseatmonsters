/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         MenuKeyboardNavigation.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Declares shared keyboard-navigation helpers for menu-style UI.
					The module builds per-scene focus scopes, gathers authored
					button groups for different menu screens, and exposes the
					functions used to keep keyboard focus, mouse hover, and
					submit handling consistent across menus and overlays.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <initializer_list>
#include <string>
#include <vector>

class InputManager;
class Scene;

/**
 * @brief Shared helpers for keyboard-driven menu focus and submit handling.
 * @details
 * The helpers in this namespace build stable focus scopes per scene, collect
 * authored button groups for the active menu state, and reconcile keyboard
 * focus with mouse hover so menu screens can support both input styles cleanly.
 */
namespace MenuKeyboardNavigation {
	/**
	 * @brief Builds a focus-scope key for the current scene and the supplied scope name.
	 * @param scene Active scene whose level path should prefix the scope.
	 * @param scopeName Logical scope name for the menu region.
	 * @return Stable scope key used to store keyboard-focus state.
	 */
	std::string BuildScopeKey(Scene& scene, const char* scopeName);

	/**
	 * @brief Returns the focus scope key for the current scene's top-level menu.
	 * @param scene Active scene containing the menu.
	 * @return Scope key used for top-level menu navigation.
	 */
	std::string GetCurrentSceneTopLevelScopeKey(Scene& scene);

	/**
	 * @brief Returns the focus scope key for the pause overlay.
	 * @param scene Active scene containing the pause overlay.
	 * @return Scope key used for pause-overlay navigation.
	 */
	std::string GetPauseOverlayScopeKey(Scene& scene);

	/**
	 * @brief Collects scene objects whose tags match any of the supplied authored tags.
	 * @param scene Active scene to scan.
	 * @param tags Authored object tags that should be included.
	 * @return Ordered object IDs for the matching scene objects.
	 */
	std::vector<int> CollectObjectsByTags(Scene& scene, std::initializer_list<const char*> tags);

	/**
	 * @brief Collects the current scene's top-level menu buttons.
	 * @param scene Active scene to inspect.
	 * @return Ordered object IDs for the current scene's top-level buttons.
	 */
	std::vector<int> CollectCurrentSceneTopLevelButtons(Scene& scene);

	/**
	 * @brief Collects the currently visible pause-overlay buttons.
	 * @param scene Active scene to inspect.
	 * @return Ordered object IDs for the pause-overlay buttons.
	 */
	std::vector<int> CollectPauseOverlayButtons(Scene& scene);

	/**
	 * @brief Updates keyboard focus for a menu scope and returns the focused button.
	 * @param scene Active scene containing the candidate buttons.
	 * @param input Input manager used to read and consume navigation keys.
	 * @param scopeKey Stable focus scope key for this menu region.
	 * @param buttonIds Candidate button IDs for the scope.
	 * @param mouseHoveredId Currently mouse-hovered button ID, or `-1` when none is hovered.
	 * @return Focused button ID after processing input, or `-1` when no focus remains.
	 */
	int UpdateFocus(Scene& scene, InputManager& input, const std::string& scopeKey, const std::vector<int>& buttonIds, int mouseHoveredId);

	/**
	 * @brief Consumes an Enter-based submit press if one is pending.
	 * @param input Input manager used to inspect and consume submit keys.
	 * @return True if a submit key press was consumed.
	 */
	bool ConsumeSubmitPress(InputManager& input);

	/**
	 * @brief Clears remembered keyboard focus for a specific menu scope.
	 * @param scopeKey Focus scope key whose state should be erased.
	 */
	void ClearFocus(const std::string& scopeKey);
}

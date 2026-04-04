#include "GameCore/MenuKeyboardNavigation.hpp"

#include <algorithm>
#include <array>
#include <unordered_map>

#include "EngineCore/FilePaths.hpp"
#include "EngineCore/InputManager.hpp"
#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/SceneManager.hpp"

namespace {
	struct FocusState {
		int focusedId = -1;
		int lastMouseHoveredId = -1;
		bool initialAutoFocusPending = true;
		glm::dvec2 lastMousePos{ 0.0, 0.0 };
		bool hasMousePos = false;
	};

	std::unordered_map<std::string, FocusState> gFocusStates;

	bool ContainsId(const std::vector<int>& buttonIds, int objectId) {
		return std::find(buttonIds.begin(), buttonIds.end(), objectId) != buttonIds.end();
	}

	std::vector<int> SortButtonsByDisplayOrder(Scene& scene, std::vector<int> buttonIds) {
		buttonIds.erase(
			std::remove_if(buttonIds.begin(), buttonIds.end(),
				[&scene](int objectId) {
					return objectId < 0 || scene.GetGameObjectByID(objectId) == nullptr;
				}),
			buttonIds.end());

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

	bool MatchesTextureVariant(const std::string& texturePath, const char* normalTexturePath) {
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

	std::string GetSceneObjectTag(Scene& scene, int objectId) {
		std::string tag = scene.GetObjectTag(objectId);
		if (!tag.empty()) {
			return tag;
		}

		const Scene::Defaults defaults = scene.GetDefaults(objectId);
		return defaults.tag;
	}

	std::vector<int> CollectMainMenuButtons(Scene& scene) {
		return MenuKeyboardNavigation::CollectObjectsByTags(
			scene,
			{ "btn_play", "btn_howtoplay", "btn_settings", "btn_credits", "btn_quit" });
	}

	std::vector<int> CollectSettingsMenuButtons(Scene& scene) {
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

	std::vector<int> CollectWinMenuButtons(Scene& scene) {
		return MenuKeyboardNavigation::CollectObjectsByTags(
			scene,
			{ "btn_next_level", "btn_howtoplay", "btn_main_menu" });
	}

	std::vector<int> CollectLoseMenuButtons(Scene& scene) {
		return MenuKeyboardNavigation::CollectObjectsByTags(
			scene,
			{ "btn_retry_level", "btn_howtoplay", "btn_main_menu" });
	}

	std::vector<int> CollectCreditsMenuButtons(Scene& scene) {
		return MenuKeyboardNavigation::CollectObjectsByTags(scene, { "btn_main_menu" });
	}

	bool ShouldAutoFocusFirstButtonForScope(Scene& scene, const std::string& scopeKey, const std::vector<int>& orderedButtonIds) {
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
	std::string BuildScopeKey(Scene& scene, const char* scopeName) {
		const std::string& levelPath = scene.GetCurrentLevelPath();
		return (levelPath.empty() ? std::string("__boot__") : levelPath) + "::" + (scopeName ? scopeName : "menu");
	}

	std::string GetCurrentSceneTopLevelScopeKey(Scene& scene) {
		return BuildScopeKey(scene, "top_level_menu");
	}

	std::string GetPauseOverlayScopeKey(Scene& scene) {
		return BuildScopeKey(scene, "pause_overlay");
	}

	std::vector<int> CollectObjectsByTags(Scene& scene, std::initializer_list<const char*> tags) {
		std::vector<int> buttonIds;

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

	std::vector<int> CollectCurrentSceneTopLevelButtons(Scene& scene) {
		const std::string levelPath = scene.GetCurrentLevelPath();

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

		// First-load fallback: when the initial scene bootstraps before the
		// current level path is fully promoted, detect the visible menu by its
		// authored button tags so keyboard navigation still works immediately.
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

	std::vector<int> CollectPauseOverlayButtons(Scene& scene) {
		std::vector<int> buttonIds;
		static constexpr std::array<const char*, 3> kPauseButtonTextures{
			FilePaths::Textures::BTN_RESUME,
			FilePaths::Textures::BTN_HOW,
			FilePaths::Textures::BTN_QUIT
		};

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

	int UpdateFocus(Scene& scene, InputManager& input, const std::string& scopeKey, const std::vector<int>& buttonIds, int mouseHoveredId) {
		if (buttonIds.empty()) {
			gFocusStates.erase(scopeKey);
			return -1;
		}

		const std::vector<int> orderedButtonIds = SortButtonsByDisplayOrder(scene, buttonIds);
		if (orderedButtonIds.empty()) {
			gFocusStates.erase(scopeKey);
			return -1;
		}

		FocusState& state = gFocusStates[scopeKey];

		const glm::dvec2 mousePos = input.GetMousePosition();
		const bool mouseMoved = state.hasMousePos &&
			(mousePos.x != state.lastMousePos.x || mousePos.y != state.lastMousePos.y);
		state.lastMousePos = mousePos;
		state.hasMousePos = true;

		if (!ContainsId(orderedButtonIds, state.focusedId)) {
			state.focusedId = -1;
		}

		if (!ContainsId(orderedButtonIds, state.lastMouseHoveredId)) {
			state.lastMouseHoveredId = -1;
		}

		if (ContainsId(orderedButtonIds, mouseHoveredId)) {
			state.lastMouseHoveredId = mouseHoveredId;
		}

		// Mouse activity clears keyboard focus so a missed click never activates
		// the previously selected button underneath the cursor.
		if (mouseMoved || input.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
			state.focusedId = -1;
			state.initialAutoFocusPending = false;
		}

		const bool shouldAutoFocusFirstButton =
			state.initialAutoFocusPending &&
			(state.focusedId < 0) &&
			ShouldAutoFocusFirstButtonForScope(scene, scopeKey, orderedButtonIds);
		if (shouldAutoFocusFirstButton) {
			state.focusedId = orderedButtonIds.front();
			state.initialAutoFocusPending = false;
		}

		int moveDelta = 0;
		if (input.IsKeyJustPressed(GLFW_KEY_DOWN)) {
			moveDelta = 1;
			input.ConsumeNextKeyPress(GLFW_KEY_DOWN);
		}
		else if (input.IsKeyJustPressed(GLFW_KEY_UP)) {
			moveDelta = -1;
			input.ConsumeNextKeyPress(GLFW_KEY_UP);
		}

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

	bool ConsumeSubmitPress(InputManager& input) {
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

	void ClearFocus(const std::string& scopeKey) {
		gFocusStates.erase(scopeKey);
	}
}

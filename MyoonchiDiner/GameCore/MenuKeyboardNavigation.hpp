#pragma once

#include <initializer_list>
#include <string>
#include <vector>

class InputManager;
class Scene;

namespace MenuKeyboardNavigation {
	std::string BuildScopeKey(Scene& scene, const char* scopeName);
	std::string GetCurrentSceneTopLevelScopeKey(Scene& scene);
	std::string GetPauseOverlayScopeKey(Scene& scene);

	std::vector<int> CollectObjectsByTags(Scene& scene, std::initializer_list<const char*> tags);
	std::vector<int> CollectCurrentSceneTopLevelButtons(Scene& scene);
	std::vector<int> CollectPauseOverlayButtons(Scene& scene);

	int UpdateFocus(Scene& scene, InputManager& input, const std::string& scopeKey, const std::vector<int>& buttonIds, int mouseHoveredId);
	bool ConsumeSubmitPress(InputManager& input);
	void ClearFocus(const std::string& scopeKey);
}

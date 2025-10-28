#pragma once

#include <string>

#include "LevelSerializer.hpp"

class Scene;

class LevelEditor {
public:
	void Toggle() { enabled_ = !enabled_; }
	bool IsEnabled() const { return enabled_; }
	void SetPath(std::string p) { levelPath_ = std::move(p); }

	// Call each frame when enabled
	void DrawUI(Scene& scene);

	// Call this once at startup
	bool LoadIntoScene(Scene& scene);

private:
	bool enabled_ = true;
	int selectedIndex_ = -1;
	std::string levelPath_ = "../levels/kitchen01.json";
	LevelData level_;
};

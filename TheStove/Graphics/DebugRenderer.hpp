#pragma once

#include <glm/glm.hpp>
#include "ResourceManager.hpp"
#include <vector>
#include <cmath>

class DebugRenderer {
public:
	static void Init();
	static void Shutdown();
	static void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);
	static void DrawRect(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color);
	static void DrawCircle(const glm::vec3& center, float radius, const glm::vec3& color, int segments = 32);
	static void Flush(const glm::mat4& view, const glm::mat4& proj);

	static void SetEnabled(bool enabled);
	static bool IsEnabled();
};

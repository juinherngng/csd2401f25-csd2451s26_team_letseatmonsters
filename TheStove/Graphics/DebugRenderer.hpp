/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			DebugRenderer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <map>

#include "ResourceManager.hpp"
#include "Shader.hpp"

class DebugRenderer {
public:
	static void Init();
	static void Shutdown();

	static void SetEnabled(bool enabled);
	static bool IsEnabled();

	static void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);
	static void DrawRect(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color);
	static void DrawPoint(const glm::vec3& p, const glm::vec3& color, float size = 5.0f);

	static void Flush(const glm::mat4& view, const glm::mat4& proj);
};

/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			DebugRenderer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Provides a lightweight OpenGL debug rendering utility for visualizing
					points, lines, and rectangles in 2D/3D scenes.

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
	// Initializes OpenGL resources for the debug renderer.
	static void Init();

	// Releases all OpenGL buffers/VAOs.
	static void Shutdown();

	// Enables or disables debug rendering globally.
	static void SetEnabled(bool enable);
	static bool IsEnabled();

	// Draws a line between two points in world space.
	static void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);

	// Draws a single point in world space.
	static void DrawPoint(const glm::vec3& position, const glm::vec3& color, float size = 5.0f);

	// Draws an axis-aligned rectangle (mn = min corner, mx = max corner).
	static void DrawRect(const glm::vec3& minCorner, const glm::vec3& maxCorner, const glm::vec3& color);

	// Uploads all accumulated debug geometry to GPU and draws it.
	static void Flush(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
};

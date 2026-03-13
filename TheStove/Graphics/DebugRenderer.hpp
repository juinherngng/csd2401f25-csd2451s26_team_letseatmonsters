/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			DebugRenderer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Declares a lightweight static-only DebugRenderer used to batch and draw
					debug lines, points, and rectangles for on-screen visualization overlays.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "ResourceManager.hpp"
#include "Shader.hpp"

#include <cmath>
#include <glm/glm.hpp>
#include <map>
#include <vector>

 // A lightweight static-only DebugRenderer used to batch and draw debug lines, points, and rectangles for on-screen visualization overlays.
class DebugRenderer {
public:
	// Initialization and shutdown (called by GraphicsEngine)
	static void Init();
	static void Shutdown();

	// Enable or disable debug rendering globally (no-op if not initialized).
	static void SetEnabled(bool enable);
	static bool IsEnabled();

	// Drawing API - queues primitives for drawing on the next Flush call.
	static void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);
	static void DrawPoint(const glm::vec3& position, const glm::vec3& color, float size = 5.0f);
	static void DrawRect(const glm::vec3& minCorner, const glm::vec3& maxCorner, const glm::vec3& color);

	// Flushes all queued primitives to the screen using the provided view and projection matrices, then clears the queues.
	static void Flush(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
};

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

 /**
  * @class DebugRenderer
  * @brief Static helper for batched debug drawing (lines, points, rects).
  *
  * Typical usage per-frame:
  *   1) Call DrawLine/DrawPoint/DrawRect any number of times.
  *   2) Call Flush(view, projection) at end of frame to render and clear batches.
  */
class DebugRenderer {
public:
	// ----- Lifecycle -----

	// Creates the internal GL objects for line/point batches.
	static void Init();

	// Destroys internal GL objects; safe to call on shutdown.
	static void Shutdown();

	// ----- State -----

	// Enables or disables debug rendering globally. 
	static void SetEnabled(bool enable);

	// True if debug rendering is currently enabled.
	static bool IsEnabled();

	// ----- Issue Draws -----

	// Enqueues a line segment.
	static void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);

	// Enqueues a single point with a size (in pixels).
	static void DrawPoint(const glm::vec3& position, const glm::vec3& color, float size = 5.0f);

	// Enqueues an axis-aligned rectangle from min/max corners.
	static void DrawRect(const glm::vec3& minCorner, const glm::vec3& maxCorner, const glm::vec3& color);

	// ----- Render & Clear -----

	// Renders all queued primitives with the given view/projection and clears batches.
	static void Flush(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
};

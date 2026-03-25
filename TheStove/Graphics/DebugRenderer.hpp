/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			DebugRenderer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Declares a lightweight static-only DebugRenderer used to batch and draw
					debug lines, points, and rectangles for on-screen visualization overlays.

		 All content  2025 DigiPen Institute of Technology Singapore. All rights reserved.
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

	/**
	 * @brief Initializes this object.
	 * @return Result produced by this operation.
	 */
	static void Init();

	/**
	 * @brief Performs shutdown.
	 * @return Result produced by this operation.
	 */
	static void Shutdown();

	/**
	 * @brief Sets enabled.
	 * @param enable Boolean flag controlling whether the feature is enabled.
	 * @return Result produced by this operation.
	 */
	static void SetEnabled(bool enable);

	/**
	 * @brief Returns whether enabled.
	 * @return True when the operation succeeds or the condition is met.
	 */
	static bool IsEnabled();

	/**
	 * @brief Draws line.
	 * @param start Parameter for start.
	 * @param end Parameter for end.
	 * @param color Parameter for color.
	 * @return Result produced by this operation.
	 */
	static void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);

	/**
	 * @brief Draws point.
	 * @param position Parameter for position.
	 * @param color Parameter for color.
	 * @param size Parameter for size.
	 * @return Result produced by this operation.
	 */
	static void DrawPoint(const glm::vec3& position, const glm::vec3& color, float size = 5.0f);

	/**
	 * @brief Draws rect.
	 * @param minCorner Parameter for min corner.
	 * @param maxCorner Parameter for max corner.
	 * @param color Parameter for color.
	 * @return Result produced by this operation.
	 */
	static void DrawRect(const glm::vec3& minCorner, const glm::vec3& maxCorner, const glm::vec3& color);

	/**
	 * @brief Performs flush.
	 * @param viewMatrix Parameter for view matrix.
	 * @param projectionMatrix Parameter for projection matrix.
	 * @return Result produced by this operation.
	 */
	static void Flush(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
};

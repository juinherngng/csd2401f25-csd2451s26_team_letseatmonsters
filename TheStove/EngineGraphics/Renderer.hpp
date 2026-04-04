/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Renderer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Wrapper for clear color, buffer clear, and platform init

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

 // Forward declare Renderer to avoid circular dependency.
class Renderer {
public:
	/**
	 * @brief Initializes the default OpenGL render state used by the engine.
	 */
	void Initialize();

	/**
	 * @brief Clears the active frame buffers before drawing a new frame.
	 */
	void Clear();

	/**
	 * @brief Sets the OpenGL clear color used by subsequent buffer clears.
	 * @param r Red channel in normalized range.
	 * @param g Green channel in normalized range.
	 * @param b Blue channel in normalized range.
	 * @param a Alpha channel in normalized range.
	 */
	void SetClearColor(float r, float g, float b, float a);
};

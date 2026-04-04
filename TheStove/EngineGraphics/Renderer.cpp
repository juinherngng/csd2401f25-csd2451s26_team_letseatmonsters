/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Renderer.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (85%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	    (15%)

 DESCRIPTION:		Sets up default GL state and provides Clear() and SetClearColor().

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <glad/glad.h>

#include "EngineGraphics/Renderer.hpp"

 /**
  * @brief Initializes the default OpenGL state needed by the renderer.
  */
void Renderer::Initialize() {
	// Enable depth testing so nearer geometry properly occludes farther geometry.
	glEnable(GL_DEPTH_TEST);
	// Enable alpha blending for sprites, UI, and other translucent draw calls.
	glEnable(GL_BLEND);
	// Use standard source-alpha blending for the engine's textured assets.
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

/**
 * @brief Clears both the color and depth buffers for the current frame.
 */
void Renderer::Clear() {
	// Reset the frame's render targets before any new scene draw calls are issued.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/**
 * @brief Updates the RGBA color used when clearing the framebuffer.
 * @param r Red channel in normalized range.
 * @param g Green channel in normalized range.
 * @param b Blue channel in normalized range.
 * @param a Alpha channel in normalized range.
 */
void Renderer::SetClearColor(float r, float g, float b, float a) {
	// OpenGL stores this as state and applies it the next time Clear() runs.
	glClearColor(r, g, b, a);
}

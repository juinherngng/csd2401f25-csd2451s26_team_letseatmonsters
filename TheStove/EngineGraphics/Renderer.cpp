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

void Renderer::Initialize() {
	// Initialize any OpenGL states here if needed
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer::Clear() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::SetClearColor(float r, float g, float b, float a) {
	glClearColor(r, g, b, a);
}

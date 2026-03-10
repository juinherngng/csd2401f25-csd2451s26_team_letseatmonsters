/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Renderer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		Wrapper for clear color, buffer clear, and platform init

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

 // Forward declare Renderer to avoid circular dependency.
class Renderer {
public:
	void Initialize();
	void Clear();
	void SetClearColor(float r, float g, float b, float a);
};

/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			InputControls.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Glenn Yeo Yi Heng, g.yeo@digipen.edu

 DESCRIPTION:		Input Control function declarations.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <GLFW/glfw3.h>

class InputControls {
public:
	// Called once per frame to poll keyboard's state
	static void PollKeyboard(GLFWwindow* window);

	// window -> which window triggered the callback
	// button -> which button was pressed
	// action -> what happened (Press or Release)
	// mods -> do we need mods? like if holding shift and wat not
	static void MouseButtonCallback(GLFWwindow* window, int button, int action);

	// Other scripts call these
	// e.g. if (InputControls::LeftMousePressed) , do the thing
	static bool LeftMousePressed();
	static bool WKeyPressed();
	static bool AKeyPressed();
	static bool SKeyPressed();
	static bool DKeyPressed();

	// For getting mouse position
	static double MouseXPos();
	static double MouseYPos();

private:
	static bool leftMouse;
	static bool w, a, s, d;
	static double mouseX, mouseY;
};
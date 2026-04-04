/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			RuntimeTextData.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu	(100%)

 DESCRIPTION:		Declares the runtime-owned text object data used by Scene and gameplay
					systems so authored HUD text does not depend on editor panel globals.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <string>

 /**
  * @brief Serializable/runtime data for one authored text object.
  */
struct RuntimeTextData {
	std::string name;
	std::string fontName;
	std::string text;
	std::string horizontalAlign{ "left" };
	float x{ 0.0f };
	float y{ 0.0f };
	float scale{ 1.0f };
	float rotation{ 0.0f };
	bool useBlockRotation{ true };
	float colorR{ 1.0f };
	float colorG{ 1.0f };
	float colorB{ 1.0f };
	float colorA{ 1.0f };
	std::string layer{ "1" };
	bool visible{ true };
	float depth{ 0.0f };
};

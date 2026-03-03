/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         ExitGateLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Declares the ExitGateLogic component, which defines the world-space
					exit target for NPCs and provides an optional positional offset.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "../Core/GameObjectLogic.hpp"

#include "Math.hpp"

class ExitGateLogic : public GameObjectLogic {
public:
	// Constructor
	using GameObjectLogic::GameObjectLogic;

	// Override GetName for debugging purposes
	std::string GetName() const override {
		return "ExitGateLogic";
	}

	// Where NPC should walk to (optionally offset inside the walk area)
	Math::Vector2D GetExitTargetWorld(Scene& scene) const;

	// Set an optional local offset for the exit target (e.g., to specify a point inside the walk area)
	void SetExitOffset(const Math::Vector2D& localOffset) {
		exitOffset_ = localOffset;
	}

private:
	// Local offset from the GameObject's position to define the actual exit target within the walk area
	Math::Vector2D exitOffset_{ 0.0f, 0.0f };
};

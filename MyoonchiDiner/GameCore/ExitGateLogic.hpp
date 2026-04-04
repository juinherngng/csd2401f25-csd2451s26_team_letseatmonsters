/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         ExitGateLogic.hpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Declares the ExitGateLogic component, which defines the world-space
					exit target for NPCs and provides an optional positional offset.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "EngineCore/GameObjectLogic.hpp"
#include "EngineCore/Math.hpp"

class ExitGateLogic : public GameObjectLogic {
public:
	using GameObjectLogic::GameObjectLogic;

	/**
	 * @brief Returns the stable runtime logic name used by the engine.
	 * @return Name string for this logic component.
	 */
	std::string GetName() const override {
		// Keep the logic name stable for debugging and runtime lookup helpers.
		return "ExitGateLogic";
	}

	/**
	 * @brief Returns the world-space exit target NPCs should walk toward.
	 * @param scene Active scene containing the exit gate object.
	 * @return World-space exit target including the authored local offset.
	 */
	Math::Vector2D GetExitTargetWorld(Scene& scene) const;

	/**
	 * @brief Sets an optional local offset for the exit target.
	 * @param localOffset Local offset from the gate object's position.
	 */
	void SetExitOffset(const Math::Vector2D& localOffset) {
		// Store the authored offset so NPCs aim at a precise point inside the exit area.
		exitOffset_ = localOffset;
	}

private:
	Math::Vector2D exitOffset_{ 0.0f, 0.0f };
};

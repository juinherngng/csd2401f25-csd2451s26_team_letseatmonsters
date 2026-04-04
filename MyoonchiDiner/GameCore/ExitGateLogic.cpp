/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         ExitGateLogic.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:       Defines the ExitGateLogic component, which defines the world-space
					exit target for NPCs and provides an optional positional offset.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "EngineGraphics/GameObject.hpp"
#include "EngineGraphics/SceneManager.hpp"
#include "GameCore/ExitGateLogic.hpp"

 /**
  * @brief Returns the world-space exit target NPCs should use for this gate.
  * @param scene Active scene containing the exit gate object.
  * @return World-space exit target including the authored local offset.
  */
Math::Vector2D ExitGateLogic::GetExitTargetWorld(Scene& scene) const {
	// Resolve the owning object first so the exit point follows the actual gate placement.
	GameObject* owner = GetOwner(scene);
	if (!owner) return Math::Vector2D(0.0f, 0.0f);

	// Add the authored local offset so NPCs can target a walkable point near the gate.
	glm::vec3 p = owner->GetPositionGLM();
	return Math::Vector2D(p.x + exitOffset_.x, p.y + exitOffset_.y);
}

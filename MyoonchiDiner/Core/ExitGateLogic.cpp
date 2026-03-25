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

#include "Core/ExitGateLogic.hpp"
#include "Graphics/GameObject.hpp"
#include "Graphics/SceneManager.hpp"

Math::Vector2D ExitGateLogic::GetExitTargetWorld(Scene& scene) const {
	GameObject* owner = GetOwner(scene);
	if (!owner) return Math::Vector2D(0.0f, 0.0f);

	glm::vec3 p = owner->GetPositionGLM();
	return Math::Vector2D(p.x + exitOffset_.x, p.y + exitOffset_.y);
}


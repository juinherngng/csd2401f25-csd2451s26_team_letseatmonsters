/*
----------------------------------------------------------------------------------------------------
FILE NAME:			GameObjectLogic.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Vu Phan Hung, phanhung.vu@digipen.edu

DESCRIPTION:		Implements GameObject logic base class, providing ownership linking
					and access to the parent Scene's GameObject by ID.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/
#include "GameObjectLogic.hpp"
#include "../Graphics/SceneManager.hpp" // for Scene::GetGameObjectByID

GameObject* GameObjectLogic::GetOwner(Scene& scene) const {
    return scene.GetGameObjectByID(ownerID);
}

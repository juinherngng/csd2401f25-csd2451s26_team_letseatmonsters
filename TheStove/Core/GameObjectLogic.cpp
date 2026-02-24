/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GameObjectLogic.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:		Implements the base logic helper for game objects. Provides the GetOwner()
					function, allowing logic scripts to retrieve their associated GameObject via
					the Scene's ID lookup system.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/
#include "GameObjectLogic.hpp"

#include "../Graphics/SceneManager.hpp" // for Scene::GetGameObjectByID

#include "GameObjectLogic.hpp"

GameObject* GameObjectLogic::GetOwner(Scene& scene) const {
	return scene.GetGameObjectByID(ownerID);
}

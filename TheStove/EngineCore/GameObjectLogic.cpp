/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GameObjectLogic.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (60%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (40%)

 DESCRIPTION:		Implements the base logic helper for game objects. Provides the GetOwner()
					function, allowing logic scripts to retrieve their associated GameObject via
					the Scene's ID lookup system.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "EngineCore/GameObjectLogic.hpp"
#include "EngineGraphics/SceneManager.hpp" // for Scene::GetGameObjectByID

GameObject* GameObjectLogic::GetOwner(Scene& scene) const {
	return scene.GetGameObjectByID(ownerID);
}

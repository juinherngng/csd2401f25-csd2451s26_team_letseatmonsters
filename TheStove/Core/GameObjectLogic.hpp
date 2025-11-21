/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			GameObjectLogic.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu

 DESCRIPTION:		Declares the base GameObjectLogic class used by all behaviour scripts. Stores
					the owner GameObject ID and provides helper accessors for retrieving the owning
					object through the Scene. All gameplay logic components inherit from this base.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <string>

class Scene;
class InputManager;
class GameObject;

class GameObjectLogic {
public:
	explicit GameObjectLogic(int ownerID) : ownerID(ownerID) {
	}
	virtual ~GameObjectLogic() = default;

	// suppress unused parameter warnings
	virtual void Awake(Scene& scene) {
		(void)scene;
	}
	virtual void Start(Scene& scene) {
		(void)scene;
	}
	virtual void Update(float dt, Scene& scene, InputManager& input) {
		(void)dt; (void)scene; (void)input;
	}
	virtual void OnDestroy(Scene& scene) {
		(void)scene;
	}

	int GetOwnerID() const {
		return ownerID;
	}

protected:
	GameObject* GetOwner(Scene& scene) const;

	int ownerID;
	// optional: name for debugging
	virtual std::string GetName() const {
		return "GameObjectLogic";
	}
};

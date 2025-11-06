/*
----------------------------------------------------------------------------------------------------
FILE NAME:			GameObjectLogic.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Vu Phan Hung, phanhung.vu@digipen.edu

DESCRIPTION:		Declares the base GameObjectLogic class which defines the common
                    interface for all gameplay logic components attached to GameObjects,
                    including lifecycle methods and owner access utilities.

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
    explicit GameObjectLogic(int ownerID) : ownerID(ownerID) {}
    virtual ~GameObjectLogic() = default;

    virtual void Awake(Scene& scene) {}
    virtual void Start(Scene& scene) {}
    virtual void Update(float dt, Scene& scene, InputManager& input) {}
    virtual void OnDestroy(Scene& scene) {}

    int GetOwnerID() const { return ownerID; }

protected:
    GameObject* GetOwner(Scene& scene) const;

    int ownerID;
    // optional: name for debugging
    virtual std::string GetName() const { return "GameObjectLogic"; }
};

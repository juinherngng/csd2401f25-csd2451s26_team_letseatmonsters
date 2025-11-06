// GameObjectLogic.hpp
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

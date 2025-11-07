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

	// suppress unused parameter warnings
    virtual void Awake(Scene& scene) { (void)scene; }
    virtual void Start(Scene& scene) { (void)scene; }
    virtual void Update(float dt, Scene& scene, InputManager& input) { (void)dt; (void)scene; (void)input; }
    virtual void OnDestroy(Scene& scene) { (void)scene; }

    int GetOwnerID() const { return ownerID; }

protected:
    GameObject* GetOwner(Scene& scene) const;

    int ownerID;
    // optional: name for debugging
    virtual std::string GetName() const { return "GameObjectLogic"; }
};

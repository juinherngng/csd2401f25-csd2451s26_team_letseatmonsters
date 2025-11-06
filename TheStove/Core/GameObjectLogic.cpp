// GameObjectLogic.cpp
#include "GameObjectLogic.hpp"
#include "../Graphics/SceneManager.hpp" // for Scene::GetGameObjectByID

GameObject* GameObjectLogic::GetOwner(Scene& scene) const {
    return scene.GetGameObjectByID(ownerID);
}

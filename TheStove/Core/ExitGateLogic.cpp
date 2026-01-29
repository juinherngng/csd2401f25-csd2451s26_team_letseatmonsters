#include "../Core/ExitGateLogic.hpp"
#include "../Graphics/SceneManager.hpp"
#include "../Graphics/GameObject.hpp"

Math::Vector2D ExitGateLogic::GetExitTargetWorld(Scene& scene) const
{
    GameObject* owner = GetOwner(scene);
    if (!owner) return Math::Vector2D(0.0f, 0.0f);

    glm::vec3 p = owner->GetPositionGLM();
    return Math::Vector2D(p.x + exitOffset_.x, p.y + exitOffset_.y);
}

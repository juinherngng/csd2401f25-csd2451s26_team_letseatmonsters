/*
----------------------------------------------------------------------------------------------------
FILE NAME:			PlayerLogic.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Vu Phan Hung, phanhung.vu@digipen.edu

DESCRIPTION:		Declares the PlayerLogic class that controls player input handling,
                    movement, sprite updates, item pickup/drop mechanics, and scene
                    boundary clamping behavior.

        All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/
#pragma once
#include "GameObjectLogic.hpp"
#include <glm/glm.hpp>

class PlayerLogic : public GameObjectLogic {
public:
    using GameObjectLogic::GameObjectLogic; // inherit constructor

    void Start(Scene& scene) override;
    void Update(float dt, Scene& scene, InputManager& input) override;
    std::string GetName() const override { return "PlayerLogic"; }

    // Unity: Move(Vector3 dest)
    void MoveTo(Scene& scene, const glm::vec2& dest);

    // Unity: bool ReachedDestination()
    bool HasDestination() const { return hasMoveTarget; }
    bool HasArrived()   const { return !hasMoveTarget; }

    // Unity: PickUp(GameObject item)
    void PickUp(Scene& scene, int itemID);

    // Unity: Drop(Vector3 dropPos) – for now just “drop near player”
    void Drop(Scene& scene);

private:
    // Movement state
    glm::vec2 moveTarget{ 0.f, 0.f };
    bool  hasMoveTarget{ false };
    float moveSpeed{ 200.f };  // pixels/sec – tune later
    float rotation_ = 0.0f;   // for arrow-key rotation

    void HandleScaleInput(GameObject* player, InputManager& input, float dt);
    void HandleRotationInput(GameObject* player, InputManager& input, float dt);

    // Facing / sprite state
    enum class FacingDir { Front, Back, Left, Right };
    FacingDir facingDir{ FacingDir::Front };

    // Simple “holding” state by object ID
    int carriedItemID{ -1 };

    // Internal helpers
    void HandleClickInput(Scene& scene, InputManager& input); // Unity: input + raycast
    void UpdateMovement(float dt, Scene& scene);              // Unity: NavMeshAgent movement
    void OnArrived(Scene& scene);                             // Unity: OnArrived() hook
    void UpdateSprite(Scene& scene, GameObject* player, const glm::vec2& moveDir);
};

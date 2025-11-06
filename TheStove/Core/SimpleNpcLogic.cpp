// SimpleNpcLogic.cpp
#include "SimpleNpcLogic.hpp"
#include "../Graphics/SceneManager.hpp"
#include "../Core/Physics.hpp"      // optional, if you want clamp helpers
#include "../Core/Collision.hpp"  // for WalkArea definition

void SimpleNpcLogic::Awake(Scene& scene) {
    (void)scene;
    timer = 0.0f;
    state = State::Idle;
    nextMoveUp = false; // first move: down
}

void SimpleNpcLogic::Update(float dt, Scene& scene, InputManager&) {
    // IMPORTANT: do not run logic in editor mode
    //if (!scene.IsSimulationActive()) {
    //    return;
    //}

    GameObject* npc = GetOwner(scene);
    if (!npc) return;

    timer += dt;

    glm::vec3 pos = npc->GetPositionGLM();

    // Step 1: compute desired movement based on state
    switch (state) {
    case State::Idle:
        if (timer >= idleDuration) {
            state = nextMoveUp ? State::MoveUp : State::MoveDown;
            timer = 0.0f;
        }
        break;

    case State::MoveUp:
        pos.y -= speed * dt; // in this engine, smaller y is visually "up"
        break;

    case State::MoveDown:
        pos.y += speed * dt; // larger y is "down"
        break;
    }

    // Step 2: apply our desired position
    npc->SetPosition(pos);

    // Step 3: clamp to existing walk area / gates
    // This uses your existing world collision logic.
    glm::vec3 beforeClamp = pos;
    scene.ClampToWalkArea(npc);
    glm::vec3 afterClamp = npc->GetPositionGLM();

    // Step 4: detect if we hit a vertical boundary and bounce
    const float eps = 0.1f;
    bool yChangedByClamp = std::fabs(afterClamp.y - beforeClamp.y) > eps;

    if (yChangedByClamp) {
        // We collided with top/bottom. Switch to idle and flip direction.
        if (state == State::MoveUp) {
            nextMoveUp = false; // next time, go down
        }
        else if (state == State::MoveDown) {
            nextMoveUp = true;  // next time, go up
        }

        state = State::Idle;
        timer = 0.0f;
    }
}

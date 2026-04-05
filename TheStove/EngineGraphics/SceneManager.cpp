/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (25%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(40%)
					Vu Phan Hung, phanhung.vu@digipen.edu   (15%)
					Ng Juin Herng, juinherng.ng@digipen.edu (20%)

 DESCRIPTION:		Implements the core Scene construction and high-level state access
					helpers.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "EngineCore/CollisionManager.hpp"
#include "EngineCore/InputManager.hpp"
#include "EngineCore/MovementManager.hpp"
#include "EngineCore/PhysicsManager.hpp"
#include "EngineGraphics/AnimationManager.hpp"
#include "EngineGraphics/SceneManager.hpp"

namespace {
	/**
	 * @brief Returns flow state name.
	 * @param state Parameter for state.
	 * @return Requested value.
	 */
	const char* FlowStateToString(Scene::FlowState state) {
		switch (state) {
		case Scene::FlowState::Bootstrapping: return "Bootstrapping";
		case Scene::FlowState::LoadingLevel: return "LoadingLevel";
		case Scene::FlowState::Transitioning: return "Transitioning";
		case Scene::FlowState::Cutscene: return "Cutscene";
		case Scene::FlowState::Gameplay: return "Gameplay";
		case Scene::FlowState::Paused: return "Paused";
		case Scene::FlowState::NonSimulation: return "NonSimulation";
		default: return "Unknown";
		}
	}
}

/**
 * @brief Sets simulation active.
 * @param active Parameter for active.
 * @return Result produced by this operation.
 */
void Scene::SetSimulationActive(bool active) {
	// Persist the requested simulation mode so later flow-state refreshes can reflect it.
	simulationActive = active;

	if (active) {
		// Resume animation playback whenever active gameplay simulation resumes.
		animationManager.Play();
	}
	else {
		// Main menu scenes still animate while the rest of the scene is treated as non-simulating.
		const std::string levelPath = GetCurrentLevelPath();
		const bool isMainMenu = levelPath.find("main_menu") != std::string::npos;

		if (isMainMenu) {
			animationManager.Play();
		}
		else {
			animationManager.Stop();
		}
	}

	// Keep external scene-flow observers synchronized with direct simulation toggles.
	RefreshFlowState();
}

/**
 * @brief Returns whether simulation active.
 * @return True when the operation succeeds or the condition is met.
 */
bool Scene::IsSimulationActive() const {
	return simulationActive;
}

/**
 * @brief Returns object texture path.
 * @param id Parameter for id.
 * @return Requested value.
 */
const std::string& Scene::GetObjectTexturePath(int id) const {
	// Prefer metadata-authored texture overrides before falling back to the entity's runtime texture.
	const std::string& metadataPath = objectMetadata_.GetTexturePath(id);
	if (!metadataPath.empty()) {
		return metadataPath;
	}

	return entityManager.GetTexturePath(id);
}

/**
 * @brief Sets object texture path.
 * @param id Parameter for id.
 * @param path Path to process.
 * @return Result produced by this operation.
 */
void Scene::SetObjectTexturePath(int id, const std::string& path) {
	// Keep both metadata and the live entity in sync so editor/runtime views agree on the texture path.
	objectMetadata_.SetTexturePath(id, path);
	entityManager.SetTexturePath(id, path);
}

/**
 * @brief Constructs a Scene and wires its core managers together.
 * @param engine Graphics engine used for rendering and transitions.
 * @param inputMgr Input manager used for per-frame input polling.
 * @param animMgr Animation manager used to drive animated objects.
 * @param moveMgr Movement manager used for transform integration.
 * @param physicsMgr Physics manager used for simulation updates.
 * @param collisionMgr Collision manager used for collision queries and rebuilds.
 */
Scene::Scene(GraphicsEngine& engine, InputManager& inputMgr, AnimationManager& animMgr,
	MovementManager& moveMgr, PhysicsManager& physicsMgr, CollisionManager& collisionMgr)
	: graphicsEngine(engine), inputManager(inputMgr), animationManager(animMgr),
	movementManager(moveMgr), physicsManager(physicsMgr), collisionManager(collisionMgr) {
	// Wire the cross-system references once so the scene can act as the integration hub.
	animationManager.SetEntityManager(&entityManager);
	physicsManager.SetScene(this);
	collisionManager.SetScene(this);

	// Seed the default gameplay layer and initial flow state for an empty boot scene.
	AddLayer("1");
	SetFlowState(FlowState::Bootstrapping);
}

/**
 * @brief Returns flow state name.
 * @return Requested value.
 */
const char* Scene::GetFlowStateName() const {
	return FlowStateToString(flowState_);
}

/**
 * @brief Resets resize baseline.
 * @return Result produced by this operation.
 */
void Scene::ResetResizeBaseline() {
	// Request the next resize-sensitive pass to recompute its baseline measurements.
	resetBaseline_ = true;
}

/**
 * @brief Draws ui.
 * @return Result produced by this operation.
 */
void Scene::DrawUI() {
	// Let the application-owned editor bridge render scene tooling when available.
	if (editorUiHook_ && (!editorEnabledQuery_ || editorEnabledQuery_())) {
		editorUiHook_(*this);
	}
}

/**
 * @brief Returns the graphics engine bound to this Scene.
 * @return Mutable graphics engine reference used by the Scene.
 */
GraphicsEngine& Scene::GetGraphicsEngine() {
	// Expose the shared renderer instance used for viewport, transition, and background control.
	return graphicsEngine;
}

/**
 * @brief Returns the graphics engine bound to this Scene.
 * @return Immutable graphics engine reference used by the Scene.
 */
const GraphicsEngine& Scene::GetGraphicsEngine() const {
	// Provide read-only rendering state access for const scene operations.
	return graphicsEngine;
}

/**
 * @brief Sets player id.
 * @param id Parameter for id.
 * @return Result produced by this operation.
 */
void Scene::SetPlayerID(int id) {
	// Cache the player entity id for downstream input, collision, and camera-facing helpers.
	spriteID = id;
}

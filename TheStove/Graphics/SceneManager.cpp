/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Implements the SceneManager class, which is responsible for the high-level
					management, coordination, and per-frame updating of all entities, systems,
					and game logic within a scene.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <algorithm>
#include <array>
#include <cctype>
#include <exception>
#include <fstream>
#include <glm/ext/matrix_clip_space.hpp>
#include <iostream>
#include <random>

#include "SceneManager.hpp"

 // Level constants
static constexpr float kRefW = 1200.0f; // 24 tiles
static constexpr float kRefH = 900.0f;  // 18 tiles
static constexpr float kTile = 50.0f;

// Tile-space definitions
// Walkable inner rectangle (match to background art) in tiles
static constexpr float kWalkL_T = 180.0f / kTile;  // 3.6
static constexpr float kWalkR_T = 1150.0f / kTile; // 23.0
static constexpr float kWalkT_T = 110.0f / kTile;  // 2.2
static constexpr float kWalkB_T = 825.0f / kTile;  // 16.5

// Middle divider (vertical split) in tiles
static constexpr float kWoodX0_T = 500.0f / kTile; // 10.0
static constexpr float kWoodX1_T = 590.0f / kTile; // 11.8
static constexpr float kWoodTopMinY_T = 100.0f / kTile; // 2.0
static constexpr float kWoodTopMaxY_T = 320.0f / kTile; // 6.4
static constexpr float kWoodGapMinY_T = 320.0f / kTile; // 6.4
static constexpr float kWoodGapMaxY_T = 570.0f / kTile; // 11.4
static constexpr float kWoodBotMinY_T = 570.0f / kTile; // 11.4
static constexpr float kWoodBotMaxY_T = 700.0f / kTile; // 14.0

// End-of-stage vertical gate in tiles
static constexpr float kEndVX0_T = 1080.0f / kTile; // 21.6
static constexpr float kEndVX1_T = 1200.0f / kTile; // 24.0
static constexpr float kEndVTopMinY_T = 100.0f / kTile; // 2.0
static constexpr float kEndVTopMaxY_T = 320.0f / kTile; // 6.4
static constexpr float kEndVGapMinY_T = 320.0f / kTile; // 6.4
static constexpr float kEndVGapMaxY_T = 570.0f / kTile; // 11.4
static constexpr float kEndVBotMinY_T = 570.0f / kTile; // 11.4
static constexpr float kEndVBotMaxY_T = 700.0f / kTile; // 14.0

// Pixel-space versions derived from tiles (used by physics/collision)
static constexpr float kWalkL = kWalkL_T * kTile;
static constexpr float kWalkR = kWalkR_T * kTile;
static constexpr float kWalkT = kWalkT_T * kTile;
static constexpr float kWalkB = kWalkB_T * kTile;

// Thickness of our blocking bars (thin = precise, easy to tune)
static constexpr float kEdgeThick = 3.0f;

// Middle divider (pixels)
static constexpr float kWoodX0 = kWoodX0_T * kTile;
static constexpr float kWoodX1 = kWoodX1_T * kTile;
static constexpr float kWoodTopMinY = kWoodTopMinY_T * kTile;
static constexpr float kWoodTopMaxY = kWoodTopMaxY_T * kTile;
static constexpr float kWoodGapMinY = kWoodGapMinY_T * kTile;
static constexpr float kWoodGapMaxY = kWoodGapMaxY_T * kTile;
static constexpr float kWoodBotMinY = kWoodBotMinY_T * kTile;
static constexpr float kWoodBotMaxY = kWoodBotMaxY_T * kTile;

// End-of-stage gate (pixels)
static constexpr float kEndVX0 = kEndVX0_T * kTile;
static constexpr float kEndVX1 = kEndVX1_T * kTile;
static constexpr float kEndVTopMinY = kEndVTopMinY_T * kTile;
static constexpr float kEndVTopMaxY = kEndVTopMaxY_T * kTile;
static constexpr float kEndVGapMinY = kEndVGapMinY_T * kTile;
static constexpr float kEndVGapMaxY = kEndVGapMaxY_T * kTile;
static constexpr float kEndVBotMinY = kEndVBotMinY_T * kTile;
static constexpr float kEndVBotMaxY = kEndVBotMaxY_T * kTile;

namespace {
	inline Math::Vector2D toM(const glm::vec2& v) {
		return Math::Vector2D(v.x, v.y);
	}
	inline Math::Vector3D toM(const glm::vec3& v) {
		return Math::Vector3D(v.x, v.y, v.z);
	}
	inline glm::vec2 toG(const Math::Vector2D& v) {
		return glm::vec2(v.x, v.y);
	}
	inline glm::vec3 toG(const Math::Vector3D& v) {
		return glm::vec3(v.x, v.y, v.z);
	}

	// Utility function to generate UV frames for a sprite sheet
	std::vector<glm::vec4> GenerateFrames(int startFrame, int frameCount, int totalCols, float frameWidth, float frameHeight) {
		(void)totalCols; // Suppress unused parameter warning

		std::vector<glm::vec4> frames;
		for (int i = 0; i < frameCount; ++i) {
			int col = startFrame + i;
			float offsetX = col * frameWidth;
			float offsetY = 1.0f - frameHeight; // single row, so just - frameHeight for Y offset
			frames.emplace_back(offsetX, offsetY, frameWidth, frameHeight);
		}
		return frames;
	}

	// Build an AABB in world/reference space from tile coordinates.
	static collision::AABB MakeTileRect(float tx0, float ty0, float tx1, float ty1) {
		collision::AABB r{};
		r.min = { tx0 * kTile, ty0 * kTile };
		r.max = { tx1 * kTile, ty1 * kTile };
		return r;
	}

	struct StaticRectDef {
		float tx0, ty0, tx1, ty1; // tile-space coordinates
	};

	// Benches + ingredient counter + bottom strip, all in tiles
	static constexpr std::array<StaticRectDef, 6> kStaticRectDefs{ {
			// top-middle bench
			{ 13.9f,  3.4f, 15.2f,  5.0f },
			// top-right bench
			{ 19.0f,  3.4f, 20.3f,  5.0f },
			// bottom-middle bench
			{ 13.9f, 13.3f, 15.2f, 15.0f },
			// bottom-right bench
			{ 19.0f, 13.3f, 20.3f, 15.0f },
			// Ingredient counter row (top kitchen)
			{ 4.0f,   2.0f, 11.0f,  3.0f },
			// Bottom solid area: grills + green + both posts
			{ 4.0f,  14.8f, 11.0f, 16.0f }
		} };
}

void Scene::SetSimulationActive(bool active) {
	simulationActive = active;

	if (active) {
		animationManager.Play();
	}
	else {
		animationManager.Stop();
	}
}

bool Scene::IsSimulationActive() const {
	return simulationActive;
}

const std::string& Scene::GetObjectTexturePath(int id) const {
	return entityManager.GetTexturePath(id);
}

void Scene::SetObjectTexturePath(int id, const std::string& path) {
	entityManager.SetTexturePath(id, path);
}

// Converts reference (kRefW/kRefH) X coordinate to current framebuffer X
float Scene::ScaleXToCurrent(float referenceX) const {
	const float worldWidth = static_cast<float>(graphicsEngine.GetWidth());
	return referenceX * worldWidth / kRefW;
}

// Converts reference (kRefW/kRefH) Y coordinate to current framebuffer Y
float Scene::ScaleYToCurrent(float referenceY) const {
	const float worldHeight = static_cast<float>(graphicsEngine.GetHeight());
	return referenceY * worldHeight / kRefH;
}

float Scene::ToRefX(float currentX) const {
	const float worldW = static_cast<float>(graphicsEngine.GetWidth());
	return currentX * (kRefW / worldW);
}

float Scene::ToRefY(float currentY) const {
	const float worldH = static_cast<float>(graphicsEngine.GetHeight());
	return currentY * (kRefH / worldH);
}

// Core Lifecycle
Scene::Scene(GraphicsEngine& engine, InputManager& inputMgr, AnimationManager& animMgr,
	MovementManager& moveMgr, PhysicsManager& physicsMgr, CollisionManager& collisionMgr)
	: graphicsEngine(engine), inputManager(inputMgr), animationManager(animMgr),
	movementManager(moveMgr), physicsManager(physicsMgr), collisionManager(collisionMgr) {
	// Set the EntityManager reference in AnimationManager
	animationManager.SetEntityManager(&entityManager);

	// Basic default layer used when no explicit layer name is given
	AddLayer("1");
}

void Scene::LoadScene(const std::string& sceneName) {
	(void)sceneName;

	// Remember which level JSON we're using
	currentLevelPath_ = "../levels/kitchen01.json";

	// Optional: just pre-fill the path field for convenience
	mLevelEditor.SetPath("../levels/kitchen01.json");

	// Ensure we start EMPTY per rubric (no auto-spawned objects)
	ClearAll();

	// You can keep a background even with an empty level (or move this into JSON later)
	SetSceneBackground("../assets/Background_Full.png");
}

void Scene::Update(float deltaTime, GLFWwindow* window) {
	// Input is now updated by CoreEngine's system, no need to call Update here
	inputManager.Update(deltaTime); // REMOVED - handled by CoreEngine

	UpdateAnimationControls();
	// Deferred Clear
	if (pendingClear_) {
		ClearAll();
		RebuildColliders();
		pendingClear_ = false;
		return;  // Skip rest of update this frame
	}

	// Process input commands (debug toggles, force toggle, etc.)
	inputCommandHandler.ProcessCommands(inputManager, physicsManager, movementManager, spriteID, useForces_, showAuxDebug_);

	// Level editor toggle
	if (inputManager.IsKeyJustPressed(GLFW_KEY_L)) {
		mLevelEditor.Toggle();
	}

	// Resolve physics timestep
	const float physicsDt = physicsStep_.resolveDt(inputManager, deltaTime);

	if (simulationActive) {
		// Run all scripts
		logicManager.StartAll(*this);
		logicManager.UpdateAll(deltaTime, *this, inputManager);

		if (useForces_) {
			physicsManager.UpdatePhysics(physicsDt, entityManager, inputManager);
		}

		// Update NPC AI
		const collision::WalkArea walk{ kWalkL, kWalkR, kWalkT, kWalkB, kEdgeThick };
		npcSystem.Update(physicsDt, entityManager, collisionManager, walk);

		// Handle player-NPC collisions
		HandlePlayerCollisions(physicsDt, entityManager);

		// Apply final constraints (gates, boundaries)
		ApplyFinalConstraints(entityManager);
	}

	// Debug visualization
	debugVisualizer.DrawDebugInfo(entityManager, collisionManager, movementManager, spriteID, showAuxDebug_);

	(void)window;
}

void Scene::ResetResizeBaseline() {
	resetBaseline_ = true;
}

void Scene::DrawUI() {
	if (mLevelEditor.IsEnabled()) {
		mLevelEditor.DrawUI(*this);
	}
}

void Scene::ClearAll() {
	logicManager.Clear(*this);  // <-- clear scripts first
	entityManager.Clear();
	animationManager.Clear();
	movementManager.Clear();
	npcSystem.Clear();

	spriteID = -1;
	dinoID = -1;
	otherID = -1;
	otherID2 = -1;
}

void Scene::SetPlayerID(int id) {
	spriteID = id;
}

GameObject* Scene::SpawnStaticSprite(const std::string& texturePath,
	const glm::vec3 position,
	const glm::vec2 size,
	const std::string& layer) {
	GameObject* obj = entityManager.SpawnStaticSprite(texturePath, position, size);

	if (obj) {
		int id = obj->GetID();
		AssignObjectToLayer(id, layer);
	}

	return obj;
}

GameObject* Scene::SpawnAnimatedSprite(
	const std::string& texturePath,
	const glm::vec3 position,
	const glm::vec2 size,
	const std::vector<glm::vec4> frames,
	float frameDuration, bool loop,
	const std::string& layer) {
	GameObject* obj = entityManager.SpawnAnimatedSprite(texturePath, position, size, frames, frameDuration, loop);

	if (obj) {
		int id = obj->GetID();
		AssignObjectToLayer(id, layer);
	}

	return obj;
}

GameObject* Scene::GetGameObjectByID(int targetID) {
	return entityManager.GetByID(targetID);
}


void Scene::DespawnByID(int targetID) {
	// Remove from entity manager (handles transforms too)
	entityManager.DespawnByID(targetID);
}

void Scene::CollectRenderablePointers(std::vector<GameObject*>& out) {
	out.clear();

	std::vector<GameObject*> all = entityManager.GetAllObjects();
	out.reserve(all.size());

	for (GameObject* g : all) {
		if (!g) {
			continue;
		}

		// Check the layer's visibility flag
		const std::string layerName = GetObjectLayer(g->GetID());
		Layer* layer = GetLayer(layerName);
		if (layer && !layer->IsVisible()) {
			continue;
		}

		out.push_back(g);
	}

	auto parseLayerNumber = [](const std::string& s) -> int {
		if (s.empty()) {
			return 1; // base layer
		}

		int result = 0;
		for (char c : s) {
			if (!std::isdigit(static_cast<unsigned char>(c))) {
				// Any non-numeric layer name behaves like a very "high" layer
				// so that it draws on top of numeric layers.
				return 1000000;
			}

			result = result * 10 + (c - '0');
		}

		return result;
		};

	std::sort(
		out.begin(),
		out.end(),
		[&](GameObject* a, GameObject* b) {
			const std::string laName = GetObjectLayer(a->GetID());
			const std::string lbName = GetObjectLayer(b->GetID());

			int la = parseLayerNumber(laName);
			int lb = parseLayerNumber(lbName);

			// Different layers: smaller layer number drawn first
			if (la != lb) {
				return la > lb;
			}

			// Same layer - higher Y drawn first (lower on screen appears in front)
			return a->GetPosition().y > b->GetPosition().y;
		}
	);
}

std::vector<GameObject*> Scene::GetAllObjectsRaw() {
	return entityManager.GetAllObjects();
}

// Scene / Transform Utilities
void Scene::SetSceneBackground(const std::string& texturePath) {
	graphicsEngine.SetBackground(texturePath);
}

void Scene::SetTransformFromLevel(int id, const glm::vec3& pos, const glm::vec3& scale, float rotation) {
	entityManager.SetPosition(id, pos);
	entityManager.SetScale(id, scale);
	entityManager.SetRotation(id, rotation);

	GameObject* obj = GetGameObjectByID(id);
	if (obj) {
		obj->SetPosition(pos);
		obj->SetScale(scale);
		obj->SetRotation(rotation, glm::vec3(0, 0, 1));
	}
}

void Scene::ClampToWalkArea(GameObject* obj) {
	if (obj == nullptr) {
		return;
	}

	const collision::WalkArea walk{ kWalkL, kWalkR, kWalkT, kWalkB, kEdgeThick };
	Math::Vector3D p = Math::Vector3D(obj->GetPosition().x, obj->GetPosition().y, obj->GetPosition().z);
	physics::ClampInsideWalk(walk, obj, p);

	const glm::vec3 pg = toG(p);
	obj->SetPosition(pg);

}

glm::vec2 Scene::ResolveWorldStep(GameObject* obj, const glm::vec2& desiredDelta) {
	if (!obj) {
		return desiredDelta;
	}

	// Use the same collision world that MovementManager / PhysicsManager use
	collision::World& world = collisionManager.GetCollisionWorld();

	// Build start AABB from current position + collider
	const glm::vec3 posG = obj->GetPositionGLM();
	Math::Vector3D posM(posG.x, posG.y, posG.z);

	const collision::AABB start = physics::MakeColliderBox(obj, posM);

	// Ask world how much of desiredDelta we’re allowed to move
	Math::Vector2D desired(desiredDelta.x, desiredDelta.y);
	Math::Vector2D allowed = world.resolve(start, desired);

	return glm::vec2(allowed.x, allowed.y);
}

// Animation
bool Scene::HasAnimations(int id) const {
	return animationManager.HasAnimator(id);
}

std::vector<std::string> Scene::GetAnimationList(int id) const {
	return animationManager.GetAnimationNames(id);
}

std::string Scene::GetCurrentAnimationName(int id) const {
	return animationManager.GetCurrentAnimation(id);
}

void Scene::SetAnimation(int objID, const std::string& animName) {
	animationManager.SetAnimation(objID, animName);
}

void Scene::AttachDinoAnimations(int objID) {
	animationManager.AttachDinoAnimations(objID);
}

void Scene::MarkAnimated(int id, bool state) {
	if (!state) {
		// Turning OFF animation: remove any per-object animation state
		if (GameObject* obj = GetGameObjectByID(id)) {
			// Ensure it renders the full texture as a static sprite
			obj->SetUVRect({ 0.f, 0.f, 1.f, 1.f });
		}
	}
	else {
		// Turning ON: no-op here; AttachDinoAnimations() will populate maps.
		// (HasAnimations() will start returning true once frames are attached.)
	}
}

void Scene::HandlePlayerCollisions(float physicsDt, EntityManager& entityMgr) {
	if (spriteID < 0) {
		return;
	}

	GameObject* sprite = entityMgr.GetByID(spriteID);
	if (!sprite) {
		return;
	}

	// If the player's layer is non-collidable, skip all player–object collisions.
	{
		std::string playerLayer = GetObjectLayer(spriteID);
		Layer* pl = GetLayer(playerLayer);
		if (pl && !pl->IsCollidable()) {
			return;
		}
	}

	const glm::vec3 position = sprite->GetPositionGLM();

	// Build player's current AABB for spatial query
	const collision::AABB queryBox = physics::MakeColliderBox(
		sprite,
		Math::Vector3D(position.x, position.y, position.z)
	);

	// Query spatial grid for nearby candidates
	std::vector<GameObject*> candidates;
	collisionManager.GetSpatialGrid().Query(queryBox, candidates);

	// Desired movement from movement system
	Math::Vector2D desiredMoveM{ 0.0f, 0.0f };

	for (GameObject* other : candidates) {
		if (other == nullptr || other == sprite) {
			continue;
		}

		// Skip objects whose layer has collisions turned off
		{
			std::string otherLayer = GetObjectLayer(other->GetID());
			Layer* ol = GetLayer(otherLayer);
			if (ol && !ol->IsCollidable()) {
				continue;
			}
		}

		const Math::Vector2D gSize = other->GetColliderSize();
		if (gSize.x <= 0.0f || gSize.y <= 0.0f) {
			continue;
		}

		const int currentOtherID = other->GetID();
		if (npcSystem.IsLaneNPC(currentOtherID)) {
			continue; // lane NPCs ignore player collision
		}

		// Current positions (M-space)
		Math::Vector3D playerPosM(position.x, position.y, position.z);
		Math::Vector3D otherPosM(other->GetPosition().x, other->GetPosition().y, other->GetPosition().z);

		// Build AABBs at those positions
		const collision::AABB playerBox = physics::MakeColliderBox(sprite, playerPosM);
		const collision::AABB otherBox = physics::MakeColliderBox(other, otherPosM);

		// Minimum translation vector to separate player from goat
		Math::Vector2D mtv;
		if (!collision::overlapMTV(playerBox, otherBox, mtv)) {
			continue;
		}

		// World-aware push: try to move the goat, clamped by walls
		constexpr float kGoatShare = 0.50f; // you can tune 0.25f..0.50f
		const Math::Vector2D desiredOtherDelta(-mtv.x * kGoatShare, -mtv.y * kGoatShare);

		// Clamp goat movement against static walls
		Math::Vector2D allowedOtherDelta = collisionManager.GetCollisionWorld().resolve(otherBox, desiredOtherDelta);

		// Move goat by the allowed portion (could be zero if pinned)
		otherPosM.x += allowedOtherDelta.x;
		otherPosM.y += allowedOtherDelta.y;

		// Player receives the remainder so relative separation equals MTV
		Math::Vector2D playerDelta(mtv.x, mtv.y);
		playerDelta.x += allowedOtherDelta.x; // note: allowedOtherDelta is opposite-signed to MTV
		playerDelta.y += allowedOtherDelta.y;

		// Apply small bias to avoid re-penetration next frame
		constexpr float kEps = 0.5f;
		if (playerDelta.x > 0.0f) {
			playerPosM.x += kEps;
		}
		if (playerDelta.x < 0.0f) {
			playerPosM.x -= kEps;
		}
		if (playerDelta.y > 0.0f) {
			playerPosM.y += kEps;
		}
		if (playerDelta.y < 0.0f) {
			playerPosM.y -= kEps;
		}

		// Apply the separation
		playerPosM.x += playerDelta.x;
		playerPosM.y += playerDelta.y;

		// If the goat barely moved (pinned) and we were trying to move into it,
		// remove our inward component, keep tangent (glide along goat/wall).
		{
			// Intent this frame from movement system
			glm::vec2 v = movementManager.GetVelocity(spriteID);
			const float vLen = std::sqrt(v.x * v.x + v.y * v.y);
			const float mtvLen = std::sqrt(mtv.x * mtv.x + mtv.y * mtv.y);

			// How much goat actually moved vs we wanted it to move
			const float desiredLen = std::sqrt(desiredOtherDelta.x * desiredOtherDelta.x + desiredOtherDelta.y * desiredOtherDelta.y);
			const float allowedLen = std::sqrt(allowedOtherDelta.x * allowedOtherDelta.x + allowedOtherDelta.y * allowedOtherDelta.y);
			const bool goatPinned = (desiredLen > 0.0f) && (allowedLen < 0.1f * desiredLen);

			if (goatPinned && vLen > 0.0001f && mtvLen > 0.0001f && physicsDt > 0.0f) {
				// Normal pointing from goat to player (same dir as MTV applied to player)
				const glm::vec2 n = glm::vec2(mtv.x / mtvLen, mtv.y / mtvLen);

				// Inward component of our desired displacement this frame
				const glm::vec2 desiredDelta = v * physicsDt;
				const float into = desiredDelta.x * n.x + desiredDelta.y * n.y;

				if (into > 0.0f) {
					// Remove inward component; keep tangential part (slide)
					const glm::vec2 tangential = desiredDelta - into * n;

					// Clamp slide against world (so we don't scrape into walls)
					const Math::Vector2D csz = sprite->GetColliderSize();
					const collision::AABB pAfterSep = collision::World::makeAABBFromCenter(playerPosM, Math::Vector3D(csz.x, csz.y, 1.0f));

					const Math::Vector2D slideDesired(tangential.x, tangential.y);
					const Math::Vector2D slideAllowed = collisionManager.GetCollisionWorld().resolve(pAfterSep, slideDesired);

					// Apply the allowed slide
					playerPosM.x += slideAllowed.x;
					playerPosM.y += slideAllowed.y;
				}
				else {
				}
			}
			else if (goatPinned && vLen > 0.0001f && mtvLen > 0.0001f) {
				const float dotInto = (mtv.x / mtvLen) * (v.x / (vLen + 1e-6f)) + (mtv.y / mtvLen) * (v.y / (vLen + 1e-6f));
				if (dotInto > 0.1f) {
					movementManager.ClearMoveTarget(spriteID);
				}
			}
		}

		// Write back
		sprite->SetPosition(toG(playerPosM));
		other->SetPosition(toG(otherPosM));
	}
}

void Scene::BuildLevelColliders() {
	collision::WalkArea walk{ kWalkL, kWalkR, kWalkT, kWalkB, kEdgeThick };

	collision::WoodVertical wood{
	  kWoodX0, kWoodX1,
	  kWoodTopMinY, kWoodTopMaxY,
	  kWoodGapMinY, kWoodGapMaxY,
	  kWoodBotMinY, kWoodBotMaxY
	};

	collision::StageEndGateVertical gate{
	  kEndVX0, kEndVX1,
	  kEndVTopMinY, kEndVTopMaxY,
	  kEndVGapMinY, kEndVGapMaxY,
	  kEndVBotMinY, kEndVBotMaxY
	};

	// Build all static walls (outer frame + wood + gate)
	collisionManager.BuildWalls(walk, wood, gate);

	// Benches + counters + bottom strip
	std::vector<collision::AABB> staticRects;
	staticRects.reserve(kStaticRectDefs.size());
	for (const auto& def : kStaticRectDefs) {
		staticRects.push_back(MakeTileRect(def.tx0, def.ty0, def.tx1, def.ty1));
	}

	collisionManager.AddStaticRects(staticRects);

	// Get a pointer to the shared collision world
	collision::World* world = &collisionManager.GetCollisionWorld();

	// Give that world to movement + NPC + physics
	movementManager.SetCollisionWorld(world);
	movementManager.SetNPCSystem(&npcSystem);

	physicsManager.SetMovementManager(&movementManager);
	physicsManager.SetCollisionWorld(world);
}

void Scene::ApplyFinalConstraints(EntityManager& entityMgr) {
	if (spriteID < 0) {
		return;
	}

	GameObject* sprite = entityMgr.GetByID(spriteID);
	if (!sprite) {
		return;
	}

	glm::vec3 position = sprite->GetPositionGLM();

	// Gate clamp (stage end)
	const collision::StageEndGateVertical gate{
		kEndVX0, kEndVX1,
		kEndVTopMinY, kEndVTopMaxY,
		kEndVGapMinY, kEndVGapMaxY,
		kEndVBotMinY, kEndVBotMaxY
	};

	const collision::WalkArea walk{ kWalkL, kWalkR, kWalkT, kWalkB, kEdgeThick };

	Math::Vector3D posM = toM(position);
	physics::ClampInsideWalkWithGate(walk, gate, sprite, posM);
	position = toG(posM);

	// Final world boundary clamps
	const float worldW = static_cast<float>(graphicsEngine.GetWidth());
	const float worldH = static_cast<float>(graphicsEngine.GetHeight());

	position.x = glm::clamp(position.x, 0.0f, worldW);
	position.y = glm::clamp(position.y, 0.0f, worldH);

	// Apply final transforms
	sprite->SetPosition(position);
}

// Returns true if AABB "box" overlaps the axis-aligned rectangle [x0,x1]x[y0,y1].
static bool OverlapsRect(const collision::AABB& box, float x0, float x1, float y0, float y1) {
	return (box.min.x < x1 && box.max.x > x0 && box.min.y < y1 && box.max.y > y0);
}

// Snap a dynamic object horizontally out of the vertical wood segment it overlaps (minimal move).
static void SnapHorizontallyOutOfBand(const collision::AABB& box, float bandX0, float bandX1, Math::Vector3D& posM) {
	// Move by the smallest magnitude either to the left or right so the AABB clears the band.
	const float moveLeft = bandX0 - box.max.x - 0.5f; // small epsilon
	const float moveRight = bandX1 - box.min.x + 0.5f;
	if (std::abs(moveLeft) < std::abs(moveRight)) {
		posM.x += moveLeft;
	}
	else {
		posM.x += moveRight;
	}
}

// Snap a dynamic object vertically out of a horizontal band it overlaps (minimal move).
static void SnapVerticallyOutOfBand(const collision::AABB& box,
	float bandY0, float bandY1,
	Math::Vector3D& posM) {
	const float moveUp = bandY0 - box.max.y - 0.5f; // small epsilon
	const float moveDown = bandY1 - box.min.y + 0.5f;

	if (std::abs(moveUp) < std::abs(moveDown)) {
		posM.y += moveUp;
	}
	else {
		posM.y += moveDown;
	}
}

void Scene::ResolveInitialStaticOverlaps() {
	// Wood (middle divider) in pixels
	const float woodX0 = kWoodX0;
	const float woodX1 = kWoodX1;
	const float woodTopY0 = kWoodTopMinY, woodTopY1 = kWoodTopMaxY;
	const float woodBotY0 = kWoodBotMinY, woodBotY1 = kWoodBotMaxY;

	// End-of-stage gate in pixels (same shape used by BuildWalls)
	const float gateX0 = kEndVX0;
	const float gateX1 = kEndVX1;
	const float gateTopY0 = kEndVTopMinY, gateTopY1 = kEndVTopMaxY;
	const float gateBotY0 = kEndVBotMinY, gateBotY1 = kEndVBotMaxY;
	// (We intentionally ignore the gap region kEndVGap* – that is walkable.)

	// Benches + counters + bottom strip: pre-build their AABBs in pixel space.
	std::array<collision::AABB, kStaticRectDefs.size()> benchRects{};
	for (size_t i = 0; i < kStaticRectDefs.size(); ++i) {
		const auto& def = kStaticRectDefs[i];
		benchRects[i] = MakeTileRect(def.tx0, def.ty0, def.tx1, def.ty1);
	}

	// Walkable outer frame – clamp into this first.
	const collision::WalkArea walk{ kWalkL, kWalkR, kWalkT, kWalkB, kEdgeThick };

	std::vector<GameObject*> objs = entityManager.GetAllObjects();

	for (GameObject* g : objs) {
		if (!g) continue;

		// Work in M-space (your math structs)
		Math::Vector3D pM(g->GetPositionGLM().x,
			g->GetPositionGLM().y,
			g->GetPositionGLM().z);

		// Keep inside big walk rect (outer boundary)
		physics::ClampInsideWalk(walk, g, pM);

		// Build the object's collider AABB at this tentative position
		collision::AABB box = physics::MakeColliderBox(g, pM);

		// Wood divider: snap horizontally out of the top/bottom vertical planks
		if (OverlapsRect(box, woodX0, woodX1, woodTopY0, woodTopY1)) {
			SnapHorizontallyOutOfBand(box, woodX0, woodX1, pM);
			box = physics::MakeColliderBox(g, pM);
		}
		if (OverlapsRect(box, woodX0, woodX1, woodBotY0, woodBotY1)) {
			SnapHorizontallyOutOfBand(box, woodX0, woodX1, pM);
			box = physics::MakeColliderBox(g, pM);
		}

		// Gate: same idea as wood (vertical band, top/bottom solid segments)
		if (OverlapsRect(box, gateX0, gateX1, gateTopY0, gateTopY1)) {
			SnapHorizontallyOutOfBand(box, gateX0, gateX1, pM);
			box = physics::MakeColliderBox(g, pM);
		}
		if (OverlapsRect(box, gateX0, gateX1, gateBotY0, gateBotY1)) {
			SnapHorizontallyOutOfBand(box, gateX0, gateX1, pM);
			box = physics::MakeColliderBox(g, pM);
		}

		// Benches + ingredient counter + bottom strip: snap vertically out.
		for (size_t i = 0; i < benchRects.size(); ++i) {
			const auto& r = benchRects[i];

			if (!OverlapsRect(box, r.min.x, r.max.x, r.min.y, r.max.y)) {
				continue;
			}

			if (i <= 3) {
				SnapHorizontallyOutOfBand(box, r.min.x, r.max.x, pM);
			}
			else {
				SnapVerticallyOutOfBand(box, r.min.y, r.max.y, pM);
			}

			// Rebuild after the snap so subsequent checks use the new position
			box = physics::MakeColliderBox(g, pM);
		}

		// Commit corrected position
		g->SetPosition(glm::vec3(pM.x, pM.y, pM.z));
	}

	// Rebuild collision world once with the final positions.
	RebuildColliders();
}

void Scene::RebuildColliders() {
	collisionManager.Clear();
	BuildLevelColliders();
}

void Scene::GenerateStressTest(int objectCount) {
	std::cout << "[Scene] Generating stress test with " << objectCount << " objects...\n";

	// Random number generation setup
	std::random_device rd;
	std::mt19937 gen(rd());

	// Random position ranges 
	std::uniform_real_distribution<float> posX(50.0f, 1150.0f);
	std::uniform_real_distribution<float> posY(50.0f, 750.0f);

	// Random velocity ranges
	std::uniform_real_distribution<float> velX(-100.0f, 100.0f);
	std::uniform_real_distribution<float> velY(-100.0f, 100.0f);

	// Random size range
	std::uniform_real_distribution<float> sizeRand(24.0f, 64.0f);

	// Textures to render
	std::vector<std::string> texturePaths = {
		"../assets/goat_sprite_front.png",
		"../assets/mc_sprite_front.png"
	};

	std::uniform_int_distribution<size_t> texIndex(0, texturePaths.size() - 1);

	// Spawn objects with random properties
	for (int i = 0; i < objectCount; i++) {
		// Random position
		glm::vec3 randomPos(posX(gen), posY(gen), 0.0f);

		// Random size
		float size = sizeRand(gen);
		glm::vec2 randomSize(size, size);

		// Random texture
		std::string texPath = texturePaths[texIndex(gen)];

		// Spawn using EntityManager
		GameObject* obj = entityManager.SpawnStaticSprite(texPath, randomPos, randomSize);

		if (obj) {
			// Set random velocity 
			Math::Vector2D randomVel(velX(gen), velY(gen));
			obj->SetVelocity(randomVel);
		}
	}
	std::cout << "[Scene] Stress test loaded\n";
}

void Scene::RequestClearAll() {
	pendingClear_ = true;
}

void Scene::AttachLogicForTag(int id, const std::string& tag) {
	if (tag == "player") {
		logicManager.AddLogic<PlayerLogic>(id);
		spriteID = id; // keep existing usage
	}
	else if (tag == "npc1" || tag == "npc2") {
		logicManager.AddLogic<SimpleNpcLogic>(id);
	}
	else if (tag == "dino") {
		logicManager.AddLogic<SimpleNpcLogic>(id);
		dinoID = id; // preserve your special ID if you rely on it elsewhere
	}
	// you can extend with more tags later
}

GraphicsEngine& Scene::GetGraphicsEngine() {
	return graphicsEngine;
}

const GraphicsEngine& Scene::GetGraphicsEngine() const {
	return graphicsEngine;
}

void Scene::AddLayer(const std::string& name) {
	layers.try_emplace(name, name); // Only add if missing
}

Layer* Scene::GetLayer(const std::string& name) {
	auto it = layers.find(name);
	return it != layers.end() ? &(it->second) : nullptr;
}

const std::unordered_map<std::string, Layer>& Scene::GetAllLayers() const {
	return layers;
}

std::string Scene::GetObjectLayer(int objectID) const {
	auto it = defaults_.find(objectID);
	if (it != defaults_.end()) {
		return it->second.layer;
	}

	return "";
}

void Scene::AssignObjectToLayer(int id, const std::string& newLayer) {
	std::string layerName = newLayer;
	if (layerName.empty()) layerName = "1";

	// Remove object from all layers' ID lists
	for (auto& pair : layers) {
		pair.second.RemoveObject(id);
	}

	// Register object ID with chosen layer (creates if missing)
	Layer& layer = layers[layerName];
	if (layer.GetName().empty()) {
		layer.SetName(layerName);
	}

	layer.AddObject(id);

	// Store on metadata used by the editor + JSON
	defaults_[id].layer = layerName;
}

void Scene::RemoveLayer(const std::string& name) {
	auto it = layers.find(name);
	if (it != layers.end()) {
		layers.erase(it);
	}
}

void Scene::UpdateAnimationControls() {
	// Do nothing when simulation is paused
	if (!simulationActive) return;

	// If UI is capturing keyboard, do not change gameplay animation state
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard) return;

	// Edge detection for single key presses
	const bool k1 = inputManager.IsKeyJustPressed(GLFW_KEY_1);
	const bool k2 = inputManager.IsKeyJustPressed(GLFW_KEY_2);
	const bool k3 = inputManager.IsKeyJustPressed(GLFW_KEY_3);
	if (!k1 && !k2 && !k3) return;

	const std::string anim = k1 ? "IDLE" : (k2 ? "WALK" : "ATTACK");

	// Make these known dino IDs use the chosen animation (skip missing objects)
	const int dinoIDs[] = { 1, 2, 3 };
	for (int id : dinoIDs) {
		GameObject* obj = GetGameObjectByID(id);
		if (!obj) continue; // not present in scene

		// Ensure animator is attached
		if (!animationManager.HasAnimator(id)) {
			animationManager.AttachDinoAnimations(id);
		}

		animationManager.SetAnimation(id, anim);
		std::cout << "[Scene] Playing animation " << anim << " for dino id=" << id << "\n";
	}
}

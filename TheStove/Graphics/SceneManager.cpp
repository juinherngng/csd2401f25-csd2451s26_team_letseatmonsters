/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Implements the Scene class, handling object spawning,
					animation, collisions, and per-frame updates.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <iostream>
#include <algorithm>
#include <random>
#include <glm/ext/matrix_clip_space.hpp>

#include "SceneManager.hpp"

 // Level constants
static constexpr float kRefW = 1200.0f;
static constexpr float kRefH = 800.0f;

// Walkable inner rectangle (match to background art)
static constexpr float kWalkL = 150.0f;  // left
static constexpr float kWalkR = 1100.0f; // right
static constexpr float kWalkT = 80.0f;   // top
static constexpr float kWalkB = 733.0f;  // bottom

// Thickness of our blocking bars (thin = precise, easy to tune)
static constexpr float kEdgeThick = 3.0f;

// Wooden divider (vertical split)
static constexpr float kWoodX0 = 562.0f;
static constexpr float kWoodX1 = 590.0f;
static constexpr float kWoodTopMinY = 100.0f;
static constexpr float kWoodTopMaxY = 300.0f;
static constexpr float kWoodGapMinY = 300.0f;
static constexpr float kWoodGapMaxY = 500.0f;
static constexpr float kWoodBotMinY = 500.0f;
static constexpr float kWoodBotMaxY = 700.0f;

// End-of-stage vertical gate
static constexpr float kEndVX0 = 1100.0f;
static constexpr float kEndVX1 = 1200.0f;
static constexpr float kEndVTopMinY = 100.0f;
static constexpr float kEndVTopMaxY = 300.0f;
static constexpr float kEndVGapMinY = 300.0f;
static constexpr float kEndVGapMaxY = 500.0f;
static constexpr float kEndVBotMinY = 500.0f;
static constexpr float kEndVBotMaxY = 700.0f;

namespace {
	inline Math::Vector2D toM(const glm::vec2& v) { return Math::Vector2D(v.x, v.y); }
	inline Math::Vector3D toM(const glm::vec3& v) { return Math::Vector3D(v.x, v.y, v.z); }
	inline glm::vec2 toG(const Math::Vector2D& v) { return glm::vec2(v.x, v.y); }
	inline glm::vec3 toG(const Math::Vector3D& v) { return glm::vec3(v.x, v.y, v.z); }

	// Debug helpers for drawing grid cells
	static void DebugDrawCellRect(float cellSize, int cx, int cy) {
		const float x0 = cx * cellSize;
		const float y0 = cy * cellSize;
		const float x1 = x0 + cellSize;
		const float y1 = y0 + cellSize;

		// outline rectangle using four lines
		DebugRenderer::DrawLine({ x0, y0, 0 }, { x1, y0, 0 }, { 0, 1, 0 }); // bottom
		DebugRenderer::DrawLine({ x1, y0, 0 }, { x1, y1, 0 }, { 0, 1, 0 }); // right
		DebugRenderer::DrawLine({ x1, y1, 0 }, { x0, y1, 0 }, { 0, 1, 0 }); // top
		DebugRenderer::DrawLine({ x0, y1, 0 }, { x0, y0, 0 }, { 0, 1, 0 }); // left
	}

	static void DebugDrawNeighborhood(const collision::AABB& box, float cellSize) {
		const int minCx = static_cast<int>(std::floor(box.min.x / cellSize)) - 1;
		const int maxCx = static_cast<int>(std::floor(box.max.x / cellSize)) + 1;
		const int minCy = static_cast<int>(std::floor(box.min.y / cellSize)) - 1;
		const int maxCy = static_cast<int>(std::floor(box.max.y / cellSize)) + 1;

		for (int cy = minCy; cy <= maxCy; ++cy) {
			for (int cx = minCx; cx <= maxCx; ++cx) {
				DebugDrawCellRect(cellSize, cx, cy);
			}
		}
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

bool Scene::IsSimulationActive() const { return simulationActive; }

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
Scene::Scene(GraphicsEngine& engine) : graphicsEngine(engine) {}

void Scene::LoadScene(const std::string& sceneName) {
	(void)sceneName;

	// Optional: just pre-fill the path field for convenience
	mLevelEditor.SetPath("../levels/kitchen01.json");

	// Ensure we start EMPTY per rubric (no auto-spawned objects)
	ClearAll();

	// You can keep a background even with an empty level (or move this into JSON later)
	SetSceneBackground("../assets/Background.png");

	//GenerateStressTest(2500);
	//SetSimulationActive(true);
}

void Scene::Update(float deltaTime, GLFWwindow* window) {
	// Update input
	inputManager.Update(window);

	// Process input commands (debug toggles, force toggle, etc.)
	inputCommandHandler.ProcessCommands(inputManager, physicsManager,
		spriteID, useForces_, showAuxDebug_);

	// Level editor toggle
	if (inputManager.IsKeyJustPressed(GLFW_KEY_L)) {
		mLevelEditor.Toggle();
	}

	// Resolve physics timestep
	const float physicsDt = physicsStep_.resolveDt(inputManager, deltaTime);

	// Update animations
	animationManager.Update(deltaTime, entityManager);

	// Update collision system
	collisionManager.Update(entityManager);

	if (simulationActive) {
		// Handle player input
		playerController.HandleInput(deltaTime, inputManager, entityManager,
			movementManager, physicsManager,
			graphicsEngine, spriteID, useForces_);

		// Update movement system (kinematic or physics-based)
		if (useForces_) {
			physicsManager.Update(physicsDt, entityManager, inputManager);
		}
		else {
			movementManager.Update(physicsDt, entityManager, inputManager);
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
	debugVisualizer.DrawDebugInfo(entityManager, collisionManager,
		movementManager, spriteID, showAuxDebug_);
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
	movementManager.SetPlayerID(id);  // inform movement manager
}

GameObject* Scene::SpawnStaticSprite(const std::string& texturePath,
	const glm::vec3 position,
	const glm::vec2 size) {
	return entityManager.SpawnStaticSprite(texturePath, position, size);
}

GameObject* Scene::SpawnAnimatedSprite(
	const std::string& texturePath,
	const glm::vec3 position,
	const glm::vec2 size,
	const std::vector<glm::vec4> frames,
	float frameDuration, bool loop)
{
	return entityManager.SpawnAnimatedSprite(texturePath, position, size, frames, frameDuration, loop);
}

GameObject* Scene::GetGameObjectByID(int targetID) {
	return entityManager.GetByID(targetID);
}


void Scene::DespawnByID(int targetID) {
	// Remove from entity manager (handles transforms too)
	entityManager.DespawnByID(targetID);
}


void Scene::CollectRenderablePointers(std::vector<GameObject*>& out) {
	out = entityManager.GetAllObjects();
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

// Animation
bool Scene::HasAnimations(int id) const {
	return animationManager.HasAnimator(id);
}

std::vector<std::string> Scene::GetAnimationList(int id) const {
	// AnimationManager doesn't expose animation lists yet
	// Return empty for now - can extend AnimationManager later if needed
	return {};
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

void Scene::HandlePlayerCollisions(float physicsDt, EntityManager& entityManager) {
	if (spriteID < 0) return;

	GameObject* sprite = entityManager.GetByID(spriteID);
	if (!sprite) return;

	const glm::vec3 position = sprite->GetPositionGLM();

	// Build player's current AABB
	const collision::AABB pBox = physics::MakeColliderBox(
		sprite,
		Math::Vector3D(position.x, position.y, position.z)
	);

	// Query spatial grid for nearby candidates
	std::vector<GameObject*> candidates;
	collisionManager.GetSpatialGrid().Query(pBox, candidates);

	// Desired movement from movement system
	Math::Vector2D desiredMoveM{ 0.0f, 0.0f };

	for (GameObject* other : candidates) {
		if (other == nullptr || other == sprite) {
			continue;
		}

		const Math::Vector2D gSize = other->GetColliderSize();
		if (gSize.x <= 0.0f || gSize.y <= 0.0f) {
			continue;
		}

		const int otherID = other->GetID();
		if (npcSystem.IsLaneNPC(otherID)) {
			continue; // lane NPCs ignore player collision
		}

		// Current positions (M-space)
		Math::Vector3D playerPosM(position.x, position.y, position.z);
		Math::Vector3D otherPosM(other->GetPosition().x, other->GetPosition().y, other->GetPosition().z);

		// Build AABBs at those positions
		const collision::AABB pBox = physics::MakeColliderBox(sprite, playerPosM);
		const collision::AABB oBox = physics::MakeColliderBox(other, otherPosM);

		// Minimum translation vector to separate player from goat
		Math::Vector2D mtv;
		if (!collision::overlapMTV(pBox, oBox, mtv)) {
			continue;
		}

		// --- World-aware push: try to move the goat, clamped by walls ---
		constexpr float kGoatShare = 0.50f; // you can tune 0.25f..0.50f
		const Math::Vector2D desiredOtherDelta(-mtv.x * kGoatShare, -mtv.y * kGoatShare);

		// Clamp goat movement against static walls
		Math::Vector2D allowedOtherDelta = collisionManager.GetCollisionWorld().resolve(oBox, desiredOtherDelta);

		// Move goat by the allowed portion (could be zero if pinned)
		otherPosM.x += allowedOtherDelta.x;
		otherPosM.y += allowedOtherDelta.y;

		// Player receives the remainder so relative separation equals MTV
		Math::Vector2D playerDelta(mtv.x, mtv.y);
		playerDelta.x += allowedOtherDelta.x; // note: allowedOtherDelta is opposite-signed to MTV
		playerDelta.y += allowedOtherDelta.y;

		// Apply small bias to avoid re-penetration next frame
		constexpr float kEps = 0.5f;
		if (playerDelta.x > 0.0f) { playerPosM.x += kEps; }
		if (playerDelta.x < 0.0f) { playerPosM.x -= kEps; }
		if (playerDelta.y > 0.0f) { playerPosM.y += kEps; }
		if (playerDelta.y < 0.0f) { playerPosM.y -= kEps; }

		// Apply the separation
		playerPosM.x += playerDelta.x;
		playerPosM.y += playerDelta.y;

		// ---------------------------
		// Smooth SLIDE when goat is pinned
		// ---------------------------
		// If the goat barely moved (pinned) and we were trying to move into it,
		// remove our inward component, keep tangent (glide along goat/wall).
		{
			// Intent this frame from movement system
			const glm::vec2 v = movementManager.GetVelocity(spriteID);
			const float vLen = std::sqrt(v.x * v.x + v.y * v.y);

			// Contact normal is MTV normalized (player must move by +MTV to exit),
			// so "into" the goat means our velocity is opposite the MTV direction.
			const float mtvLen = std::sqrt(mtv.x * mtv.x + mtv.y * mtv.y);

			// How much goat actually moved vs we wanted it to move
			const float desiredLen = std::sqrt(desiredOtherDelta.x * desiredOtherDelta.x +
				desiredOtherDelta.y * desiredOtherDelta.y);
			const float allowedLen = std::sqrt(allowedOtherDelta.x * allowedOtherDelta.x +
				allowedOtherDelta.y * allowedOtherDelta.y);
			const bool goatPinned = (desiredLen > 0.0f) && (allowedLen < 0.1f * desiredLen);

			if (goatPinned && vLen > 0.0001f && mtvLen > 0.0001f && physicsDt > 0.0f) {
				// Normal pointing from goat to player (same dir as MTV applied to player)
				const glm::vec2 n = glm::vec2(mtv.x / mtvLen, mtv.y / mtvLen);

				// Inward component of our desired displacement this frame
				const glm::vec2 desiredDelta = v * physicsDt;               // what we wanted to move
				const float into = desiredDelta.x * n.x + desiredDelta.y * n.y;

				if (into > 0.0f) {
					// Remove inward component; keep tangential part (slide)
					const glm::vec2 tangential = desiredDelta - into * n;

					// Clamp slide against world (so we don’t scrape into walls)
					const Math::Vector2D csz = sprite->GetColliderSize();
					const collision::AABB pAfterSep =
						collision::World::makeAABBFromCenter(
							playerPosM,
							Math::Vector3D(csz.x, csz.y, 1.0f));

					const Math::Vector2D slideDesired(tangential.x, tangential.y);
					const Math::Vector2D slideAllowed =
						collisionManager.GetCollisionWorld().resolve(pAfterSep, slideDesired);

					// Apply the allowed slide
					playerPosM.x += slideAllowed.x;
					playerPosM.y += slideAllowed.y;

					// We *don’t* clear the click target here; player is gliding along nicely.
				}
				else {
					// We’re not pushing into the goat (moving away or parallel) – no special handling.
				}
			}
			else if (goatPinned && vLen > 0.0001f && mtvLen > 0.0001f) {
				// If we cannot compute a valid slide (e.g., physicsDt==0), at least
				// stop the long click-run to prevent jitter.
				const float dotInto = (mtv.x / mtvLen) * (v.x / (vLen + 1e-6f))
					+ (mtv.y / mtvLen) * (v.y / (vLen + 1e-6f));
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


void Scene::ApplyFinalConstraints(EntityManager& entityManager) {
	if (spriteID < 0) return;

	GameObject* sprite = entityManager.GetByID(spriteID);
	if (!sprite) return;

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
	if (std::abs(moveLeft) < std::abs(moveRight)) { posM.x += moveLeft; }
	else { posM.x += moveRight; }
}

void Scene::ResolveInitialStaticOverlaps() {
	// World rectangles (same constants you use to build the wood)
	const float woodX0 = kWoodX0;
	const float woodX1 = kWoodX1;

	// Two solid vertical segments (top and bottom). The gap is between them.
	const float topY0 = kWoodTopMinY, topY1 = kWoodTopMaxY;
	const float botY0 = kWoodBotMinY, botY1 = kWoodBotMaxY;

	// Walkable frame (outer walls) – we’ll clamp to this too.
	const collision::WalkArea walk{ kWalkL, kWalkR, kWalkT, kWalkB, kEdgeThick };

	std::vector<GameObject*> objs = entityManager.GetAllObjects();

	for (GameObject* g : objs) {
		if (!g) continue;

		// Work in M-space (your math structs)
		Math::Vector3D pM(g->GetPositionGLM().x, g->GetPositionGLM().y, g->GetPositionGLM().z);

		// First, keep inside the big walk rect (matches your art frame)
		physics::ClampInsideWalk(walk, g, pM);

		// Build the object's collider AABB at this tentative position
		collision::AABB box = physics::MakeColliderBox(g, pM);

		// If overlapping TOP wood plank, nudge horizontally to nearest side.
		if (OverlapsRect(box, woodX0, woodX1, topY0, topY1)) {
			SnapHorizontallyOutOfBand(box, woodX0, woodX1, pM);
			// Rebuild AABB after moving
			box = physics::MakeColliderBox(g, pM);
		}

		// If overlapping BOTTOM wood plank, nudge horizontally to nearest side.
		if (OverlapsRect(box, woodX0, woodX1, botY0, botY1)) {
			SnapHorizontallyOutOfBand(box, woodX0, woodX1, pM);
			box = physics::MakeColliderBox(g, pM);
		}

		// Done. Commit the corrected position.
		g->SetPosition(glm::vec3(pM.x, pM.y, pM.z));
	}

	// Optional: if any objects moved significantly, update world structures once.
	RebuildColliders();
}


void Scene::RebuildColliders() {
	collisionManager.Clear();
	BuildLevelColliders();
}

// World / Collision
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

	collisionManager.BuildWalls(walk, wood, gate);

	movementManager.SetCollisionWorld(&collisionManager.GetCollisionWorld());

	movementManager.SetNPCSystem(&npcSystem);

	physicsManager.SetMovementManager(&movementManager);
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

	std::cout << "[Scene] Stress test complete:\n";
	std::cout << "  - Total objects: " << objectCount << "\n";
	std::cout << "  - Random positions\n";
	std::cout << "  - Random velocities\n";
	std::cout << "  - Mixed textures (" << texturePaths.size() << " types)\n";
	std::cout << "  - Scene total: " << entityManager.GetObjectCount() << " objects\n";
}






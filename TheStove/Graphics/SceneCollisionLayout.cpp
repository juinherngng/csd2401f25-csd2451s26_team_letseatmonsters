/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:         SceneCollisionLayout.cpp
 PROJECT NAME:      Project GAM200
 AUTHOR:            Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:       Implements Scene methods and helpers related to:
					- Level reference resolution (ref <-> framebuffer)
					- Static level geometry (kitchen tiles / benches / gate)
					- Collision-world construction for this level
					- Initial static overlap resolution
					- Final movement constraints and playerNPC collision push

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Core/JSONInclude.hpp"
#include "SceneManager.hpp"

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

 // Level constants (reference resolution + tile size)
static constexpr float kRefW = static_cast<float>(GraphicsEngine::kRefW);
static constexpr float kRefH = static_cast<float>(GraphicsEngine::kRefH);

// Tile size
static constexpr float kTile = 50.0f;

// Apply horizontal scaling factor to all X pixel-space values that were authored for 1200px width
static constexpr float kDesignRefW = 1200.0f;          // original authoring width
static constexpr float kScaleX = kRefW / kDesignRefW;

// Tile-space definitions
// Walkable inner rectangle (match to background art) in tiles
static constexpr float kWalkL_T = (116.0f * kScaleX) / kTile; // bigger number = more to the right
static constexpr float kWalkR_T = (1300.0f * kScaleX) / kTile;
static constexpr float kWalkT_T = 100.0f / kTile;  // bigger number = more down
static constexpr float kWalkB_T = 825.0f / kTile;

// Middle divider (vertical split) in tiles
static constexpr float kWoodX0_T = (563.0f * kScaleX) / kTile; // bigger number = more to the right
static constexpr float kWoodX1_T = (587.0f * kScaleX) / kTile;
static constexpr float kWoodTopMinY_T = 100.0f / kTile;
static constexpr float kWoodTopMaxY_T = 320.0f / kTile;
static constexpr float kWoodGapMinY_T = 320.0f / kTile;
static constexpr float kWoodGapMaxY_T = 570.0f / kTile;
static constexpr float kWoodBotMinY_T = 570.0f / kTile;
static constexpr float kWoodBotMaxY_T = 825.0f / kTile;

// End-of-stage vertical gate in tiles
static constexpr float kEndVX0_T = (1080.0f * kScaleX) / kTile;
static constexpr float kEndVX1_T = (1200.0f * kScaleX) / kTile;
static constexpr float kEndVTopMinY_T = 100.0f / kTile;
static constexpr float kEndVTopMaxY_T = 320.0f / kTile;
static constexpr float kEndVGapMinY_T = 320.0f / kTile;
static constexpr float kEndVGapMaxY_T = 570.0f / kTile;
static constexpr float kEndVBotMinY_T = 570.0f / kTile;
static constexpr float kEndVBotMaxY_T = 825.0f / kTile;

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

	/**
	 * @brief Performs to m.
	 * @param v Parameter for v.
	 * @return Result produced by this operation.
	 */
	inline Math::Vector2D toM(const glm::vec2& v) {
		return Math::Vector2D(v.x, v.y);
	}

	/**
	 * @brief Performs to m.
	 * @param v Parameter for v.
	 * @return Result produced by this operation.
	 */
	inline Math::Vector3D toM(const glm::vec3& v) {
		return Math::Vector3D(v.x, v.y, v.z);
	}

	/**
	 * @brief Performs to g.
	 * @param v Parameter for v.
	 * @return Result produced by this operation.
	 */
	inline glm::vec2 toG(const Math::Vector2D& v) {
		return glm::vec2(v.x, v.y);
	}

	/**
	 * @brief Performs to g.
	 * @param v Parameter for v.
	 * @return Result produced by this operation.
	 */
	inline glm::vec3 toG(const Math::Vector3D& v) {
		return glm::vec3(v.x, v.y, v.z);
	}

	struct CollisionLayoutData {
		collision::WalkArea walkArea{};
		std::vector<collision::AABB> walls;
	};

	constexpr const char* kCollisionLevel1File = "collision_level1.json";
	constexpr const char* kCollisionLevel2File = "collision_level2.json";

	/**
	 * @brief Returns whether level2 path.
	 * @param levelPath Path to the level resource.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsLevel2Path(const std::string& levelPath) {
		return levelPath.find("kitchen02") != std::string::npos;
	}

	/**
	 * @brief Resolves collision layout path from file.
	 * @param filename Parameter for filename.
	 * @return Result produced by this operation.
	 */
	std::string ResolveCollisionLayoutPathFromFile(const char* filename) {
		namespace fs = std::filesystem;
		const std::array<fs::path, 5> candidates = {
			fs::path("levels") / filename,
			fs::path("../levels") / filename,
			fs::path("../../levels") / filename,
			fs::path("../../../levels") / filename,
			fs::path("../../../../levels") / filename
		};

		for (const fs::path& candidate : candidates) {
			if (fs::exists(candidate)) {
				return candidate.lexically_normal().string();
			}
		}

		return candidates.front().lexically_normal().string();
	}

	/**
	 * @brief Resolves collision layout path.
	 * @param levelPath Path to the level resource.
	 * @return Result produced by this operation.
	 */
	std::string ResolveCollisionLayoutPath(const std::string& levelPath) {
		if (IsLevel2Path(levelPath)) {
			return ResolveCollisionLayoutPathFromFile(kCollisionLevel2File);
		}
		return ResolveCollisionLayoutPathFromFile(kCollisionLevel1File);
	}

	/**
	 * @brief Builds fallback layout.
	 * @param isLevel2 Parameter for is level2.
	 * @return Result produced by this operation.
	 */
	CollisionLayoutData BuildFallbackLayout(bool isLevel2) {
		CollisionLayoutData data;
		data.walkArea = collision::WalkArea{ kWalkL, kWalkR, kWalkT, kWalkB, kEdgeThick };

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

		auto pushRect = [&data](float x0, float y0, float x1, float y1) {
			collision::AABB r{};
			r.min = Math::Vector2D(x0, y0);
			r.max = Math::Vector2D(x1, y1);
			data.walls.push_back(r);
			};

		pushRect(data.walkArea.L - data.walkArea.edgeThick, data.walkArea.T, data.walkArea.L, data.walkArea.B);
		if (isLevel2) {
			pushRect(data.walkArea.R, data.walkArea.T, data.walkArea.R + data.walkArea.edgeThick, data.walkArea.B);
		}
		else {
			pushRect(data.walkArea.R, data.walkArea.T, data.walkArea.R + data.walkArea.edgeThick, gate.gapMinY);
			pushRect(data.walkArea.R, gate.gapMaxY, data.walkArea.R + data.walkArea.edgeThick, data.walkArea.B);
		}
		pushRect(data.walkArea.L, data.walkArea.T - data.walkArea.edgeThick, data.walkArea.R, data.walkArea.T);
		pushRect(data.walkArea.L, data.walkArea.B, data.walkArea.R, data.walkArea.B + data.walkArea.edgeThick);
		if (!isLevel2) {
			pushRect(wood.x0, wood.topMinY, wood.x1, wood.topMaxY);
			pushRect(wood.x0, wood.botMinY, wood.x1, wood.botMaxY);
			pushRect(gate.x0, gate.topMinY, gate.x1, gate.topMaxY);
			pushRect(gate.x0, gate.botMinY, gate.x1, gate.botMaxY);
		}

		return data;
	}

	/**
	 * @brief Loads collision layout.
	 * @param path Path to process.
	 * @param out Output value for out.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool LoadCollisionLayout(const std::string& path, CollisionLayoutData& out) {
		std::ifstream input(path);
		if (!input.is_open()) {
			return false;
		}

		nlohmann::json root;
		input >> root;

		const auto& walk = root.at("walk_area");
		out.walkArea = collision::WalkArea{
			walk.at("left").get<float>(),
			walk.at("right").get<float>(),
			walk.at("top").get<float>(),
			walk.at("bottom").get<float>(),
			walk.at("edge_thickness").get<float>()
		};

		out.walls.clear();
		for (const auto& wall : root.at("walls")) {
			const auto& min = wall.at("min");
			const auto& max = wall.at("max");

			collision::AABB box{};
			box.min = Math::Vector2D(min.at(0).get<float>(), min.at(1).get<float>());
			box.max = Math::Vector2D(max.at(0).get<float>(), max.at(1).get<float>());
			out.walls.push_back(box);
		}

		return !out.walls.empty();
	}

	/**
	 * @brief Returns collision layout for scene.
	 * @param scene Scene being processed.
	 * @return Requested value.
	 */
	CollisionLayoutData GetCollisionLayoutForScene(const Scene& scene) {
		CollisionLayoutData data;
		const std::string levelPath = scene.GetCurrentLevelPath();
		const std::string path = ResolveCollisionLayoutPath(levelPath);
		const bool isLevel2 = IsLevel2Path(levelPath);

		try {
			if (LoadCollisionLayout(path, data)) {
				return data;
			}
		}
		catch (const std::exception& ex) {
			std::cerr << "[SceneCollisionLayout] Failed to parse collision layout '" << path << "': " << ex.what() << std::endl;
		}

		std::cerr << "[SceneCollisionLayout] Using fallback collision layout for: " << path << std::endl;
		return BuildFallbackLayout(isLevel2);
	}

	/**
	 * @brief Performs make tile rect.
	 * @param tx0 Parameter for tx0.
	 * @param ty0 Parameter for ty0.
	 * @param tx1 Parameter for tx1.
	 * @param ty1 Parameter for ty1.
	 * @return Result produced by this operation.
	 */
	collision::AABB MakeTileRect(float tx0, float ty0, float tx1, float ty1) {
		collision::AABB r{};
		r.min = { tx0 * kTile, ty0 * kTile };
		r.max = { tx1 * kTile, ty1 * kTile };
		return r;
	}

	/**
	 * @brief Returns whether overlapsrect.
	 * @param box Parameter for box.
	 * @param x0 Parameter for x0.
	 * @param x1 Parameter for x1.
	 * @param y0 Parameter for y0.
	 * @param y1 Parameter for y1.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool OverlapsRect(const collision::AABB& box, float x0, float x1, float y0, float y1) {
		return (box.min.x < x1 && box.max.x > x0 &&
			box.min.y < y1 && box.max.y > y0);
	}

	/**
	 * @brief Performs snap horizontally out of band.
	 * @param box Parameter for box.
	 * @param bandX0 Parameter for band x0.
	 * @param bandX1 Parameter for band x1.
	 * @param posM Parameter for pos m.
	 */
	void SnapHorizontallyOutOfBand(const collision::AABB& box, float bandX0, float bandX1, Math::Vector3D& posM) {
		const float moveLeft = bandX0 - box.max.x - 0.5f; // small epsilon
		const float moveRight = bandX1 - box.min.x + 0.5f;

		if (std::abs(moveLeft) < std::abs(moveRight)) {
			posM.x += moveLeft;
		}
		else {
			posM.x += moveRight;
		}
	}

	/**
	 * @brief Performs snap vertically out of band.
	 * @param box Parameter for box.
	 * @param bandY0 Parameter for band y0.
	 * @param bandY1 Parameter for band y1.
	 * @param posM Parameter for pos m.
	 */
	void SnapVerticallyOutOfBand(const collision::AABB& box, float bandY0, float bandY1, Math::Vector3D& posM) {
		const float moveUp = bandY0 - box.max.y - 0.5f; // small epsilon
		const float moveDown = bandY1 - box.min.y + 0.5f;

		if (std::abs(moveUp) < std::abs(moveDown)) {
			posM.y += moveUp;
		}
		else {
			posM.y += moveDown;
		}
	}
}

/**
 * @brief Performs scale xto current.
 * @param referenceX Parameter for reference x.
 * @return Result produced by this operation.
 */
float Scene::ScaleXToCurrent(float referenceX) const {
	const float worldWidth = static_cast<float>(graphicsEngine.GetWidth());
	return referenceX * worldWidth / kRefW;
}

/**
 * @brief Performs scale yto current.
 * @param referenceY Parameter for reference y.
 * @return Result produced by this operation.
 */
float Scene::ScaleYToCurrent(float referenceY) const {
	const float worldHeight = static_cast<float>(graphicsEngine.GetHeight());
	return referenceY * worldHeight / kRefH;
}

/**
 * @brief Performs to ref x.
 * @param currentX Parameter for current x.
 * @return Result produced by this operation.
 */
float Scene::ToRefX(float currentX) const {
	const float worldW = static_cast<float>(graphicsEngine.GetWidth());
	return currentX * (kRefW / worldW);
}

/**
 * @brief Performs to ref y.
 * @param currentY Parameter for current y.
 * @return Result produced by this operation.
 */
float Scene::ToRefY(float currentY) const {
	const float worldH = static_cast<float>(graphicsEngine.GetHeight());
	return currentY * (kRefH / worldH);
}

/**
 * @brief Performs clamp to walk area.
 * @param obj Parameter for obj.
 * @return Result produced by this operation.
 */
void Scene::ClampToWalkArea(GameObject* obj) {
	if (obj == nullptr) {
		return;
	}

	// Skip clamping for objects whose tag are empty
	const std::string tag = GetObjectTag(obj->GetID());
	if (tag == "") {
		return;
	}

	// Skip clamping for non - collidable layers
	const std::string layerName = GetObjectLayer(obj->GetID());
	Layer* layer = GetLayer(layerName);
	if (layer && !layer->IsCollidable()) {
		return;
	}

	Math::Vector3D pos(obj->GetPosition().x, obj->GetPosition().y, obj->GetPosition().z);
	const collision::WalkArea w = GetWalkArea();
	physics::ClampInsideWalk(w, obj, pos);

	const glm::vec3 clampedPos(pos.x, pos.y, pos.z);
	obj->SetPosition(clampedPos);
	entityManager.SetPosition(obj->GetID(), clampedPos);
}

/**
 * @brief Resolves world step.
 * @param obj Parameter for obj.
 * @param desiredDelta Parameter for desired delta.
 * @return Result produced by this operation.
 */
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

	// Ask world how much of desiredDelta were allowed to move
	Math::Vector2D desired(desiredDelta.x, desiredDelta.y);
	Math::Vector2D allowed = world.resolve(start, desired);

	return glm::vec2(allowed.x, allowed.y);
}

/**
 * @brief Returns walk area.
 * @return Requested value.
 */
collision::WalkArea Scene::GetWalkArea() const {
	return GetCollisionLayoutForScene(*this).walkArea;
}

/**
 * @brief Builds level colliders.
 * @return Result produced by this operation.
 */
void Scene::BuildLevelColliders() {
	const CollisionLayoutData layout = GetCollisionLayoutForScene(*this);

	collisionManager.Clear();
	collisionManager.AddStaticRects(layout.walls);

	// Get a pointer to the shared collision world
	collision::World* world = &collisionManager.GetCollisionWorld();

	// Give that world to movement + NPC + physics
	movementManager.SetCollisionWorld(world);
	movementManager.SetNPCSystem(&npcSystem);

	physicsManager.SetMovementManager(&movementManager);
	physicsManager.SetCollisionWorld(world);
}

/**
 * @brief Applies final constraints.
 * @param entityMgr Parameter for entity mgr.
 * @return Result produced by this operation.
 */
void Scene::ApplyFinalConstraints(EntityManager& entityMgr) {
	if (spriteID < 0) {
		return;
	}

	GameObject* sprite = entityMgr.GetByID(spriteID);
	if (!sprite) {
		return;
	}

	glm::vec3 position = sprite->GetPositionGLM();

	const collision::WalkArea walk = GetWalkArea();

	Math::Vector3D posM = toM(position);
	if (IsLevel2Path(GetCurrentLevelPath())) {
		physics::ClampInsideWalk(walk, sprite, posM);
	}
	else {
		const collision::StageEndGateVertical gate{
			kEndVX0, kEndVX1,
			kEndVTopMinY, kEndVTopMaxY,
			kEndVGapMinY, kEndVGapMaxY,
			kEndVBotMinY, kEndVBotMaxY
		};
		physics::ClampInsideWalkWithGate(walk, gate, sprite, posM);
	}
	position = toG(posM);

	// Final world boundary clamps
	const float worldW = static_cast<float>(graphicsEngine.GetWidth());
	const float worldH = static_cast<float>(graphicsEngine.GetHeight());

	position.x = glm::clamp(position.x, 0.0f, worldW);
	position.y = glm::clamp(position.y, 0.0f, worldH);

	// Apply final transforms
	sprite->SetPosition(position);
}

/**
 * @brief Resolves initial static overlaps.
 * @return Result produced by this operation.
 */
void Scene::ResolveInitialStaticOverlaps() {
	const bool isLevel2 = IsLevel2Path(GetCurrentLevelPath());

	// Wood (middle divider) in pixels
	const float woodX0 = kWoodX0;
	const float woodX1 = kWoodX1;
	const float woodTopY0 = kWoodTopMinY, woodTopY1 = kWoodTopMaxY;
	const float woodBotY0 = kWoodBotMinY, woodBotY1 = kWoodBotMaxY;

	// End-of-stage gate in pixels
	const float gateX0 = kEndVX0;
	const float gateX1 = kEndVX1;
	const float gateTopY0 = kEndVTopMinY, gateTopY1 = kEndVTopMaxY;
	const float gateBotY0 = kEndVBotMinY, gateBotY1 = kEndVBotMaxY;

	const collision::WalkArea walk = GetWalkArea();

	std::vector<GameObject*> objs = entityManager.GetAllObjects();

	for (GameObject* g : objs) {
		if (!g) continue;

		// Skip UI / non-collidable layers
		const std::string layerName = GetObjectLayer(g->GetID());
		Layer* layer = GetLayer(layerName);
		if (layer && !layer->IsCollidable()) continue;

		// Skip empty-tag objects
		const std::string tag = GetObjectTag(g->GetID());
		if (tag == "") continue;

		Math::Vector3D pM(g->GetPositionGLM().x, g->GetPositionGLM().y, g->GetPositionGLM().z);

		// Keep inside big walk rect
		physics::ClampInsideWalk(walk, g, pM);

		// Build collider AABB at tentative position
		collision::AABB box = physics::MakeColliderBox(g, pM);

		if (!isLevel2) {
			// Wood divider
			if (OverlapsRect(box, woodX0, woodX1, woodTopY0, woodTopY1)) {
				SnapHorizontallyOutOfBand(box, woodX0, woodX1, pM);
				box = physics::MakeColliderBox(g, pM);
			}
			if (OverlapsRect(box, woodX0, woodX1, woodBotY0, woodBotY1)) {
				SnapHorizontallyOutOfBand(box, woodX0, woodX1, pM);
				box = physics::MakeColliderBox(g, pM);
			}

			// Gate
			if (OverlapsRect(box, gateX0, gateX1, gateTopY0, gateTopY1)) {
				SnapHorizontallyOutOfBand(box, gateX0, gateX1, pM);
				box = physics::MakeColliderBox(g, pM);
			}
			if (OverlapsRect(box, gateX0, gateX1, gateBotY0, gateBotY1)) {
				SnapHorizontallyOutOfBand(box, gateX0, gateX1, pM);
				box = physics::MakeColliderBox(g, pM);
			}
		}

		// Commit corrected position (ALWAYS, not only level1)
		g->SetPosition(glm::vec3(pM.x, pM.y, pM.z));
	}

	// Rebuild collision world once with final positions.
	RebuildColliders();
}

/**
 * @brief Performs rebuild colliders.
 * @return Result produced by this operation.
 */
void Scene::RebuildColliders() {
	BuildLevelColliders();
}

/**
 * @brief Handles player collisions.
 * @param physicsDt Parameter for physics dt.
 * @param entityMgr Parameter for entity mgr.
 * @return Result produced by this operation.
 */
void Scene::HandlePlayerCollisions(float physicsDt, EntityManager& entityMgr) {
	if (spriteID < 0) {
		return;
	}

	GameObject* sprite = entityMgr.GetByID(spriteID);
	if (!sprite) {
		return;
	}

	// If the player's layer is non-collidable, skip all playerobject collisions.
	{
		std::string playerLayer = GetObjectLayer(spriteID);
		Layer* pl = GetLayer(playerLayer);
		if (pl && !pl->IsCollidable()) {
			return;
		}
	}

	const glm::vec3 position = sprite->GetPositionGLM();

	// Build player's current AABB for spatial query
	const collision::AABB queryBox = physics::MakeColliderBox(sprite, Math::Vector3D(position.x, position.y, position.z));

	// Query spatial grid for nearby candidates
	std::vector<GameObject*> candidates;
	collisionManager.GetSpatialGrid().Query(queryBox, candidates);

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

		// Always read the latest player position (it may have been modified in a previous iteration)
		glm::vec3 playerPosG = sprite->GetPositionGLM();
		Math::Vector3D playerPosM(playerPosG.x, playerPosG.y, playerPosG.z);
		Math::Vector3D otherPosM(other->GetPosition().x, other->GetPosition().y, other->GetPosition().z);


		// Build AABBs at those positions
		const collision::AABB playerBox = physics::MakeColliderBox(sprite, playerPosM);
		const collision::AABB otherBox = physics::MakeColliderBox(other, otherPosM);

		// Minimum translation vector to separate player from goat
		Math::Vector2D mtv;
		if (!collision::overlapMTV(playerBox, otherBox, mtv)) {
			continue;
		}

		// NEW: respect "movable by physics" flag on the other object (tables, walls, etc.)
		const bool otherMovable = other->IsMovableByPhysics();

		if (!otherMovable) {
			// For immovable objects (e.g. tables), only move the player out of overlap.

			// Make the player's correction world-safe (don't push them through walls).
			const collision::AABB startPlayerBox = physics::MakeColliderBox(sprite, playerPosM);

			Math::Vector2D desiredPlayerDelta(mtv.x, mtv.y); // MTV is in the direction we need to move the player
			Math::Vector2D allowedPlayerDelta =
				collisionManager.GetCollisionWorld().resolve(startPlayerBox, desiredPlayerDelta);

			playerPosM.x += allowedPlayerDelta.x;
			playerPosM.y += allowedPlayerDelta.y;

			// Apply the new position and continue to next candidate
			sprite->SetPosition(toG(playerPosM));
			// Do NOT move 'other' at all
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
		playerDelta.x += allowedOtherDelta.x; // opposite-signed to MTV
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

		// Sliding behaviour when goat is pinned
		{
			glm::vec2 v = movementManager.GetVelocity(spriteID);
			const float vLen = std::sqrt(v.x * v.x + v.y * v.y);
			const float mtvLen = std::sqrt(mtv.x * mtv.x + mtv.y * mtv.y);

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

					const Math::Vector2D csz = sprite->GetColliderSize();
					const collision::AABB pAfterSep = collision::World::makeAABBFromCenter(playerPosM, Math::Vector3D(csz.x, csz.y, 1.0f));

					const Math::Vector2D slideDesired(tangential.x, tangential.y);
					const Math::Vector2D slideAllowed = collisionManager.GetCollisionWorld().resolve(pAfterSep, slideDesired);

					// Apply the allowed slide
					playerPosM.x += slideAllowed.x;
					playerPosM.y += slideAllowed.y;
				}
			}
			else if (goatPinned && vLen > 0.0001f && mtvLen > 0.0001f) {
				const float dotInto =
					(mtv.x / mtvLen) * (v.x / (vLen + 1e-6f)) +
					(mtv.y / mtvLen) * (v.y / (vLen + 1e-6f));

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

/**
 * @brief Collects navigation blocker boxes.
 * @param moverObjectID Parameter for mover object id.
 * @param outBoxes Output value for out boxes.
 * @return Result produced by this operation.
 */
void Scene::CollectNavigationBlockerBoxes(int moverObjectID, std::vector<collision::AABB>& outBoxes) {
	outBoxes.clear();
	if (navigationBlockerCollector_) {
		navigationBlockerCollector_(*this, moverObjectID, outBoxes);
	}
}

namespace {

	/**
	 * @brief Performs compress cell path to waypoints.
	 * @param grid Parameter for grid.
	 * @param cells Parameter for cells.
	 * @param outWaypoints Output value for out waypoints.
	 */
	void CompressCellPathToWaypoints(const NavGrid& grid,
		const std::vector<GridCoord>& cells,
		const glm::vec2& /*goalWorld*/,
		std::vector<glm::vec2>& outWaypoints) {
		outWaypoints.clear();
		if (cells.empty()) return;

		// Same cell -> go to that cell center only
		if (cells.size() == 1) {
			outWaypoints.push_back(grid.CellCenter(cells[0]));
			return;
		}

		GridCoord prevDir{
			cells[1].x - cells[0].x,
			cells[1].y - cells[0].y
		};

		for (std::size_t i = 2; i < cells.size(); ++i) {
			GridCoord curDir{
				cells[i].x - cells[i - 1].x,
				cells[i].y - cells[i - 1].y
			};

			if (curDir.x != prevDir.x || curDir.y != prevDir.y) {
				outWaypoints.push_back(grid.CellCenter(cells[i - 1]));
				prevDir = curDir;
			}
		}

		// End exactly at the center of the final cell
		outWaypoints.push_back(grid.CellCenter(cells.back()));
	}

	/**
	 * @brief Collects navigation blockers.
	 * @param scene Scene being processed.
	 * @param moverObjectID Parameter for mover object id.
	 * @param outBoxes Output value for out boxes.
	 */
	void CollectNavigationBlockers(Scene& scene,
		int moverObjectID,
		std::vector<collision::AABB>& outBoxes) {
		scene.CollectNavigationBlockerBoxes(moverObjectID, outBoxes);
	}

	/**
	 * @brief Returns whether boxhitsanynavigationblocker.
	 * @param box Parameter for box.
	 * @param world Parameter for world.
	 * @param blockers Parameter for blockers.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool BoxHitsAnyNavigationBlocker(const collision::AABB& box,
		const collision::World& world,
		const std::vector<collision::AABB>& blockers) {
		if (world.overlapsAnyWall(box)) {
			return true;
		}

		Math::Vector2D mtv;
		for (const collision::AABB& b : blockers) {
			if (collision::overlapMTV(box, b, mtv)) {
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief Performs smooth waypoint path.
	 * @param scene Scene being processed.
	 * @param moverObjectID Parameter for mover object id.
	 * @param startWorld Parameter for start world.
	 * @param path Path to process.
	 */
	void SmoothWaypointPath(Scene& scene,
		int moverObjectID,
		const glm::vec2& startWorld,
		std::vector<glm::vec2>& path) {
		if (path.size() <= 1) {
			return;
		}

		std::vector<glm::vec2> smoothed;
		glm::vec2 anchor = startWorld;

		std::size_t i = 0;
		while (i < path.size()) {
			std::size_t furthestVisible = i;

			for (std::size_t j = i; j < path.size(); ++j) {
				if (scene.HasDirectPathForObject(moverObjectID, anchor, path[j])) {
					furthestVisible = j;
				}
				else {
					break;
				}
			}

			smoothed.push_back(path[furthestVisible]);
			anchor = path[furthestVisible];
			i = furthestVisible + 1;
		}

		path.swap(smoothed);
	}
}

/**
 * @brief Builds navigation grid for object.
 * @param moverObjectID Parameter for mover object id.
 * @param outGrid Output value for out grid.
 * @return Result produced by this operation.
 */
bool Scene::BuildNavigationGridForObject(int moverObjectID, NavGrid& outGrid) {
	outGrid = NavGrid();

	GameObject* mover = GetGameObjectByID(moverObjectID);
	if (!mover) {
		return false;
	}

	const collision::WalkArea walk = GetWalkArea();

	// Use the same tile size as your kitchen layout grid
	constexpr float kNavCellSize = kTile;

	const int width = std::max(1, static_cast<int>(std::ceil((walk.R - walk.L) / kNavCellSize)));
	const int height = std::max(1, static_cast<int>(std::ceil((walk.B - walk.T) / kNavCellSize)));

	outGrid.Reset(walk.L, walk.T, kNavCellSize, width, height);

	// Cache all navigation blocker AABBs via the game-layer hook
	std::vector<collision::AABB> tableBoxes;
	CollectNavigationBlockerBoxes(moverObjectID, tableBoxes);

	// For each cell, test whether THIS mover can stand there
	const Math::Vector2D moverOffset = mover->GetColliderOffset();

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			GridCoord c{ x, y };
			const glm::vec2 cellCenter = outGrid.CellCenter(c);

			// Convert desired collider-center back into object position
			const Math::Vector3D probePos(
				cellCenter.x - moverOffset.x,
				cellCenter.y - moverOffset.y,
				0.0f
			);

			const collision::AABB probeBox = physics::MakeColliderBox(mover, probePos);

			bool blocked = false;

			// Static walls / divider / gate
			if (GetCollisionWorld().overlapsAnyWall(probeBox)) {
				blocked = true;
			}

			// Dynamic table blockers
			if (!blocked) {
				Math::Vector2D mtv;
				for (const collision::AABB& tableBox : tableBoxes) {
					if (collision::overlapMTV(probeBox, tableBox, mtv)) {
						blocked = true;
						break;
					}
				}
			}

			outGrid.SetBlocked(x, y, blocked);
		}
	}

	return true;
}

/**
 * @brief Finds path for object.
 * @param moverObjectID Parameter for mover object id.
 * @param startWorld Parameter for start world.
 * @param goalWorld Parameter for goal world.
 * @param outPath Output value for out path.
 * @return Result produced by this operation.
 */
bool Scene::FindPathForObject(int moverObjectID, const glm::vec2& startWorld, const glm::vec2& goalWorld, std::vector<glm::vec2>& outPath) {
	outPath.clear();

	NavGrid grid;
	if (!BuildNavigationGridForObject(moverObjectID, grid)) {
		return false;
	}

	GridCoord startCell = grid.WorldToCell(startWorld);
	GridCoord goalCell = grid.WorldToCell(goalWorld);

	if (!grid.FindNearestWalkable(startCell, startCell)) {
		return false;
	}

	if (!grid.FindNearestWalkable(goalCell, goalCell)) {
		return false;
	}

	std::vector<GridCoord> rawCells;
	if (!GridPathfinder::FindPath(grid, startCell, goalCell, rawCells)) {
		return false;
	}

	CompressCellPathToWaypoints(grid, rawCells, goalWorld, outPath);
	SmoothWaypointPath(*this, moverObjectID, startWorld, outPath);
	return !outPath.empty();
}

/**
 * @brief Returns nearest navigation cell center for object.
 * @param moverObjectID Parameter for mover object id.
 * @param worldPos Parameter for world pos.
 * @param outCenter Output value for out center.
 * @return Requested value.
 */
bool Scene::GetNearestNavigationCellCenterForObject(int moverObjectID,
	const glm::vec2& worldPos,
	glm::vec2& outCenter) {
	NavGrid grid;
	if (!BuildNavigationGridForObject(moverObjectID, grid)) {
		return false;
	}

	GridCoord cell = grid.WorldToCell(worldPos);

	if (!grid.FindNearestWalkable(cell, cell)) {
		return false;
	}

	outCenter = grid.CellCenter(cell);
	return true;
}

/**
 * @brief Returns whether direct path for object.
 * @param moverObjectID Parameter for mover object id.
 * @param startWorld Parameter for start world.
 * @param goalWorld Parameter for goal world.
 * @return True when the operation succeeds or the condition is met.
 */
bool Scene::HasDirectPathForObject(int moverObjectID,
	const glm::vec2& startWorld,
	const glm::vec2& goalWorld) {
	GameObject* mover = GetGameObjectByID(moverObjectID);
	if (!mover) {
		return false;
	}

	std::vector<collision::AABB> blockers;
	CollectNavigationBlockers(*this, moverObjectID, blockers);

	glm::vec2 delta = goalWorld - startWorld;
	float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);

	if (distance <= 0.001f) {
		return true;
	}

	const Math::Vector2D moverSize = mover->GetColliderSize();

	// smaller step = safer, but more checks
	const float sampleStep = std::max(8.0f, std::min(moverSize.x, moverSize.y) * 0.25f);
	const int sampleCount = std::max(1, static_cast<int>(std::ceil(distance / sampleStep)));

	const float z = mover->GetPositionGLM().z;

	// skip t=0 so we don't fail just because the player is already very close to something
	for (int i = 1; i <= sampleCount; ++i) {
		float t = static_cast<float>(i) / static_cast<float>(sampleCount);
		glm::vec2 probeWorld = startWorld + delta * t;

		collision::AABB probeBox = physics::MakeColliderBox(
			mover,
			Math::Vector3D(probeWorld.x, probeWorld.y, z)
		);

		if (BoxHitsAnyNavigationBlocker(probeBox, GetCollisionWorld(), blockers)) {
			return false;
		}
	}

	return true;
}

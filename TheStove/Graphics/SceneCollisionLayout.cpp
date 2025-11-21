/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:          SceneCollisionLayout.cpp
 PROJECT NAME:       Project GAM200
 AUTHOR:             Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:        Implements Scene methods and helpers related to:
						- Level reference resolution (ref <-> framebuffer)
						- Static level geometry (kitchen tiles / benches / gate)
						- Collision-world construction for this level
						- Initial static overlap resolution
						- Final movement constraints and player–NPC collision push

	 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "SceneManager.hpp"

#include <array>
#include <cmath>
#include <vector>

 // Level constants (reference resolution + tile size)
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
static constexpr float kWoodX0_T = 500.0f / kTile;      // 10.0
static constexpr float kWoodX1_T = 590.0f / kTile;      // 11.8
static constexpr float kWoodTopMinY_T = 100.0f / kTile; // 2.0
static constexpr float kWoodTopMaxY_T = 320.0f / kTile; // 6.4
static constexpr float kWoodGapMinY_T = 320.0f / kTile; // 6.4
static constexpr float kWoodGapMaxY_T = 570.0f / kTile; // 11.4
static constexpr float kWoodBotMinY_T = 570.0f / kTile; // 11.4
static constexpr float kWoodBotMaxY_T = 700.0f / kTile; // 14.0

// End-of-stage vertical gate in tiles
static constexpr float kEndVX0_T = 1080.0f / kTile;     // 21.6
static constexpr float kEndVX1_T = 1200.0f / kTile;     // 24.0
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

// Benches + ingredient counter + bottom strip, all in tiles
struct StaticRectDef {
	float tx0, ty0, tx1, ty1; // tile-space coordinates
};

static constexpr std::array<StaticRectDef, 6> kStaticRectDefs{ {
		// top-middle bench
		{ 13.9f, 3.4f, 15.2f, 5.0f },
		// top-right bench
		{ 19.0f, 3.4f, 20.3f, 5.0f },
		// bottom-middle bench
		{ 13.9f, 13.3f, 15.2f, 15.0f },
		// bottom-right bench
		{ 19.0f, 13.3f, 20.3f, 15.0f },
		// Ingredient counter row (top kitchen)
		{ 4.0f, 2.0f, 11.0f, 3.5f },
		// Bottom solid area: grills + green + both posts
		{ 4.0f, 14.8f, 11.0f, 16.0f }
	} };

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

	// Build an AABB in world/reference space from tile coordinates.
	collision::AABB MakeTileRect(float tx0, float ty0, float tx1, float ty1) {
		collision::AABB r{};
		r.min = { tx0 * kTile, ty0 * kTile };
		r.max = { tx1 * kTile, ty1 * kTile };
		return r;
	}

	// Returns true if AABB "box" overlaps the axis-aligned rectangle [x0,x1]x[y0,y1].
	bool OverlapsRect(const collision::AABB& box, float x0, float x1, float y0, float y1) {
		return (box.min.x < x1 && box.max.x > x0 &&
				box.min.y < y1 && box.max.y > y0);
	}

	// Snap a dynamic object horizontally out of the vertical wood segment it overlaps (minimal move).
	void SnapHorizontallyOutOfBand(const collision::AABB& box,
								   float bandX0, float bandX1,
								   Math::Vector3D& posM) {
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
	void SnapVerticallyOutOfBand(const collision::AABB& box,
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
}

// Reference <-> current framebuffer conversion
float Scene::ScaleXToCurrent(float referenceX) const {
	const float worldWidth = static_cast<float>(graphicsEngine.GetWidth());
	return referenceX * worldWidth / kRefW;
}

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

// Clamp helpers / world step resolution
void Scene::ClampToWalkArea(GameObject* obj) {
	if (obj == nullptr) {
		return;
	}

	const collision::WalkArea walk{ kWalkL, kWalkR, kWalkT, kWalkB, kEdgeThick };
	Math::Vector3D p(obj->GetPosition().x,
					 obj->GetPosition().y,
					 obj->GetPosition().z);

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

collision::WalkArea Scene::GetWalkArea() const {
	// Uses the internal level constants defined at top of this file
	return collision::WalkArea{ kWalkL, kWalkR, kWalkT, kWalkB, kEdgeThick };
}

// Collision world construction for this level
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

// Final constraints (stage gate + framebuffer bounds)
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

// Resolve initial static overlaps vs benches / dividers
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
		if (!g) {
			continue;
		}

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

		// Benches + ingredient counter + bottom strip
		for (size_t i = 0; i < benchRects.size(); ++i) {
			const auto& r = benchRects[i];

			if (!OverlapsRect(box, r.min.x, r.max.x, r.min.y, r.max.y)) {
				continue;
			}

			if (i <= 3) {
				// Four benches: treat as vertical barrier, snap horizontally
				SnapHorizontallyOutOfBand(box, r.min.x, r.max.x, pM);
			}
			else {
				// Ingredient counter + bottom strip: snap vertically out
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

// Player–NPC collision push + sliding
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
		Math::Vector3D(position.x, position.y, position.z));

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

		// Current positions (M-space)
		Math::Vector3D playerPosM(position.x, position.y, position.z);
		Math::Vector3D otherPosM(other->GetPosition().x,
								 other->GetPosition().y,
								 other->GetPosition().z);

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
		const Math::Vector2D desiredOtherDelta(-mtv.x * kGoatShare,
											   -mtv.y * kGoatShare);

		// Clamp goat movement against static walls
		Math::Vector2D allowedOtherDelta =
			collisionManager.GetCollisionWorld().resolve(otherBox, desiredOtherDelta);

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

			const float desiredLen = std::sqrt(desiredOtherDelta.x * desiredOtherDelta.x +
											   desiredOtherDelta.y * desiredOtherDelta.y);
			const float allowedLen = std::sqrt(allowedOtherDelta.x * allowedOtherDelta.x +
											   allowedOtherDelta.y * allowedOtherDelta.y);
			const bool goatPinned =
				(desiredLen > 0.0f) && (allowedLen < 0.1f * desiredLen);

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
					const collision::AABB pAfterSep =
						collision::World::makeAABBFromCenter(
							playerPosM, Math::Vector3D(csz.x, csz.y, 1.0f));

					const Math::Vector2D slideDesired(tangential.x, tangential.y);
					const Math::Vector2D slideAllowed =
						collisionManager.GetCollisionWorld().resolve(pAfterSep,
																	 slideDesired);

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

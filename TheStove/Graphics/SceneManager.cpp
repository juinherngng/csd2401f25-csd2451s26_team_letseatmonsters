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

void Scene::SetSimulationActive(bool active) { simulationActive_ = active; }
bool Scene::IsSimulationActive() const { return simulationActive_; }

void Scene::RebuildColliders() {
	BuildLevelColliders();
}

int Scene::AcquireID() {
	if (!mFreeIDs.empty()) {
		int id = mFreeIDs.back();
		mFreeIDs.pop_back();
		return id;
	}

	return nextID++;
}

const std::string& Scene::GetObjectTexturePath(int id) const {
	static const std::string kEmpty{};
	auto it = mTexturePathByID.find(id);
	return it == mTexturePathByID.end() ? kEmpty : it->second;
}
void Scene::SetObjectTexturePath(int id, const std::string& path) { mTexturePathByID[id] = path; }

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
	BuildLevelColliders();
}

void Scene::Update(float deltaTime, GLFWwindow* window) {
	// Editor toggle
	if (inputManager.IsKeyJustPressed(GLFW_KEY_L)) {
		mLevelEditor.Toggle();
	}

	// Input & fixed-step time slice
	inputManager.Update(window);
	const float physicsDt = physicsStep_.resolveDt(inputManager, deltaTime);

	//{
	//	const int curW = graphicsEngine.GetWidth();
	//	const int curH = graphicsEngine.GetHeight();

	//	// If the editor just loaded/restored a level, don't rescale everything once more.
	//	// Instead, adopt the current framebuffer as the new baseline.
	//	if (resetBaseline_) {
	//		lastWidth_ = curW;
	//		lastHeight_ = curH;
	//		resetBaseline_ = false;
	//		// Nothing else to do this frame.
	//		return;
	//	}

	//	//if (curW != lastWidth_ || curH != lastHeight_) {
	//	//	const float baseW = (lastWidth_ > 0) ? float(lastWidth_) : kRefW;
	//	//	const float baseH = (lastHeight_ > 0) ? float(lastHeight_) : kRefH;
	//	//	const float rx = float(curW) / baseW;
	//	//	const float ry = float(curH) / baseH;

	//	//	// Rescale positions, visual scales, colliders, and caches
	//	//	for (auto& up : sceneObjects) {
	//	//		if (!up) { continue; }
	//	//		GameObject* g = up.get();

	//	//		glm::vec3 p = g->GetPositionGLM();
	//	//		p.x *= rx; p.y *= ry;
	//	//		g->SetPosition(p);

	//	//		glm::vec3 s = g->GetScaleGLM();
	//	//		s.x *= rx; s.y *= ry;
	//	//		g->SetScale(s);

	//	//		Math::Vector2D cs = g->GetColliderSize();
	//	//		Math::Vector2D co = g->GetColliderOffset();
	//	//		g->SetColliderSize({ cs.x * rx, cs.y * ry });
	//	//		g->SetColliderOffset({ co.x * rx, co.y * ry });

	//	//		const int id = g->GetID();
	//	//		spritePositions[id] = g->GetPositionGLM();
	//	//		spriteScales[id] = g->GetScaleGLM();
	//	//	}

	//	//	// Rescale click/seek targets if you use them
	//	//	clickTarget.x *= rx; clickTarget.y *= ry;
	//	//	seekTargetM.x *= rx; seekTargetM.y *= ry;
	//	//	playerPosM2D_.x *= rx; playerPosM2D_.y *= ry;

	//	//	RebuildColliders();

	//	//	lastWidth_ = curW;
	//	//	lastHeight_ = curH;
	//	//}
	//}


	// Walk area for clamps (scaled to current framebuffer size)
	const collision::WalkArea walk{
	ScaleXToCurrent(kWalkL), ScaleXToCurrent(kWalkR),
	ScaleYToCurrent(kWalkT), ScaleYToCurrent(kWalkB),
	std::max(ScaleXToCurrent(kEdgeThick), ScaleYToCurrent(kEdgeThick))
	};

	// Advance animations (per-object)
	for (auto& [id, animMap] : objectAnimations) {
		std::string& animName = currentAnimation[id];
		Animator2D& animator = animMap[animName];

		// Only move the animator clock when the simulation is running.
		if (simulationActive_) {
			animator.Update(deltaTime);
		}

		// Always push whatever the current frame is to the sprite,
		// so pausing leaves a stable, non-smeared frame on screen.
		if (GameObject* obj = GetGameObjectByID(id)) {
			obj->SetUVRect(animator.GetCurrentFrameUV());
		}
	}

	// Basic transforms
	const float rotationSpeed = deltaTime; // degrees per second
	float moveSpeed = 200.0f * physicsDt;

	GameObject* sprite = nullptr;
	bool hasPlayer = (spriteID >= 0);
	if (hasPlayer) {
		sprite = GetGameObjectByID(spriteID);

		if (sprite == nullptr) {
			std::cerr << "Sprite with ID " << spriteID << " not found\n";
			spriteID = -1;
			hasPlayer = false;
		}
	}

	// Lazy-attach player force rig (works for JSON-loaded player too)
	if (playerRB_ == nullptr) {
		playerRB_ = new RigidBody2D();
		playerRB_->Initialize();
		playerRB_->SetForceRegistry(&mForceRegistry);
		playerRB_->SetMass(1.0f);
		playerRB_->SetLinearDamping(0.98f);
		playerRB_->SetUseGravity(false);
		playerRB_->Stop();

		// Start seek target; if no player, use (0,0) as neutral
		if (hasPlayer && sprite) {
			const Math::Vector3D p0 = sprite->GetPosition();
			seekTargetM = Math::Vector2D(p0.x, p0.y);
			playerPosM2D_ = Math::Vector2D(p0.x, p0.y);
		}
		else {
			seekTargetM = Math::Vector2D(0.0f, 0.0f);
			playerPosM2D_ = Math::Vector2D(0.0f, 0.0f);
		}

		static DragForce drag(0.8f, 0.02f);
		static SeekForce seek(&seekTargetM, &playerPosM2D_, 800.0f);
		mForceRegistry.Add(playerRB_, &drag);
		mForceRegistry.Add(playerRB_, &seek);
	}

	GameObject* other1 = GetGameObjectByID(otherID);
	if (!other1 && otherID != -1) {
		std::cerr << "Sprite with ID " << otherID << " not found" << std::endl;
		otherID = -1;
	}
	GameObject* other2 = GetGameObjectByID(otherID2);
	if (!other2 && otherID2 != -1) {
		std::cerr << "Sprite with ID " << otherID2 << " not found" << std::endl;
		otherID2 = -1;
	}
	GameObject* dino = GetGameObjectByID(dinoID);
	if (!dino && dinoID != -1) {
		dinoID = -1;
	}

	glm::vec3 dummyPos{ 0.0f, 0.0f, 0.0f };
	glm::vec3 dummyScale{ 1.0f, 1.0f, 1.0f };
	float dummyRot = 0.0f;

	glm::vec3* positionPtr = hasPlayer ? &spritePositions[spriteID] : &dummyPos;
	glm::vec3* scalePtr = hasPlayer ? &spriteScales[spriteID] : &dummyScale;
	float* rotationPtr = hasPlayer ? &spriteRotations[spriteID] : &dummyRot;

	auto& position = *positionPtr;
	auto& scale = *scalePtr;
	auto& rotation = *rotationPtr;

	glm::vec3* o1position = (other1 ? &spritePositions[otherID] : nullptr);
	glm::vec3* o2position = (other2 ? &spritePositions[otherID2] : nullptr);

	// Rebuild spatial grid
	mSpatialGrid.Clear();
	std::vector<GameObject*> allObjects;
	CollectRenderablePointers(allObjects);
	for (GameObject* obj : allObjects) {
		const Math::Vector3D posM(obj->GetPosition().x, obj->GetPosition().y, obj->GetPosition().z);
		const collision::AABB box = physics::MakeColliderBox(obj, posM);
		mSpatialGrid.Insert(obj, box);
	}

	// Per-frame desired displacement & NPC velocities
	Math::Vector2D desiredMoveM{ 0.0f, 0.0f };
	Math::Vector2D other1VelM = toM(GetNPCVelocity(otherID));
	Math::Vector2D other2VelM = toM(GetNPCVelocity(otherID2));
	Math::Vector2D allowedM{ 0.0f, 0.0f };

	// Debug toggles
	if (inputManager.IsKeyJustPressed(GLFW_KEY_R)) {
		DebugRenderer::SetEnabled(!DebugRenderer::IsEnabled());
		std::cout << "[DebugRenderer] Collider box visibility: "
			<< (DebugRenderer::IsEnabled() ? "ON" : "OFF") << std::endl;
	}
	if (inputManager.IsKeyJustPressed(GLFW_KEY_T)) {
		showAuxDebug_ = !showAuxDebug_;
		std::cout << "[Debug] Points/Lines: "
			<< (showAuxDebug_ ? "ON" : "OFF") << "\n";
	}
	if (inputManager.IsKeyJustPressed(GLFW_KEY_F)) {
		useForceForClickMove_ = !useForceForClickMove_;
		std::cout << "[Force] useForceForClickMove_ = "
			<< (useForceForClickMove_ ? "ON" : "OFF") << "\n";
	}

	if (hasPlayer) {
		if (inputManager.IsKeyPressed(GLFW_KEY_UP)) {
			std::cout << "Up key pressed: scale = " << scale.x << "," << scale.y << "," << scale.z << std::endl;
			scale *= 1.01f;

			// Clamp max scale
			scale = glm::min(scale, glm::vec3(500.0f));
		}
		if (inputManager.IsKeyPressed(GLFW_KEY_DOWN)) {
			std::cout << "Down key pressed: scale = " << scale.x << "," << scale.y << "," << scale.z << std::endl;
			scale *= 0.99f;

			// Clamp min scale
			scale = glm::max(scale, glm::vec3(50.0f));
		}
		if (inputManager.IsKeyPressed(GLFW_KEY_RIGHT)) {
			rotation += rotationSpeed;
			if (rotation > 360.0f) rotation -= 360.0f;

			std::cout << "Right key pressed: rotation = " << rotation << std::endl;
		}
		if (inputManager.IsKeyPressed(GLFW_KEY_LEFT)) {
			rotation -= rotationSpeed;
			if (rotation < 0.0f) rotation += 360.0f;

			std::cout << "Left key pressed: rotation = " << rotation << std::endl;
		}

		// Keyboard WASD movement + facing textures
		if (inputManager.IsKeyPressed(GLFW_KEY_W)) {
			sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_back", "../assets/mc_sprite_back.png"));
			desiredMoveM.y -= moveSpeed; // up
		}
		if (inputManager.IsKeyPressed(GLFW_KEY_S)) {
			sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_front", "../assets/mc_sprite_front.png"));
			desiredMoveM.y += moveSpeed; // down
		}
		if (inputManager.IsKeyPressed(GLFW_KEY_A)) {
			sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideleft", "../assets/mc_sprite_left.png"));
			desiredMoveM.x -= moveSpeed; // left
		}
		if (inputManager.IsKeyPressed(GLFW_KEY_D)) {
			sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideright", "../assets/mc_sprite_right.png"));
			desiredMoveM.x += moveSpeed; // right
		}

		const bool hasKeyboardInput = (desiredMoveM.x != 0.0f) || (desiredMoveM.y != 0.0f);
		if (hasKeyboardInput) {
			hasClickTarget = false; // stop click-to-move
			seekTargetM = Math::Vector2D(position.x, position.y);
			if (playerRB_ != nullptr) {
				playerRB_->Stop();
			}
		}

		/*if (inputManager.IsKeyPressed(GLFW_KEY_1)) {
			SetAnimation(dinoID, "WALK");
			std::cout << "Set to Walk Animation" << std::endl;
		}
		if (inputManager.IsKeyPressed(GLFW_KEY_2)) {
			SetAnimation(dinoID, "ATTACK");
			std::cout << "Set to Attack Animation" << std::endl;
		}
		if (inputManager.IsKeyPressed(GLFW_KEY_3)) {
			SetAnimation(dinoID, "IDLE");
			std::cout << "Set to Idle Animation" << std::endl;
		}*/

		// Click-to-Move: selection and target setting
		if (inputManager.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
			glm::vec2 mouseWorld;
			// ✅ New FBO-based picking call
			if (graphicsEngine.GetMouseWorldInScene(mouseWorld)) {
				const glm::vec2 mouse = mouseWorld;

				if (!playerSelected) {
					const Math::Vector2D csize = sprite->GetColliderSize();
					const Math::Vector2D coff = sprite->GetColliderOffset();

					const Math::Vector3D selCenterM(position.x + coff.x, position.y + coff.y, position.z);
					const Math::Vector3D selScaleM(csize.x, csize.y, 1.0f);

					if (collision::pointInsideCenterAABB(toM(mouse), selCenterM, selScaleM)) {
						playerSelected = true;
						hasClickTarget = false;
						stuckFrames = 0;

						seekTargetM = Math::Vector2D(position.x, position.y);
						if (playerRB_ != nullptr) {
							playerRB_->Stop();
						}
					}
				}
				else {
					clickTarget = glm::vec3(mouse.x, mouse.y, 0.0f);
					hasClickTarget = true;
					stuckFrames = 0;

					seekTargetM = Math::Vector2D(clickTarget.x, clickTarget.y);

					// Face toward the new target (dominant axis)
					glm::vec2 toTarget = clickTarget - glm::vec2(position.x, position.y);
					if (glm::length(toTarget) > 0.001f) {
						float ax = std::abs(toTarget.x);
						float ay = std::abs(toTarget.y);
						if (ax >= ay) {
							if (toTarget.x >= 0.0f)
								sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideright", "../assets/mc_sprite_right.png"));
							else
								sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideleft", "../assets/mc_sprite_left.png"));
						}
						else {
							if (toTarget.y >= 0.0f)
								sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_front", "../assets/mc_sprite_front.png"));
							else
								sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_back", "../assets/mc_sprite_back.png"));
						}
					}
				}
			}
		}


		if (inputManager.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
			playerSelected = false;
			hasClickTarget = false;
			stuckFrames = 0;

			seekTargetM = Math::Vector2D(position.x, position.y);
			if (playerRB_ != nullptr) {
				playerRB_->Stop();
			}
		}
	}

	// Build current AABB from collider size/offset for collision resolution
	collision::AABB startBox{};
	if (hasPlayer && sprite) {
		const Math::Vector3D centerM(
			position.x + sprite->GetColliderOffset().x,
			position.y + sprite->GetColliderOffset().y,
			position.z
		);
		const Math::Vector3D scaleM(
			sprite->GetColliderSize().x,
			sprite->GetColliderSize().y,
			1.0f
		);
		startBox = collision::World::makeAABBFromCenter(centerM, scaleM);
	}

	// Click-to-move displacement (either kinematic OR force-driven)
	if (playerSelected && hasClickTarget) {
		const glm::vec2 pos2(position.x, position.y);
		const glm::vec2 toTarget = clickTarget - pos2;
		const float dist = glm::length(toTarget);

		// Arrive & stop
		constexpr float kArriveEps = 6.0f; // same as SeekForce::arriveRadius
		if (dist <= kArriveEps) {
			hasClickTarget = false;
			seekTargetM = Math::Vector2D(position.x, position.y);
			if (playerRB_ != nullptr) {
				playerRB_->Stop();
			}
			desiredMoveM = Math::Vector2D(0.0f, 0.0f);
		}
		else {
			if (!useForceForClickMove_) {
				// Kinematic click-to-move (old behavior)
				const float maxStep = playerSpeed * physicsDt;
				const glm::vec2 step = (dist <= maxStep) ? toTarget : (toTarget / dist) * maxStep;
				desiredMoveM = Math::Vector2D(desiredMoveM.x + step.x,
					desiredMoveM.y + step.y);
			}
			else {
				// physics-driven; do not add kinematic displacement
			}
		}

		// Update facing each frame while pathing (dominant axis)
		if (hasPlayer && sprite && dist > 0.001f) {
			float ax = std::abs(toTarget.x);
			float ay = std::abs(toTarget.y);
			if (ax >= ay) {
				if (toTarget.x >= 0.0f) {
					sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideright", "../assets/mc_sprite_right.png"));
				}
				else {
					sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_sideleft", "../assets/mc_sprite_left.png"));
				}
			}
			else {
				if (toTarget.y >= 0.0f) {
					sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_front", "../assets/mc_sprite_front.png"));
				}
				else {
					sprite->SetTexture(ResourceManager::Instance().LoadTexture("mc_back", "../assets/mc_sprite_back.png"));
				}
			}
		}
	}

	if (simulationActive_) {
		// NPC lane updates
		const float kLaneX = ScaleXToCurrent(1000.0f);
		if (other1 != nullptr && o1position != nullptr) {
			Math::Vector3D posM = toM(*o1position);
			physics::MoveYLaneWithBounce(mCollision, other1, posM, other1VelM, kLaneX, physicsDt);
			physics::ClampInsideWalk(walk, other1, posM);
			*o1position = toG(posM);
			other1->SetPosition(*o1position);
		}

		if (other2 != nullptr && o2position != nullptr) {
			Math::Vector3D posM = toM(*o2position);
			physics::MoveYLaneWithBounce(mCollision, other2, posM, other2VelM, kLaneX, physicsDt);
			physics::ClampInsideWalk(walk, other2, posM);
			*o2position = toG(posM);
			other2->SetPosition(*o2position);
		}

		// NPC–NPC elastic bounce (only if both exist)
		if (other1 != nullptr && other2 != nullptr && o1position != nullptr && o2position != nullptr) {
			Math::Vector3D p1M = toM(*o1position);
			Math::Vector3D p2M = toM(*o2position);
			physics::ElasticBounceEqualMass(other1, other2, p1M, p2M, other1VelM, other2VelM);

			npcVelocities_[otherID] = toG(other1VelM);
			npcVelocities_[otherID2] = toG(other2VelM);

			*o1position = toG(p1M);
			*o2position = toG(p2M);
			other1->SetPosition(*o1position);
			other2->SetPosition(*o2position);
		}
	}

	// Use spatial grid to collide player vs nearby objects (split-weight stop)
	if (hasPlayer) {
		// Build player's current AABB once
		const collision::AABB pBox = physics::MakeColliderBox(sprite, Math::Vector3D(position.x, position.y, position.z));

		// Ask the grid for only nearby candidates
		std::vector<GameObject*> candidates;
		mSpatialGrid.Query(pBox, candidates);

		// Split weight heuristic
		auto pickWeight = [&](float otherSpeed) {
			constexpr float kIdle = 5.0f;
			constexpr float kPushBiasIdle = 0.50f;
			constexpr float kPushBiasMoving = 0.50f;

			const float intentSpeed = (physicsDt > 0.0f)
				? (Math::Vector2D(
					desiredMoveM.x / physicsDt,
					desiredMoveM.y / physicsDt).Length())
				: 0.0f;

			if (otherSpeed < kIdle && intentSpeed > 0.0f) {
				return kPushBiasIdle;
			}
			else {
				return kPushBiasMoving;
			}
			};

		for (GameObject* other : candidates) {
			if (other == nullptr || other == sprite) {
				continue;
			}

			const Math::Vector2D gSize = other->GetColliderSize();
			if (gSize.x <= 0.0f || gSize.y <= 0.0f) {
				continue;
			}

			Math::Vector3D playerPosM(position.x, position.y, position.z);
			Math::Vector3D otherPosM(other->GetPosition().x, other->GetPosition().y, other->GetPosition().z);

			// Pick speed for weight (use your lane velocities where applicable)
			float otherSpeed = 0.0f;
			if (other->GetID() == otherID) {
				otherSpeed = other1VelM.Length();
			}
			else if (other->GetID() == otherID2) {
				otherSpeed = other2VelM.Length();
			}

			physics::SeparatePlayerVsOther_StopPlayerOnly(
				mCollision,
				sprite,
				other,
				playerPosM,
				otherPosM,
				desiredMoveM,
				hasClickTarget,
				pickWeight(otherSpeed));

			// write back positions
			position = toG(playerPosM);
			other->SetPosition(toG(otherPosM));
			sprite->SetPosition(position);
		}
	}

	// Apply forces to player movement (when enabled)
	if (playerRB_ != nullptr) {
		if (useForceForClickMove_ && hasClickTarget) {
			// Feed SeekForce the real position and integrate one physics slice
			playerPosM2D_ = Math::Vector2D(position.x, position.y);
			playerRB_->Update(physicsDt);

			const Math::Vector2D v = playerRB_->GetVelocity();
			desiredMoveM.x += v.x * physicsDt;
			desiredMoveM.y += v.y * physicsDt;
		}
		else {
			// Ensure no residual drift while physics is “off”
			playerRB_->Stop();
		}
	}

	// Resolve desired movement against world walls (X then Y sweep)
	if (hasPlayer && sprite) {
		// Resolve desired movement against world walls (X then Y sweep)
		allowedM = mCollision.resolve(startBox, desiredMoveM);

		// If that fails, try Y then X sweep
		if (allowedM.x == 0.f && allowedM.y == 0.f && (desiredMoveM.x != 0.f || desiredMoveM.y != 0.f)) {
			const Math::Vector2D tryX = mCollision.resolve(startBox, Math::Vector2D(desiredMoveM.x, 0.f));
			const Math::Vector2D tryY = mCollision.resolve(startBox, Math::Vector2D(0.f, desiredMoveM.y));

			if (std::abs(tryX.x) > std::abs(tryY.y)) {
				allowedM = tryX;
			}
			else {
				allowedM = tryY;
			}
		}

		// Apply allowed move
		position.x += allowedM.x;
		position.y += allowedM.y;
	}

	// Gate clamp (stage end)
	const collision::StageEndGateVertical gate{
	ScaleXToCurrent(kEndVX0), ScaleXToCurrent(kEndVX1),
	ScaleYToCurrent(kEndVTopMinY), ScaleYToCurrent(kEndVTopMaxY),
	ScaleYToCurrent(kEndVGapMinY), ScaleYToCurrent(kEndVGapMaxY),
	ScaleYToCurrent(kEndVBotMinY), ScaleYToCurrent(kEndVBotMaxY)
	};

	if (hasPlayer && sprite) {
		Math::Vector3D posM = toM(position);
		physics::ClampInsideWalkWithGate(walk, gate, sprite, posM);
		position = toG(posM);
		sprite->SetPosition(position);
	}

	// Stuck detection for click-to-move (physics-step frames only)
	if (hasPlayer && playerSelected && hasClickTarget) {
		// Only judge progress on frames where a fixed physics slice actually ran
		if (physicsDt > 0.0f) {
			const float intended = Math::Vector2D(desiredMoveM.x, desiredMoveM.y).Length();
			if (intended > 0.0f) {
				const float moved = Math::Vector2D(allowedM.x, allowedM.y).Length();

				const glm::vec2 prevPos2 = glm::vec2(position.x, position.y) - glm::vec2(allowedM.x, allowedM.y);
				const float prevDist = glm::length(clickTarget - prevPos2);
				const float newDist = glm::length(clickTarget - glm::vec2(position.x, position.y));

				const bool noProgress = (newDist >= prevDist - 0.25f);
				const bool barelyMoved = (moved <= 0.05f);

				if (noProgress || barelyMoved) {
					++stuckFrames;
				}
				else {
					stuckFrames = 0;
				}
			}
		}

		if (stuckFrames >= kStuckFramesToCancel) {
			hasClickTarget = false;
			stuckFrames = 0;
		}
	}

	// Final clamps + transforms
	if (hasPlayer) {
		const float worldW = static_cast<float>(graphicsEngine.GetWidth());
		const float worldH = static_cast<float>(graphicsEngine.GetHeight());

		position.x = glm::clamp(position.x, 0.0f, worldW);
		position.y = glm::clamp(position.y, 0.0f, worldH);

		sprite->SetScale(scale);
		sprite->SetRotation(rotation, glm::vec3(0, 0, 1));
		sprite->SetPosition(position);
	}

	// Debug draws
	// Debug draws (colliders for ALL objects; extra helpers only if we still have a player)
	if (DebugRenderer::IsEnabled()) {
		// Always draw colliders for every object so R/T works even without a player
		std::vector<GameObject*> debugObjects;
		CollectRenderablePointers(debugObjects);

		for (GameObject* g : debugObjects) {
			if (g == nullptr) {
				continue;
			}

			const auto colSize = g->GetColliderSize();
			if (colSize.x <= 0.0f || colSize.y <= 0.0f) {
				continue;
			}

			const auto colOff = g->GetColliderOffset();
			const glm::vec3 gp = g->GetPositionGLM();

			const float hx = colSize.x * 0.5f;
			const float hy = colSize.y * 0.5f;

			const glm::vec3 mn(gp.x + colOff.x - hx, gp.y + colOff.y - hy, 0.0f);
			const glm::vec3 mx(gp.x + colOff.x + hx, gp.y + colOff.y + hy, 0.0f);

			// red rectangle for any collider
			DebugRenderer::DrawRect(mn, mx, { 1.0f, 0.0f, 0.0f });
		}

		// Extra helper visuals (path line, neighborhood cells, candidate outlines)
		// only when toggled AND when the player still exists
		if (showAuxDebug_ && hasPlayer) {
			// Example: path line from player to click target if you keep that feature
			if (playerSelected && hasClickTarget) {
				DebugRenderer::DrawLine(
					{ position.x, position.y, 0.0f },
					{ clickTarget.x, clickTarget.y, 0.0f },
					{ 0.0f, 1.0f, 0.0f }
				);
			}

			// Player collider corner dots (guard all sprite uses!)
			const auto pSize = sprite->GetColliderSize();
			const auto pOff = sprite->GetColliderOffset();

			if (pSize.x > 0.0f && pSize.y > 0.0f) {
				const glm::vec3 c(position.x + pOff.x, position.y + pOff.y, 0.0f);
				const glm::vec3 mn(c.x - pSize.x * 0.5f, c.y - pSize.y * 0.5f, 0.0f);
				const glm::vec3 mx(c.x + pSize.x * 0.5f, c.y + pSize.y * 0.5f, 0.0f);

				// Red rectangle for the player collider
				DebugRenderer::DrawRect(mn, mx, { 1.0f, 0.0f, 0.0f });

				// Yellow corner points
				DebugRenderer::DrawPoint({ mn.x, mn.y, 0.0f }, { 1.0f, 1.0f, 0.0f }, 6.0f);
				DebugRenderer::DrawPoint({ mx.x, mn.y, 0.0f }, { 1.0f, 1.0f, 0.0f }, 6.0f);
				DebugRenderer::DrawPoint({ mx.x, mx.y, 0.0f }, { 1.0f, 1.0f, 0.0f }, 6.0f);
				DebugRenderer::DrawPoint({ mn.x, mx.y, 0.0f }, { 1.0f, 1.0f, 0.0f }, 6.0f);

				// Red center point
				DebugRenderer::DrawPoint(c, { 1.0f, 0.0f, 0.0f }, 7.0f);
			}

			// Draw grid neighborhood around player (green lines)
			const Math::Vector2D pSizeM = sprite->GetColliderSize();
			const Math::Vector2D pOffM = sprite->GetColliderOffset();
			const Math::Vector3D pCenterM(position.x + pOffM.x, position.y + pOffM.y, position.z);
			const Math::Vector3D pScaleM(pSizeM.x, pSizeM.y, 1.0f);
			const collision::AABB pBox = collision::World::makeAABBFromCenter(pCenterM, pScaleM);

			DebugDrawNeighborhood(pBox, mSpatialGrid.CellSize());

			// Candidate highlights (cyan rectangles)
			std::vector<GameObject*> candidates;
			mSpatialGrid.Query(
				collision::World::makeAABBFromCenter(
					{ position.x + sprite->GetColliderOffset().x, position.y + sprite->GetColliderOffset().y, position.z },
					{ sprite->GetColliderSize().x, sprite->GetColliderSize().y, 1.0f }),
				candidates
			);

			for (GameObject* g : candidates) {
				if (g == nullptr || g == sprite) {
					continue;
				}

				const auto gSize = g->GetColliderSize();
				const auto gOff = g->GetColliderOffset();
				if (gSize.x <= 0.0f || gSize.y <= 0.0f) {
					continue;
				}

				const glm::vec3 gp = g->GetPositionGLM();
				const float hx = gSize.x * 0.5f;
				const float hy = gSize.y * 0.5f;

				const glm::vec3 mn(gp.x + gOff.x - hx, gp.y + gOff.y - hy, 0.0f);
				const glm::vec3 mx(gp.x + gOff.x + hx, gp.y + gOff.y + hy, 0.0f);

				// Cyan rectangle for nearby colliders
				DebugRenderer::DrawRect(mn, mx, { 0.0f, 1.0f, 1.0f });

				// Cyan center point
				DebugRenderer::DrawPoint({ gp.x + gOff.x, gp.y + gOff.y, 0.0f }, { 0.0f, 1.0f, 1.0f }, 5.0f);
			}
		}
	}
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
	sceneObjects.clear();
	mTexturePathByID.clear();
	animators.clear();
	objectAnimations.clear();
	currentAnimation.clear();
	spritePositions.clear();
	spriteScales.clear();
	spriteRotations.clear();
	spriteID = otherID = otherID2 = dinoID = -1;

	mFreeIDs.clear();
	nextID = 0;
}

// Spawning / Object Management
GameObject* Scene::SpawnTriangle(const glm::vec3 position, const glm::vec3 scale, float rotation) {
	// Load resources
	Mesh* mesh = ResourceManager::Instance().GetMesh("triangle");
	Shader* shader = ResourceManager::Instance().GetShader("basic");
	if (!mesh || !shader) { std::cerr << "Missing resources for triangle\n"; return nullptr; }
	auto obj = std::make_unique<GameObject>(mesh, shader);
	obj->SetID(AcquireID());
	obj->SetPosition(position);
	obj->SetScale(scale);
	obj->SetRotation(glm::radians(rotation), glm::vec3(0, 0, 1));
	GameObject* raw = obj.get();
	sceneObjects.push_back(std::move(obj));
	return raw;
}

GameObject* Scene::SpawnStaticSprite(const std::string& texturePath, const glm::vec3 position, const glm::vec2 size) {
	// Load resources
	std::string textureName = "sprite_" + texturePath;
	Texture* spriteTex = ResourceManager::Instance().LoadTexture(textureName, texturePath);
	Mesh* mesh = ResourceManager::Instance().GetMesh("sprite");
	Shader* shader = ResourceManager::Instance().GetShader("staticsprite");
	if (!spriteTex || !mesh || !shader) { std::cerr << "Missing resources for static sprite\n"; return nullptr; }

	auto obj = std::make_unique<GameObject>(mesh, shader);
	obj->SetID(AcquireID());
	obj->SetPosition(position);
	obj->SetScale(glm::vec3(size.x, size.y, 1.0f));
	obj->SetTexture(spriteTex);
	mTexturePathByID[obj->GetID()] = texturePath;
	GameObject* raw = obj.get();
	sceneObjects.push_back(std::move(obj));
	return raw;
}

GameObject* Scene::SpawnAnimatedSprite(const std::string& texturePath, const glm::vec3 position, const glm::vec2 size,
	const std::vector<glm::vec4> frames, float frameDuration, bool loop) {
	// Load resources
	std::string textureName = "sprite_" + texturePath;
	Texture* spriteTex = ResourceManager::Instance().LoadTexture(textureName, texturePath);
	Mesh* mesh = ResourceManager::Instance().GetMesh("sprite");
	Shader* shader = ResourceManager::Instance().GetShader("animatedsprite");
	if (!spriteTex || !mesh || !shader) { std::cerr << "Missing resources for animated sprite\n"; return nullptr; }

	auto obj = std::make_unique<GameObject>(mesh, shader);
	obj->SetID(AcquireID());
	obj->SetPosition(position);
	obj->SetScale(glm::vec3(size.x, size.y, 1.0f));
	obj->SetTexture(spriteTex);
	GameObject* raw = obj.get();
	sceneObjects.push_back(std::move(obj));

	Animator2D animator;
	animator.SetFrames(frames, frameDuration, loop);
	animator.Play();
	animators[raw->GetID()] = animator;
	return raw;
}

GameObject* Scene::GetGameObjectByID(int targetID) {
	for (const auto& obj : sceneObjects) {
		if (obj->GetID() == targetID) {
			return obj.get();
		}
	}
	return nullptr; // Not found
}

void Scene::DespawnByID(int id) {
	// Remove from container
	auto it = std::remove_if(sceneObjects.begin(), sceneObjects.end(),
		[id](const std::unique_ptr<GameObject>& g) {
			return g && g->GetID() == id;
		});
	sceneObjects.erase(it, sceneObjects.end());

	// Remove texture bookkeeping
	mTexturePathByID.erase(id);
	spritePositions.erase(id);
	spriteScales.erase(id);
	spriteRotations.erase(id);

	// Remove all per-object state
	animators.erase(id);
	objectAnimations.erase(id);
	currentAnimation.erase(id);

	mFreeIDs.push_back(id);

	// Invalidate named handles
	if (spriteID == id) { spriteID = -1; }
	if (otherID == id) { otherID = -1; }
	if (otherID2 == id) { otherID2 = -1; }
	if (dinoID == id) { dinoID = -1; }

	std::cout << "Despawned object with ID " << id << std::endl;
}

void Scene::CollectRenderablePointers(std::vector<GameObject*>& out) const {
	out.clear();
	out.reserve(sceneObjects.size());
	for (const auto& up : sceneObjects) {
		if (up) {
			out.push_back(up.get());
		}
	}
}

std::vector<GameObject*> Scene::GetAllObjectsRaw() {
	std::vector<GameObject*> out;
	CollectRenderablePointers(out);
	return out;
}

// Scene / Transform Utilities
void Scene::SetSceneBackground(const std::string& texturePath) {
	graphicsEngine.SetBackground(texturePath);
}

void Scene::SetTransformFromLevel(int id, const glm::vec3& pos, const glm::vec3& scale, float rotation) {
	// populate the maps so Update() reads correct values on first frame
	spritePositions[id] = pos;
	spriteScales[id] = scale;
	spriteRotations[id] = rotation;

	// also sync the GameObject right now (so it renders correctly before first Update)
	if (auto* g = GetGameObjectByID(id)) {
		g->SetPosition(pos);
		g->SetScale(scale);
		g->SetRotation(glm::radians(rotation), { 0,0,1 });
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

	// keep maps in sync so Update() reads the corrected value
	spritePositions[obj->GetID()] = pg;
}

// Animation
bool Scene::HasAnimations(int id) const {
	auto it = objectAnimations.find(id);
	return (it != objectAnimations.end()) && !it->second.empty();
}

std::vector<std::string> Scene::GetAnimationList(int id) const {
	std::vector<std::string> names;
	auto it = objectAnimations.find(id);
	if (it != objectAnimations.end()) {
		names.reserve(it->second.size());
		for (const auto& kv : it->second) {
			names.push_back(kv.first);
		}
	}

	return names;
}

std::string Scene::GetCurrentAnimationName(int id) const {
	auto it = currentAnimation.find(id);
	return (it != currentAnimation.end()) ? it->second : std::string{};
}

void Scene::SetAnimation(int objID, const std::string& newAnim) {
	if (objectAnimations.count(objID) &&
		objectAnimations[objID].count(newAnim) &&
		currentAnimation[objID] != newAnim)
	{
		currentAnimation[objID] = newAnim;
		objectAnimations[objID][newAnim].Play();  // restart animation
	}
}

void Scene::AttachDinoAnimations(int objID) {
	// same frame setup as your old LoadTest
	const int cols = 24;
	const int rows = 1;
	const float frameWidth = 1.0f / float(cols);
	const float frameHeight = 1.0f / float(rows);

	std::vector<glm::vec4> idleFrames = GenerateFrames(0, 4, cols, frameWidth, frameHeight);
	std::vector<glm::vec4> walkFrames = GenerateFrames(4, 6, cols, frameWidth, frameHeight);
	std::vector<glm::vec4> attackFrames = GenerateFrames(6, 7, cols, frameWidth, frameHeight);

	Animator2D idle; idle.SetFrames(idleFrames, 0.25f, true); idle.Play();
	Animator2D walk; walk.SetFrames(walkFrames, 0.15f, true); walk.Play();
	Animator2D attack; attack.SetFrames(attackFrames, 0.15f, true); attack.Play();

	objectAnimations[objID]["IDLE"] = idle;
	objectAnimations[objID]["WALK"] = walk;
	objectAnimations[objID]["ATTACK"] = attack;
	currentAnimation[objID] = "IDLE";
}

// World / Collision
void Scene::BuildLevelColliders() {
	collision::WalkArea walk{
	ScaleXToCurrent(kWalkL), ScaleXToCurrent(kWalkR),
	ScaleYToCurrent(kWalkT), ScaleYToCurrent(kWalkB),
	std::max(ScaleXToCurrent(kEdgeThick), ScaleYToCurrent(kEdgeThick))
	};

	collision::WoodVertical wood{
		ScaleXToCurrent(kWoodX0), ScaleXToCurrent(kWoodX1),
		ScaleYToCurrent(kWoodTopMinY), ScaleYToCurrent(kWoodTopMaxY),
		ScaleYToCurrent(kWoodGapMinY), ScaleYToCurrent(kWoodGapMaxY),
		ScaleYToCurrent(kWoodBotMinY), ScaleYToCurrent(kWoodBotMaxY)
	};

	collision::StageEndGateVertical gate{
		ScaleXToCurrent(kEndVX0), ScaleXToCurrent(kEndVX1),
		ScaleYToCurrent(kEndVTopMinY), ScaleYToCurrent(kEndVTopMaxY),
		ScaleYToCurrent(kEndVGapMinY), ScaleYToCurrent(kEndVGapMaxY),
		ScaleYToCurrent(kEndVBotMinY), ScaleYToCurrent(kEndVBotMaxY)
	};

	mCollision.build(walk, wood, gate);
}

// Test Scene (kept for debugging/demo parity)
void Scene::LoadTest() {
	// Set background image
	SetSceneBackground("../assets/Background.png");

	// Visual size & spawn
	const glm::vec2 kVisualSizePx = { 128.0f, 128.0f };
	const glm::vec3 kSpawnPos = { 400.0f, 400.0f, 0.0f };

	// Collider (relative to visual)
	const glm::vec2 colliderSizePx = { 64.0f, 128.0f };
	const glm::vec2 colliderOffsetPx{ 0.0f, 0.0f };
	const float kLaneX = 1000.0f;

	// Player
	if (GameObject* player = SpawnStaticSprite("../assets/mc_sprite_front.png", kSpawnPos, kVisualSizePx)) {
		spriteID = player->GetID();
		std::cout << "Spawned player sprite with ID: " << spriteID << std::endl;

		spriteScales[spriteID] = glm::vec3(kVisualSizePx, 1.0f);
		spriteRotations[spriteID] = 0.0f;
		spritePositions[spriteID] = kSpawnPos;

		player->SetScale(glm::vec3(kVisualSizePx, 1.0f));
		player->SetColliderSize(Math::Vector2D(colliderSizePx.x, colliderSizePx.y));
		player->SetColliderOffset(Math::Vector2D(colliderOffsetPx.x, colliderOffsetPx.y));

		// Attach force rig for demo
		RigidBody2D* rb = new RigidBody2D();
		rb->Initialize();
		rb->SetForceRegistry(&mForceRegistry);
		rb->SetMass(1.0f);
		rb->SetLinearDamping(0.98f);
		rb->SetUseGravity(false);

		playerRB_ = rb;
		playerRB_->Stop();

		// neutralize Seek: target == current pos
		seekTargetM = Math::Vector2D(kSpawnPos.x, kSpawnPos.y);
		playerPosM2D_ = Math::Vector2D(kSpawnPos.x, kSpawnPos.y);

		// Create force generators (static lifetime = safe)
		static DragForce drag(0.8f, 0.02f);
		static SeekForce seek(&seekTargetM, &playerPosM2D_, 800.0f);  // NOTE: pass &playerPosM2D_

		// Register forces
		mForceRegistry.Add(rb, &drag);
		mForceRegistry.Add(rb, &seek);

		// Compute UV frames for a 4x4 grid sprite sheet
		std::vector<glm::vec4> frames;
		const int cols = 24;
		const int rows = 1;
		float frameWidth = 1.0f / (float)cols;
		float frameHeight = 1.0f / (float)rows;

		for (int row = 0; row < rows; ++row) {
			for (int col = 0; col < cols; ++col) {
				float offsetX = col * frameWidth;
				float offsetY = 1.0f - (row + 1) * frameHeight; // Flip Y
				frames.emplace_back(offsetX, offsetY, frameWidth, frameHeight);
			}
		}

		std::vector<glm::vec4> idleFrames = GenerateFrames(0, 4, cols, frameWidth, frameHeight);
		std::vector<glm::vec4> walkFrames = GenerateFrames(4, 6, cols, frameWidth, frameHeight);
		std::vector<glm::vec4> attackFrames = GenerateFrames(6, 7, cols, frameWidth, frameHeight);

		GameObject* dinoRed = SpawnAnimatedSprite("../assets/dino_red.png",
			glm::vec3(700, 650, 0),
			glm::vec2(128, 128),
			frames, 0.25f, true);

		if (dinoRed) {
			dinoID = dinoRed->GetID();
			std::cout << "Spawned dinoRed sprite with ID: " << dinoID << std::endl;

			// Store all animations
			Animator2D idleAnimator;
			idleAnimator.SetFrames(idleFrames, 0.25f, true);
			idleAnimator.Play();

			Animator2D walkAnimator;
			walkAnimator.SetFrames(walkFrames, 0.15f, true);
			walkAnimator.Play();

			Animator2D attackAnimator;
			attackAnimator.SetFrames(attackFrames, 0.15f, true);
			attackAnimator.Play();

			objectAnimations[dinoID]["IDLE"] = idleAnimator;
			objectAnimations[dinoID]["WALK"] = walkAnimator;
			objectAnimations[dinoID]["ATTACK"] = attackAnimator;
			currentAnimation[dinoID] = "IDLE";
		}

		GameObject* dinoBlue = SpawnAnimatedSprite("../assets/dino_blue.png",
			glm::vec3(700, 500, 0),
			glm::vec2(128, 128),
			frames, 0.25f, true);

		GameObject* dinoGreen = SpawnAnimatedSprite("../assets/dino_green.png",
			glm::vec3(700, 350, 0),
			glm::vec2(128, 128),
			frames, 0.25f, true);

		GameObject* dinoYellow = SpawnAnimatedSprite("../assets/dino_yellow.png",
			glm::vec3(700, 200, 0),
			glm::vec2(128, 128),
			frames, 0.25f, true);

		// Suppress unused variable warning
		(void)dinoBlue;
		(void)dinoGreen;
		(void)dinoYellow;
	}
	else {
		spriteID = -1; //invalid
		std::cerr << "Failed to spawn sprite" << std::endl;
	}

	// Other1
	const glm::vec3 kOtherSpawn1 = { kLaneX, 200.0f, 0.0f };

	if (GameObject* other1 = SpawnStaticSprite("../assets/goat_sprite_front.png", kOtherSpawn1, kVisualSizePx)) {
		otherID = other1->GetID();

		spritePositions[otherID] = kOtherSpawn1;
		spriteScales[otherID] = glm::vec3(kVisualSizePx, 1.0f);
		spriteRotations[otherID] = 0.0f;

		other1->SetColliderSize(Math::Vector2D(colliderSizePx.x, colliderSizePx.y));
		other1->SetColliderOffset(Math::Vector2D(colliderOffsetPx.x, colliderOffsetPx.y));
	}
	else {
		otherID = -1; //invalid
		std::cerr << "Failed to spawn sprite" << std::endl;
	}

	// Other2
	const glm::vec3 kOtherSpawn2 = { kLaneX, 800.0f, 0.0f };

	if (GameObject* other2 = SpawnStaticSprite("../assets/goat_sprite_front.png", kOtherSpawn2, kVisualSizePx)) {
		otherID2 = other2->GetID();

		spritePositions[otherID2] = kOtherSpawn2;
		spriteScales[otherID2] = glm::vec3(kVisualSizePx, 1.0f);
		spriteRotations[otherID2] = 0.0f;

		other2->SetColliderSize(Math::Vector2D(colliderSizePx.x, colliderSizePx.y));
		other2->SetColliderOffset(Math::Vector2D(colliderOffsetPx.x, colliderOffsetPx.y));
	}
	else {
		otherID2 = -1; //invalid
		std::cerr << "Failed to spawn sprite" << std::endl;
	}

	BuildLevelColliders();
}

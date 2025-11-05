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

void Scene::RebuildColliders() {
	BuildLevelColliders();
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

	// Walk area for clamps (scaled to current framebuffer size)
	const collision::WalkArea walk{ kWalkL, kWalkR, kWalkT, kWalkB, kEdgeThick };

	// Advance animations (per-object)
	animationManager.Update(deltaTime, entityManager);

	// Basic transforms
	const float rotationSpeed = 15.0f; // degrees per second
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

	// Get player transforms directly from GameObject (no redundant maps)
	glm::vec3 position = hasPlayer ? sprite->GetPositionGLM() : glm::vec3{ 0.0f };
	glm::vec3 scale = hasPlayer ? sprite->GetScaleGLM() : glm::vec3{ 1.0f };
	float rotation = hasPlayer ? sprite->GetRotationAngleZ() : 0.0f;

	// Get NPC positions directly
	glm::vec3 o1position = other1 ? other1->GetPositionGLM() : glm::vec3{ 0.0f };
	glm::vec3 o2position = other2 ? other2->GetPositionGLM() : glm::vec3{ 0.0f };


	collisionManager.Update(entityManager);

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
			playerRotation += 15.0f * deltaTime;
			//if (rotation > 360.0f) rotation -= 360.0f;

			std::cout << "Right key pressed: rotation = " << Math::ToDegrees(rotation) << std::endl;
		}
		if (inputManager.IsKeyPressed(GLFW_KEY_LEFT)) {
			playerRotation -= 15.0f * deltaTime;
			//if (rotation < 0.0f) rotation += 360.0f;

			std::cout << "Left key pressed: rotation = " << Math::ToDegrees(rotation) << std::endl;
		}

		if (simulationActive) {

			// Handle click-to-move
			if (inputManager.IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
				std::cout << "Mouse button pressed!" << std::endl;
				glm::vec2 mouseWorld;
				if (graphicsEngine.GetMouseWorldInScene(mouseWorld)) {
					std::cout << "Mouse world position: (" << mouseWorld.x << ", " << mouseWorld.y << ")" << std::endl;
					// Only set move target if not clicking on ImGui window
					ImGuiIO& io = ImGui::GetIO();
					std::cout << "WantCaptureMouse: " << io.WantCaptureMouse << std::endl;  
					std::cout << "hasPlayer: " << hasPlayer << std::endl;  
					std::cout << "sprite != nullptr: " << (sprite != nullptr) << std::endl;
					if (hasPlayer && sprite) {
						std::cout << "All conditions passed! Calling SetMoveTarget" << std::endl;
						// Set the move target in MovementManager
						movementManager.SetMoveTarget(spriteID, mouseWorld);

						// Face toward the new target (dominant axis)
						glm::vec2 toTarget = mouseWorld - glm::vec2(position.x, position.y);
						if (glm::length(toTarget) > 0.001f) {
							float ax = std::abs(toTarget.x);
							float ay = std::abs(toTarget.y);

							if (ax > ay) {
								// Horizontal movement dominant
								if (toTarget.x > 0.0f) {
									sprite->SetTexture(ResourceManager::Instance().LoadTexture(
										"../assets/mcspriteright.png", "../assets/mcspriteright.png"));
								}
								else {
									sprite->SetTexture(ResourceManager::Instance().LoadTexture(
										"../assets/mcspriteleft.png", "../assets/mcspriteleft.png"));
								}
							}
							else {
								// Vertical movement dominant
								if (toTarget.y > 0.0f) {
									sprite->SetTexture(ResourceManager::Instance().LoadTexture(
										"../assets/mcspritefront.png", "../assets/mcspritefront.png"));
								}
								else {
									sprite->SetTexture(ResourceManager::Instance().LoadTexture(
										"../assets/mcspriteback.png", "../assets/mcspriteback.png"));
								}
							}
						}
					}
					else{
						std::cout << "Conditions FAILED for SetMoveTarget" << std::endl;
					}
				}
				else{
					std::cout << "GetMouseWorldInScene FAILED" << std::endl;
				}
			}

			movementManager.Update(physicsDt, entityManager, inputManager);

			if (hasPlayer && sprite) {
				position = sprite->GetPositionGLM();
			}

			UpdateSpriteDirections();
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

	if (simulationActive) {
		// NPC lane updates
		const float kLaneX = 1000.0f;
		if (other1 != nullptr) {
			Math::Vector3D posM = toM(o1position);
			physics::MoveYLaneWithBounce(collisionManager.GetCollisionWorld(), other1, posM, other1VelM, kLaneX, physicsDt);
			physics::ClampInsideWalk(walk, other1, posM);
			o1position = toG(posM);
			other1->SetPosition(o1position);
		}

		if (other2 != nullptr) {
			Math::Vector3D posM = toM(o2position);
			physics::MoveYLaneWithBounce(collisionManager.GetCollisionWorld(), other2, posM, other2VelM, kLaneX, physicsDt);
			physics::ClampInsideWalk(walk, other2, posM);
			o2position = toG(posM);
			other2->SetPosition(o2position);
		}

		// NPC–NPC elastic bounce (only if both exist)
		if (other1 != nullptr && other2 != nullptr) {
			Math::Vector3D p1M = toM(o1position);
			Math::Vector3D p2M = toM(o2position);
			physics::ElasticBounceEqualMass(other1, other2, p1M, p2M, other1VelM, other2VelM);
			npcVelocities_[otherID] = toG(other1VelM);
			npcVelocities_[otherID2] = toG(other2VelM);
			o1position = toG(p1M);
			o2position = toG(p2M);
			other1->SetPosition(o1position);
			other2->SetPosition(o2position);
		}


		// Generic per-object velocity integration (for any object edited in the editor)
		for (auto const& kv : npcVelocities_) {
			const int id = kv.first;
			const glm::vec2 v = kv.second;

			// Skip the two special lane NPCs (already updated above)
			if (id == otherID || id == otherID2) {
				continue;
			}

			GameObject* g = GetGameObjectByID(id);
			if (!g) {
				continue;
			}

			Math::Vector3D posM(g->GetPosition().x, g->GetPosition().y, g->GetPosition().z);

			// simple Euler step
			posM.x += v.x * physicsDt;
			posM.y += v.y * physicsDt;

			// keep inside walk area
			physics::ClampInsideWalk(walk, g, posM);

			// write back
			g->SetPosition(glm::vec3(posM.x, posM.y, posM.z));
		}
	}

	// Use spatial grid to collide player vs nearby objects (split-weight stop)
	if (hasPlayer) {
		// Build player's current AABB once
		const collision::AABB pBox = physics::MakeColliderBox(sprite, Math::Vector3D(position.x, position.y, position.z));

		// Ask the grid for only nearby candidates
		std::vector<GameObject*> candidates;
		collisionManager.GetSpatialGrid().Query(pBox, candidates);

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

			bool playerIsMoving = movementManager.IsMoving(spriteID);

			physics::SeparatePlayerVsOther_StopPlayerOnly(
				collisionManager.GetCollisionWorld(),
				sprite,
				other,
				playerPosM,
				otherPosM,
				desiredMoveM,
				playerIsMoving,
				pickWeight(otherSpeed));

			// write back positions
			position = toG(playerPosM);
			other->SetPosition(toG(otherPosM));
			sprite->SetPosition(position);
		}
	}

	// Resolve desired movement against world walls (X then Y sweep)
	if (hasPlayer && sprite) {
		// Resolve desired movement against world walls (X then Y sweep)
		allowedM = collisionManager.GetCollisionWorld().resolve(startBox, desiredMoveM);

		// If that fails, try Y then X sweep
		if (allowedM.x == 0.f && allowedM.y == 0.f && (desiredMoveM.x != 0.f || desiredMoveM.y != 0.f)) {
			const Math::Vector2D tryX = collisionManager.GetCollisionWorld().resolve(startBox, Math::Vector2D(desiredMoveM.x, 0.f));
			const Math::Vector2D tryY = collisionManager.GetCollisionWorld().resolve(startBox, Math::Vector2D(0.f, desiredMoveM.y));

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
  kEndVX0, kEndVX1,
  kEndVTopMinY, kEndVTopMaxY,
  kEndVGapMinY, kEndVGapMaxY,
  kEndVBotMinY, kEndVBotMaxY
	};

	if (hasPlayer && sprite) {
		Math::Vector3D posM = toM(position);
		physics::ClampInsideWalkWithGate(walk, gate, sprite, posM);
		position = toG(posM);
		sprite->SetPosition(position);
	}

	// Final clamps + transforms
	if (hasPlayer) {
		const float worldW = static_cast<float>(graphicsEngine.GetWidth());
		const float worldH = static_cast<float>(graphicsEngine.GetHeight());

		position.x = glm::clamp(position.x, 0.0f, worldW);
		position.y = glm::clamp(position.y, 0.0f, worldH);

		sprite->SetScale(scale);
		sprite->SetRotation(playerRotation, glm::vec3(0, 0, 1));
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
			if (movementManager.HasMoveTarget(spriteID)) {
				glm::vec2 target = movementManager.GetMoveTarget(spriteID);
				DebugRenderer::DrawLine(
					glm::vec3(position.x, position.y, 0.0f),
					glm::vec3(target.x, target.y, 0.0f),
					glm::vec3(0.0f, 1.0f, 0.0f)
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

			DebugDrawNeighborhood(pBox, collisionManager.GetSpatialGrid().CellSize());

			// Candidate highlights (cyan rectangles)
			std::vector<GameObject*> candidates;
			collisionManager.GetSpatialGrid().Query(
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

void Scene::UpdateSpriteDirections() {
	// Update player facing direction based on movement velocity
	if (spriteID >= 0) {
		GameObject* player = entityManager.GetByID(spriteID);
		if (player) {
			glm::vec2 vel = movementManager.GetVelocity(spriteID);

			// Only change texture if moving (threshold to avoid jitter)
			if (glm::length(vel) > 10.0f) {
				float ax = std::abs(vel.x);
				float ay = std::abs(vel.y);

				if (ax > ay) {
					// Horizontal movement dominant
					if (vel.x > 0.0f) {
						player->SetTexture(ResourceManager::Instance().LoadTexture(
							"../assets/mc_sprite_right.png", "../assets/mc_sprite_right.png"));
					}
					else {
						player->SetTexture(ResourceManager::Instance().LoadTexture(
							"../assets/mc_sprite_left.png", "../assets/mc_sprite_left.png"));
					}
				}
				else {
					// Vertical movement dominant
					if (vel.y > 0.0f) {
						player->SetTexture(ResourceManager::Instance().LoadTexture(
							"../assets/mc_sprite_front.png", "../assets/mc_sprite_front.png"));
					}
					else {
						player->SetTexture(ResourceManager::Instance().LoadTexture(
							"../assets/mc_sprite_back.png", "../assets/mc_sprite_back.png"));
					}
				}
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

	entityManager.Clear();
	animationManager.Clear();
	movementManager.Clear();

	spriteID = -1;
	dinoID = -1;
	otherID = -1;
	otherID2 = -1;
}

void Scene::SetPlayerID(int id) {
	spriteID = id;
	movementManager.SetPlayerID(id);  // ✅ TELL MOVEMENT MANAGER
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
		// Turning ON: no-op here; your AttachDinoAnimations() will populate maps.
		// (HasAnimations() will start returning true once frames are attached.)
	}
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
}

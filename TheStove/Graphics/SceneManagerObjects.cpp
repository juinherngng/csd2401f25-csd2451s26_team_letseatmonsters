/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			SceneManagerObjects.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implements Scene object spawning, layer management, animation helpers,
					and tag/state utility functions.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Core/CollisionManager.hpp"
#include "../Core/GameStateManager.hpp"
#include "../Core/MovementManager.hpp"

#include "AnimationManager.hpp"
#include "GraphicsEngine.hpp"
#include "SceneManager.hpp"

#include <algorithm>
#include <cctype>

namespace {
	/**
	 * @brief Initializes a default collider for a spawned object when prefab data did not provide one.
	 * @param obj Scene object to seed with fallback collider data.
	 */
	void InitDefaultCollider(GameObject* obj) {
		if (!obj) {
			return;
		}

		Math::Vector2D collider = obj->GetColliderSize();
		if (collider.x > 0.0f && collider.y > 0.0f) {
			return;
		}

		glm::vec3 scale = obj->GetScaleGLM();
		obj->SetColliderSize(Math::Vector2D{ scale.x, scale.y });
		obj->SetColliderOffset(Math::Vector2D{ 0.0f, 0.0f });
	}
}

GameObject* Scene::SpawnStaticSprite(const std::string& texturePath,
	const glm::vec3 position,
	const glm::vec2 size,
	const std::string& layer) {
	// Spawn the sprite first, then attach scene-specific metadata and layer bookkeeping.
	GameObject* obj = entityManager.SpawnStaticSprite(texturePath, position, size);

	if (obj) {
		int id = obj->GetID();
		AssignObjectToLayer(id, layer);
		SetObjectTexturePath(id, texturePath);
		InitDefaultCollider(obj);

		// Seed a consistent shadow preset for simple static world sprites.
		obj->EnableShadow(false);
		obj->SetShadowSize(glm::vec2(size.x * 0.8f, size.y * 0.33f));
		obj->SetShadowOffset(glm::vec2(0.0f, 55.0f));
		obj->SetShadowOpacity(0.65f);
	}

	return obj;
}

/**
 * @brief Spawns an animated sprite and registers its runtime animation metadata.
 * @param texturePath Texture used by the animated sprite.
 * @param position World position of the new object.
 * @param size Render size of the new object.
 * @param frames Animation frame UVs.
 * @param frameDuration Seconds per frame.
 * @param loop Whether the animation should loop.
 * @param layer Target scene layer name.
 * @return Pointer to the spawned object, or `nullptr` if creation failed.
 */
GameObject* Scene::SpawnAnimatedSprite(const std::string& texturePath,
	const glm::vec3 position,
	const glm::vec2 size,
	const std::vector<glm::vec4>& frames,
	float frameDuration, bool loop,
	const std::string& layer) {
	// Spawn the animated entity first so follow-up metadata can reference its id.
	GameObject* obj = entityManager.SpawnAnimatedSprite(texturePath, position, size, frames, frameDuration, loop);

	if (obj) {
		int id = obj->GetID();
		AssignObjectToLayer(id, layer);
		SetObjectTexturePath(id, texturePath);
		InitDefaultCollider(obj);
		// Mirror the authored animation into the runtime animation manager.
		animationManager.AttachRuntimeAnimation(id, frames, frameDuration, loop);

		obj->EnableShadow(false);
		obj->SetShadowSize(glm::vec2(size.x * 0.8f, size.y * 0.33f));
		obj->SetShadowOffset(glm::vec2(0.0f, 55.0f));
		obj->SetShadowOpacity(0.65f);
	}

	return obj;
}

/**
 * @brief Spawns a static sprite at the same world position as an existing owner object.
 * @param ownerID Id of the object whose position should be reused.
 * @param texturePath Texture used by the spawned sprite.
 * @param width Spawned object width.
 * @param height Spawned object height.
 * @param layer Target scene layer.
 * @return Pointer to the spawned object, or `nullptr` if the owner/spawn fails.
 */
GameObject* Scene::SpawnStaticSpriteAtSamePos(int ownerID,
	const std::string& texturePath,
	float width,
	float height,
	const std::string& layer) {
	// Reuse the owner's current position so spawned pickups/effects align with it immediately.
	GameObject* owner = GetGameObjectByID(ownerID);
	if (!owner) {
		return nullptr;
	}

	glm::vec3 pos = owner->GetPositionGLM();
	GameObject* obj = SpawnStaticSprite(texturePath, glm::vec3(pos.x, pos.y, pos.z), glm::vec2(width, height), layer);
	if (!obj) {
		return nullptr;
	}

	const int id = obj->GetID();
	SetObjectTexturePath(id, texturePath);
	// Override collider/default metadata so the spawned object serializes correctly if needed.
	obj->SetColliderSize(Math::Vector2D(width, height));
	obj->SetColliderOffset(Math::Vector2D(0.0f, 0.0f));

	Scene::Defaults defs{};
	defs.pos = glm::vec3(pos.x, pos.y, pos.z);
	defs.size = glm::vec3(width, height, 1.0f);
	defs.rot = 0.0f;
	defs.colSize = Math::Vector2D(width, height);
	defs.colOff = Math::Vector2D(0.0f, 0.0f);
	defs.vel = Math::Vector2D(0.0f, 0.0f);
	defs.texture = texturePath;
	defs.tag = "ingredient";
	defs.layer = layer;
	SetDefaults(id, defs);

	ClampToWalkArea(obj);
	return obj;
}

/**
 * @brief Returns a scene object by id.
 * @param targetID Object id to search for.
 * @return Pointer to the requested object, or `nullptr` if not found.
 */
GameObject* Scene::GetGameObjectByID(int targetID) {
	// Forward directly to the entity manager, which owns the actual object storage.
	return entityManager.GetByID(targetID);
}

/**
 * @brief Returns raw pointers to all active scene objects.
 * @return Snapshot vector of raw object pointers.
 */
std::vector<GameObject*> Scene::GetAllObjectsRaw() {
	// Expose the entity manager's flat object view for systems that need transient iteration.
	return entityManager.GetAllObjects();
}

/**
 * @brief Returns the owning storage for all active scene objects.
 * @return Reference to the entity manager's unique-pointer storage.
 */
const std::vector<std::unique_ptr<GameObject>>& Scene::GetObjectStorageRaw() const {
	// Provide direct read-only access to the underlying owned object container.
	return entityManager.GetObjectStorage();
}

/**
 * @brief Despawns an object and removes all scene-side bookkeeping tied to it.
 * @param targetID Id of the object to destroy.
 */
void Scene::DespawnByID(int targetID) {
	// Play any configured teardown audio before removing object metadata and logic.
	PlayDestroyAudio(targetID);

	if (const Defaults* defaults = objectMetadata_.FindDefaults(targetID)) {
		const std::string& layerName = defaults->layer;
		if (!layerName.empty()) {
			auto layerIt = layers.find(layerName);
			if (layerIt != layers.end()) {
				layerIt->second.RemoveObject(targetID);
			}
		}
	}

	logicManager.RemoveAllFor(targetID, *this);
	objectMetadata_.Erase(targetID);
	// Remove runtime animation state before freeing the entity itself.
	animationManager.RemoveAnimator(targetID);
	entityManager.DespawnByID(targetID);
}

/**
 * @brief Builds a sorted render list of visible objects for the current frame.
 * @param out Destination vector that receives the ordered raw pointers.
 */
void Scene::CollectRenderablePointers(std::vector<GameObject*>& out) {
	// Rebuild the list every frame so layer visibility, cutscenes, and sort order stay current.
	out.clear();
	const bool cutsceneActive = IsAnyCutsceneActive();

	const auto& all = entityManager.GetObjectStorage();
	out.reserve(all.size());

	for (const auto& objPtr : all) {
		GameObject* g = objPtr.get();
		if (!g) {
			continue;
		}

		const int objId = g->GetID();
		const Defaults* defaults = objectMetadata_.FindDefaults(objId);
		if (defaults && !defaults->visible) {
			continue;
		}

		const std::string layerName = (defaults != nullptr) ? defaults->layer : "";
		// Hide non-UI objects while cutscene-specific presentation owns the screen.
		if (cutsceneActive && layerName != cutTrans_.uiLayer && layerName != cutscene_.uiLayer && layerName != "999999") {
			continue;
		}

		Layer* layer = GetObjectLayerPtr(objId);
		if (layer && (!layer->IsEnabled() || !layer->IsVisible())) {
			continue;
		}

		const std::string& texturePath = GetObjectTexturePath(objId);
		const bool isFootstepVfx = texturePath.find("run_vfx.png") != std::string::npos;
		// Footstep VFX intentionally render below the regular layer ordering.
		g->SetRenderLayer(isFootstepVfx ? 0 : GetLayerSortKeyCached(layerName));
		out.push_back(g);
	}

	// Sort by layer, explicit sort order, then y-position for simple painter's ordering.
	std::sort(out.begin(), out.end(), [&](GameObject* a, GameObject* b) {
		int la = a->GetRenderLayer();
		int lb = b->GetRenderLayer();
		if (la != lb) {
			return la < lb;
		}

		int sa = a->GetRenderSortOrder();
		int sb = b->GetRenderSortOrder();
		if (sa != sb) {
			return sa < sb;
		}

		return a->GetPosition().y < b->GetPosition().y;
		});
}

/**
 * @brief Sets the main scene background texture.
 * @param texturePath Texture path used for the background image.
 */
void Scene::SetSceneBackground(const std::string& texturePath) {
	// Cache the chosen path so reloads and editor views can recover the authored background.
	sceneBackgroundPath_ = texturePath;
	graphicsEngine.SetBackground(texturePath);
}

/**
 * @brief Sets the optional scene background overlay texture.
 * @param texturePath Texture path used for the overlay image.
 */
void Scene::SetSceneBackgroundOverlay(const std::string& texturePath) {
	// Track the overlay path separately from the base background for scene serialization.
	sceneBackgroundOverlayPath_ = texturePath;
	graphicsEngine.SetBackgroundOverlay(texturePath);
}

/**
 * @brief Clears the active scene background overlay.
 */
void Scene::ClearSceneBackgroundOverlay() {
	// Remove the cached path and the renderer's active overlay in tandem.
	sceneBackgroundOverlayPath_.clear();
	graphicsEngine.ClearBackgroundOverlay();
}

/**
 * @brief Applies authored transform data from a level record to an existing object.
 * @param id Id of the object to update.
 * @param pos New world position.
 * @param scale New scale.
 * @param rotationDeg Rotation in degrees.
 */
void Scene::SetTransformFromLevel(int id, const glm::vec3& pos, const glm::vec3& scale, float rotationDeg) {
	// Convert authoring-friendly degrees to the runtime radians used by the entity transform.
	const float rotationRad = rotationDeg * 3.14159265358979323846f / 180.0f;

	entityManager.SetPosition(id, pos);
	entityManager.SetScale(id, scale);
	entityManager.SetRotation(id, rotationRad);

	GameObject* obj = GetGameObjectByID(id);
	if (obj) {
		// Keep the GameObject facade synchronized with the entity manager's transform state.
		obj->SetPosition(pos);
		obj->SetScale(scale);
		obj->SetRotation(rotationRad, glm::vec3(0.0f, 0.0f, 1.0f));
	}

	collisionManager.MarkStaticStateDirty();
}

/**
 * @brief Returns whether an object currently has runtime animation data.
 * @param id Id of the object to query.
 * @return `true` if the object has an animator.
 */
bool Scene::HasAnimations(int id) const {
	return animationManager.HasAnimator(id);
}

/**
 * @brief Returns the registered animation names for an object.
 * @param id Id of the object to query.
 * @return List of animation clip names.
 */
std::vector<std::string> Scene::GetAnimationList(int id) const {
	return animationManager.GetAnimationNames(id);
}

/**
 * @brief Returns the current animation clip name for an object.
 * @param id Id of the object to query.
 * @return Current animation clip name.
 */
std::string Scene::GetCurrentAnimationName(int id) const {
	return animationManager.GetCurrentAnimation(id);
}

/**
 * @brief Switches an object to a named animation clip.
 * @param objID Id of the animated object.
 * @param animName Name of the animation clip to play.
 */
void Scene::SetAnimation(int objID, const std::string& animName) {
	// Forward the animation change to the centralized animation manager.
	animationManager.SetAnimation(objID, animName);
}

/**
 * @brief Attaches the player animation set to an object.
 * @param objID Id of the player object.
 */
void Scene::AttachPlayerAnimations(int objID) {
	animationManager.AttachPlayerAnimations(objID);
}

/**
 * @brief Attaches the dinosaur animation set to an object.
 * @param objID Id of the dinosaur object.
 */
void Scene::AttachDinoAnimations(int objID) {
	animationManager.AttachDinoAnimations(objID);
}

/**
 * @brief Attaches customer animations using the object's registered texture path.
 * @param objID Id of the customer object.
 */
void Scene::AttachCustomersAnimations(int objID) {
	// Resolve the object's current texture first so the animation manager can choose the right set.
	animationManager.AttachCustomersAnimations(objID, GetObjectTexturePath(objID));
}

/**
 * @brief Attaches customer animations using an explicit texture path.
 * @param objID Id of the customer object.
 * @param texturePath Texture path used to select the animation set.
 */
void Scene::AttachCustomersAnimations(int objID, const std::string& texturePath) {
	animationManager.AttachCustomersAnimations(objID, texturePath);
}

/**
 * @brief Attaches cut-station work VFX animations to an object.
 * @param objID Id of the effect object.
 */
void Scene::AttachWorkVfxCutAnimations(int objID) {
	animationManager.AttachWorkVfxCutAnimations(objID);
}

/**
 * @brief Attaches grill-station work VFX animations to an object.
 * @param objID Id of the effect object.
 */
void Scene::AttachWorkVfxGrillAnimations(int objID) {
	animationManager.AttachWorkVfxGrillAnimations(objID);
}

/**
 * @brief Attaches stove-station work VFX animations to an object.
 * @param objID Id of the effect object.
 */
void Scene::AttachWorkVfxStoveAnimations(int objID) {
	animationManager.AttachWorkVfxStoveAnimations(objID);
}

/**
 * @brief Attaches menu-specific animations to an object.
 * @param objID Id of the menu object.
 */
void Scene::AttachMenuAnimations(int objID) {
	animationManager.AttachMenuAnimations(objID);
}

/**
 * @brief Marks an object as animated or resets its UVs when animation is disabled.
 * @param id Id of the object to update.
 * @param state Whether the object should remain treated as animated.
 */
void Scene::MarkAnimated(int id, bool state) {
	if (!state) {
		if (GameObject* obj = GetGameObjectByID(id)) {
			// Restore the default full-texture UV when animation no longer drives the sprite.
			obj->SetUVRect({ 0.f, 0.f, 1.f, 1.f });
		}
	}
}

/**
 * @brief Rebinds gameplay logic for a tag-driven object.
 * @param id Id of the object whose logic should be rebound.
 * @param tag Tag that determines the logic binding.
 */
void Scene::AttachLogicForTag(int id, const std::string& tag) {
	// Clear old scripts first so the object never accumulates duplicated logic components.
	logicManager.RemoveAllFor(id, *this);
	if (tagLogicBinder_) {
		tagLogicBinder_(*this, id, tag);
	}
}

/**
 * @brief Stores the gameplay tag associated with an object.
 * @param id Id of the object to update.
 * @param tag Tag value to store.
 */
void Scene::SetObjectTag(int id, const std::string& tag) {
	objectMetadata_.SetTag(id, tag);
}

/**
 * @brief Returns the gameplay tag associated with an object.
 * @param id Id of the object to query.
 * @return Stored gameplay tag string.
 */
std::string Scene::GetObjectTag(int id) const {
	return objectMetadata_.GetTag(id);
}

/**
 * @brief Returns whether a tag is configured to use velocity-based movement rules.
 * @param tag Tag value to query.
 * @return `true` when the registered hook marks the tag as velocity-driven.
 */
bool Scene::TagUsesVelocity(const std::string& tag) const {
	if (tagUsesVelocityHook_) {
		return tagUsesVelocityHook_(tag);
	}
	return false;
}

/**
 * @brief Applies any external tag rules to an object.
 * @param id Id of the object being updated.
 * @param tag Tag whose rules should run.
 * @param speedX Horizontal speed parameter.
 * @param speedY Vertical speed parameter.
 */
void Scene::ApplyTagRules(int id, const std::string& tag, float speedX, float speedY) {
	if (tagRuleHook_) {
		tagRuleHook_(*this, id, tag, speedX, speedY);
	}
}

/**
 * @brief Returns the movement manager associated with this Scene.
 * @return Mutable movement manager reference.
 */
MovementManager& Scene::GetMovementManager() {
	// Expose the shared movement manager so helper code can inspect or mutate movement state.
	return movementManager;
}

/**
 * @brief Returns the movement manager associated with this Scene.
 * @return Immutable movement manager reference.
 */
const MovementManager& Scene::GetMovementManager() const {
	return movementManager;
}

/**
 * @brief Returns the collision manager associated with this Scene.
 * @return Mutable collision manager reference.
 */
CollisionManager& Scene::GetCollisionManager() {
	return collisionManager;
}

/**
 * @brief Returns the collision manager associated with this Scene.
 * @return Immutable collision manager reference.
 */
const CollisionManager& Scene::GetCollisionManager() const {
	return collisionManager;
}

/**
 * @brief Returns the underlying collision world used by the collision manager.
 * @return Mutable collision world reference.
 */
collision::World& Scene::GetCollisionWorld() {
	return collisionManager.GetCollisionWorld();
}

/**
 * @brief Returns the underlying collision world used by the collision manager.
 * @return Immutable collision world reference.
 */
const collision::World& Scene::GetCollisionWorld() const {
	return collisionManager.GetCollisionWorld();
}

/**
 * @brief Returns a cached numeric sort key for a layer name.
 * @param layerName Layer name to parse and cache.
 * @return Numeric sort key used for render ordering.
 */
int Scene::GetLayerSortKeyCached(const std::string& layerName) const {
	auto it = layerSortKeyCache_.find(layerName);
	if (it != layerSortKeyCache_.end()) {
		return it->second;
	}

	// Non-numeric layer names fall back to a very large sort key so explicit numeric layers win.
	int result = 1;
	if (!layerName.empty()) {
		result = 0;
		for (char c : layerName) {
			if (!std::isdigit(static_cast<unsigned char>(c))) {
				result = 1000000;
				break;
			}
			result = result * 10 + (c - '0');
		}
	}

	layerSortKeyCache_.emplace(layerName, result);
	return result;
}

/**
 * @brief Adds a scene layer if it does not already exist.
 * @param name Name of the layer to add.
 */
void Scene::AddLayer(const std::string& name) {
	// Ensure layer creation also invalidates any cached numeric sort key for the same name.
	layers.try_emplace(name, name);
	layerSortKeyCache_.erase(name);
	collisionManager.MarkStaticStateDirty();
}

/**
 * @brief Returns a mutable layer by name.
 * @param name Name of the layer to retrieve.
 * @return Pointer to the layer, or `nullptr` if not found.
 */
Layer* Scene::GetLayer(const std::string& name) {
	auto it = layers.find(name);
	return it != layers.end() ? &(it->second) : nullptr;
}

/**
 * @brief Returns all scene layers.
 * @return Map of layer names to layer objects.
 */
const std::unordered_map<std::string, Layer>& Scene::GetAllLayers() const {
	return layers;
}

/**
 * @brief Returns the assigned layer name for an object.
 * @param objectID Id of the object to query.
 * @return Layer name, or an empty string when none is assigned.
 */
std::string Scene::GetObjectLayer(int objectID) const {
	if (const Defaults* defaults = objectMetadata_.FindDefaults(objectID)) {
		return defaults->layer;
	}
	return "";
}

/**
 * @brief Returns the mutable layer assigned to an object.
 * @param objectID Id of the object to query.
 * @return Pointer to the assigned layer, or `nullptr` if none exists.
 */
Layer* Scene::GetObjectLayerPtr(int objectID) {
	const Defaults* defaults = objectMetadata_.FindDefaults(objectID);
	if (defaults == nullptr || defaults->layer.empty()) {
		return nullptr;
	}

	auto layerIt = layers.find(defaults->layer);
	return layerIt != layers.end() ? &(layerIt->second) : nullptr;
}

/**
 * @brief Returns the immutable layer assigned to an object.
 * @param objectID Id of the object to query.
 * @return Pointer to the assigned layer, or `nullptr` if none exists.
 */
const Layer* Scene::GetObjectLayerPtr(int objectID) const {
	const Defaults* defaults = objectMetadata_.FindDefaults(objectID);
	if (defaults == nullptr || defaults->layer.empty()) {
		return nullptr;
	}

	auto layerIt = layers.find(defaults->layer);
	return layerIt != layers.end() ? &(layerIt->second) : nullptr;
}

/**
 * @brief Returns whether a named layer is enabled.
 * @param layerName Layer name to query.
 * @return `true` when the layer is enabled or does not exist.
 */
bool Scene::IsLayerEnabled(const std::string& layerName) const {
	auto it = layers.find(layerName);
	if (it == layers.end()) {
		return true;
	}
	return it->second.IsEnabled();
}

/**
 * @brief Returns whether the layer containing an object is enabled.
 * @param objectID Id of the object to query.
 * @return `true` when the object has no layer or its layer is enabled.
 */
bool Scene::IsObjectLayerEnabled(int objectID) const {
	const std::string layerName = GetObjectLayer(objectID);
	if (layerName.empty()) {
		return true;
	}
	return IsLayerEnabled(layerName);
}

/**
 * @brief Assigns an object to a new layer and updates bookkeeping.
 * @param id Id of the object to move.
 * @param newLayer Destination layer name.
 */
void Scene::AssignObjectToLayer(int id, const std::string& newLayer) {
	// Default blank layer assignments to the base gameplay layer.
	std::string layerName = newLayer;
	if (layerName.empty()) {
		layerName = "1";
	}

	if (const Defaults* defaults = objectMetadata_.FindDefaults(id)) {
		const std::string& oldLayerName = defaults->layer;
		if (!oldLayerName.empty() && oldLayerName != layerName) {
			auto oldLayerIt = layers.find(oldLayerName);
			if (oldLayerIt != layers.end()) {
				oldLayerIt->second.RemoveObject(id);
			}
		}
	}

	Layer& layer = layers[layerName];
	if (layer.GetName().empty()) {
		// Name lazily-created layers the first time an object targets them.
		layer.SetName(layerName);
	}

	layer.AddObject(id);
	objectMetadata_.EnsureDefaults(id).layer = layerName;
	collisionManager.MarkStaticStateDirty();
}

/**
 * @brief Removes a named layer from the scene.
 * @param name Name of the layer to remove.
 */
void Scene::RemoveLayer(const std::string& name) {
	auto it = layers.find(name);
	if (it != layers.end()) {
		layers.erase(it);
	}

	layerSortKeyCache_.erase(name);
	collisionManager.MarkStaticStateDirty();
}

/**
 * @brief Requests a deferred game-state change.
 * @param newState State that should be applied after the current frame.
 */
void Scene::RequestStateChange(Framework::GameState newState) {
	// Store the request and let the application consume it at a safe handoff point.
	pendingState_ = newState;
	hasPendingStateChange_ = true;
	SetFlowState(FlowState::Transitioning);
}

/**
 * @brief Requests a transition back to the main menu state.
 */
void Scene::RequestMainMenuStateChange() {
	RequestStateChange(Framework::GameState::MainMenu);
}

/**
 * @brief Requests a transition to the tutorial state.
 */
void Scene::RequestTutorialStateChange() {
	RequestStateChange(Framework::GameState::Tutorial);
}

/**
 * @brief Requests a transition to the first kitchen gameplay state.
 */
void Scene::RequestKitchen01StateChange() {
	RequestStateChange(Framework::GameState::Kitchen01);
}

/**
 * @brief Consumes any pending deferred game-state change.
 * @return Pending state value when one exists, otherwise `std::nullopt`.
 */
std::optional<Framework::GameState> Scene::ConsumePendingStateChange() {
	if (!hasPendingStateChange_) {
		return std::nullopt;
	}

	// Clear the pending flag so each request is consumed at most once.
	hasPendingStateChange_ = false;
	return pendingState_;
}

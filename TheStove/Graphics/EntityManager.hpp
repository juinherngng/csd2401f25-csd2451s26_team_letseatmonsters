/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			EntityManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		This file declares the EntityManager class, a core engine system responsible for
					creating, storing, and managing all GameObjects in a level or scene.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include "GameObject.hpp"
#include "ResourceManager.hpp"

#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class EntityManager {
public:
	// Default constructor
	EntityManager() = default;

	// Default destructor
	~EntityManager() = default;

	EntityManager(const EntityManager&) = delete;
	EntityManager& operator=(const EntityManager&) = delete;

	EntityManager(EntityManager&&) noexcept = default;
	EntityManager& operator=(EntityManager&&) noexcept = default;

	// Spawns a static sprite with a given texture, position, and size.
	GameObject* SpawnStaticSprite(const std::string& texturePath,
		const glm::vec3& pos,
		const glm::vec2& size);

	// Spawns an animated sprite with a given texture, size, and animation frames.
	GameObject* SpawnAnimatedSprite(const std::string& texturePath,
		const glm::vec3& pos,
		const glm::vec2& size,
		const std::vector<glm::vec4>& frames,
		float frameDuration,
		bool loop);

	// Spawns a static sprite at the same position as ownerID, with given texture, size, and layer.
	GameObject* GetByID(int id);
	const GameObject* GetByID(int id) const;

	// Returns a vector of raw pointers to all alive GameObjects.
	std::vector<GameObject*> GetAllObjects();

	// Returns a const reference to the internal vector of unique_ptrs.
	const std::vector<std::unique_ptr<GameObject>>& GetObjectStorage() const {
		return sceneObjects_;
	}

	// Returns the number of currently alive GameObjects.
	size_t GetObjectCount() const {
		return sceneObjects_.size();
	}

	// Despawn the object with the given ID, if it exists. Also invokes any registered despawn callbacks with the ID.
	void DespawnByID(int id);

	// Collect raw pointers to all alive GameObjects into the provided vector.
	void Clear();

	// Register a callback to be invoked when an object is despawned.
	void RegisterDespawnCallback(const std::function<void(int)>& cb);

	// Set the position, scale, and rotation (in radians) of an object by its ID.
	void SetPosition(int id, const glm::vec3& pos);
	void SetScale(int id, const glm::vec3& scale);
	void SetRotation(int id, float rot);

	// Get the position, scale, and rotation of an object by its ID.
	glm::vec3 GetPosition(int id) const;
	glm::vec3 GetScale(int id) const;
	float GetRotation(int id) const;

	// Texture path metadata management for objects.
	void SetTexturePath(int id, const std::string& path) {
		texturePathByID_[id] = path;
	}
	const std::string& GetTexturePath(int id) const;

private:
	// Internal storage of GameObjects using unique_ptr for automatic memory management.
	std::vector<std::unique_ptr<GameObject>> sceneObjects_;
	std::vector<int> freeIDs_;
	int nextID_ = 0;

	// Callbacks invoked when an entity is despawned. Signature: void(int id)
	std::vector<std::function<void(int)>> despawnCbs_;

	// Metadata mapping from object ID to texture path (used for level editor and JSON serialization)
	std::unordered_map<int, std::string> texturePathByID_;

	// Cached transform data for quick access by ID (optional optimization)
	int AcquireID();

	// Returns an ID to the free pool for reuse. Does not check if the ID is valid or currently in use, so caller must ensure correctness.
	void ReleaseID(int id);
};

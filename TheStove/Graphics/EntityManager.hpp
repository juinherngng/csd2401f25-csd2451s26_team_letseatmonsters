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
	/** @brief Construct an empty entity manager. */
	EntityManager() = default;

	/** @brief Destroy the entity manager and owned entities. */
	~EntityManager() = default;

	EntityManager(const EntityManager&) = delete;
	EntityManager& operator=(const EntityManager&) = delete;

	EntityManager(EntityManager&&) noexcept = default;
	EntityManager& operator=(EntityManager&&) noexcept = default;

	/**
	 * @brief Spawn a static sprite object.
	 * @param texturePath Source texture file path.
	 * @param pos World position for the sprite.
	 * @param size 2D sprite size.
	 * @return Pointer to the spawned GameObject.
	 */
	GameObject* SpawnStaticSprite(const std::string& texturePath,
		const glm::vec3& pos,
		const glm::vec2& size);

	/**
	 * @brief Spawn an animated sprite object.
	 * @param texturePath Source texture file path.
	 * @param pos World position for the sprite.
	 * @param size 2D sprite size.
	 * @param frames UV frame rectangles for animation.
	 * @param frameDuration Seconds each frame should stay visible.
	 * @param loop Whether animation repeats after the final frame.
	 * @return Pointer to the spawned GameObject.
	 */
	GameObject* SpawnAnimatedSprite(const std::string& texturePath,
		const glm::vec3& pos,
		const glm::vec2& size,
		const std::vector<glm::vec4>& frames,
		float frameDuration,
		bool loop);

	/** @brief Lookup a GameObject by its unique ID. */
	GameObject* GetByID(int id);

	/** @brief Return raw pointers to all currently alive objects. */
	std::vector<GameObject*> GetAllObjects();

	/** @brief Return the total number of alive objects. */
	size_t GetObjectCount() const {
		return sceneObjects_.size();
	}

	/** @brief Despawn a GameObject by ID and trigger despawn callbacks. */
	void DespawnByID(int id);

	/** @brief Despawn all objects and reset manager-owned bookkeeping. */
	void Clear();

	/** @brief Register a callback invoked whenever an entity is despawned. */
	void RegisterDespawnCallback(const std::function<void(int)>& cb);

	/** @brief Update cached position for an object ID. */
	void SetPosition(int id, const glm::vec3& pos) {
		spritePositions_[id] = pos;
	}

	/** @brief Update cached scale for an object ID. */
	void SetScale(int id, const glm::vec3& scale) {
		spriteScales_[id] = scale;
	}

	/** @brief Update cached rotation for an object ID. */
	void SetRotation(int id, float rot) {
		spriteRotations_[id] = rot;
	}

	/** @brief Read cached position for an object ID. */
	glm::vec3 GetPosition(int id) const;

	/** @brief Read cached scale for an object ID. */
	glm::vec3 GetScale(int id) const;

	/** @brief Read cached rotation for an object ID. */
	float GetRotation(int id) const;

	/** @brief Cache texture path used by an object ID. */
	void SetTexturePath(int id, const std::string& path) {
		texturePathByID_[id] = path;
	}

	/** @brief Read cached texture path for an object ID. */
	const std::string& GetTexturePath(int id) const;

private:
	std::vector<std::unique_ptr<GameObject>> sceneObjects_;
	std::vector<int> freeIDs_;
	int nextID_ = 0;

	// Callbacks invoked when an entity is despawned. Signature: void(int id)
	std::vector<std::function<void(int)>> despawnCbs_;

	// Transform maps 
	std::unordered_map<int, glm::vec3> spritePositions_;
	std::unordered_map<int, glm::vec3> spriteScales_;
	std::unordered_map<int, float> spriteRotations_;
	std::unordered_map<int, std::string> texturePathByID_;

	/** @brief Acquire a reusable ID, or allocate a new one if needed. */
	int AcquireID();

	/** @brief Return an ID to the free-list for future reuse. */
	void ReleaseID(int id);
};

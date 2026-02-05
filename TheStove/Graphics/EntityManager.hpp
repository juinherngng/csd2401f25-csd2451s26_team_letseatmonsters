/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			EntityManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		This file declares the EntityManager class, a core engine system responsible for
					creating, storing, and managing all GameObjects in a level or scene.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "GameObject.hpp"
#include "ResourceManager.hpp" 

class EntityManager {
public:
	EntityManager() = default;
	~EntityManager() = default;

	EntityManager(const EntityManager&) = delete;
	EntityManager& operator=(const EntityManager&) = delete;

	EntityManager(EntityManager&&) noexcept = default;
	EntityManager& operator=(EntityManager&&) noexcept = default;

	// Spawning
	GameObject* SpawnStaticSprite(const std::string& texturePath,
								  const glm::vec3& pos,
								  const glm::vec2& size);

	GameObject* SpawnAnimatedSprite(const std::string& texturePath,
									const glm::vec3& pos,
									const glm::vec2& size,
									const std::vector<glm::vec4>& frames,
									float frameDuration,
									bool loop);

	// Lookup GameObject pointer by its unique ID.
	GameObject* GetByID(int id);
	std::vector<GameObject*> GetAllObjects();
	size_t GetObjectCount() const {
		return sceneObjects_.size();
	}

	// Despawning
	void DespawnByID(int id);
	void Clear();

	// Transform tracking (mirrors your existing maps)
	void SetPosition(int id, const glm::vec3& pos) {
		spritePositions_[id] = pos;
	}
	void SetScale(int id, const glm::vec3& scale) {
		spriteScales_[id] = scale;
	}
	void SetRotation(int id, float rot) {
		spriteRotations_[id] = rot;
	}

	glm::vec3 GetPosition(int id) const;
	glm::vec3 GetScale(int id) const;
	float GetRotation(int id) const;

	// Texture path tracking
	void SetTexturePath(int id, const std::string& path) {
		texturePathByID_[id] = path;
	}
	const std::string& GetTexturePath(int id) const;

private:
	std::vector<std::unique_ptr<GameObject>> sceneObjects_;
	std::vector<int> freeIDs_;
	int nextID_ = 0;

	// Transform maps 
	std::unordered_map<int, glm::vec3> spritePositions_;
	std::unordered_map<int, glm::vec3> spriteScales_;
	std::unordered_map<int, float> spriteRotations_;
	std::unordered_map<int, std::string> texturePathByID_;

	int AcquireID();
	void ReleaseID(int id);
};

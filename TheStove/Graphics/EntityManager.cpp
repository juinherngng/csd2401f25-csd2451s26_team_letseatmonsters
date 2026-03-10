/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			EntityManager.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (100%)

 DESCRIPTION:		This file implements the EntityManager class's core logic for creation and
					lifecycle management of GameObjects.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "EntityManager.hpp"
#include "ResourceManager.hpp"

#include <algorithm>
#include <iostream>

namespace {
	using EntityStorage = std::vector<std::unique_ptr<GameObject>>;
	using EntityIter = EntityStorage::iterator;
	using EntityConstIter = EntityStorage::const_iterator;

	EntityIter FindEntityByIdInternal(EntityStorage& sceneObjects, int id) {
		return std::find_if(sceneObjects.begin(), sceneObjects.end(),
			[id](const std::unique_ptr<GameObject>& obj) {
				return obj && obj->GetID() == id;
			});
	}

	EntityConstIter FindEntityByIdInternal(const EntityStorage& sceneObjects, int id) {
		return std::find_if(sceneObjects.begin(), sceneObjects.end(),
			[id](const std::unique_ptr<GameObject>& obj) {
				return obj && obj->GetID() == id;
			});
	}

	void LogMissingEntity(const char* operation, int id) {
		std::cerr << "EntityManager::" << operation << " failed: no entity with ID " << id << '\n';
	}

	template <typename T>
	const T& ReadCachedValueOrDefault(const std::unordered_map<int, T>& cache,
		int id,
		const char* operation,
		const T& defaultValue) {
		auto it = cache.find(id);
		if (it != cache.end()) {
			return it->second;
		}

		LogMissingEntity(operation, id);
		return defaultValue;
	}
}

int EntityManager::AcquireID() {
	if (!freeIDs_.empty()) {
		// Always re-use the lowest free ID
		auto it = std::min_element(freeIDs_.begin(), freeIDs_.end());
		int id = *it;
		freeIDs_.erase(it);
		return id;
	}
	return nextID_++;
}

void EntityManager::ReleaseID(int id) {
	freeIDs_.push_back(id);
}

GameObject* EntityManager::SpawnStaticSprite(const std::string& texturePath,
	const glm::vec3& pos,
	const glm::vec2& size) {
	int id = AcquireID();

	Mesh* quadMesh = ResourceManager::Instance().GetMesh("sprite");
	Shader* spriteShader = ResourceManager::Instance().GetShader("staticsprite");

	std::string textureName = "staticsprite_" + texturePath;
	Texture* texture = ResourceManager::Instance().LoadTexture(textureName, texturePath);

	auto obj = std::make_unique<GameObject>(quadMesh, spriteShader);
	obj->SetID(id);

	// Configure transform
	obj->SetPosition(pos);
	obj->SetScale(glm::vec3(size.x, size.y, 1.0f));
	obj->SetRotation(0.0f, glm::vec3(0, 0, 1));

	// Set texture
	obj->SetTexture(texture);

	GameObject* ptr = obj.get();
	sceneObjects_.push_back(std::move(obj));

	// Store metadata
	texturePathByID_[id] = texturePath;

	return ptr;
}

GameObject* EntityManager::SpawnAnimatedSprite(const std::string& texturePath,
	const glm::vec3& pos,
	const glm::vec2& size,
	const std::vector<glm::vec4>& frames,
	float frameDuration,
	bool loop) {
	int id = AcquireID();

	Mesh* quadMesh = ResourceManager::Instance().GetMesh("sprite");
	Shader* spriteShader = ResourceManager::Instance().GetShader("animatedsprite");

	std::string textureName = "animatedsprite_" + texturePath;
	Texture* texture = ResourceManager::Instance().LoadTexture(textureName, texturePath);

	// Use GameObject(Mesh*, Shader*) constructor
	auto obj = std::make_unique<GameObject>(quadMesh, spriteShader);
	obj->SetID(id);

	// Configure transform
	obj->SetPosition(pos);
	obj->SetScale(glm::vec3(size.x, size.y, 1.0f));
	obj->SetRotation(0.0f, glm::vec3(0, 0, 1));

	// Set texture
	obj->SetTexture(texture);

	std::vector<glm::vec4> safeFrames = frames;
	if (safeFrames.empty()) {
		safeFrames.push_back(glm::vec4(0.f, 0.f, 1.f, 1.f)); // fallback
	}
	obj->SetUVRect(safeFrames.front());  // Set first frame immediately

	GameObject* ptr = obj.get();
	sceneObjects_.push_back(std::move(obj));

	// Store metadata
	texturePathByID_[id] = texturePath;

	// Note: Animation frames are handled by Scene/AnimationManager

	(void)frameDuration;
	(void)loop;

	return ptr;
}

GameObject* EntityManager::GetByID(int id) {
	auto it = std::find_if(sceneObjects_.begin(), sceneObjects_.end(),
		[id](const std::unique_ptr<GameObject>& obj) {
			return obj && obj->GetID() == id;
		});
	return (it != sceneObjects_.end()) ? it->get() : nullptr;
}

const GameObject* EntityManager::GetByID(int id) const {
	auto it = std::find_if(sceneObjects_.begin(), sceneObjects_.end(),
		[id](const std::unique_ptr<GameObject>& obj) {
			return obj && obj->GetID() == id;
		});
	return (it != sceneObjects_.end()) ? it->get() : nullptr;
}

std::vector<GameObject*> EntityManager::GetAllObjects() {
	std::vector<GameObject*> result;
	result.reserve(sceneObjects_.size());
	for (const auto& obj : sceneObjects_) {
		if (obj) result.push_back(obj.get());
	}
	return result;
}

void EntityManager::DespawnByID(int id) {
	auto it = std::find_if(sceneObjects_.begin(), sceneObjects_.end(),
		[id](const std::unique_ptr<GameObject>& obj) {
			return obj && obj->GetID() == id;
		});
	if (it != sceneObjects_.end()) {
		sceneObjects_.erase(it);
		texturePathByID_.erase(id);
		// Notify listeners
		for (const auto& cb : despawnCbs_) {
			if (cb) {
				cb(id);
			}
		}
		ReleaseID(id);
	}
}

void EntityManager::Clear() {
	// Notify callbacks for each existing ID before clearing
	for (const auto& obj : sceneObjects_) {
		if (obj) {
			int id = obj->GetID();
			for (const auto& cb : despawnCbs_) {
				if (cb) {
					cb(id);
				}
			}
		}
	}

	sceneObjects_.clear();
	texturePathByID_.clear();
	freeIDs_.clear();
	nextID_ = 0;
}

void EntityManager::RegisterDespawnCallback(const std::function<void(int)>& cb) {
	despawnCbs_.push_back(cb);
}

void EntityManager::SetPosition(int id, const glm::vec3& pos) {
	if (GameObject* obj = GetByID(id)) {
		obj->SetPosition(pos);
	}
}

void EntityManager::SetScale(int id, const glm::vec3& scale) {
	if (GameObject* obj = GetByID(id)) {
		obj->SetScale(scale);
	}
}

void EntityManager::SetRotation(int id, float rot) {
	if (GameObject* obj = GetByID(id)) {
		obj->SetRotation(rot, glm::vec3(0.0f, 0.0f, 1.0f));
	}
}

glm::vec3 EntityManager::GetPosition(int id) const {
	if (const GameObject* obj = GetByID(id)) {
		return obj->GetPositionGLM();
	}
	return glm::vec3(0.0f);
}

glm::vec3 EntityManager::GetScale(int id) const {
	if (const GameObject* obj = GetByID(id)) {
		return obj->GetScaleGLM();
	}
	return glm::vec3(1.0f);
}

float EntityManager::GetRotation(int id) const {
	if (const GameObject* obj = GetByID(id)) {
		return obj->GetRotationAngleZ();
	}
	return 0.0f;
}

const std::string& EntityManager::GetTexturePath(int id) const {
	auto it = texturePathByID_.find(id);
	static const std::string empty;
	return (it != texturePathByID_.end()) ? it->second : empty;
}

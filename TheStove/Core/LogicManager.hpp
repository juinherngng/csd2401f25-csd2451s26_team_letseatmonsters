/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			LogicManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (100%)

 DESCRIPTION:		Declares the LogicManager class responsible for managing and updating
					all GameObjectLogic instances within a scene, including initialization,
					update cycles, and cleanup.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/
#pragma once

#include "GameObjectLogic.hpp"

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

class Scene;
class InputManager;

class LogicManager {
public:
	/**
	 * @brief Attach a new logic component of type T to a GameObject.
	 * @tparam T Logic type derived from GameObjectLogic.
	 * @tparam Args Constructor argument pack for T.
	 * @param objectID Owning GameObject ID.
	 * @param args Forwarded constructor arguments.
	 * @return Raw pointer to the newly created logic component.
	 */
	template<typename T, typename... Args>
	T* AddLogic(int objectID, Args&&... args) {
		auto logic = std::make_unique<T>(objectID, std::forward<Args>(args)...);
		T* ptr = logic.get();
		logicMap[objectID].push_back(std::move(logic));
		return ptr;
	}

	/** @brief Call Awake on all logic components and reset start state. */
	void AwakeAll(Scene& scene) {
		for (auto& [id, list] : logicMap)
			for (auto& l : list)
				l->Awake(scene);
		started = false;
	}

	/** @brief Call Start on all logic components once after Awake. */
	void StartAll(Scene& scene) {
		if (started) return;
		for (auto& [id, list] : logicMap)
			for (auto& l : list)
				l->Start(scene);
		started = true;
	}

	/** @brief Call Update on every logic component each frame. */
	void UpdateAll(float dt, Scene& scene, InputManager& input) {
		for (auto& [id, list] : logicMap)
			for (auto& l : list)
				l->Update(dt, scene, input);
	}

	/**
	 * @brief Destroy and remove all logic components attached to one object.
	 * @param objectID Owner ID whose logic list should be removed.
	 */
	void RemoveAllFor(int objectID, Scene& scene) {
		auto it = logicMap.find(objectID);
		if (it == logicMap.end()) return;
		for (auto& l : it->second)
			l->OnDestroy(scene);
		logicMap.erase(it);
	}

	/** @brief Destroy and remove all logic components across the whole scene. */
	void Clear(Scene& scene) {
		for (auto& [id, list] : logicMap)
			for (auto& l : list)
				l->OnDestroy(scene);
		logicMap.clear();
		started = false;
	}

	/**
	   * @brief Get the first logic component of type T attached to an object.
	   * @tparam T Requested logic type.
	   * @param ownerID Owning GameObject ID.
	   * @return Pointer to the first matching component, or nullptr if missing.
	   */
	template<typename T>
	T* GetLogicForObject(int ownerID) {
		auto it = logicMap.find(ownerID);
		if (it == logicMap.end())
			return nullptr;

		auto& list = it->second;
		for (auto& logicPtr : list) {
			if (auto* casted = dynamic_cast<T*>(logicPtr.get()))
				return casted;
		}
		return nullptr;
	}

	/**
	 * @brief Const overload of GetLogicForObject.
	 * @tparam T Requested logic type.
	 * @param ownerID Owning GameObject ID.
	 * @return Const pointer to the first matching component, or nullptr.
	 */
	template<typename T>
	const T* GetLogicForObject(int ownerID) const {
		auto it = logicMap.find(ownerID);
		if (it == logicMap.end())
			return nullptr;

		const auto& list = it->second;
		for (const auto& logicPtr : list) {
			if (auto* casted = dynamic_cast<const T*>(logicPtr.get()))
				return casted;
		}
		return nullptr;
	}


private:
	std::unordered_map<int, std::vector<std::unique_ptr<GameObjectLogic>>> logicMap;
	bool started{ false };
};
/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			LogicManager.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu

 DESCRIPTION:		Declares the LogicManager responsible for registering GameObjectLogic scripts,
					starting them when scenes are loaded, and updating all active logic components
					each frame. Provides helper functions to add, remove, and look up scripts by
					owner GameObject ID.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "GameObjectLogic.hpp"

class Scene;
class InputManager;

class LogicManager {
public:
	template<typename T, typename... Args>
	T* AddLogic(int objectID, Args&&... args) {
		auto logic = std::make_unique<T>(objectID, std::forward<Args>(args)...);
		T* ptr = logic.get();
		logicMap[objectID].push_back(std::move(logic));
		return ptr;
	}

	void AwakeAll(Scene& scene) {
		for (auto& [id, list] : logicMap)
			for (auto& l : list)
				l->Awake(scene);
		started = false;
	}

	void StartAll(Scene& scene) {
		if (started) return;
		for (auto& [id, list] : logicMap)
			for (auto& l : list)
				l->Start(scene);
		started = true;
	}

	void UpdateAll(float dt, Scene& scene, InputManager& input) {
		for (auto& [id, list] : logicMap)
			for (auto& l : list)
				l->Update(dt, scene, input);
	}

	void RemoveAllFor(int objectID, Scene& scene) {
		auto it = logicMap.find(objectID);
		if (it == logicMap.end()) return;
		for (auto& l : it->second)
			l->OnDestroy(scene);
		logicMap.erase(it);
	}

	void Clear(Scene& scene) {
		for (auto& [id, list] : logicMap)
			for (auto& l : list)
				l->OnDestroy(scene);
		logicMap.clear();
		started = false;
	}

private:
	std::unordered_map<int, std::vector<std::unique_ptr<GameObjectLogic>>> logicMap;
	bool started{ false };
};
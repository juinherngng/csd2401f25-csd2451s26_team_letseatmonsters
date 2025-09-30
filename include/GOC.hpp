/*
----------------------------------------------------------------------------------------------------
FILE NAME:			GOC.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Vu Phan Hung, phanhung.vu@digipen.edu

DESCRIPTION:
	GOC (Game Object Composition) represents a single "entity" in the engine.
	Each GOC contains a map of components (inherited from GameComponent) keyed
	by std::type_index.

	Responsibilities:
	- Attach and manage components at runtime.
	- Initialize and update all attached components.
	- Provide typed access to components (via Get<T>).
	- Defer destruction requests to the Factory for safe cleanup.

	This forms the "GameObject" part of the GameObject-Component system.

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <optional>
#include <string>
#include "GameComponent.hpp"

//Game Object
class GOC
{
public:
	std::string name;

	~GOC();

	//this function get the component and return it if this GameObject is holding it
	//return null if not
	//usage: auto enemyTransform = enemy->Get<Transform>();
	template <typename T>
	std::optional<T*> Get() const
	{
		auto it = m_components.find(typeid(T));

		if (it == m_components.end())
			return std::nullopt;
		return static_cast<T*>(it->second);
	}

	template <typename T>
	bool Has() const
	{
		return m_components.find(typeid(T)) != m_components.end();
	}

	//Initializing all components in this GameObject
	void Initialize();

	//Destroy the object
	void Destroy();

	//This function attach the component to this GameObject
	void AddComponent(std::type_index id, GameComponent* c);

	//Add Component to GameObject with value
	//for eg: player->AddComponent<Transform>(0.0f, 0.0f, 0.0f);
	template <typename T, typename ... Args>
	T* AddComponent(Args&&... args);

	//Remove Component
	template <typename T>
	void RemoveComponent()
	{
		auto it = m_components.find(typeid(T));
		if (it != m_components.end())
		{
			delete it->second;
			m_components.erase(it);
		}
	}

	//Return GameObject unique ID
	unsigned int GetId()
	{
		return ObjectId;
	}

	const std::unordered_map<std::type_index, GameComponent*> GetComponentList() const
	{
		return m_components;
	}

	//object unique id
	unsigned int ObjectId = 0;

private:
	//In this GOC aka GameObject, we have a list of GameComponents and you cannot add the same GameComponents twice
	//Meaning a GameObject cannot have 2 Collider, 2 Transform, 2 Sprite
	//unorder map ensure that there can only be a single key 
	//while using a vector will need to scan if it has the same componet or not before adding
	//Std::type_index is just to look up what is the component
	//Honestly Im only using std::type_index cause I can just auto it lmao
	std::unordered_map<std::type_index, GameComponent*> m_components;

	//example:

	//auto* col = new Collider(/* ctor args */);
	//enemy->AddComponent(TypeId(typeid(Collider)), col);

	//in this example
	//std::type_index(typeid(Collider)) means the key is Collider
	//col is the actual component being stored

	//so you can later access it like this
	//auto col = enemy->Get<Collider>()

};

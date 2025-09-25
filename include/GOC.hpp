#pragma once
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <optional>

#include "GameComponent.hpp"

//Game Object
class GOC
{
public:

	//This function attach the component to this GameObject
	void AddComponent(std::type_index id, GameComponent* c)
	{
		c->SetOwner(this);
		m_components[id] = c;
	}

	//this function look up the component and return it if this GameObject is holding it
	//return null if not
	template <typename T>
	std::optional<T*> Get() const
	{
		auto it = m_components.find(std::type_index(typeid(T)));
		//return it == m_components.end() ? nullptr : static_cast<T*>(it->second);
		if (it == m_components.end())
			return std::nullopt;
		return static_cast<T*>(it->second);
	}

	//Initializing all components in this GameObject
	void Initialize()
	{
		for (auto& kv : m_components)
		{
			kv.second->Initialize();
		}
	}

	//dtor
	~GOC()
	{
		for (auto& kv : m_components)
		{
			delete kv.second;
		}
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

	//in the factory:
	//auto* col = new Collider(/* ctor args */);
	//enemy->AddComponent(TypeId(typeid(Collider)), col);

	//in this example
	//std::type_index(typeid(Collider)) means the key is Collider
	//col is the actual component being stored

	//so you can later access it like this
	//auto col = enemy->Get<Collider>()
};
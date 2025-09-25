#pragma once
#include <typeindex>
#include "GameComponent.hpp"

//Inherit this class to create Component for GameObject
class ComponentCreator
{
public:
	//the key to access the Component in GOC
	std::type_index Type;

	//ctor
	explicit ComponentCreator(std::type_index type) : Type(type) {}

	//dtor
	virtual ~ComponentCreator() = default;
	
	//Creating a new Component instance
	virtual GameComponent* Create() = 0;
};

//Templated Creator
template <typename T>
class TCreator : ComponentCreator
{
public:
	//default ctor
	TCreator()
	{
		Type = std::type_index(typeid(T));
	}

	//Creating a new Component instace
	GameComponent* Create() override
	{
		return new T();
	}
};

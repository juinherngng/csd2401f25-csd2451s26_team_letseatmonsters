#pragma once
#include <typeindex>
#include "GameComponent.hpp"

//Actually creating the Game Component
//This is use inside the factory
class ComponentCreator
{
public:
	//ctor
	explicit ComponentCreator(std::type_index _type) : type(_type) {}

	//the key to access the Component in GOC
	std::type_index type;

	//Creating a new Component instance
	virtual GameComponent* Create() = 0;

	//dtor
	virtual ~ComponentCreator() {};
};

//Templated Creator
template <typename T>
class TCreator : public ComponentCreator
{
public:
	//default ctor
	explicit TCreator(std::type_index id) : ComponentCreator(id) {}

	//Creating a new Component instace
	virtual GameComponent* Create() override
	{
		return new T();
	}
};

/*
----------------------------------------------------------------------------------------------------
FILE NAME:			ComponentCreator.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Vu Phan Hung, phanhung.vu@digipen.edu

DESCRIPTION:
	Factory helper classes for creating GameComponent instances.
	- ComponentCreator: abstract base with a Create() interface.
	- TCreator<T>: templated implementation that constructs components of type T.

	Used by the Factory to register available component types and create them
	dynamically at runtime based on string keys (e.g., "Transform", "RigidBody2D").

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

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

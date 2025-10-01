/*
----------------------------------------------------------------------------------------------------
FILE NAME:			Factory.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Vu Phan Hung, phanhung.vu@digipen.edu

DESCRIPTION:
	Central manager for creating, tracking, and destroying GOC instances.

	Responsibilities:
	- Register ComponentCreators for data-driven composition.
	- Build new GOCs via BuildAndSerialize (e.g., Player, Table).
	- Assign unique IDs to each GOC and maintain an ID-to-object map.
	- Safely schedule and process destruction of GOCs.
	- Update all active components each frame (calling Update on enabled ones).

	The Factory serves as the global composition root of the GOC system.

All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "Factory.hpp"
#include <stdexcept>
#include <string>
#include "Transform.hpp"
#include "RigidBody2D.hpp"

Factory* FACTORY = NULL;

Factory::Factory()
{
	if (FACTORY != NULL)
	{
		throw "Factory already created";
	}
	FACTORY = this;
	lastId = 0;
}
Factory::~Factory()
{
	for (auto c : creatorsMap)
	{
		delete c.second;
	}
}

GOC* Factory::Create()
{
	GOC* gameObject = CreateEmptyComposition();
	if (gameObject)
	{
		gameObject->Initialize();
	}
	return gameObject;
}

void Factory::AddDestroy(GOC* g)
{
	toDelete.insert(g);
}

void Factory::Update(float dt)
{
	for (auto* g : toDelete)
	{
		auto it = idMap.find(g->ObjectId);
		if (it != idMap.end())
		{
			delete g;
			idMap.erase(it);
		}
	}

	toDelete.clear();

	for (auto& kv : idMap)
	{
		GOC* g = kv.second;

		for (auto& list : g->GetComponentList())
		{
			GameComponent* c = list.second;
			if (c->IsEnabled())
			{
				if (!c->IsStarted())
				{
					c->SetStarted(true);
				}
				c->Update(dt);
			}
		}
	}
}

//Destroy all the GOCs in the world. Used for final shutdown.
void Factory::DestroyAllObjects()
{
	for (auto o : idMap)
	{
		delete o.second;
	}

	idMap.clear();
}

//Create and Id a GOC at runtime. Used to dynamically build GOC.
//After components have been added call GOC->Initialize().
GOC* Factory::CreateEmptyComposition()
{
	GOC* gameObject = new GOC();
	IdGameObject(gameObject);
	return gameObject;
}

//Build a composition and serialize from the data file but do not initialize the GOC.
//Used to create a composition and then adjust its data before initialization
//see GameObjectComposition::Initialize for details.
GOC* Factory::BuildAndSerialize(const std::string& filename)
{
	GOC* gameObject = new GOC();

	if (filename.empty())
	{
		throw std::runtime_error("building an empty game object");
	}

	if (filename == "Player")
	{
		gameObject->name = "Player";
	}

	if (filename == "Table")
	{
		gameObject->name = "Table";
	}

	//Find the component's creator
	auto transformIt = creatorsMap.find("Transform");
	if (transformIt == creatorsMap.end())
	{
		throw std::runtime_error("Could not find component creator with name Transform");
	}

	//ComponentCreator is an object that creates the component
	ComponentCreator* creator = transformIt->second;

	//Create the component by using the interface
	GameComponent* transformComponent = creator->Create();

	//Add the new component to the composition
	gameObject->AddComponent(creator->type, transformComponent);

	//for testing right now later this will be replace with serialization
	Transform* t = static_cast<Transform*>(transformComponent);
	if (filename == "Player")
	{
		t->SetPosition(Math::Vector2D(0.0f, 0.0f));
		t->SetRotation(0.0f);
		t->SetScale(Math::Vector2D(1.0f, 1.0f));
	}
	else if (filename == "Table")
	{
		t->SetPosition(Math::Vector2D(5.0f, 3.0f));
		t->SetRotation(0.0f);
		t->SetScale(Math::Vector2D(1.0f, 1.0f));
	}


	if (filename == "Player")
	{
		auto rigidBodyIt = creatorsMap.find("RigidBody2D");
		if (rigidBodyIt == creatorsMap.end())
			throw std::runtime_error("Could not find component creator: RigidBody2D");

		ComponentCreator* rigidBodyCreator = rigidBodyIt->second;
		GameComponent* rigidBodyComponent = rigidBodyCreator->Create();
		gameObject->AddComponent(rigidBodyCreator->type, rigidBodyComponent);

		// Setup Rigidbody defaults
		RigidBody2D* rb = static_cast<RigidBody2D*>(rigidBodyComponent);
		rb->SetVelocity(Math::Vector2D(1.0f, 0.0f)); // moving right
	}

	//Id and initialize the game object composition
	IdGameObject(gameObject);

	return gameObject;
}

//Id object and store it in the object map.
void Factory::IdGameObject(GOC* gameObject)
{
	//Just increment the last id used. Does not handle 
//overflow but it would take over 4 billion objects
//to break
	++lastId;
	gameObject->ObjectId = lastId;

	//Store the game object in the global object id map
	idMap[lastId] = gameObject;
}

//Add a component creator enabling data driven composition
void Factory::AddComponentCreator(const std::string& name, ComponentCreator* creator)
{
	std::cout << "Adding component " << name << std::endl;
	creatorsMap[name] = creator;
}

//Get the game object with given id. This function will return NULL if
//the object has been destroyed.
GOC* Factory::GetObjectWithId(unsigned int id)
{
	auto it = idMap.find(id);
	if (it != idMap.end())
	{
		return it->second;
	}
	return NULL;
}
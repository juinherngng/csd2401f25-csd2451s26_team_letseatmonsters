/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Factory.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (70%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (30%)

 DESCRIPTION:		Central manager for creating, tracking, and destroying GOC instances.

					Responsibilities:
					- Register ComponentCreators for data-driven composition.
					- Build new GOCs via BuildAndSerialize (e.g., Player, Table).
					- Assign unique IDs to each GOC and maintain an ID-to-object map.
					- Safely schedule and process destruction of GOCs.
					- Update all active components each frame (calling Update on enabled ones).

					The Factory serves as the global composition root of the GOC system.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <memory>
#include <set>
#include <string>
#include <unordered_map>

#include "EngineCore/ComponentCreator.hpp"
#include "EngineCore/GOC.hpp"

class Factory {
public:
	//ctor
	Factory();

	//dtor
	~Factory();

	//Create a Game Object
	GOC* Create();

	//Add the Game Object to the destroy list 
	void AddDestroy(GOC* g);

	//Update the factory, destroying dead objects.
	virtual void Update(float dt);

	//Name of the system is factory.
	virtual std::string GetName() {
		return "Factory";
	}

	//Destroy all the GOCs in the world. Used for final shutdown.
	void DestroyAllObjects();

	//Create and Id a GOC at runtime. Used to dynamically build GOC.
	//After components have been added call GOC->Initialize().
	GOC* CreateEmptyComposition();

	//Build a composition and serialize from the data file but do not initialize the GOC.
	//Used to create a composition and then adjust its data before initialization
	//see GameObjectComposition::Initialize for details.
	GOC* BuildAndSerialize(const std::string& filename);

	//Id object and store it in the object map.
	void IdGameObject(GOC* gameObject);

	//Add a component creator enabling data driven composition
	void AddComponentCreator(const std::string& name, ComponentCreator* creator);

	//Get the game object with given id. This function will return NULL if
	//the object has been destroyed.
	GOC* GetObjectWithId(unsigned int id);

private:
	unsigned int lastId = 0;
	std::unordered_map<std::string, ComponentCreator*> creatorsMap;
	std::unordered_map<unsigned int, GOC*> idMap;
	std::set<GOC*> toDelete;
};

extern Factory* FACTORY;

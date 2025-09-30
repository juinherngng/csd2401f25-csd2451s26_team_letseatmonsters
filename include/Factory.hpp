#pragma once

#include "ComponentCreator.hpp"
#include "GOC.hpp"
#include <string>
#include <unordered_map>
#include <set>

class Factory
{
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
	virtual std::string GetName() { return "Factory"; }

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
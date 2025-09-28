#include "Factory.hpp"
#include <stdexcept>
#include <string>
#include "Transform.hpp"

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

	if (filename == "Player" || "Table")
	{
		//Find the component's creator
		auto it = creatorsMap.find("Transform");
		if (it == creatorsMap.end())
		{
			throw std::runtime_error("Could not find component creator with name " + it->first);
		}

		//ComponentCreator is an object that creates the component
		ComponentCreator* creator = it->second;

		//Create the component by using the interface
		GameComponent* component = creator->Create();

		//Add the new component to the composition
		gameObject->AddComponent(creator->type, component);

		Transform* t = static_cast<Transform*>(component);
		if (filename == "Player")
		{
			t->position = Math::Vector2D(0, 0);
			t->rotation = 0.0f;
		}
		else if (filename == "Table")
		{
			t->position = Math::Vector2D(5, 3);
			t->rotation = 0.0f;
		}
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
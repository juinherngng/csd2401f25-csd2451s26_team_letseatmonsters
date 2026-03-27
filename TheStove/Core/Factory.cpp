/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Factory.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (100%)

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

#include <stdexcept>
#include <string>

#include "Factory.hpp"
#include "ISerializer.hpp"
#include "Logger.hpp"
#include "RigidBody2D.hpp"
#include "Transform.hpp"

Factory* FACTORY = NULL;

Factory::Factory() {
	if (FACTORY != NULL) {
		throw "Factory already created";
	}
	FACTORY = this;
	lastId = 0;
}
Factory::~Factory() {
	for (auto c : creatorsMap) {
		delete c.second;
	}
}

GOC* Factory::Create() {
	GOC* gameObject = CreateEmptyComposition();
	if (gameObject) {
		gameObject->Initialize();
	}
	return gameObject;
}

void Factory::AddDestroy(GOC* g) {
	toDelete.insert(g);
}

void Factory::Update(float dt) {
	for (auto* g : toDelete) {
		auto it = idMap.find(g->ObjectId);
		if (it != idMap.end()) {
			delete g;
			idMap.erase(it);
		}
	}

	toDelete.clear();

	for (auto& kv : idMap) {
		GOC* g = kv.second;

		for (auto& list : g->GetComponentList()) {
			GameComponent* c = list.second;
			if (c->IsEnabled()) {
				if (!c->IsStarted()) {
					c->SetStarted(true);
				}
				c->Update(dt);
			}
		}
	}
}

//Destroy all the GOCs in the world. Used for final shutdown.
void Factory::DestroyAllObjects() {
	for (auto o : idMap) {
		delete o.second;
	}

	idMap.clear();
}

//Create and Id a GOC at runtime. Used to dynamically build GOC.
//After components have been added call GOC->Initialize().
GOC* Factory::CreateEmptyComposition() {
	GOC* gameObject = new GOC();
	IdGameObject(gameObject);
	return gameObject;
}

//Build a composition and serialize from the data file but do not initialize the GOC.
//Used to create a composition and then adjust its data before initialization
//see GameObjectComposition::Initialize for details.
GOC* Factory::BuildAndSerialize(const std::string& filename) {
	GOC* gameObject = new GOC();
	gameObject->name = filename;

	ISerializer serializer;

	if (!serializer.Load("../assets/data/" + filename)) {
		throw std::runtime_error("Failed to load file: " + filename);
	}

	for (const auto& compData : serializer.GetComponents()) {
		auto it = creatorsMap.find(compData.type);
		if (it == creatorsMap.end()) {
			throw std::runtime_error("No ComponentCreator registered for " + compData.type);
		}

		ComponentCreator* creator = it->second;
		GameComponent* component = creator->Create();
		gameObject->AddComponent(creator->type, component);

		// Deserialize Transform
		if (compData.type == "Transform") {
			Transform* t = static_cast<Transform*>(component);
			float posX = std::stof(compData.properties.at("posX"));
			float posY = std::stof(compData.properties.at("posY"));
			float rot = std::stof(compData.properties.at("rot"));
			float scaleX = std::stof(compData.properties.at("scaleX"));
			float scaleY = std::stof(compData.properties.at("scaleY"));
			t->SetPosition({ posX, posY });
			t->SetRotation(rot);
			t->SetScale({ scaleX, scaleY });
		}

		// Deserialize RigidBody2D
		if (compData.type == "RigidBody2D") {
			RigidBody2D* rb = static_cast<RigidBody2D*>(component);
			float velX = std::stof(compData.properties.at("velX"));
			float velY = std::stof(compData.properties.at("velY"));
			float accX = std::stof(compData.properties.at("accX"));
			float accY = std::stof(compData.properties.at("accY"));
			bool grav = std::stoi(compData.properties.at("useGravity")) != 0;
			rb->SetVelocity({ velX, velY });
			rb->SetAcceleration({ accX, accY });
			rb->SetUseGravity(grav);
		}
	}

	//Id and initialize the game object composition
	IdGameObject(gameObject);

	return gameObject;
}

//Id object and store it in the object map.
void Factory::IdGameObject(GOC* gameObject) {
	//Just increment the last id used. Does not handle 
//overflow but it would take over 4 billion objects
//to break
	++lastId;
	gameObject->ObjectId = lastId;

	//Store the game object in the global object id map
	idMap[lastId] = gameObject;
}

//Add a component creator enabling data driven composition
void Factory::AddComponentCreator(const std::string& name, ComponentCreator* creator) {
	TS_LOG_INFO("[Factory] Adding component creator: " << name);
	creatorsMap[name] = creator;
}

//Get the game object with given id. This function will return NULL if
//the object has been destroyed.
GOC* Factory::GetObjectWithId(unsigned int id) {
	auto it = idMap.find(id);
	if (it != idMap.end()) {
		return it->second;
	}
	return NULL;
}

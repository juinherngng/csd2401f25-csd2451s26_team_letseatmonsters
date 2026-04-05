/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Factory.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (60%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (40%)

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

#include <memory>
#include <stdexcept>
#include <string>

#include "EngineCore/Factory.hpp"
#include "EngineCore/ISerializer.hpp"
#include "EngineCore/Logger.hpp"
#include "EngineCore/RigidBody2D.hpp"
#include "EngineCore/Transform.hpp"

Factory* FACTORY = nullptr;

/**
 * @brief Constructs the singleton-style factory and initializes ID tracking.
 */
Factory::Factory() {
	if (FACTORY != nullptr) {
		throw std::runtime_error("Factory already created");
	}
	// Expose this instance through the legacy global pointer expected by the engine.
	FACTORY = this;
	lastId = 0;
}

/**
 * @brief Destroys all owned objects and registered component creators.
 */
Factory::~Factory() {
	// Tear down live compositions before destroying the creator registry itself.
	DestroyAllObjects();

	for (auto c : creatorsMap) {
		delete c.second;
	}
	creatorsMap.clear();
	FACTORY = nullptr;
}

/**
 * @brief Creates and initializes an empty game-object composition.
 * @return Pointer to the newly created game object.
 */
GOC* Factory::Create() {
	GOC* gameObject = CreateEmptyComposition();
	if (gameObject) {
		// Fully initialize the composition once all default components are attached.
		gameObject->Initialize();
	}
	return gameObject;
}

/**
 * @brief Schedules a game object for deferred destruction.
 * @param g Game object to destroy later.
 */
void Factory::AddDestroy(GOC* g) {
	if (g == nullptr) {
		return;
	}

	// Use a set so repeated destroy requests for the same object collapse into one entry.
	toDelete.insert(g);
}

/**
 * @brief Processes deferred destruction and updates all active components.
 * @param dt Frame delta time in seconds.
 */
void Factory::Update(float dt) {
	// Destroy objects requested last frame before updating the remaining world state.
	for (auto* g : toDelete) {
		auto it = idMap.find(g->ObjectId);
		if (it != idMap.end()) {
			delete g;
			idMap.erase(it);
		}
	}

	toDelete.clear();

	// Update every enabled component on every remaining game object.
	for (auto& kv : idMap) {
		GOC* g = kv.second;

		for (auto& list : g->GetComponentList()) {
			GameComponent* c = list.second.get();
			if (c->IsEnabled()) {
				if (!c->IsStarted()) {
					// Fire Start() lazily the first time the component participates in Update().
					c->SetStarted(true);
				}
				c->Update(dt);
			}
		}
	}
}

/**
 * @brief Destroys every game object currently owned by the factory.
 */
void Factory::DestroyAllObjects() {
	for (auto o : idMap) {
		delete o.second;
	}

	idMap.clear();
}

/**
 * @brief Creates an uninitialized game-object composition and assigns it an ID.
 * @return Pointer to the newly created composition.
 */
GOC* Factory::CreateEmptyComposition() {
	auto gameObject = std::make_unique<GOC>();
	// Register the object before returning it so other systems can reference it immediately.
	IdGameObject(gameObject.get());
	return gameObject.release();
}

/**
 * @brief Builds a game-object composition from serialized component data.
 * @param filename Data file describing the composition to build.
 * @return Pointer to the newly built composition.
 */
GOC* Factory::BuildAndSerialize(const std::string& filename) {
	auto gameObject = std::make_unique<GOC>();
	gameObject->name = filename;

	ISerializer serializer;

	if (!serializer.Load("../assets/data/" + filename)) {
		throw std::runtime_error("Failed to load file: " + filename);
	}

	// Recreate each authored component using its registered factory creator.
	for (const auto& compData : serializer.GetComponents()) {
		auto it = creatorsMap.find(compData.type);
		if (it == creatorsMap.end()) {
			throw std::runtime_error("No ComponentCreator registered for " + compData.type);
		}

		ComponentCreator* creator = it->second;
		GameComponent* component = gameObject->AddComponent(creator->type, creator->Create());

		// Apply serialized transform data directly onto the newly created Transform component.
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

		// Apply serialized rigid-body state onto the newly created physics component.
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

	// Assign a unique ID before handing the uninitialized composition back to the caller.
	IdGameObject(gameObject.get());
	return gameObject.release();
}

/**
 * @brief Assigns a unique ID to a game object and stores it in the lookup map.
 * @param gameObject Game object to register.
 */
void Factory::IdGameObject(GOC* gameObject) {
	// Increment the last issued ID; overflow is not handled because the practical limit is enormous.
	++lastId;
	gameObject->ObjectId = lastId;

	// Store the object in the global lookup table for fast ID-based retrieval.
	idMap[lastId] = gameObject;
}

/**
 * @brief Registers a component creator used during serialized composition building.
 * @param name Serialized component type name.
 * @param creator Creator capable of instantiating that component type.
 */
void Factory::AddComponentCreator(const std::string& name, ComponentCreator* creator) {
	TS_LOG_INFO("[Factory] Adding component creator: " << name);
	creatorsMap[name] = creator;
}

/**
 * @brief Looks up a game object by unique ID.
 * @param id Object ID to search for.
 * @return Pointer to the matching game object, or nullptr if absent.
 */
GOC* Factory::GetObjectWithId(unsigned int id) {
	auto it = idMap.find(id);
	if (it != idMap.end()) {
		// Return the live object pointer when the ID is still registered.
		return it->second;
	}
	return nullptr;
}

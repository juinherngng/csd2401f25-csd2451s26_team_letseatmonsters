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
	/**
	 * @brief Constructs the global factory instance.
	 */
	Factory();

	/**
	 * @brief Destroys the factory and releases all registered objects and creators.
	 */
	~Factory();

	/**
	 * @brief Creates and initializes an empty game-object composition.
	 * @return Pointer to the newly created game object.
	 */
	GOC* Create();

	/**
	 * @brief Schedules a game object for destruction on the next update.
	 * @param g Game object to destroy later.
	 */
	void AddDestroy(GOC* g);

	/**
	 * @brief Updates deferred destruction and active components.
	 * @param dt Frame delta time in seconds.
	 */
	virtual void Update(float dt);

	/**
	 * @brief Returns the debug-facing name of the factory.
	 * @return Factory system name string.
	 */
	virtual std::string GetName() {
		// Keep the name stable for logs and debug tooling.
		return "Factory";
	}

	/**
	 * @brief Destroys all game objects currently owned by the factory.
	 */
	void DestroyAllObjects();

	/**
	 * @brief Creates an uninitialized game-object composition and assigns it an ID.
	 * @return Pointer to the newly created composition.
	 */
	GOC* CreateEmptyComposition();

	/**
	 * @brief Builds a game-object composition from serialized component data.
	 * @param filename Data file describing the composition to build.
	 * @return Pointer to the newly built composition.
	 */
	GOC* BuildAndSerialize(const std::string& filename);

	/**
	 * @brief Assigns a unique ID to a game object and registers it in the lookup map.
	 * @param gameObject Game object to register.
	 */
	void IdGameObject(GOC* gameObject);

	/**
	 * @brief Registers a component creator for data-driven composition building.
	 * @param name Component type name used by serialized data.
	 * @param creator Creator object that can instantiate that component type.
	 */
	void AddComponentCreator(const std::string& name, ComponentCreator* creator);

	/**
	 * @brief Looks up a game object by its unique ID.
	 * @param id Object ID to search for.
	 * @return Pointer to the game object, or nullptr if not found.
	 */
	GOC* GetObjectWithId(unsigned int id);

private:
	unsigned int lastId = 0;
	std::unordered_map<std::string, ComponentCreator*> creatorsMap;
	std::unordered_map<unsigned int, GOC*> idMap;
	std::set<GOC*> toDelete;
};

extern Factory* FACTORY;

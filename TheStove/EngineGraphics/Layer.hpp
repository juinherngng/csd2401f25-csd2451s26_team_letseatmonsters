/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Layer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (40%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(60%)

 DESCRIPTION:		The Layer class maintains a collection of game object IDs that belong to this layer.
					Core functionality includes adding and removing object IDs to/from the layer, retrieving
					the current list of objects, and accessing the layer's name.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <algorithm>
#include <string>
#include <vector>

 // Forward declare GameObject to avoid circular dependency.
class Layer {
public:

	/**
	 * @brief Constructs a `Layer` instance.
	 */
	Layer() : name("") {
	}

	/**
	 * @brief Constructs a `Layer` instance.
	 * @param name Parameter for name.
	 */
	Layer(const std::string& name) : name(name) {
	}

	/**
	 * @brief Adds object.
	 * @param id Parameter for id.
	 */
	void AddObject(int id) {
		if (std::find(objectIDs.begin(), objectIDs.end(), id) == objectIDs.end()) {
			objectIDs.push_back(id);
		}
	}

	/**
	 * @brief Removes object.
	 * @param id Parameter for id.
	 */
	void RemoveObject(int id) {
		objectIDs.erase(
			std::remove(objectIDs.begin(), objectIDs.end(), id),
			objectIDs.end());
	}

	/**
	 * @brief Returns objects.
	 * @return Requested value.
	 */
	const std::vector<int>& GetObjects() const {
		return objectIDs;
	}

	/**
	 * @brief Returns the stable name for this object.
	 * @return Requested value.
	 */
	std::string GetName() const {
		return name;
	}

	/**
	 * @brief Sets name.
	 * @param newName Parameter for new name.
	 */
	void SetName(const std::string& newName) {
		name = newName;
	}

	/**
	 * @brief Returns whether visible.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsVisible() const {
		return visible;
	}

	/**
	 * @brief Sets visible.
	 * @param v Parameter for v.
	 */
	void SetVisible(bool v) {
		visible = v;
	}

	/**
	 * @brief Returns whether collidable.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsCollidable() const {
		return collidable;
	}

	/**
	 * @brief Sets collidable.
	 * @param c Parameter for c.
	 */
	void SetCollidable(bool c) {
		collidable = c;
	}

	/**
	 * @brief Returns whether enabled.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsEnabled() const {
		return enabled;
	}

	/**
	 * @brief Sets enabled.
	 * @param e Parameter for e.
	 */
	void SetEnabled(bool e) {
		enabled = e;
		// Optional: if a layer is disabled, it should not be visible/collidable either
		if (!enabled) {
			visible = false;
			collidable = false;
		}
	}

private:
	// The name of the layer, used for organization and referencing in the level editor and JSON.
	std::string name;
	std::vector<int> objectIDs;

	// Layer properties that affect rendering and collision logic. These can be set per layer to control visibility and collision behavior of all objects in the layer.
	bool visible = true;
	bool collidable = true;
	bool enabled = true;
};

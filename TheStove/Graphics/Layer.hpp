/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Layer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (40%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(60%)

 DESCRIPTION:		The Layer class maintains a collection of game object IDs that belong to this layer.
					Core functionality includes adding and removing object IDs to/from the layer, retrieving
					the current list of objects, and accessing the layer's name.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include <algorithm>
#include <string>
#include <vector>

class Layer {
public:
	/** @brief Construct an unnamed layer with default enabled flags. */
	Layer() : name("") {
	}

	/** @brief Construct a layer with a custom display name. */
	Layer(const std::string& name) : name(name) {
	}

	/** @brief Add an object ID if it is not already present. */
	void AddObject(int id) {
		if (std::find(objectIDs.begin(), objectIDs.end(), id) == objectIDs.end()) {
			objectIDs.push_back(id);
		}
	}

	/** @brief Remove all occurrences of an object ID from this layer. */
	void RemoveObject(int id) {
		objectIDs.erase(
			std::remove(objectIDs.begin(), objectIDs.end(), id),
			objectIDs.end());
	}

	/** @brief Get the ordered list of object IDs assigned to this layer. */
	const std::vector<int>& GetObjects() const {
		return objectIDs;
	}

	/** @brief Get the layer name. */
	std::string GetName() const {
		return name;
	}

	/** @brief Rename the layer. */
	void SetName(const std::string& newName) {
		name = newName;
	}

	/** @brief Check whether this layer should be rendered. */
	bool IsVisible() const {
		return visible;
	}

	/** @brief Set whether this layer should be rendered. */
	void SetVisible(bool v) {
		visible = v;
	}

	/** @brief Check whether this layer participates in collision logic. */
	bool IsCollidable() const {
		return collidable;
	}

	/** @brief Set whether this layer participates in collision logic. */
	void SetCollidable(bool c) {
		collidable = c;
	}

	/** @brief Check whether this layer is enabled for simulation. */
	bool IsEnabled() const {
		return enabled;
	}

	/** @brief Enable/disable this layer and sync visibility/collision when disabled. */
	void SetEnabled(bool e) {
		enabled = e;
		// Optional: if a layer is disabled, it should not be visible/collidable either
		if (!enabled) {
			visible = false;
			collidable = false;
		}
	}

private:
	std::string name;
	std::vector<int> objectIDs;

	bool visible = true;
	bool collidable = true;
	bool enabled = true;
};

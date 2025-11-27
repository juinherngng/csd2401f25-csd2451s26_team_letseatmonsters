/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			Layer.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu

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

	Layer() : name("") {
	}

	Layer(const std::string& name) : name(name) {
	}

	void AddObject(int id) {
		if (std::find(objectIDs.begin(), objectIDs.end(), id) == objectIDs.end()) {
			objectIDs.push_back(id);
		}
	}

	void RemoveObject(int id) {
		objectIDs.erase(
			std::remove(objectIDs.begin(), objectIDs.end(), id),
			objectIDs.end());
	}

	const std::vector<int>& GetObjects() const {
		return objectIDs;
	}

	std::string GetName() const {
		return name;
	}
	void SetName(const std::string& newName) {
		name = newName;
	}

	bool IsVisible() const {
		return visible;
	}
	void SetVisible(bool v) {
		visible = v;
	}

	bool IsCollidable() const {
		return collidable;
	}
	void SetCollidable(bool c) {
		collidable = c;
	}

private:
	std::string name;
	std::vector<int> objectIDs;

	bool visible = true;
	bool collidable = true;

};

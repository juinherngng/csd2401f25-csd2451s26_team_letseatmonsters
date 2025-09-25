#pragma once

#include "ComponentCreator.hpp"
#include "GOC.hpp"
#include <string>
#include <unordered_map>
#include <set>

class Factory
{
public:
	

private:
	std::unordered_map<std::string, ComponentCreator*> creators;
	std::unordered_map<unsigned int, GOC*> idMap;
	std::set<GOC*> toDelete;
	unsigned int lastId = 0;
}
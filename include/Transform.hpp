#include "Math.hpp"
#include "GameComponent.hpp"
#include "string"
#include <iostream>

class Transform : public GameComponent
{
public:
	Transform(float posX = 0.0f, float posY = 0.0f, float rot = 0.0f, float scaleX = 0.0f, float scaleY = 0.0f) 
		: position(posX, posY), rotation(rot), scale(scaleX, scaleY) {}
	Math::Vector2D position;
	float rotation;
	Math::Vector2D scale;
	void Initialize() override {
		// For now, just a debug message
		std::cout << "Transform initialized at ("
			<< position.x << ", " << position.y
			<< "), rotation " << rotation << "\n";
	}
	//void Serialization(ISerializer& std::string);
};
#include "GameComponent.hpp"
#include "Math.hpp"
#include "string"
#include <iostream>

class Transform : public GameComponent
{
public:
	Transform() : position(0, 0), rotation(0.0f), scale(1.0f, 1.0f) {};
	Transform(float posX, float posY) : position(posX, posY), rotation(0.0f), scale(1.0f, 1.0f) {};
	Transform(float posX, float posY, float rot) : position(posX, posY), rotation(rot), scale(1.0f, 1.0f) {};
	Transform(float posX, float posY, float rot, float scaleX, float scaleY) : position(posX, posY), rotation(rot), scale(scaleX, scaleY) {}

	// getters
	const Math::Vector2D& GetPosition() const { return position; }
	float GetRotation() const { return rotation; }
	const Math::Vector2D& GetScale() const { return scale; }

	// setters
	void SetPosition(const Math::Vector2D& pos) { position = pos; }
	void SetRotation(float rot) { rotation = rot; }
	void SetScale(const Math::Vector2D& s) { scale = s; }

	void Initialize() override;

	void OnEnable() override;
	void OnDisable() override;

	//Translate
	void Translate(float, float);
	void Translate(const Math::Vector2D&);

	//Rotate
	void Rotate(float);
	//Rotate towards the target
	void LookAt(const Math::Vector2D&);

	//Scale
	void Scale(float);
	void Scale(const Math::Vector2D&);

	//Direction the transform is facing
	Math::Vector2D Forward() const;
	//Perpendicular to Forward
	Math::Vector2D Right() const;

	//Distance between this transform and the other transform
	float Distance(const Transform&) const;
	float Distance(const Math::Vector2D&) const;

	//void Serialization(ISerializer& std::string);

	std::string ToString() const override;

	~Transform() override
	{
		std::cout << "Deleting Transform's component " << "\n";
	}

private:
	Math::Vector2D position;
	float rotation;
	Math::Vector2D scale;
};
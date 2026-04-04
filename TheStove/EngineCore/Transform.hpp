/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Transform.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (45%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (55%)

 DESCRIPTION:		Transform component storing spatial information for a GOC.
					Provides position, rotation, and scale in 2D space, along with getter/setter
					methods to modify them.

					Responsibilities:
					- Store translation, rotation, and scale values.
					- Provide utility for other components/systems to read or modify transforms.
					- Forms the backbone for rendering and physics.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <string>

#include "EngineCore/GameComponent.hpp"
#include "EngineCore/Math.hpp"

class Transform : public GameComponent {
public:
	Transform() : position(0, 0), rotation(0.0f), scale(1.0f, 1.0f) {};
	Transform(float posX, float posY) : position(posX, posY), rotation(0.0f), scale(1.0f, 1.0f) {};
	Transform(float posX, float posY, float rot) : position(posX, posY), rotation(rot), scale(1.0f, 1.0f) {};
	Transform(float posX, float posY, float rot, float scaleX, float scaleY) : position(posX, posY), rotation(rot), scale(scaleX, scaleY) {}

	// getters
	const Math::Vector2D& GetPosition() const {
		return position;
	}
	float GetRotation() const {
		return rotation;
	}
	const Math::Vector2D& GetScale() const {
		return scale;
	}

	// setters
	void SetPosition(const Math::Vector2D& pos) {
		position = pos;
	}
	void SetRotation(float rot) {
		rotation = rot;
	}
	void SetScale(const Math::Vector2D& s) {
		scale = s;
	}

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

	~Transform() override = default;

	GameComponent* Clone() const override;


private:
	Math::Vector2D position;
	float rotation;
	Math::Vector2D scale;
};

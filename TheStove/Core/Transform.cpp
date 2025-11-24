/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Transform.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu

 DESCRIPTION:		Transform component storing spatial information for a GOC.
					Provides position, rotation, and scale in 2D space, along with getter/setter
					methods to modify them.

					Responsibilities:
					- Store translation, rotation, and scale values.
					- Provide utility for other components/systems to read or modify transforms.
					- Forms the backbone for rendering and physics.

		All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "Transform.hpp"

void Transform::Initialize() {
	// For now, just a debug message
	std::cout << "Transform initialized at ("
		<< position.x << ", " << position.y
		<< "), rotation " << rotation << "\n";
}

void Transform::Translate(float x, float y) {
	position = Math::Vector2D(position.x + x, position.y + y);
}

void Transform::Translate(const Math::Vector2D& delta) {
	position = Math::Vector2D(position.x + delta.x, position.y + delta.y);
}

void Transform::Rotate(float degree) {
	rotation += degree;
}

void Transform::LookAt(const Math::Vector2D& target) {
	float dirX = target.x - position.x;
	float dirY = target.y - position.y;
	rotation = Math::ToDegrees(std::atan2(dirY, dirX));
}

void Transform::Scale(float factor) {
	scale = Math::Vector2D(scale.x * factor, scale.y * factor);
}

void Transform::Scale(const Math::Vector2D& factors) {
	scale = Math::Vector2D(scale.x * factors.x, scale.y * factors.y);
}

Math::Vector2D Transform::Forward() const {
	float rad = Math::ToRadians(rotation);
	return Math::Vector2D(std::cos(rad), std::sin(rad));
}

Math::Vector2D Transform::Right() const {
	float rad = Math::ToRadians(rotation);
	return Math::Vector2D(-std::sin(rad), std::cos(rad));
}

float Transform::Distance(const Transform& other) const {
	return position.Normalized().Dot(other.position);
}

float Transform::Distance(const Math::Vector2D& point) const {
	Math::Vector2D diff(point.x - position.x, point.y - position.y);
	return diff.Length();
}

std::string Transform::ToString() const {
	return "Transform: pos(" + std::to_string(position.x) + ", " + std::to_string(position.y) +
		"), rot(" + std::to_string(rotation) + ")";
}

void Transform::OnEnable() {
	{
		std::cout << "Transform enabled\n";
	}
}

void Transform::OnDisable() {
	{
		std::cout << "Transform disabled\n";
	}
}

GameComponent* Transform::Clone() const {
	return new Transform(*this);
}


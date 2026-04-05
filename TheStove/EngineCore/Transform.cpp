/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Transform.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Vu Phan Hung, phanhung.vu@digipen.edu (70%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	  (30%)

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

#include "EngineCore/Logger.hpp"
#include "EngineCore/Transform.hpp"

/**
 * @brief Performs component initialization work.
 */
void Transform::Initialize() {
	// Log the initial transform state so component setup is visible in debug traces.
	TS_LOG_DEBUG("Transform initialized at ("
		<< position.x << ", " << position.y
		<< "), rotation " << rotation);
}

/**
 * @brief Offsets the transform by explicit x and y values.
 * @param x Delta to add on the x axis.
 * @param y Delta to add on the y axis.
 */
void Transform::Translate(float x, float y) {
	// Apply the translation directly to the stored 2D position.
	position = Math::Vector2D(position.x + x, position.y + y);
}

/**
 * @brief Offsets the transform by a delta vector.
 * @param delta Delta vector to add to the current position.
 */
void Transform::Translate(const Math::Vector2D& delta) {
	// Reuse the vector overload when callers already have a packed delta.
	position = Math::Vector2D(position.x + delta.x, position.y + delta.y);
}

/**
 * @brief Rotates the transform by an incremental amount.
 * @param degree Rotation delta in degrees.
 */
void Transform::Rotate(float degree) {
	// Accumulate the new angular offset on top of the current orientation.
	rotation += degree;
}

/**
 * @brief Rotates the transform to face a target point.
 * @param target World-space target point to face.
 */
void Transform::LookAt(const Math::Vector2D& target) {
	// Build the direction vector from this transform toward the requested target.
	float dirX = target.x - position.x;
	float dirY = target.y - position.y;

	// Convert the direction angle into the engine's degree-based rotation convention.
	rotation = Math::ToDegrees(std::atan2(dirY, dirX));
}

/**
 * @brief Applies a uniform scale multiplier.
 * @param factor Uniform scale multiplier.
 */
void Transform::Scale(float factor) {
	// Multiply both axes by the same factor for uniform scaling.
	scale = Math::Vector2D(scale.x * factor, scale.y * factor);
}

/**
 * @brief Applies a non-uniform per-axis scale multiplier.
 * @param factors Scale factors to multiply into the current scale.
 */
void Transform::Scale(const Math::Vector2D& factors) {
	// Multiply each axis independently for non-uniform scaling.
	scale = Math::Vector2D(scale.x * factors.x, scale.y * factors.y);
}

/**
 * @brief Returns the normalized forward direction implied by the current rotation.
 * @return Forward direction vector.
 */
Math::Vector2D Transform::Forward() const {
	// Convert the stored degree rotation into radians before evaluating trig functions.
	float rad = Math::ToRadians(rotation);
	return Math::Vector2D(std::cos(rad), std::sin(rad));
}

/**
 * @brief Returns the right-facing direction perpendicular to `Forward()`.
 * @return Right direction vector.
 */
Math::Vector2D Transform::Right() const {
	// Rotate the forward basis by ninety degrees to derive the right-hand vector.
	float rad = Math::ToRadians(rotation);
	return Math::Vector2D(-std::sin(rad), std::cos(rad));
}

/**
 * @brief Computes the Euclidean distance to another transform.
 * @param other Other transform to compare against.
 * @return Distance from this transform's position to the other transform's position.
 */
float Transform::Distance(const Transform& other) const {
	// Measure the offset between both transform positions and return its magnitude.
	Math::Vector2D diff(other.position.x - position.x, other.position.y - position.y);
	return diff.Length();
}

/**
 * @brief Computes the Euclidean distance to a point.
 * @param point World-space point to compare against.
 * @return Distance from the current position to the point.
 */
float Transform::Distance(const Math::Vector2D& point) const {
	// Measure the offset from the stored position to the queried point.
	Math::Vector2D diff(point.x - position.x, point.y - position.y);
	return diff.Length();
}

/**
 * @brief Builds a human-readable summary of the transform state.
 * @return String containing position and rotation values.
 */
std::string Transform::ToString() const {
	// Format the current position and rotation for debug-oriented string output.
	return "Transform: pos(" + std::to_string(position.x) + ", " + std::to_string(position.y) +
		"), rot(" + std::to_string(rotation) + ")";
}

/**
 * @brief Handles component enable events.
 */
void Transform::OnEnable() {
	// Emit a trace so component activation can be followed in debug logs.
	TS_LOG_DEBUG("Transform enabled");
}

/**
 * @brief Handles component disable events.
 */
void Transform::OnDisable() {
	// Emit a trace so component deactivation can be followed in debug logs.
	TS_LOG_DEBUG("Transform disabled");
}

/**
 * @brief Creates a heap-allocated copy of this component.
 * @return Newly allocated clone of this transform.
 */
GameComponent* Transform::Clone() const {
	// Delegate to the compiler-generated copy semantics for the full component state.
	return new Transform(*this);
}

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
	/**
	 * @brief Initializes a transform at the origin with no rotation and unit scale.
	 */
	Transform() : position(0, 0), rotation(0.0f), scale(1.0f, 1.0f) {}

	/**
	 * @brief Initializes a transform with a position and default rotation and scale.
	 * @param posX Initial x position.
	 * @param posY Initial y position.
	 */
	Transform(float posX, float posY) : position(posX, posY), rotation(0.0f), scale(1.0f, 1.0f) {}

	/**
	 * @brief Initializes a transform with a position and rotation.
	 * @param posX Initial x position.
	 * @param posY Initial y position.
	 * @param rot Initial rotation in degrees.
	 */
	Transform(float posX, float posY, float rot) : position(posX, posY), rotation(rot), scale(1.0f, 1.0f) {}

	/**
	 * @brief Initializes a transform with full position, rotation, and scale data.
	 * @param posX Initial x position.
	 * @param posY Initial y position.
	 * @param rot Initial rotation in degrees.
	 * @param scaleX Initial x scale.
	 * @param scaleY Initial y scale.
	 */
	Transform(float posX, float posY, float rot, float scaleX, float scaleY) : position(posX, posY), rotation(rot), scale(scaleX, scaleY) {}

	/**
	 * @brief Returns the current 2D position.
	 * @return Reference to the stored position vector.
	 */
	const Math::Vector2D& GetPosition() const {
		// Expose the stored position without copying it.
		return position;
	}

	/**
	 * @brief Returns the current rotation in degrees.
	 * @return Stored rotation value.
	 */
	float GetRotation() const {
		// Expose the stored angular state directly.
		return rotation;
	}

	/**
	 * @brief Returns the current 2D scale.
	 * @return Reference to the stored scale vector.
	 */
	const Math::Vector2D& GetScale() const {
		// Expose the stored scale without copying it.
		return scale;
	}

	/**
	 * @brief Replaces the current position.
	 * @param pos New world position to store.
	 */
	void SetPosition(const Math::Vector2D& pos) {
		// Overwrite the transform's translation with the caller-provided value.
		position = pos;
	}

	/**
	 * @brief Replaces the current rotation.
	 * @param rot New rotation in degrees.
	 */
	void SetRotation(float rot) {
		// Overwrite the transform's orientation.
		rotation = rot;
	}

	/**
	 * @brief Replaces the current scale.
	 * @param s New per-axis scale.
	 */
	void SetScale(const Math::Vector2D& s) {
		// Overwrite the transform's scale with the supplied per-axis factors.
		scale = s;
	}

	/**
	 * @brief Performs component initialization work.
	 */
	void Initialize() override;

	/**
	 * @brief Handles component enable events.
	 */
	void OnEnable() override;

	/**
	 * @brief Handles component disable events.
	 */
	void OnDisable() override;

	/**
	 * @brief Offsets the transform by explicit x and y values.
	 * @param x Delta to add on the x axis.
	 * @param y Delta to add on the y axis.
	 */
	void Translate(float, float);

	/**
	 * @brief Offsets the transform by a delta vector.
	 * @param delta Delta vector to add to the current position.
	 */
	void Translate(const Math::Vector2D&);

	/**
	 * @brief Rotates the transform by an incremental amount.
	 * @param degree Rotation delta in degrees.
	 */
	void Rotate(float);

	/**
	 * @brief Rotates the transform to face a target point.
	 * @param target World-space target point to face.
	 */
	void LookAt(const Math::Vector2D&);

	/**
	 * @brief Applies a uniform scale multiplier.
	 * @param factor Uniform scale multiplier.
	 */
	void Scale(float);

	/**
	 * @brief Applies a non-uniform per-axis scale multiplier.
	 * @param factors Scale factors to multiply into the current scale.
	 */
	void Scale(const Math::Vector2D&);

	/**
	 * @brief Returns the normalized forward direction implied by the current rotation.
	 * @return Forward direction vector.
	 */
	Math::Vector2D Forward() const;

	/**
	 * @brief Returns the right-facing direction perpendicular to `Forward()`.
	 * @return Right direction vector.
	 */
	Math::Vector2D Right() const;

	/**
	 * @brief Computes the distance-like comparison against another transform.
	 * @param other Other transform to compare against.
	 * @return Result of the current transform-distance helper implementation.
	 */
	float Distance(const Transform&) const;

	/**
	 * @brief Computes the Euclidean distance to a point.
	 * @param point World-space point to compare against.
	 * @return Distance from the current position to the point.
	 */
	float Distance(const Math::Vector2D&) const;

	/**
	 * @brief Builds a human-readable summary of the transform state.
	 * @return String containing position and rotation values.
	 */
	std::string ToString() const override;

	/**
	 * @brief Destroys the transform component.
	 */
	~Transform() override = default;

	/**
	 * @brief Creates a heap-allocated copy of this component.
	 * @return Newly allocated clone of this transform.
	 */
	GameComponent* Clone() const override;

private:
	Math::Vector2D position;
	float rotation;
	Math::Vector2D scale;
};

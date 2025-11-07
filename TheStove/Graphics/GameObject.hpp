/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			GameObject.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Represents a renderable game object with mesh, shader, texture, transform, and collider.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "Mesh.hpp"
#include "Texture.hpp"
#include "Shader.hpp"
#include "DebugRenderer.hpp"
#include "../Core/Math.hpp"
#include <glm/glm.hpp>

 /**
  * @class GameObject
  * @brief Represents a renderable and collidable entity in the game world.
  *
  * A GameObject contains mesh, shader, texture, and transform data for rendering.
  * It also includes collider size/offset for physics and debugging.
  */
class GameObject {

public:
	/** @brief Construct an empty object (no mesh/shader). */
	GameObject()
		: m_Mesh(nullptr),
		m_Shader(nullptr),
		m_Position(0.0f),
		m_Scale(1.0f),
		m_Rotation(1.0f) {
		UpdateModelMatrix();
	}

	/**
	 * @brief Construct a GameObject with mesh and shader.
	 * @param mesh   Pointer to mesh resource.
	 * @param shader Pointer to shader resource.
	 */
	GameObject(Mesh* mesh, Shader* shader);

	/**
	 * @brief Construct a GameObject with a given unique ID.
	 * @param objectID Unique identifier.
	 */
	GameObject(int objectID);

	/** @brief Get the GameObject�s unique ID. */
	int GetID() const;

	/**
	 * @brief Assign a new unique ID.
	 * @param newID The ID to assign.
	 */
	void SetID(int newID);

	/**
	 * @brief Set the world position.
	 * @param position New position (XYZ).
	 */
	void SetPosition(const glm::vec3& position);
	glm::vec3 GetPositionGLM() const;

	void SetPosition(const Math::Vector3D& position);
	Math::Vector3D GetPosition() const;

	/**
	 * @brief Set the object scale.
	 * @param scale New scale (XYZ).
	 */
	void SetScale(const glm::vec3& scale);

	/**
	 * @brief Set the object rotation.
	 * @param angleRadians Rotation angle in radians.
	 * @param axis         Axis to rotate around.
	 */
	void SetRotation(float angleRadians, const glm::vec3& axis);

	void SetVelocity(const Math::Vector2D& velocity);
	Math::Vector2D GetVelocity() const;

	/** @brief Recompute the model matrix based on position/rotation/scale. */
	void UpdateModelMatrix();

	void SetTexture(Texture* tex) { m_Texture = tex; }

	Shader* GetShader() const { return m_Shader; }
	Mesh* GetMesh() const { return m_Mesh; }
	glm::mat4 GetModelMatrix() const { return m_ModelMatrix; }
	Texture* GetTexture() const { return m_Texture; }

	// Collider handling
	/** @brief Set the colliders full size (width/height). */
	void SetColliderSize(const Math::Vector2D& size);

	/** @brief Set the colliders positional offset relative to object center. */
	void SetColliderOffset(const Math::Vector2D& offset);

	/** @brief Get the collider size. */
	Math::Vector2D GetColliderSize() const;

	/** @brief Get the collider offset. */
	Math::Vector2D GetColliderOffset() const;

	glm::vec3 GetScaleGLM() const;
	float GetRotationAngleZ() const;
	float GetRotation() const { return rotation_; }

	/**
	 * @brief Draw the colliders bounding box for debugging.
	 * @param view  Camera view matrix.
	 * @param proj  Camera projection matrix.
	 * @param color Debug line color (default = red).
	 */
	void DrawBoundingBox(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& color) const;

	void SetUVRect(const glm::vec4& r) { m_uvRect = r; }
	glm::vec4 GetUVRect() const { return m_uvRect; }

private:
	Mesh* m_Mesh;
	Shader* m_Shader;
	Texture* m_Texture = nullptr;

	glm::vec3 m_Position;
	glm::vec3 m_Scale;
	glm::mat4 m_Rotation;

	glm::mat4 m_ModelMatrix;

	glm::vec4 m_uvRect{ 0.f, 0.f, 1.f, 1.f };

	int id; // Unique identifier for GameObjects
	float rotation_ = 0.0f;

	// Physics-friendly state
	Math::Vector2D m_Velocity{ 0.f, 0.f };
	Math::Vector2D m_ColliderSize{ 1.f, 1.f };
	Math::Vector2D m_ColliderOffset{ 0.f, 0.f };
};

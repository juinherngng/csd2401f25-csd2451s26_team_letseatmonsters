/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			GameObject.h
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
					Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Represents a renderable game object with mesh, shader, texture, transform, and collider.

		 All content � 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "Mesh.h"
#include "Texture.h"
#include "Shader.h"
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

	/** @brief Recompute the model matrix based on position/rotation/scale. */
	void UpdateModelMatrix();
	void Draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
	void SetTexture(Texture* tex) { m_Texture = tex; }

	// Collider handling
	/** @brief Set the collider�s full size (width/height). */
	void SetColliderSize(const glm::vec2& size) { m_ColliderSize = size; }

	/** @brief Set the collider�s positional offset relative to object center. */
	void SetColliderOffset(const glm::vec2& offs) { m_ColliderOffset = offs; }

	/** @brief Get the collider size. */
	glm::vec2 GetColliderSize() const { return m_ColliderSize; }

	/** @brief Get the collider offset. */
	glm::vec2 GetColliderOffset() const { return m_ColliderOffset; }

	/**
	 * @brief Draw the collider�s bounding box for debugging.
	 * @param view  Camera view matrix.
	 * @param proj  Camera projection matrix.
	 * @param color Debug line color (default = red).
	 */
	void DrawBoundingBox(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& color = { 1,0,0 }) const;

private:
	Mesh* m_Mesh;
	Shader* m_Shader;
	Texture* m_Texture = nullptr;

	glm::vec3 m_Position;
	glm::vec3 m_Scale;
	glm::mat4 m_Rotation;

	glm::mat4 m_ModelMatrix;

	int id; // Unique identifier for GameObjects

	glm::vec2 m_ColliderSize = { 1.0f, 1.0f };
	glm::vec2 m_ColliderOffset = { 0.0f, 0.0f };
};

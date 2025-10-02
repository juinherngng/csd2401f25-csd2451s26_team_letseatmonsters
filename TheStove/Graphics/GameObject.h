/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			GameObject.h
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
					Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Represents a renderable game object with mesh, shader, texture, transform, and collider.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "Mesh.h"
#include "Texture.h"
#include "Shader.h"
#include <glm/glm.hpp>

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

	GameObject(Mesh* mesh, Shader* shader);
	GameObject(int objectID);

	int GetID() const;
	void SetID(int newID);

	void SetPosition(const glm::vec3& position);
	void SetScale(const glm::vec3& scale);
	void SetRotation(float angleRadians, const glm::vec3& axis);

	void UpdateModelMatrix();
	void Draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
	void SetTexture(Texture* tex) { m_Texture = tex; }

	// Collider handling
	void SetColliderSize(const glm::vec2& size) { m_ColliderSize = size; }
	void SetColliderOffset(const glm::vec2& offs) { m_ColliderOffset = offs; }
	glm::vec2 GetColliderSize()   const { return m_ColliderSize; }
	glm::vec2 GetColliderOffset() const { return m_ColliderOffset; }

	/** @brief Draw collider bounding box for debugging. */
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

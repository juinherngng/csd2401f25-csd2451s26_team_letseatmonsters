/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			GameObject.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
					Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Represents a renderable game object with mesh, shader, texture,
					transform, and collider. Provides draw routines and debug bounding box.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "GameObject.h"
#include "Collision.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "ResourceManager.h"

GameObject::GameObject(Mesh* mesh, Shader* shader)
	: m_Mesh(mesh), m_Shader(shader), m_Position(0.0f), m_Scale(1.0f), m_Rotation(1.0f) {
	UpdateModelMatrix();
}

GameObject::GameObject(int objectID)
	: id(objectID) {
	m_Mesh = nullptr;
	m_ModelMatrix = glm::mat4(1.0f);
	m_Position = glm::vec3(0.0f);
	m_Scale = glm::vec3(1.0f);
	m_Rotation = glm::mat4(1.0f);
	m_Shader = nullptr;
}

void GameObject::SetID(int newID) {
	id = newID;
}

int GameObject::GetID() const {
	return id;
}

void GameObject::SetPosition(const glm::vec3& position) {
	m_Position = position;
	UpdateModelMatrix();
}

void GameObject::SetScale(const glm::vec3& scale) {
	m_Scale = scale;
	UpdateModelMatrix();
}

void GameObject::SetRotation(float angleRadians, const glm::vec3& axis) {
	m_Rotation = glm::rotate(glm::mat4(1.0f), angleRadians, axis);
	UpdateModelMatrix();
}

void GameObject::UpdateModelMatrix() {
	m_ModelMatrix = glm::translate(glm::mat4(1.0f), m_Position)
		* m_Rotation
		* glm::scale(glm::mat4(1.0f), m_Scale);
}

void GameObject::Draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const {
	if (!m_Shader || !m_Mesh) {
		std::cerr << "GameObject: Missing shader or mesh!" << std::endl;
		return;
	}

	m_Shader->Use();

	// Set matrices efficiently
	m_Shader->SetModelMatrix(m_ModelMatrix);
	m_Shader->SetViewMatrix(viewMatrix);
	m_Shader->SetProjectionMatrix(projectionMatrix);

	// Bind texture if available
	if (m_Texture) {
		m_Texture->Bind(0);
		m_Shader->SetTexture("u_Texture", 0);
	}

	m_Mesh->Draw();
}

// Debug purposes for collision
void GameObject::DrawBoundingBox(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& color) const {

	// Center = sprite position + offset
	glm::vec3 center = m_Position + glm::vec3(m_ColliderOffset, 0.0f);

	// Size = collider size (tight box)
	collision::AABB box = collision::World::makeAABBFromCenter(center, glm::vec3(m_ColliderSize, 1.0f));

	glm::vec3 verts[4] = {
		{ box.min.x, box.min.y, 0.f },
		{ box.max.x, box.min.y, 0.f },
		{ box.max.x, box.max.y, 0.f },
		{ box.min.x, box.max.y, 0.f }
	};

	GLuint vao, vbo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	// Use the BASIC shader (solid/untextured), not the sprite shader
	Shader* dbg = ResourceManager::Instance().GetShader("basic");
	if (dbg) {
		dbg->Use();
		dbg->SetModelMatrix(glm::mat4(1.0f));
		dbg->SetViewMatrix(view);
		dbg->SetProjectionMatrix(proj);
		dbg->SetColorTint(glm::vec4(color, 1.0f));
	}

	// Optional: thicker line to see better
	glLineWidth(2.0f);
	glDrawArrays(GL_LINE_LOOP, 0, 4);

	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}

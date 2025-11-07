/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			GameObject.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu

 DESCRIPTION:		Represents a renderable game object with mesh, shader, texture,
					transform, and collider. Provides draw routines and debug bounding box.

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "GameObject.hpp"
#include "../Core/Collision.hpp"
#include "ResourceManager.hpp"

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

glm::vec3 GameObject::GetPositionGLM() const {
	return m_Position;
}

void GameObject::SetPosition(const Math::Vector3D& position) {
	SetPosition(glm::vec3(position.x, position.y, position.z));
}

Math::Vector3D GameObject::GetPosition() const {
	return Math::Vector3D(m_Position.x, m_Position.y, m_Position.z);
}

void GameObject::SetScale(const glm::vec3& scale) {
	m_Scale = scale;
	UpdateModelMatrix();
}

void GameObject::SetRotation(float angleRadians, const glm::vec3& axis) {
	m_Rotation = glm::rotate(glm::mat4(1.0f), angleRadians, axis);
	UpdateModelMatrix();
}

void GameObject::SetVelocity(const Math::Vector2D& velocity) {
	m_Velocity = velocity;
}

Math::Vector2D GameObject::GetVelocity() const {
	return m_Velocity;
}

void GameObject::SetColliderSize(const Math::Vector2D& size) {
	m_ColliderSize = size;
}

Math::Vector2D GameObject::GetColliderSize() const {
	return m_ColliderSize;
}

void GameObject::SetColliderOffset(const Math::Vector2D& offset) {
	m_ColliderOffset = offset;
}

Math::Vector2D GameObject::GetColliderOffset() const {
	return m_ColliderOffset;
}

void GameObject::UpdateModelMatrix() {
	m_ModelMatrix = glm::translate(glm::mat4(1.0f), m_Position)
		* m_Rotation
		* glm::scale(glm::mat4(1.0f), m_Scale);
}

// Debug purposes for collision
void GameObject::DrawBoundingBox(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& color) const {
	// Center = sprite position + offset
	const Math::Vector3D centerM(
		m_Position.x + m_ColliderOffset.x,
		m_Position.y + m_ColliderOffset.y,
		m_Position.z
	);

	// Scale = collider size (tight box)
	const Math::Vector3D scaleM(m_ColliderSize.x, m_ColliderSize.y, 1.0f);

	const collision::AABB box = collision::World::makeAABBFromCenter(centerM, scaleM);

	if (scaleM.x <= 0.0f || scaleM.y <= 0.0f) return;

	DebugRenderer::DrawRect(
		{ box.min.x, box.min.y, 0.0f },
		{ box.max.x, box.max.y, 0.0f },
		color
	);
}

glm::vec3 GameObject::GetScaleGLM() const {
	return m_Scale;
}

float GameObject::GetRotationAngleZ() const {
	// Extract 2D rotation angle (around Z axis) from rotation matrix
	// Assuming rotation matrix represents rotation in XY plane
	float angle = std::atan2(m_Rotation[1][0], m_Rotation[0][0]);
	return angle;
}

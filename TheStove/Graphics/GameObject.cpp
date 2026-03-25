/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			GameObject.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (40%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(60%)

 DESCRIPTION:		Implements the GameObject class, which encapsulates the state, transform, and
					rendering details for every entity that appears in the scene.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "../Core/Collision.hpp"

#include "GameObject.hpp"
#include "ResourceManager.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

/**
 * @brief Performs game object.
 * @param mesh Parameter for mesh.
 * @param shader Parameter for shader.
 * @return Result produced by this operation.
 */
GameObject::GameObject(Mesh* mesh, Shader* shader)
	: m_Mesh(mesh), m_Shader(shader), m_Position(0.0f), m_Scale(1.0f), m_Rotation(1.0f), m_RotationAngle(0.0f) {
	UpdateModelMatrix();
}

/**
 * @brief Performs game object.
 * @param objectID Identifier of the target object.
 * @return Result produced by this operation.
 */
GameObject::GameObject(int objectID)
	: id(objectID) {
	m_Mesh = nullptr;
	m_ModelMatrix = glm::mat4(1.0f);
	m_Position = glm::vec3(0.0f);
	m_Scale = glm::vec3(1.0f);
	m_Rotation = glm::mat4(1.0f);
	m_RotationAngle = 0.0f;
	m_Shader = nullptr;
}

/**
 * @brief Sets id.
 * @param newID Parameter for new id.
 * @return Result produced by this operation.
 */
void GameObject::SetID(int newID) {
	id = newID;
}

/**
 * @brief Returns id.
 * @return Requested value.
 */
int GameObject::GetID() const {
	return id;
}

/**
 * @brief Sets position.
 * @param position Parameter for position.
 * @return Result produced by this operation.
 */
void GameObject::SetPosition(const glm::vec3& position) {
	m_Position = position;
	UpdateModelMatrix();
	m_TransformDirty = true;
	m_BroadphaseDirty = true;
}

/**
 * @brief Returns position glm.
 * @return Requested value.
 */
glm::vec3 GameObject::GetPositionGLM() const {
	return m_Position;
}

/**
 * @brief Sets position.
 * @param position Parameter for position.
 * @return Result produced by this operation.
 */
void GameObject::SetPosition(const Math::Vector3D& position) {
	SetPosition(glm::vec3(position.x, position.y, position.z));
}

/**
 * @brief Returns position.
 * @return Requested value.
 */
Math::Vector3D GameObject::GetPosition() const {
	return Math::Vector3D(m_Position.x, m_Position.y, m_Position.z);
}

/**
 * @brief Sets scale.
 * @param scale Parameter for scale.
 * @return Result produced by this operation.
 */
void GameObject::SetScale(const glm::vec3& scale) {
	m_Scale = scale;
	UpdateModelMatrix();
	m_TransformDirty = true;
	m_BroadphaseDirty = true;
}

/**
 * @brief Sets rotation.
 * @param angleRadians Parameter for angle radians.
 * @param axis Parameter for axis.
 * @return Result produced by this operation.
 */
void GameObject::SetRotation(float angleRadians, const glm::vec3& axis) {
	m_RotationAngle = angleRadians;
	m_Rotation = glm::rotate(glm::mat4(1.0f), m_RotationAngle, axis);
	UpdateModelMatrix();
	m_TransformDirty = true;
}

/**
 * @brief Sets velocity.
 * @param velocity Parameter for velocity.
 * @return Result produced by this operation.
 */
void GameObject::SetVelocity(const Math::Vector2D& velocity) {
	m_Velocity = velocity;
}

/**
 * @brief Returns velocity.
 * @return Requested value.
 */
Math::Vector2D GameObject::GetVelocity() const {
	return m_Velocity;
}

/**
 * @brief Sets collider size.
 * @param size Parameter for size.
 * @return Result produced by this operation.
 */
void GameObject::SetColliderSize(const Math::Vector2D& size) {
	m_ColliderSize = size;
	m_BroadphaseDirty = true;
}

/**
 * @brief Returns collider size.
 * @return Requested value.
 */
Math::Vector2D GameObject::GetColliderSize() const {
	return m_ColliderSize;
}

/**
 * @brief Sets collider offset.
 * @param offset Parameter for offset.
 * @return Result produced by this operation.
 */
void GameObject::SetColliderOffset(const Math::Vector2D& offset) {
	m_ColliderOffset = offset;
	m_BroadphaseDirty = true;
}

/**
 * @brief Returns collider offset.
 * @return Requested value.
 */
Math::Vector2D GameObject::GetColliderOffset() const {
	return m_ColliderOffset;
}

/**
 * @brief Updates model matrix.
 * @return Result produced by this operation.
 */
void GameObject::UpdateModelMatrix() {
	m_ModelMatrix = glm::translate(glm::mat4(1.0f), m_Position)
		* m_Rotation
		* glm::scale(glm::mat4(1.0f), m_Scale);
}

/**
 * @brief Draws bounding box.
 * @param view Parameter for view.
 * @param proj Parameter for proj.
 * @param color Parameter for color.
 * @return Result produced by this operation.
 */
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

	(void)proj;
	(void)view;
}

/**
 * @brief Returns scale glm.
 * @return Requested value.
 */
glm::vec3 GameObject::GetScaleGLM() const {
	return m_Scale;
}

/**
 * @brief Returns rotation angle z.
 * @return Requested value.
 */
float GameObject::GetRotationAngleZ() const {
	return m_RotationAngle;
}

/**
 * @brief Returns whether animated.
 * @return True when the operation succeeds or the condition is met.
 */
bool GameObject::IsAnimated() const {
	return animator != nullptr;
}

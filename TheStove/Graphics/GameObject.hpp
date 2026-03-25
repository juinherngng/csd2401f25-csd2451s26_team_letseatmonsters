/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			GameObject.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Seah Wang Hua, wanghua.seah@digipen.edu (40%)
 CO-AUTHORS:		Yat Chun Wee, y.chunwee@digipen.edu		(60%)

 DESCRIPTION:		Defines the GameObject class, representing any renderable or interactable
					entity in the game world. Every GameObject contains references to mesh, shader,
					and texture resources for rendering, and encapsulates its position, scale,
					rotation, velocity, and collider data for gameplay systems.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#pragma once

#include "../Core/Math.hpp"

#include "Animator.hpp"
#include "DebugRenderer.hpp"
#include "Mesh.hpp"
#include "Shader.hpp"
#include "Texture.hpp"

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

	/** @brief Get the GameObject's unique ID. */
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
	 * @brief Returns position glm.
	 * @return Requested value.
	 */
	glm::vec3 GetPositionGLM() const;

	/**
	 * @brief Sets position.
	 * @param position Parameter for position.
	 */
	void SetPosition(const Math::Vector3D& position);

	/**
	 * @brief Returns position.
	 * @return Requested value.
	 */
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

	float m_RotationAngle = 0.0f;

	/**
	 * @brief Sets velocity.
	 * @param velocity Parameter for velocity.
	 */
	void SetVelocity(const Math::Vector2D& velocity);

	/**
	 * @brief Returns velocity.
	 * @return Requested value.
	 */
	Math::Vector2D GetVelocity() const;

	/** @brief Recompute the model matrix based on position/rotation/scale. */
	void UpdateModelMatrix();

	/**
	 * @brief Sets texture.
	 * @param tex Parameter for tex.
	 */
	void SetTexture(Texture* tex) {
		m_Texture = tex;
	}

	/**
	 * @brief Returns shader.
	 * @return Requested value.
	 */
	Shader* GetShader() const {
		return m_Shader;
	}

	/**
	 * @brief Sets shader.
	 * @param shader Parameter for shader.
	 */
	void SetShader(Shader* shader) {
		m_Shader = shader;
	}

	/**
	 * @brief Returns mesh.
	 * @return Requested value.
	 */
	Mesh* GetMesh() const {
		return m_Mesh;
	}

	/**
	 * @brief Returns model matrix.
	 * @return Requested value.
	 */
	glm::mat4 GetModelMatrix() const {
		return m_ModelMatrix;
	}

	/**
	 * @brief Returns texture.
	 * @return Requested value.
	 */
	Texture* GetTexture() const {
		return m_Texture;
	}

	/** @brief Set the colliders full size (width/height). */
	void SetColliderSize(const Math::Vector2D& size);

	/** @brief Set the colliders positional offset relative to object center. */
	void SetColliderOffset(const Math::Vector2D& offset);

	/**
	 * @brief Returns whether transform dirty.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsTransformDirty() const {
		return m_TransformDirty;
	}

	/**
	 * @brief Returns whether broadphase dirty.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsBroadphaseDirty() const {
		return m_BroadphaseDirty;
	}

	/**
	 * @brief Performs mark broadphase clean.
	 */
	void MarkBroadphaseClean() {
		m_BroadphaseDirty = false;
	}

	/** @brief Get the collider size. */
	Math::Vector2D GetColliderSize() const;

	/** @brief Get the collider offset. */
	Math::Vector2D GetColliderOffset() const;

	/**
	 * @brief Returns scale glm.
	 * @return Requested value.
	 */
	glm::vec3 GetScaleGLM() const;

	/**
	 * @brief Returns rotation angle z.
	 * @return Requested value.
	 */
	float GetRotationAngleZ() const;

	/**
	 * @brief Returns rotation.
	 * @return Requested value.
	 */
	float GetRotation() const {
		return rotation_;
	}

	/**
	 * @brief Draw the colliders bounding box for debugging.
	 * @param view  Camera view matrix.
	 * @param proj  Camera projection matrix.
	 * @param color Debug line color (default = red).
	 */
	void DrawBoundingBox(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& color) const;

	/**
	 * @brief Sets uvrect.
	 * @param r Parameter for r.
	 */
	void SetUVRect(const glm::vec4& r) {
		m_uvRect = r;
	}

	/**
	 * @brief Returns uvrect.
	 * @return Requested value.
	 */
	glm::vec4 GetUVRect() const {
		return m_uvRect;
	}

	/**
	 * @brief Returns whether animated.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsAnimated() const;

	/**
	 * @brief Sets movable by physics.
	 * @param movable Parameter for movable.
	 */
	void SetMovableByPhysics(bool movable) {
		m_IsMovableByPhysics = movable;
	}

	/**
	 * @brief Returns whether movable by physics.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool IsMovableByPhysics() const {
		return m_IsMovableByPhysics;
	}

	/**
	 * @brief Enables shadow.
	 * @param enable Boolean flag controlling whether the feature is enabled.
	 */
	void EnableShadow(bool enable) {
		m_HasShadow = enable;
	}

	/**
	 * @brief Returns whether shadow.
	 * @return True when the operation succeeds or the condition is met.
	 */
	bool HasShadow() const {
		return m_HasShadow;
	}

	/**
	 * @brief Sets shadow size.
	 * @param size Parameter for size.
	 */
	void SetShadowSize(const glm::vec2& size) {
		m_ShadowSize = size;
	}

	/**
	 * @brief Returns shadow size.
	 * @return Requested value.
	 */
	glm::vec2 GetShadowSize() const {
		return m_ShadowSize;
	}

	/**
	 * @brief Sets shadow offset.
	 * @param offset Parameter for offset.
	 */
	void SetShadowOffset(const glm::vec2& offset) {
		m_ShadowOffset = offset;
	}

	/**
	 * @brief Returns shadow offset.
	 * @return Requested value.
	 */
	glm::vec2 GetShadowOffset() const {
		return m_ShadowOffset;
	}

	/**
	 * @brief Sets shadow opacity.
	 * @param opacity Parameter for opacity.
	 */
	void SetShadowOpacity(float opacity) {
		m_ShadowOpacity = opacity;
	}

	/**
	 * @brief Returns shadow opacity.
	 * @return Requested value.
	 */
	float GetShadowOpacity() const {
		return m_ShadowOpacity;
	}

	/**
	 * @brief Sets color tint.
	 * @param tint Parameter for tint.
	 */
	void SetColorTint(const glm::vec4& tint) {
		colorTint_ = tint;
	}

	/**
	 * @brief Returns color tint.
	 * @return Requested value.
	 */
	const glm::vec4& GetColorTint() const {
		return colorTint_;
	}

	/**
	 * @brief Sets render layer.
	 * @param layer Parameter for layer.
	 */
	void SetRenderLayer(int layer) {
		renderLayer_ = layer;
	}

	/**
	 * @brief Returns render layer.
	 * @return Requested value.
	 */
	int GetRenderLayer() const {
		return renderLayer_;
	}

	/**
	 * @brief Sets render sort order.
	 * @param order Parameter for order.
	 */
	void SetRenderSortOrder(int order) {
		renderSortOrder_ = order;
	}

	/**
	 * @brief Returns render sort order.
	 * @return Requested value.
	 */
	int GetRenderSortOrder() const {
		return renderSortOrder_;
	}

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
	// Start with no collider so Scene::InitDefaultCollider can correctly initialize
	// newly spawned objects to their visual sprite size.
	Math::Vector2D m_ColliderSize{ 0.f, 0.f };
	Math::Vector2D m_ColliderOffset{ 0.f, 0.f };

	Animator2D* animator = nullptr;

	bool m_IsMovableByPhysics = true; // default: objects can be pushed by physics/separation

	// Shadow parameters (simple blob shadow)
	bool m_HasShadow = false;
	glm::vec2 m_ShadowSize{ 60.0f, 20.0f };   // width, height in world units
	glm::vec2 m_ShadowOffset{ 0.0f, 0.0f };   // local offset in world units
	float m_ShadowOpacity = 0.45f;            // 0..1

	glm::vec4 colorTint_{ 1.0f, 1.0f, 1.0f, 1.0f }; // RGBA tint, 1=opaque

	int renderLayer_ = 1; // Layer number for render ordering

	bool m_TransformDirty = true;
	bool m_BroadphaseDirty = true;
	int renderSortOrder_ = 0; // Tie-breaker for render ordering within a layer
};


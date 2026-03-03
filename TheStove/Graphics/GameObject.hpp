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

		 All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
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

	float m_RotationAngle = 0.0f;

	void SetVelocity(const Math::Vector2D& velocity);
	Math::Vector2D GetVelocity() const;

	/** @brief Recompute the model matrix based on position/rotation/scale. */
	void UpdateModelMatrix();

	void SetTexture(Texture* tex) {
		m_Texture = tex;
	}

	Shader* GetShader() const {
		return m_Shader;
	}
	Mesh* GetMesh() const {
		return m_Mesh;
	}
	glm::mat4 GetModelMatrix() const {
		return m_ModelMatrix;
	}
	Texture* GetTexture() const {
		return m_Texture;
	}

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

	void SetUVRect(const glm::vec4& r) {
		m_uvRect = r;
	}
	glm::vec4 GetUVRect() const {
		return m_uvRect;
	}

	// Check if the GameObject is currently using an animated sprite
	bool IsAnimated() const;

	// Physics / pushability flag
	void SetMovableByPhysics(bool movable) {
		m_IsMovableByPhysics = movable;
	}
	bool IsMovableByPhysics() const {
		return m_IsMovableByPhysics;
	}

	// ----- Shadow controls -----
	void EnableShadow(bool enable) {
		m_HasShadow = enable;
	}
	bool HasShadow() const {
		return m_HasShadow;
	}
	void SetShadowSize(const glm::vec2& size) {
		m_ShadowSize = size;
	}
	glm::vec2 GetShadowSize() const {
		return m_ShadowSize;
	}
	void SetShadowOffset(const glm::vec2& offset) {
		m_ShadowOffset = offset;
	}
	glm::vec2 GetShadowOffset() const {
		return m_ShadowOffset;
	}
	void SetShadowOpacity(float opacity) {
		m_ShadowOpacity = opacity;
	}
	float GetShadowOpacity() const {
		return m_ShadowOpacity;
	}

	// For cross blending / tinting
	void SetColorTint(const glm::vec4& tint) {
		colorTint_ = tint;
	}
	const glm::vec4& GetColorTint() const {
		return colorTint_;
	}

	// Layer number for rendering order (set by Scene during collection)
	void SetRenderLayer(int layer) {
		renderLayer_ = layer;
	}
	int GetRenderLayer() const {
		return renderLayer_;
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
	Math::Vector2D m_ColliderSize{ 1.f, 1.f };
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
};

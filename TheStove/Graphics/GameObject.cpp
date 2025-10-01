#include "GameObject.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

GameObject::GameObject(Mesh* mesh, Shader* shader)
    : m_Mesh(mesh), m_Shader(shader), m_Position(0.0f), m_Scale(1.0f), m_Rotation(1.0f) {
    UpdateModelMatrix();
}

GameObject::GameObject(int objectID)
    : id(objectID) {
    // other initializations...
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

void GameObject::Draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
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

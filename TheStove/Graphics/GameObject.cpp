#include "GameObject.h"
#include <glm/gtc/matrix_transform.hpp>

GameObject::GameObject(Mesh* mesh, Shader* shader)
    : m_Mesh(mesh), m_Shader(shader), m_Position(0.0f), m_Scale(1.0f), m_Rotation(1.0f) {
    UpdateModelMatrix();
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
    m_Shader->Use();
    m_Shader->SetModelMatrix(m_ModelMatrix);
    m_Shader->SetViewMatrix(viewMatrix);
    m_Shader->SetProjectionMatrix(projectionMatrix);
    m_Mesh->Draw();
}

#pragma once

#include "Mesh.h"
#include "Shader.h"
#include <glm/glm.hpp>

class GameObject {
public:
    GameObject(Mesh* mesh, Shader* shader);

    void SetPosition(const glm::vec3& position);
    void SetScale(const glm::vec3& scale);
    void SetRotation(float angleRadians, const glm::vec3& axis);

    void UpdateModelMatrix();
    void Draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

private:
    Mesh* m_Mesh;
    Shader* m_Shader;

    glm::vec3 m_Position;
    glm::vec3 m_Scale;
    glm::mat4 m_Rotation;

    glm::mat4 m_ModelMatrix;
};

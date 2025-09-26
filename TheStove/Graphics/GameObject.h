#pragma once

#include "Mesh.h"
#include "Texture.h"
#include "Shader.h"
#include <glm/glm.hpp>

class GameObject {

public:
    GameObject()
        : m_Mesh(nullptr),
          m_Shader(nullptr),
          m_Position(0.0f),
          m_Scale(1.0f),
          m_Rotation(1.0f) {
          UpdateModelMatrix();
    }
    GameObject(Mesh* mesh, Shader* shader);

    void SetPosition(const glm::vec3& position);
    void SetScale(const glm::vec3& scale);
    void SetRotation(float angleRadians, const glm::vec3& axis);

    void UpdateModelMatrix();
    void Draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
    void SetTexture(Texture* tex) { m_Texture = tex; }

    

private:
    Mesh* m_Mesh;
    Shader* m_Shader;
    Texture* m_Texture = nullptr;

    glm::vec3 m_Position;
    glm::vec3 m_Scale;
    glm::mat4 m_Rotation;

    glm::mat4 m_ModelMatrix;
};

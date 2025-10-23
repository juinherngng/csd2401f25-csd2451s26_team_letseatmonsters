#include "DebugRenderer.hpp"
#include <glad/glad.h>

namespace {
	std::vector<glm::vec3> lineVerts;
	GLuint vao = 0, vbo = 0;
}

void DebugRenderer::Init() {
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
}

void DebugRenderer::Shutdown() {
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}

void DebugRenderer::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color) {
	lineVerts.push_back(start);
	lineVerts.push_back(end);
}

void DebugRenderer::DrawRect(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color) {
	DrawLine({ min.x, min.y, 0 }, { max.x, min.y, 0 }, color);
	DrawLine({ max.x, min.y, 0 }, { max.x, max.y, 0 }, color);
	DrawLine({ max.x, max.y, 0 }, { min.x, max.y, 0 }, color);
	DrawLine({ min.x, max.y, 0 }, { min.x, min.y, 0 }, color);
}

void DebugRenderer::DrawCircle(const glm::vec3& center, float radius, const glm::vec3& color, int segments) {
	float step = 2.0f * 3.1415926f / segments;
	for (int i = 0; i < segments; ++i) {
		float a1 = i * step;
		float a2 = (i + 1) * step;
		glm::vec3 p1 = center + glm::vec3(cos(a1) * radius, sin(a1) * radius, 0);
		glm::vec3 p2 = center + glm::vec3(cos(a2) * radius, sin(a2) * radius, 0);
		DrawLine(p1, p2, color);
	}
}

namespace { bool s_enabled = true; }

void DebugRenderer::SetEnabled(bool e) { s_enabled = e; }
bool DebugRenderer::IsEnabled() { return s_enabled; }

void DebugRenderer::Flush(const glm::mat4& view, const glm::mat4& proj) {
	if (!s_enabled) {
		lineVerts.clear();
		return;
	}

	if (lineVerts.empty()) return;

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, lineVerts.size() * sizeof(glm::vec3), lineVerts.data(), GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	Shader* dbg = ResourceManager::Instance().GetShader("basic");
	dbg->Use();
	dbg->SetModelMatrix(glm::mat4(1.0f));
	dbg->SetViewMatrix(view);
	dbg->SetProjectionMatrix(proj);
	dbg->SetColorTint(glm::vec4(1, 0, 0, 1)); // Red for now

	glLineWidth(2.0f);
	glDrawArrays(GL_LINES, 0, (GLsizei)lineVerts.size());

	lineVerts.clear();
}

#include "DebugRenderer.hpp"
#include <glad/glad.h>

namespace {
	struct LineBatch {
		std::vector<glm::vec3> verts; // pairs (a,b)
		glm::vec4 color;
	};
	struct PointBatch {
		std::vector<glm::vec3> pts;
		glm::vec4 color;
		float size = 5.0f;
	};

	// color key helper
	inline uint32_t packColor(const glm::vec4& c) {
		auto to255 = [](float f) -> uint32_t {
			int v = (int)std::lround(f * 255.0f);
			if (v < 0) v = 0; else if (v > 255) v = 255;
			return (uint32_t)v;
			};
		const uint32_t r = to255(c.r);
		const uint32_t g = to255(c.g);
		const uint32_t b = to255(c.b);
		const uint32_t a = to255(c.a);
		return (r) |
			(g << 8) |
			(b << 16) |
			(a << 24);
	}
	inline glm::vec4 as4(const glm::vec3& c) { return glm::vec4(c, 1.0f); }

	std::map<uint32_t, LineBatch> s_lines;
	std::map<uint32_t, PointBatch> s_points;

	GLuint s_vaoLines = 0, s_vboLines = 0;
	GLuint s_vaoPoints = 0, s_vboPoints = 0;
	bool s_enabled = true;
}

void DebugRenderer::Init() {
	glGenVertexArrays(1, &s_vaoLines);
	glGenBuffers(1, &s_vboLines);
	glGenVertexArrays(1, &s_vaoPoints);
	glGenBuffers(1, &s_vboPoints);
}

void DebugRenderer::Shutdown() {
	glDeleteBuffers(1, &s_vboLines);
	glDeleteVertexArrays(1, &s_vaoLines);
	glDeleteBuffers(1, &s_vboPoints);
	glDeleteVertexArrays(1, &s_vaoPoints);
}

void DebugRenderer::SetEnabled(bool e) { s_enabled = e; }
bool DebugRenderer::IsEnabled() { return s_enabled; }

void DebugRenderer::DrawLine(const glm::vec3& a, const glm::vec3& b, const glm::vec3& color) {
	auto key = packColor(as4(color));
	LineBatch& L = s_lines[key];
	L.color = as4(color);
	L.verts.push_back(a);
	L.verts.push_back(b);
}

void DebugRenderer::DrawPoint(const glm::vec3& p, const glm::vec3& color, float size) {
	auto key = packColor(as4(color));
	PointBatch& P = s_points[key];
	P.color = as4(color);
	P.size = size;
	P.pts.push_back(p);
}

void DebugRenderer::DrawRect(const glm::vec3& mn, const glm::vec3& mx, const glm::vec3& color) {
	DrawLine({ mn.x, mn.y, 0 }, { mx.x, mn.y, 0 }, color);
	DrawLine({ mx.x, mn.y, 0 }, { mx.x, mx.y, 0 }, color);
	DrawLine({ mx.x, mx.y, 0 }, { mn.x, mx.y, 0 }, color);
	DrawLine({ mn.x, mx.y, 0 }, { mn.x, mn.y, 0 }, color);
}

void DebugRenderer::Flush(const glm::mat4& view, const glm::mat4& proj) {
	if (!s_enabled) { s_lines.clear(); s_points.clear(); return; }

	Shader* dbg = ResourceManager::Instance().GetShader("basic");
	if (!dbg) { s_lines.clear(); s_points.clear(); return; }

	dbg->Use();
	dbg->SetModelMatrix(glm::mat4(1.0f));
	dbg->SetViewMatrix(view);
	dbg->SetProjectionMatrix(proj);

	// Lines
	glBindVertexArray(s_vaoLines);
	glBindBuffer(GL_ARRAY_BUFFER, s_vboLines);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	glLineWidth(2.0f);
	for (auto& [key, batch] : s_lines) {
		if (batch.verts.empty()) continue;
		dbg->SetColorTint(batch.color);
		glBufferData(GL_ARRAY_BUFFER, batch.verts.size() * sizeof(glm::vec3), batch.verts.data(), GL_DYNAMIC_DRAW);
		glDrawArrays(GL_LINES, 0, (GLsizei)batch.verts.size());
	}
	s_lines.clear();

	// Points
	glBindVertexArray(s_vaoPoints);
	glBindBuffer(GL_ARRAY_BUFFER, s_vboPoints);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	for (auto& [key, pb] : s_points) {
		if (pb.pts.empty()) continue;
		dbg->SetColorTint(pb.color);
		glPointSize(pb.size);
		glBufferData(GL_ARRAY_BUFFER, pb.pts.size() * sizeof(glm::vec3), pb.pts.data(), GL_DYNAMIC_DRAW);
		glDrawArrays(GL_POINTS, 0, (GLsizei)pb.pts.size());
	}
	s_points.clear();
}

/*
 ----------------------------------------------------------------------------------------------------
 FILE NAME:			DebugRenderer.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu (100%)

 DESCRIPTION:		Implementation of DebugRenderer. Handles point and line batching
					for visual debugging overlays.

		 All content  2025 DigiPen Institute of Technology Singapore. All rights reserved.
 ----------------------------------------------------------------------------------------------------
 */

#include "DebugRenderer.hpp"

#include <glad/glad.h>

namespace {
	// Internal Data Structures
	struct LineBatch {
		std::vector<glm::vec3> vertices;
		glm::vec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
	};

	// A batch of points with the same color and size.
	struct PointBatch {
		std::vector<glm::vec3> points;
		glm::vec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
		float size{ 5.0f };
	};

	/**
	 * @brief Performs pack color.
	 * @param color Parameter for color.
	 * @return Result produced by this operation.
	 */
	inline std::uint32_t PackColor(const glm::vec4& color) {
		auto toByte = [](float f) -> std::uint32_t {
			int v = static_cast<int>(std::lround(f * 255.0f));
			if (v < 0) {
				v = 0;
			}
			else if (v > 255) {
				v = 255;
			}
			return static_cast<std::uint32_t>(v);
			};

		const std::uint32_t r = toByte(color.r);
		const std::uint32_t g = toByte(color.g);
		const std::uint32_t b = toByte(color.b);
		const std::uint32_t a = toByte(color.a);

		return (r) | (g << 8) | (b << 16) | (a << 24);
	}

	/**
	 * @brief Performs to vec4.
	 * @param c Parameter for c.
	 * @return Result produced by this operation.
	 */
	inline glm::vec4 ToVec4(const glm::vec3& c) {
		return glm::vec4(c, 1.0f);
	}

	// Internal State
	std::map<std::uint32_t, LineBatch> lineBatches;
	std::map<std::uint32_t, PointBatch> pointBatches;

	GLuint vaoLines = 0;
	GLuint vboLines = 0;
	GLuint vaoPoints = 0;
	GLuint vboPoints = 0;

	bool isEnabled = true;
}

/**
 * @brief Initializes this object.
 * @return Result produced by this operation.
 */
void DebugRenderer::Init() {
	glGenVertexArrays(1, &vaoLines);
	glGenBuffers(1, &vboLines);

	glGenVertexArrays(1, &vaoPoints);
	glGenBuffers(1, &vboPoints);
}

/**
 * @brief Performs shutdown.
 * @return Result produced by this operation.
 */
void DebugRenderer::Shutdown() {
	glDeleteBuffers(1, &vboLines);
	glDeleteVertexArrays(1, &vaoLines);

	glDeleteBuffers(1, &vboPoints);
	glDeleteVertexArrays(1, &vaoPoints);
}

/**
 * @brief Sets enabled.
 * @param enable Boolean flag controlling whether the feature is enabled.
 * @return Result produced by this operation.
 */
void DebugRenderer::SetEnabled(bool enable) {
	isEnabled = enable;
}

/**
 * @brief Returns whether enabled.
 * @return True when the operation succeeds or the condition is met.
 */
bool DebugRenderer::IsEnabled() {
	return isEnabled;
}

/**
 * @brief Draws line.
 * @param start Parameter for start.
 * @param end Parameter for end.
 * @param color Parameter for color.
 * @return Result produced by this operation.
 */
void DebugRenderer::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color) {
	std::uint32_t key = PackColor(ToVec4(color));
	LineBatch& batch = lineBatches[key];

	batch.color = ToVec4(color);
	batch.vertices.push_back(start);
	batch.vertices.push_back(end);
}

/**
 * @brief Draws point.
 * @param position Parameter for position.
 * @param color Parameter for color.
 * @param size Parameter for size.
 * @return Result produced by this operation.
 */
void DebugRenderer::DrawPoint(const glm::vec3& position, const glm::vec3& color, float size) {
	std::uint32_t key = PackColor(ToVec4(color));
	PointBatch& batch = pointBatches[key];

	batch.color = ToVec4(color);
	batch.size = size;
	batch.points.push_back(position);
}

/**
 * @brief Draws rect.
 * @param minCorner Parameter for min corner.
 * @param maxCorner Parameter for max corner.
 * @param color Parameter for color.
 * @return Result produced by this operation.
 */
void DebugRenderer::DrawRect(const glm::vec3& minCorner, const glm::vec3& maxCorner, const glm::vec3& color) {
	DrawLine({ minCorner.x, minCorner.y, 0.0f }, { maxCorner.x, minCorner.y, 0.0f }, color);
	DrawLine({ maxCorner.x, minCorner.y, 0.0f }, { maxCorner.x, maxCorner.y, 0.0f }, color);
	DrawLine({ maxCorner.x, maxCorner.y, 0.0f }, { minCorner.x, maxCorner.y, 0.0f }, color);
	DrawLine({ minCorner.x, maxCorner.y, 0.0f }, { minCorner.x, minCorner.y, 0.0f }, color);
}

/**
 * @brief Performs flush.
 * @param viewMatrix Parameter for view matrix.
 * @param projectionMatrix Parameter for projection matrix.
 * @return Result produced by this operation.
 */
void DebugRenderer::Flush(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
	if (!isEnabled) {
		lineBatches.clear();
		pointBatches.clear();
		return;
	}

	Shader* shader = ResourceManager::Instance().GetShader("basic");
	if (shader == nullptr) {
		lineBatches.clear();
		pointBatches.clear();
		return;
	}

	shader->Use();
	shader->SetModelMatrix(glm::mat4(1.0f));
	shader->SetViewMatrix(viewMatrix);
	shader->SetProjectionMatrix(projectionMatrix);

	// Draw Lines
	glBindVertexArray(vaoLines);
	glBindBuffer(GL_ARRAY_BUFFER, vboLines);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glLineWidth(2.0f);

	for (auto& [key, batch] : lineBatches) {
		if (batch.vertices.empty()) {
			continue;
		}

		shader->SetColorTint(batch.color);
		glBufferData(GL_ARRAY_BUFFER,
			batch.vertices.size() * sizeof(glm::vec3),
			batch.vertices.data(),
			GL_DYNAMIC_DRAW);
		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(batch.vertices.size()));
	}

	lineBatches.clear();

	// Draw Points
	glBindVertexArray(vaoPoints);
	glBindBuffer(GL_ARRAY_BUFFER, vboPoints);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	for (auto& [key, batch] : pointBatches) {
		if (batch.points.empty()) {
			continue;
		}

		shader->SetColorTint(batch.color);
		glPointSize(batch.size);
		glBufferData(GL_ARRAY_BUFFER, batch.points.size() * sizeof(glm::vec3), batch.points.data(), GL_DYNAMIC_DRAW);
		glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(batch.points.size()));
	}

	pointBatches.clear();
}

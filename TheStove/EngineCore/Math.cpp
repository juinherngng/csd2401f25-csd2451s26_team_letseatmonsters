/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Math.cpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (80%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	    (20%)

 DESCRIPTION:		Math library definitions.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include <algorithm>
#include <cmath>

#include "EngineCore/Math.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace Math {
	Vector2D const Vector2D::ZERO{ 0.0f, 0.0f };
	Vector2D const Vector2D::ONE{ 1.0f, 1.0f };

	/**
	 * @brief Initializes a vector from x and y components.
	 * @param x Horizontal component to store.
	 * @param y Vertical component to store.
	 */
	Vector2D::Vector2D(float const x, float const y) : x(x), y(y) {}

	/**
	 * @brief Returns the Euclidean length of the vector.
	 * @return Magnitude of the vector.
	 */
	float Vector2D::Length() const {
		// Measure the vector directly from its Cartesian components.
		return std::sqrt(x * x + y * y);
	}

	/**
	 * @brief Returns a unit-length copy of the vector when possible.
	 * @return Normalized vector, or `ZERO` when the input has no length.
	 */
	Vector2D Vector2D::Normalized() const {
		// Reuse the shared magnitude helper before guarding against divide-by-zero.
		float length = Length();
		return (length > 0.0f) ? Vector2D(x / length, y / length) : Vector2D::ZERO;
	}

	/**
	 * @brief Computes the dot product with another vector.
	 * @param other Vector to compare against.
	 * @return Dot product between both vectors.
	 */
	float Vector2D::Dot(Vector2D const& other) const {
		// Accumulate the pairwise component products into a scalar projection measure.
		return x * other.x + y * other.y;
	}

	/**
	 * @brief Adds another vector component-wise.
	 * @param rhs Vector to add.
	 * @return Sum of both vectors.
	 */
	Vector2D Vector2D::operator+(Vector2D const& rhs) const {
		// Combine each axis independently to preserve vector semantics.
		return Vector2D(x + rhs.x, y + rhs.y);
	}

	/**
	 * @brief Subtracts another vector component-wise.
	 * @param rhs Vector to subtract.
	 * @return Difference between both vectors.
	 */
	Vector2D Vector2D::operator-(Vector2D const& rhs) const {
		// Compute the directional delta from this vector to the right-hand operand.
		return Vector2D(x - rhs.x, y - rhs.y);
	}

	/**
	 * @brief Multiplies both components by a scalar.
	 * @param scalar Scalar value to apply.
	 * @return Scaled vector.
	 */
	Vector2D Vector2D::operator*(float const scalar) const {
		// Apply the same scale factor to each component.
		return Vector2D(x * scalar, y * scalar);
	}

	/**
	 * @brief Compares this vector against another for exact equality.
	 * @param rhs Vector to compare against.
	 * @return `true` when both components match exactly.
	 */
	bool Vector2D::operator==(Vector2D const& rhs) const {
		// Preserve exact comparisons for callers that require deterministic identity checks.
		return x == rhs.x && y == rhs.y;
	}

	/**
	 * @brief Copies another vector into this one.
	 * @param rhs Source vector to copy from.
	 * @return Reference to this vector after assignment.
	 */
	Vector2D& Vector2D::operator=(Vector2D const& rhs) {
		// Skip the copy when both references already point at the same object.
		if (this != &rhs) {
			x = rhs.x;
			y = rhs.y;
		}

		return *this;
	}

	const Matrix3x3 Matrix3x3::IDENTITY{
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f
	};

	/**
	 * @brief Initializes a matrix from nine float values in row-major order.
	 * @param m00 First-row, first-column element.
	 * @param m01 First-row, second-column element.
	 * @param m02 First-row, third-column element.
	 * @param m10 Second-row, first-column element.
	 * @param m11 Second-row, second-column element.
	 * @param m12 Second-row, third-column element.
	 * @param m20 Third-row, first-column element.
	 * @param m21 Third-row, second-column element.
	 * @param m22 Third-row, third-column element.
	 */
	Matrix3x3::Matrix3x3(
		float const m00, float const m01, float const m02,
		float const m10, float const m11, float const m12,
		float const m20, float const m21, float const m22)
		: m{ m00, m01, m02, m10, m11, m12, m20, m21, m22 } {}

	/**
	 * @brief Multiplies this matrix by another matrix.
	 * @param rhs Right-hand matrix operand.
	 * @return Matrix product.
	 */
	Matrix3x3 Matrix3x3::operator*(Matrix3x3 const& rhs) const {
		Matrix3x3 result;
		// Evaluate the standard row-by-column product for each output element.
		for (int row = 0; row < 3; row++) {
			for (int col = 0; col < 3; col++) {
				result.m[row * 3 + col] =
					m[row * 3 + 0] * rhs.m[0 * 3 + col] +
					m[row * 3 + 1] * rhs.m[1 * 3 + col] +
					m[row * 3 + 2] * rhs.m[2 * 3 + col];
			}
		}

		return result;
	}

	/**
	 * @brief Transforms a 2D point by this matrix.
	 * @param vector Point to transform.
	 * @return Transformed point.
	 */
	Vector2D Matrix3x3::TransformPoint(Vector2D const& vector) const {
		// Treat the input as an affine point and fold translation into the result.
		return Vector2D(
			m[0] * vector.x + m[1] * vector.y + m[2],
			m[3] * vector.x + m[4] * vector.y + m[5]);
	}

	/**
	 * @brief Builds a translation matrix for a 2D offset.
	 * @param offset Translation amount to encode.
	 * @return Translation matrix.
	 */
	Matrix3x3 Matrix3x3::Translate(Vector2D const& offset) {
		// Encode the offset in the affine translation column.
		return Matrix3x3(
			1.0f, 0.0f, offset.x,
			0.0f, 1.0f, offset.y,
			0.0f, 0.0f, 1.0f);
	}

	/**
	 * @brief Builds a scaling matrix for a 2D scale factor.
	 * @param factors Per-axis scale factors.
	 * @return Scaling matrix.
	 */
	Matrix3x3 Matrix3x3::Scale(Vector2D const& factors) {
		// Store each scale factor on the matching diagonal entry.
		return Matrix3x3(
			factors.x, 0.0f, 0.0f,
			0.0f, factors.y, 0.0f,
			0.0f, 0.0f, 1.0f);
	}

	/**
	 * @brief Builds a rotation matrix for a 2D angle.
	 * @param degrees Counter-clockwise rotation angle in degrees.
	 * @return Rotation matrix.
	 */
	Matrix3x3 Matrix3x3::Rotate(float const degrees) {
		// Convert the public degree API into radians before evaluating trig values.
		float radians = ToRadians(degrees);
		float cosTheta = std::cos(radians);
		float sinTheta = std::sin(radians);

		return Matrix3x3(
			cosTheta, -sinTheta, 0.0f,
			sinTheta, cosTheta, 0.0f,
			0.0f, 0.0f, 1.0f);
	}

	/**
	 * @brief Multiplies this matrix by another transform.
	 * @param other Matrix to concatenate after this one.
	 * @return Product of this matrix and `other`.
	 */
	Matrix3x3 Matrix3x3::Concatenate(Matrix3x3 const& other) const {
		// Forward to the matrix multiplication operator to keep composition behavior consistent.
		return (*this) * other;
	}

	/**
	 * @brief Multiplies an ordered list of matrices into one result.
	 * @param matrices Pointer to the first matrix in the sequence.
	 * @param count Number of matrices to multiply.
	 * @return Combined matrix product.
	 */
	Matrix3x3 Matrix3x3::Concatenate(Matrix3x3 const* matrices, std::size_t count) {
		Matrix3x3 result = Matrix3x3::IDENTITY;

		// Fold the provided sequence from left to right so order remains explicit.
		for (std::size_t i = 0; i < count; i++) {
			result = result * matrices[i];
		}

		return result;
	}

	/**
	 * @brief Compares this matrix against another for exact equality.
	 * @param rhs Matrix to compare against.
	 * @return `true` when every stored element matches exactly.
	 */
	bool Matrix3x3::operator==(Matrix3x3 const& rhs) const {
		// Exit early as soon as any element differs.
		for (int i = 0; i < 9; i++) {
			if (m[i] != rhs.m[i]) {
				return false;
			}
		}

		return true;
	}

	/**
	 * @brief Returns the inverse matrix when the determinant is non-zero.
	 * @return Inverse matrix, or `IDENTITY` when the matrix is singular.
	 */
	Matrix3x3 Matrix3x3::Inverse() const {
		// Compute the affine determinant to decide whether inversion is valid.
		float det =
			m[0] * (m[4] * m[8] - m[5] * m[7]) -
			m[1] * (m[3] * m[8] - m[5] * m[6]) +
			m[2] * (m[3] * m[7] - m[4] * m[6]);

		if (std::fabs(det) < 1e-6f) {
			// Fall back to identity so callers never receive undefined matrix contents.
			return Matrix3x3::IDENTITY;
		}

		float invDet = 1.0f / det;
		Matrix3x3 inv;

		// Build the adjugate matrix and scale it by the reciprocal determinant.
		inv.m[0] = (m[4] * m[8] - m[5] * m[7]) * invDet;
		inv.m[1] = -(m[1] * m[8] - m[2] * m[7]) * invDet;
		inv.m[2] = (m[1] * m[5] - m[2] * m[4]) * invDet;

		inv.m[3] = -(m[3] * m[8] - m[5] * m[6]) * invDet;
		inv.m[4] = (m[0] * m[8] - m[2] * m[6]) * invDet;
		inv.m[5] = -(m[0] * m[5] - m[2] * m[3]) * invDet;

		inv.m[6] = (m[3] * m[7] - m[4] * m[6]) * invDet;
		inv.m[7] = -(m[0] * m[7] - m[1] * m[6]) * invDet;
		inv.m[8] = (m[0] * m[4] - m[1] * m[3]) * invDet;

		return inv;
	}

	/**
	 * @brief Returns a transposed copy of a matrix.
	 * @param mat Matrix to transpose.
	 * @return Transposed matrix.
	 */
	Matrix3x3 Matrix3x3::Transpose(Matrix3x3 const& mat) {
		// Swap rows and columns while preserving the original matrix object.
		return Matrix3x3(
			mat.m[0], mat.m[3], mat.m[6],
			mat.m[1], mat.m[4], mat.m[7],
			mat.m[2], mat.m[5], mat.m[8]);
	}

	Vector3D const Vector3D::ZERO{ 0.0f, 0.0f, 0.0f };
	Vector3D const Vector3D::ONE{ 1.0f, 1.0f, 1.0f };

	/**
	 * @brief Initializes a vector from x, y, and z components.
	 * @param x X-axis component to store.
	 * @param y Y-axis component to store.
	 * @param z Z-axis component to store.
	 */
	Vector3D::Vector3D(float const x, float const y, float const z) : x(x), y(y), z(z) {}

	/**
	 * @brief Returns the Euclidean length of the vector.
	 * @return Magnitude of the vector.
	 */
	float Vector3D::Length() const {
		// Measure the 3D vector using all three Cartesian axes.
		return std::sqrt(x * x + y * y + z * z);
	}

	/**
	 * @brief Returns a unit-length copy of the vector when possible.
	 * @return Normalized vector, or `ZERO` when the input has no length.
	 */
	Vector3D Vector3D::Normalized() const {
		// Normalize only when the source vector is long enough to avoid division by zero.
		float len = Length();
		return (len > 0.0f) ? Vector3D(x / len, y / len, z / len) : Vector3D::ZERO;
	}

	/**
	 * @brief Computes the dot product with another vector.
	 * @param other Vector to compare against.
	 * @return Dot product between both vectors.
	 */
	float Vector3D::Dot(Vector3D const& other) const {
		// Reduce the component-wise products into a single projection scalar.
		return x * other.x + y * other.y + z * other.z;
	}

	/**
	 * @brief Computes the cross product with another vector.
	 * @param other Vector to compare against.
	 * @return Vector perpendicular to both inputs.
	 */
	Vector3D Vector3D::Cross(Vector3D const& other) const {
		// Build the orthogonal vector using the standard determinant expansion.
		return Vector3D(
			y * other.z - z * other.y,
			z * other.x - x * other.z,
			x * other.y - y * other.x);
	}

	/**
	 * @brief Adds another vector component-wise.
	 * @param rhs Vector to add.
	 * @return Sum of both vectors.
	 */
	Vector3D Vector3D::operator+(Vector3D const& rhs) const {
		// Combine each 3D axis independently.
		return Vector3D(x + rhs.x, y + rhs.y, z + rhs.z);
	}

	/**
	 * @brief Subtracts another vector component-wise.
	 * @param rhs Vector to subtract.
	 * @return Difference between both vectors.
	 */
	Vector3D Vector3D::operator-(Vector3D const& rhs) const {
		// Compute the directional offset between both points in space.
		return Vector3D(x - rhs.x, y - rhs.y, z - rhs.z);
	}

	/**
	 * @brief Multiplies each component by a scalar.
	 * @param scalar Scalar value to apply.
	 * @return Scaled vector.
	 */
	Vector3D Vector3D::operator*(float const scalar) const {
		// Apply the same scale factor to all three coordinates.
		return Vector3D(x * scalar, y * scalar, z * scalar);
	}

	/**
	 * @brief Compares this vector against another for exact equality.
	 * @param rhs Vector to compare against.
	 * @return `true` when every component matches exactly.
	 */
	bool Vector3D::operator==(Vector3D const& rhs) const {
		// Preserve exact comparisons for deterministic transform checks.
		return x == rhs.x && y == rhs.y && z == rhs.z;
	}

	Matrix4x4 const Matrix4x4::IDENTITY{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	/**
	 * @brief Initializes a matrix from sixteen float values in row-major order.
	 * @param m00 First-row, first-column element.
	 * @param m01 First-row, second-column element.
	 * @param m02 First-row, third-column element.
	 * @param m03 First-row, fourth-column element.
	 * @param m10 Second-row, first-column element.
	 * @param m11 Second-row, second-column element.
	 * @param m12 Second-row, third-column element.
	 * @param m13 Second-row, fourth-column element.
	 * @param m20 Third-row, first-column element.
	 * @param m21 Third-row, second-column element.
	 * @param m22 Third-row, third-column element.
	 * @param m23 Third-row, fourth-column element.
	 * @param m30 Fourth-row, first-column element.
	 * @param m31 Fourth-row, second-column element.
	 * @param m32 Fourth-row, third-column element.
	 * @param m33 Fourth-row, fourth-column element.
	 */
	Matrix4x4::Matrix4x4(
		float const m00, float const m01, float const m02, float const m03,
		float const m10, float const m11, float const m12, float const m13,
		float const m20, float const m21, float const m22, float const m23,
		float const m30, float const m31, float const m32, float const m33)
		: m{ m00, m01, m02, m03,
			 m10, m11, m12, m13,
			 m20, m21, m22, m23,
			 m30, m31, m32, m33 } {}

	/**
	 * @brief Transforms a 3D point by this matrix.
	 * @param v Point to transform.
	 * @return Transformed point after homogeneous divide when applicable.
	 */
	Vector3D Matrix4x4::TransformPoint(Vector3D const& v) const {
		// Multiply the input by the matrix as a homogeneous point.
		float x_ = m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3];
		float y_ = m[4] * v.x + m[5] * v.y + m[6] * v.z + m[7];
		float z_ = m[8] * v.x + m[9] * v.y + m[10] * v.z + m[11];
		float w_ = m[12] * v.x + m[13] * v.y + m[14] * v.z + m[15];

		if (w_ != 0.0f) {
			// Project back into Cartesian space when the transform introduced perspective division.
			x_ /= w_;
			y_ /= w_;
			z_ /= w_;
		}

		return Vector3D(x_, y_, z_);
	}

	/**
	 * @brief Builds a translation matrix for a 3D offset.
	 * @param offset Translation amount to encode.
	 * @return Translation matrix.
	 */
	Matrix4x4 Matrix4x4::Translate(Vector3D const& offset) {
		Matrix4x4 mat = Matrix4x4::IDENTITY;

		// Store the translation in the final column of the affine matrix.
		mat.m[3] = offset.x;
		mat.m[7] = offset.y;
		mat.m[11] = offset.z;

		return mat;
	}

	/**
	 * @brief Builds a scaling matrix for a 3D scale factor.
	 * @param factors Per-axis scale factors.
	 * @return Scaling matrix.
	 */
	Matrix4x4 Matrix4x4::Scale(Vector3D const& factors) {
		Matrix4x4 mat = Matrix4x4::IDENTITY;

		// Place each axis scale factor on the matching diagonal entry.
		mat.m[0] = factors.x;
		mat.m[5] = factors.y;
		mat.m[10] = factors.z;

		return mat;
	}

	/**
	 * @brief Builds a rotation matrix around the X axis.
	 * @param degrees Rotation angle in degrees.
	 * @return X-axis rotation matrix.
	 */
	Matrix4x4 Matrix4x4::RotateX(float const degrees) {
		// Convert the public degree API into trig-friendly radians.
		float rad = ToRadians(degrees);
		float c = std::cos(rad);
		float s = std::sin(rad);

		return Matrix4x4(
			1, 0, 0, 0,
			0, c, -s, 0,
			0, s, c, 0,
			0, 0, 0, 1);
	}

	/**
	 * @brief Builds a rotation matrix around the Y axis.
	 * @param degrees Rotation angle in degrees.
	 * @return Y-axis rotation matrix.
	 */
	Matrix4x4 Matrix4x4::RotateY(float const degrees) {
		// Convert the public degree API into trig-friendly radians.
		float rad = ToRadians(degrees);
		float c = std::cos(rad);
		float s = std::sin(rad);

		return Matrix4x4(
			c, 0, s, 0,
			0, 1, 0, 0,
			-s, 0, c, 0,
			0, 0, 0, 1);
	}

	/**
	 * @brief Builds a rotation matrix around the Z axis.
	 * @param degrees Rotation angle in degrees.
	 * @return Z-axis rotation matrix.
	 */
	Matrix4x4 Matrix4x4::RotateZ(float const degrees) {
		// Convert the public degree API into trig-friendly radians.
		float rad = ToRadians(degrees);
		float c = std::cos(rad);
		float s = std::sin(rad);

		return Matrix4x4(
			c, -s, 0, 0,
			s, c, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1);
	}

	/**
	 * @brief Multiplies this matrix by another transform.
	 * @param other Matrix to concatenate after this one.
	 * @return Product of this matrix and `other`.
	 */
	Matrix4x4 Matrix4x4::Concatenate(Matrix4x4 const& other) const {
		// Forward to the multiplication operator so all concatenation paths stay identical.
		return (*this) * other;
	}

	/**
	 * @brief Multiplies an ordered list of matrices into one result.
	 * @param matrices Pointer to the first matrix in the sequence.
	 * @param count Number of matrices to multiply.
	 * @return Combined matrix product.
	 */
	Matrix4x4 Matrix4x4::Concatenate(Matrix4x4 const* matrices, std::size_t count) {
		Matrix4x4 result = Matrix4x4::IDENTITY;

		// Fold the provided sequence from left to right so transform order remains explicit.
		for (std::size_t i = 0; i < count; ++i) {
			result = result * matrices[i];
		}

		return result;
	}

	/**
	 * @brief Multiplies this matrix by another matrix.
	 * @param rhs Right-hand matrix operand.
	 * @return Matrix product.
	 */
	Matrix4x4 Matrix4x4::operator*(Matrix4x4 const& rhs) const {
		Matrix4x4 result;

		// Evaluate the standard row-by-column product for each output element.
		for (int row = 0; row < 4; ++row) {
			for (int col = 0; col < 4; ++col) {
				result.m[row * 4 + col] =
					m[row * 4 + 0] * rhs.m[0 * 4 + col] +
					m[row * 4 + 1] * rhs.m[1 * 4 + col] +
					m[row * 4 + 2] * rhs.m[2 * 4 + col] +
					m[row * 4 + 3] * rhs.m[3 * 4 + col];
			}
		}

		return result;
	}

	/**
	 * @brief Compares this matrix against another for exact equality.
	 * @param rhs Matrix to compare against.
	 * @return `true` when every stored element matches exactly.
	 */
	bool Matrix4x4::operator==(Matrix4x4 const& rhs) const {
		// Exit early as soon as any stored element differs.
		for (int i = 0; i < 16; ++i) {
			if (m[i] != rhs.m[i]) {
				return false;
			}
		}

		return true;
	}

	/**
	 * @brief Returns a transposed copy of a matrix.
	 * @param mat Matrix to transpose.
	 * @return Transposed matrix.
	 */
	Matrix4x4 Matrix4x4::Transpose(Matrix4x4 const& mat) {
		Matrix4x4 result;

		// Swap rows and columns while preserving the original matrix object.
		for (int row = 0; row < 4; ++row) {
			for (int col = 0; col < 4; ++col) {
				result.m[row * 4 + col] = mat.m[col * 4 + row];
			}
		}

		return result;
	}

	/**
	 * @brief Converts degrees to radians.
	 * @param degrees Angle in degrees.
	 * @return Equivalent angle in radians.
	 */
	float ToRadians(float const degrees) {
		// Scale the degree measure by pi over 180 to enter radian space.
		return degrees * static_cast<float>(M_PI) / 180.0f;
	}

	/**
	 * @brief Converts radians to degrees.
	 * @param radians Angle in radians.
	 * @return Equivalent angle in degrees.
	 */
	float ToDegrees(float const radians) {
		// Scale the radian measure by 180 over pi to enter degree space.
		return radians * 180.0f / static_cast<float>(M_PI);
	}

	/**
	 * @brief Transforms a 2D point by a 3x3 matrix.
	 * @param matrix Matrix to apply.
	 * @param vector Point to transform.
	 * @return Transformed point.
	 */
	Vector2D Transform2D(Matrix3x3 const& matrix, Vector2D const& vector) {
		// Delegate to the matrix helper so all affine-point transforms share the same implementation.
		return matrix.TransformPoint(vector);
	}

	/**
	 * @brief Computes the Euclidean distance between two 2D points.
	 * @param a First point.
	 * @param b Second point.
	 * @return Distance between both points.
	 */
	float Distance(Vector2D const& a, Vector2D const& b) {
		// Measure the vector separating both points before taking its magnitude.
		float dx = a.x - b.x;
		float dy = a.y - b.y;
		return std::sqrt(dx * dx + dy * dy);
	}

	/**
	 * @brief Linearly interpolates between two 2D vectors.
	 * @param a Start vector.
	 * @param b End vector.
	 * @param t Interpolation factor in the inclusive range `[0, 1]`.
	 * @return Interpolated vector.
	 */
	Vector2D Lerp(Vector2D const& a, Vector2D const& b, float t) {
		// Blend each component independently along the straight line between both vectors.
		return Vector2D(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
	}

	/**
	 * @brief Checks whether two 2D vectors differ by less than an epsilon.
	 * @param a First vector.
	 * @param b Second vector.
	 * @param epsilon Allowed absolute difference per component.
	 * @return `true` when both components are within the supplied tolerance.
	 */
	bool AlmostEqual(Vector2D const& a, Vector2D const& b, float epsilon) {
		// Compare each axis independently so callers can tolerate tiny floating-point drift.
		return std::fabs(a.x - b.x) < epsilon && std::fabs(a.y - b.y) < epsilon;
	}

	/**
	 * @brief Transforms a 3D point by a 4x4 matrix.
	 * @param matrix Matrix to apply.
	 * @param vector Point to transform.
	 * @return Transformed point.
	 */
	Vector3D Transform3D(Matrix4x4 const& matrix, Vector3D const& vector) {
		// Delegate to the matrix helper so all homogeneous point transforms stay consistent.
		return matrix.TransformPoint(vector);
	}

	/**
	 * @brief Computes the Euclidean distance between two 3D points.
	 * @param a First point.
	 * @param b Second point.
	 * @return Distance between both points.
	 */
	float Distance(Vector3D const& a, Vector3D const& b) {
		// Measure the vector separating both points before taking its magnitude.
		float dx = a.x - b.x;
		float dy = a.y - b.y;
		float dz = a.z - b.z;
		return std::sqrt(dx * dx + dy * dy + dz * dz);
	}

	/**
	 * @brief Linearly interpolates between two 3D vectors.
	 * @param a Start vector.
	 * @param b End vector.
	 * @param t Interpolation factor in the inclusive range `[0, 1]`.
	 * @return Interpolated vector.
	 */
	Vector3D Lerp(Vector3D const& a, Vector3D const& b, float t) {
		// Blend each component independently along the straight line between both vectors.
		return Vector3D(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
	}

	/**
	 * @brief Checks whether two 3D vectors differ by less than an epsilon.
	 * @param a First vector.
	 * @param b Second vector.
	 * @param epsilon Allowed absolute difference per component.
	 * @return `true` when all components are within the supplied tolerance.
	 */
	bool AlmostEqual(Vector3D const& a, Vector3D const& b, float epsilon) {
		// Compare each axis independently so callers can tolerate tiny floating-point drift.
		return std::fabs(a.x - b.x) < epsilon &&
			std::fabs(a.y - b.y) < epsilon &&
			std::fabs(a.z - b.z) < epsilon;
	}

	/**
	 * @brief Builds a perspective projection matrix.
	 * @param fovYDegrees Vertical field of view in degrees.
	 * @param aspect Viewport aspect ratio.
	 * @param nearZ Near clipping plane distance.
	 * @param farZ Far clipping plane distance.
	 * @return Perspective projection matrix.
	 */
	Matrix4x4 Matrix4x4::Perspective(float fovYDegrees, float aspect, float nearZ, float farZ) {
		// Convert the FOV into radians before computing the projection scale.
		float fovYRad = ToRadians(fovYDegrees);
		float f = 1.0f / std::tan(fovYRad / 2.0f);
		float nf = 1.0f / (nearZ - farZ);

		Matrix4x4 mat;
		// Populate the matrix in row-major order for the engine's transform helpers.
		mat.m[0] = f / aspect;
		mat.m[1] = 0.0f;
		mat.m[2] = 0.0f;
		mat.m[3] = 0.0f;

		mat.m[4] = 0.0f;
		mat.m[5] = f;
		mat.m[6] = 0.0f;
		mat.m[7] = 0.0f;

		mat.m[8] = 0.0f;
		mat.m[9] = 0.0f;
		mat.m[10] = (farZ + nearZ) * nf;
		mat.m[11] = (2.0f * farZ * nearZ) * nf;

		mat.m[12] = 0.0f;
		mat.m[13] = 0.0f;
		mat.m[14] = -1.0f;
		mat.m[15] = 0.0f;

		return mat;
	}

	/**
	 * @brief Builds an orthographic projection matrix.
	 * @param left Left clipping plane.
	 * @param right Right clipping plane.
	 * @param bottom Bottom clipping plane.
	 * @param top Top clipping plane.
	 * @param nearZ Near clipping plane distance.
	 * @param farZ Far clipping plane distance.
	 * @return Orthographic projection matrix.
	 */
	Matrix4x4 Matrix4x4::Orthographic(float left, float right, float bottom, float top, float nearZ, float farZ) {
		Matrix4x4 mat = Matrix4x4::IDENTITY;

		// Encode the orthographic scale and translation directly in the affine matrix.
		mat.m[0] = 2.0f / (right - left);
		mat.m[5] = 2.0f / (top - bottom);
		mat.m[10] = -2.0f / (farZ - nearZ);
		mat.m[3] = -(right + left) / (right - left);
		mat.m[7] = -(top + bottom) / (top - bottom);
		mat.m[11] = -(farZ + nearZ) / (farZ - nearZ);

		return mat;
	}

	/**
	 * @brief Builds a camera view matrix that looks from one point toward another.
	 * @param eye Camera position.
	 * @param target Point the camera should face.
	 * @param up Up direction used to orient the basis.
	 * @return View matrix built from the supplied basis vectors.
	 */
	Matrix4x4 Matrix4x4::LookAt(Vector3D const& eye, Vector3D const& target, Vector3D const& up) {
		// Derive the orthonormal camera basis from the requested eye, target, and up vectors.
		Vector3D f = (target - eye).Normalized();
		Vector3D s = f.Cross(up).Normalized();
		Vector3D u = s.Cross(f);

		Matrix4x4 mat = Matrix4x4::IDENTITY;
		// Store the basis vectors and translation terms in row-major order.
		mat.m[0] = s.x;
		mat.m[1] = s.y;
		mat.m[2] = s.z;
		mat.m[3] = -s.Dot(eye);

		mat.m[4] = u.x;
		mat.m[5] = u.y;
		mat.m[6] = u.z;
		mat.m[7] = -u.Dot(eye);

		mat.m[8] = -f.x;
		mat.m[9] = -f.y;
		mat.m[10] = -f.z;
		mat.m[11] = f.Dot(eye);

		mat.m[12] = 0.0f;
		mat.m[13] = 0.0f;
		mat.m[14] = 0.0f;
		mat.m[15] = 1.0f;

		return mat;
	}

	template <typename T>
	T Clamp(T value, T min, T max) {
		// Bound the incoming value without changing the requested limit ordering.
		return std::max(min, std::min(value, max));
	}

	template <typename T>
	bool AlmostEqual(T a, T b, T epsilon) {
		// Compare scalar drift against an absolute tolerance.
		return std::fabs(a - b) < epsilon;
	}

	/**
	 * @brief Converts a world-space position into screen-space coordinates.
	 * @param worldPos Position in world space.
	 * @param viewMatrix View transform applied before scaling to screen units.
	 * @param screenSize Pixel dimensions of the screen.
	 * @return Screen-space position.
	 */
	Vector2D WorldToScreen(Vector2D const& worldPos, Matrix3x3 const& viewMatrix, Vector2D const& screenSize) {
		// Transform into view space before scaling normalized coordinates into pixels.
		Vector2D viewPos = viewMatrix.TransformPoint(worldPos);
		return Vector2D(viewPos.x * screenSize.x, viewPos.y * screenSize.y);
	}

	/**
	 * @brief Converts a screen-space position back into world space.
	 * @param screenPos Position in screen pixels.
	 * @param invViewMatrix Inverse view transform used to reconstruct world space.
	 * @param screenSize Pixel dimensions of the screen.
	 * @return World-space position.
	 */
	Vector2D ScreenToWorld(Vector2D const& screenPos, Matrix3x3 const& invViewMatrix, Vector2D const& screenSize) {
		// Normalize incoming pixel coordinates before applying the inverse view transform.
		Vector2D normPos(screenPos.x / screenSize.x, screenPos.y / screenSize.y);
		return invViewMatrix.TransformPoint(normPos);
	}

	/**
	 * @brief Converts a screen-space position into normalized coordinates.
	 * @param screenPos Position in screen pixels.
	 * @param screenSize Pixel dimensions of the screen.
	 * @return Position remapped into the `[0, 1]` range.
	 */
	Vector2D ScreenToNormalized(Vector2D const& screenPos, Vector2D const& screenSize) {
		// Divide by the screen size to map pixel units into normalized space.
		return Vector2D(screenPos.x / screenSize.x, screenPos.y / screenSize.y);
	}

	/**
	 * @brief Converts normalized coordinates into screen-space pixels.
	 * @param normPos Position in the `[0, 1]` range.
	 * @param screenSize Pixel dimensions of the screen.
	 * @return Position remapped into screen pixels.
	 */
	Vector2D NormalizedToScreen(Vector2D const& normPos, Vector2D const& screenSize) {
		// Scale normalized coordinates back into the current framebuffer dimensions.
		return Vector2D(normPos.x * screenSize.x, normPos.y * screenSize.y);
	}
}

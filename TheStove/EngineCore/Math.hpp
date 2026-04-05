/*
----------------------------------------------------------------------------------------------------
 FILE NAME:			Math.hpp
 PROJECT NAME:		Project GAM200
 AUTHOR:			Ng Juin Herng, juinherng.ng@digipen.edu (85%)
 CO-AUTHOR:			Yat Chun Wee, y.chunwee@digipen.edu	    (15%)

 DESCRIPTION:		Math library providing basic vector and matrix operations and other
					utility functions.

		All content © 2026 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#pragma once

#include <cstddef>

namespace Math {
	/**
	 * @brief Represents a 2D vector with basic arithmetic helpers.
	 */
	class Vector2D {
	public:
		float x;
		float y;

		static const Vector2D ZERO;
		static const Vector2D ONE;

		/**
		 * @brief Initializes a vector from x and y components.
		 * @param x Horizontal component to store.
		 * @param y Vertical component to store.
		 */
		Vector2D(float const x = 0.0f, float const y = 0.0f);

		/**
		 * @brief Returns the Euclidean length of the vector.
		 * @return Magnitude of the vector.
		 */
		float Length() const;

		/**
		 * @brief Returns a unit-length copy of the vector when possible.
		 * @return Normalized vector, or `ZERO` when the input has no length.
		 */
		Vector2D Normalized() const;

		/**
		 * @brief Computes the dot product with another vector.
		 * @param other Vector to compare against.
		 * @return Dot product between both vectors.
		 */
		float Dot(Vector2D const& other) const;

		/**
		 * @brief Adds another vector component-wise.
		 * @param rhs Vector to add.
		 * @return Sum of both vectors.
		 */
		Vector2D operator+(Vector2D const& rhs) const;

		/**
		 * @brief Subtracts another vector component-wise.
		 * @param rhs Vector to subtract.
		 * @return Difference between both vectors.
		 */
		Vector2D operator-(Vector2D const& rhs) const;

		/**
		 * @brief Multiplies both components by a scalar.
		 * @param scalar Scalar value to apply.
		 * @return Scaled vector.
		 */
		Vector2D operator*(float const scalar) const;

		/**
		 * @brief Compares this vector against another for exact equality.
		 * @param rhs Vector to compare against.
		 * @return `true` when both components match exactly.
		 */
		bool operator==(Vector2D const& rhs) const;

		/**
		 * @brief Copies another vector into this one.
		 * @param rhs Source vector to copy from.
		 * @return Reference to this vector after assignment.
		 */
		Vector2D& operator=(Vector2D const& rhs);

		/**
		 * @brief Returns the x component.
		 * @return Stored horizontal component.
		 */
		float GetX() const {
			// Expose the stored horizontal coordinate directly.
			return x;
		}

		/**
		 * @brief Returns the y component.
		 * @return Stored vertical component.
		 */
		float GetY() const {
			// Expose the stored vertical coordinate directly.
			return y;
		}

		/**
		 * @brief Updates the x component.
		 * @param value New horizontal component.
		 */
		void SetX(float value) {
			// Persist the caller-provided horizontal value.
			x = value;
		}

		/**
		 * @brief Updates the y component.
		 * @param value New vertical component.
		 */
		void SetY(float value) {
			// Persist the caller-provided vertical value.
			y = value;
		}
	};

	/**
	 * @brief Represents a 3x3 matrix used for 2D transforms.
	 */
	class Matrix3x3 {
	public:
		static const Matrix3x3 IDENTITY;

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
		Matrix3x3(
			float const m00 = 1.0f, float const m01 = 0.0f, float const m02 = 0.0f,
			float const m10 = 0.0f, float const m11 = 1.0f, float const m12 = 0.0f,
			float const m20 = 0.0f, float const m21 = 0.0f, float const m22 = 1.0f);

		/**
		 * @brief Transforms a 2D point by this matrix.
		 * @param vector Point to transform.
		 * @return Transformed point.
		 */
		Vector2D TransformPoint(Vector2D const& vector) const;

		/**
		 * @brief Builds a translation matrix for a 2D offset.
		 * @param offset Translation amount to encode.
		 * @return Translation matrix.
		 */
		static Matrix3x3 Translate(Vector2D const& offset);

		/**
		 * @brief Builds a scaling matrix for a 2D scale factor.
		 * @param factors Per-axis scale factors.
		 * @return Scaling matrix.
		 */
		static Matrix3x3 Scale(Vector2D const& factors);

		/**
		 * @brief Builds a rotation matrix for a 2D angle.
		 * @param degrees Counter-clockwise rotation angle in degrees.
		 * @return Rotation matrix.
		 */
		static Matrix3x3 Rotate(float const degrees);

		/**
		 * @brief Multiplies this matrix by another transform.
		 * @param other Matrix to concatenate after this one.
		 * @return Product of this matrix and `other`.
		 */
		Matrix3x3 Concatenate(Matrix3x3 const& other) const;

		/**
		 * @brief Multiplies an ordered list of matrices into one result.
		 * @param matrices Pointer to the first matrix in the sequence.
		 * @param count Number of matrices to multiply.
		 * @return Combined matrix product.
		 */
		static Matrix3x3 Concatenate(Matrix3x3 const* matrices, std::size_t count);

		/**
		 * @brief Multiplies this matrix by another matrix.
		 * @param rhs Right-hand matrix operand.
		 * @return Matrix product.
		 */
		Matrix3x3 operator*(Matrix3x3 const& rhs) const;

		/**
		 * @brief Compares this matrix against another for exact equality.
		 * @param rhs Matrix to compare against.
		 * @return `true` when every stored element matches exactly.
		 */
		bool operator==(Matrix3x3 const& rhs) const;

		/**
		 * @brief Returns the inverse matrix when the determinant is non-zero.
		 * @return Inverse matrix, or `IDENTITY` when the matrix is singular.
		 */
		Matrix3x3 Inverse() const;

		/**
		 * @brief Returns a transposed copy of a matrix.
		 * @param mat Matrix to transpose.
		 * @return Transposed matrix.
		 */
		Matrix3x3 Transpose(Matrix3x3 const& mat);

	private:
		float m[9];
	};

	/**
	 * @brief Represents a 3D vector with basic arithmetic helpers.
	 */
	class Vector3D {
	public:
		float x;
		float y;
		float z;

		static const Vector3D ZERO;
		static const Vector3D ONE;

		/**
		 * @brief Initializes a vector from x, y, and z components.
		 * @param x X-axis component to store.
		 * @param y Y-axis component to store.
		 * @param z Z-axis component to store.
		 */
		Vector3D(float const x = 0.0f, float const y = 0.0f, float const z = 0.0f);

		/**
		 * @brief Returns the Euclidean length of the vector.
		 * @return Magnitude of the vector.
		 */
		float Length() const;

		/**
		 * @brief Returns a unit-length copy of the vector when possible.
		 * @return Normalized vector, or `ZERO` when the input has no length.
		 */
		Vector3D Normalized() const;

		/**
		 * @brief Computes the dot product with another vector.
		 * @param other Vector to compare against.
		 * @return Dot product between both vectors.
		 */
		float Dot(Vector3D const& other) const;

		/**
		 * @brief Computes the cross product with another vector.
		 * @param other Vector to compare against.
		 * @return Vector perpendicular to both inputs.
		 */
		Vector3D Cross(Vector3D const& other) const;

		/**
		 * @brief Adds another vector component-wise.
		 * @param rhs Vector to add.
		 * @return Sum of both vectors.
		 */
		Vector3D operator+(Vector3D const& rhs) const;

		/**
		 * @brief Subtracts another vector component-wise.
		 * @param rhs Vector to subtract.
		 * @return Difference between both vectors.
		 */
		Vector3D operator-(Vector3D const& rhs) const;

		/**
		 * @brief Multiplies each component by a scalar.
		 * @param scalar Scalar value to apply.
		 * @return Scaled vector.
		 */
		Vector3D operator*(float const scalar) const;

		/**
		 * @brief Compares this vector against another for exact equality.
		 * @param rhs Vector to compare against.
		 * @return `true` when every component matches exactly.
		 */
		bool operator==(Vector3D const& rhs) const;
	};

	/**
	 * @brief Represents a 4x4 matrix used for 3D transforms.
	 */
	class Matrix4x4 {
	public:
		static const Matrix4x4 IDENTITY;

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
		Matrix4x4(
			float const m00 = 1.0f, float const m01 = 0.0f, float const m02 = 0.0f, float const m03 = 0.0f,
			float const m10 = 0.0f, float const m11 = 1.0f, float const m12 = 0.0f, float const m13 = 0.0f,
			float const m20 = 0.0f, float const m21 = 0.0f, float const m22 = 1.0f, float const m23 = 0.0f,
			float const m30 = 0.0f, float const m31 = 0.0f, float const m32 = 0.0f, float const m33 = 1.0f);

		/**
		 * @brief Transforms a 3D point by this matrix.
		 * @param vector Point to transform.
		 * @return Transformed point after homogeneous divide when applicable.
		 */
		Vector3D TransformPoint(Vector3D const& vector) const;

		/**
		 * @brief Builds a translation matrix for a 3D offset.
		 * @param offset Translation amount to encode.
		 * @return Translation matrix.
		 */
		static Matrix4x4 Translate(Vector3D const& offset);

		/**
		 * @brief Builds a scaling matrix for a 3D scale factor.
		 * @param factors Per-axis scale factors.
		 * @return Scaling matrix.
		 */
		static Matrix4x4 Scale(Vector3D const& factors);

		/**
		 * @brief Builds a rotation matrix around the X axis.
		 * @param degrees Rotation angle in degrees.
		 * @return X-axis rotation matrix.
		 */
		static Matrix4x4 RotateX(float const degrees);

		/**
		 * @brief Builds a rotation matrix around the Y axis.
		 * @param degrees Rotation angle in degrees.
		 * @return Y-axis rotation matrix.
		 */
		static Matrix4x4 RotateY(float const degrees);

		/**
		 * @brief Builds a rotation matrix around the Z axis.
		 * @param degrees Rotation angle in degrees.
		 * @return Z-axis rotation matrix.
		 */
		static Matrix4x4 RotateZ(float const degrees);

		/**
		 * @brief Multiplies this matrix by another transform.
		 * @param other Matrix to concatenate after this one.
		 * @return Product of this matrix and `other`.
		 */
		Matrix4x4 Concatenate(Matrix4x4 const& other) const;

		/**
		 * @brief Multiplies an ordered list of matrices into one result.
		 * @param matrices Pointer to the first matrix in the sequence.
		 * @param count Number of matrices to multiply.
		 * @return Combined matrix product.
		 */
		static Matrix4x4 Concatenate(Matrix4x4 const* matrices, std::size_t count);

		/**
		 * @brief Multiplies this matrix by another matrix.
		 * @param rhs Right-hand matrix operand.
		 * @return Matrix product.
		 */
		Matrix4x4 operator*(Matrix4x4 const& rhs) const;

		/**
		 * @brief Compares this matrix against another for exact equality.
		 * @param rhs Matrix to compare against.
		 * @return `true` when every stored element matches exactly.
		 */
		bool operator==(Matrix4x4 const& rhs) const;

		/**
		 * @brief Returns a transposed copy of a matrix.
		 * @param mat Matrix to transpose.
		 * @return Transposed matrix.
		 */
		Matrix4x4 Transpose(Matrix4x4 const& mat);

		/**
		 * @brief Builds a perspective projection matrix.
		 * @param fovYDegrees Vertical field of view in degrees.
		 * @param aspect Viewport aspect ratio.
		 * @param nearZ Near clipping plane distance.
		 * @param farZ Far clipping plane distance.
		 * @return Perspective projection matrix.
		 */
		Matrix4x4 Perspective(float fovYDegrees, float aspect, float nearZ, float farZ);

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
		Matrix4x4 Orthographic(float left, float right, float bottom, float top, float nearZ, float farZ);

		/**
		 * @brief Builds a camera view matrix that looks from one point toward another.
		 * @param eye Camera position.
		 * @param target Point the camera should face.
		 * @param up Up direction used to orient the basis.
		 * @return View matrix built from the supplied basis vectors.
		 */
		Matrix4x4 LookAt(Vector3D const& eye, Vector3D const& target, Vector3D const& up);

	private:
		float m[16];
	};

	/**
	 * @brief Converts degrees to radians.
	 * @param degrees Angle in degrees.
	 * @return Equivalent angle in radians.
	 */
	float ToRadians(float const degrees);

	/**
	 * @brief Converts radians to degrees.
	 * @param radians Angle in radians.
	 * @return Equivalent angle in degrees.
	 */
	float ToDegrees(float const radians);

	/**
	 * @brief Transforms a 2D point by a 3x3 matrix.
	 * @param matrix Matrix to apply.
	 * @param vector Point to transform.
	 * @return Transformed point.
	 */
	Vector2D Transform2D(Matrix3x3 const& matrix, Vector2D const& vector);

	/**
	 * @brief Computes the Euclidean distance between two 2D points.
	 * @param a First point.
	 * @param b Second point.
	 * @return Distance between both points.
	 */
	float Distance(Vector2D const& a, Vector2D const& b);

	/**
	 * @brief Linearly interpolates between two 2D vectors.
	 * @param a Start vector.
	 * @param b End vector.
	 * @param t Interpolation factor in the inclusive range `[0, 1]`.
	 * @return Interpolated vector.
	 */
	Vector2D Lerp(Vector2D const& a, Vector2D const& b, float t);

	/**
	 * @brief Checks whether two 2D vectors differ by less than an epsilon.
	 * @param a First vector.
	 * @param b Second vector.
	 * @param epsilon Allowed absolute difference per component.
	 * @return `true` when both components are within the supplied tolerance.
	 */
	bool AlmostEqual(Vector2D const& a, Vector2D const& b, float epsilon = 1e-6f);

	/**
	 * @brief Transforms a 3D point by a 4x4 matrix.
	 * @param matrix Matrix to apply.
	 * @param vector Point to transform.
	 * @return Transformed point.
	 */
	Vector3D Transform3D(Matrix4x4 const& matrix, Vector3D const& vector);

	/**
	 * @brief Computes the Euclidean distance between two 3D points.
	 * @param a First point.
	 * @param b Second point.
	 * @return Distance between both points.
	 */
	float Distance(Vector3D const& a, Vector3D const& b);

	/**
	 * @brief Linearly interpolates between two 3D vectors.
	 * @param a Start vector.
	 * @param b End vector.
	 * @param t Interpolation factor in the inclusive range `[0, 1]`.
	 * @return Interpolated vector.
	 */
	Vector3D Lerp(Vector3D const& a, Vector3D const& b, float t);

	/**
	 * @brief Checks whether two 3D vectors differ by less than an epsilon.
	 * @param a First vector.
	 * @param b Second vector.
	 * @param epsilon Allowed absolute difference per component.
	 * @return `true` when all components are within the supplied tolerance.
	 */
	bool AlmostEqual(Vector3D const& a, Vector3D const& b, float epsilon = 1e-6f);

	/**
	 * @brief Restricts a value to a closed range.
	 * @tparam T Comparable value type.
	 * @param value Value to clamp.
	 * @param min Minimum accepted value.
	 * @param max Maximum accepted value.
	 * @return `value` limited to the `[min, max]` interval.
	 */
	template <typename T>
	T Clamp(T value, T min, T max);

	/**
	 * @brief Checks whether two scalar values differ by less than an epsilon.
	 * @tparam T Numeric value type.
	 * @param a First value.
	 * @param b Second value.
	 * @param epsilon Allowed absolute difference.
	 * @return `true` when the values are within the supplied tolerance.
	 */
	template <typename T>
	bool AlmostEqual(T a, T b, T epsilon = static_cast<T>(1e-6f));

	/**
	 * @brief Converts a world-space position into screen-space coordinates.
	 * @param worldPos Position in world space.
	 * @param viewMatrix View transform applied before scaling to screen units.
	 * @param screenSize Pixel dimensions of the screen.
	 * @return Screen-space position.
	 */
	Vector2D WorldToScreen(Vector2D const& worldPos, Matrix3x3 const& viewMatrix, Vector2D const& screenSize);

	/**
	 * @brief Converts a screen-space position back into world space.
	 * @param screenPos Position in screen pixels.
	 * @param invViewMatrix Inverse view transform used to reconstruct world space.
	 * @param screenSize Pixel dimensions of the screen.
	 * @return World-space position.
	 */
	Vector2D ScreenToWorld(Vector2D const& screenPos, Matrix3x3 const& invViewMatrix, Vector2D const& screenSize);

	/**
	 * @brief Converts a screen-space position into normalized coordinates.
	 * @param screenPos Position in screen pixels.
	 * @param screenSize Pixel dimensions of the screen.
	 * @return Position remapped into the `[0, 1]` range.
	 */
	Vector2D ScreenToNormalized(Vector2D const& screenPos, Vector2D const& screenSize);

	/**
	 * @brief Converts normalized coordinates into screen-space pixels.
	 * @param normPos Position in the `[0, 1]` range.
	 * @param screenSize Pixel dimensions of the screen.
	 * @return Position remapped into screen pixels.
	 */
	Vector2D NormalizedToScreen(Vector2D const& normPos, Vector2D const& screenSize);
}

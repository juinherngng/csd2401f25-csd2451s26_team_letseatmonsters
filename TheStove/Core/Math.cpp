/*
----------------------------------------------------------------------------------------------------
FILE NAME:			Math.cpp
PROJECT NAME:		Project GAM200
AUTHOR:				Ng Juin Herng, juinherng.ng@digipen.edu

DESCRIPTION:		Math library definitions.

        All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/

#include "Math.hpp"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace Math
{
    // Vector2D implementations
    Vector2D const Vector2D::ZERO{ 0.0f, 0.0f };
    Vector2D const Vector2D::ONE{ 1.0f, 1.0f };

    Vector2D::Vector2D(float const x, float const y) : x(x), y(y) {}

    float Vector2D::Length() const 
    {
        return std::sqrt(x * x + y * y);
    }

    Vector2D Vector2D::Normalized() const 
    {
        float length = Length();
        return (length > 0.0f) ? Vector2D(x / length, y / length) : Vector2D::ZERO;
    }

    float Vector2D::Dot(Vector2D const& other) const
    {
        return x * other.x + y * other.y;
    }

    Vector2D Vector2D::operator+(Vector2D const& rhs) const
    {
        return Vector2D(x + rhs.x, y + rhs.y);
    }

    Vector2D Vector2D::operator-(Vector2D const& rhs) const 
    {
        return Vector2D(x - rhs.x, y - rhs.y);
    }

    Vector2D Vector2D::operator*(float const scalar) const 
    {
        return Vector2D(x * scalar, y * scalar);
    }

    bool Vector2D::operator==(Vector2D const& rhs) const
    {
        return x == rhs.x && y == rhs.y;
    }

    // Matrix3x3 implementations
    const Matrix3x3 Matrix3x3::IDENTITY
    {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };

    Matrix3x3::Matrix3x3(
        float const m00, float const m01, float const m02,
        float const m10, float const m11, float const m12,
        float const m20, float const m21, float const m22)
        : m{ m00, m01, m02, m10, m11, m12, m20, m21, m22 } {}

    Matrix3x3 Matrix3x3::operator*(Matrix3x3 const& rhs) const 
    {
        Matrix3x3 result;
        for (int row = 0; row < 3; row++) 
        {
            for (int col = 0; col < 3; col++) 
            {
                result.m[row * 3 + col] =
                    m[row * 3 + 0] * rhs.m[0 * 3 + col] +
                    m[row * 3 + 1] * rhs.m[1 * 3 + col] +
                    m[row * 3 + 2] * rhs.m[2 * 3 + col];
            }
        }

        return result;
    }

    Vector2D Matrix3x3::TransformPoint(Vector2D const& vector) const
    {
        return Vector2D(
            m[0] * vector.x + m[1] * vector.y + m[2],
            m[3] * vector.x + m[4] * vector.y + m[5]);
    }

    Matrix3x3 Matrix3x3::Translate(Vector2D const& offset) 
    {
        // Translation matrix
        return Matrix3x3(
            1.0f, 0.0f, offset.x,
            0.0f, 1.0f, offset.y,
            0.0f, 0.0f, 1.0f);
    }

    Matrix3x3 Matrix3x3::Scale(Vector2D const& factors) 
    {
        // Scaling matrix
        return Matrix3x3(
            factors.x, 0.0f, 0.0f,
            0.0f, factors.y, 0.0f,
            0.0f, 0.0f, 1.0f);
    }

    Matrix3x3 Matrix3x3::Rotate(float const degrees) 
    {
        // Rotation matrix (counter-clockwise)
        float radians = ToRadians(degrees);
        float cosTheta = std::cos(radians);
        float sinTheta = std::sin(radians);

        return Matrix3x3(
            cosTheta, -sinTheta, 0.0f,
            sinTheta, cosTheta, 0.0f,
            0.0f, 0.0f, 1.0f);
    }

    Matrix3x3 Matrix3x3::Concatenate(Matrix3x3 const& other) const
    {
        // Equivalent to multiplying this * other
        return (*this) * other;
    }

    Matrix3x3 Matrix3x3::Concatenate(Matrix3x3 const* matrices, std::size_t count)
    {
        Matrix3x3 result = Matrix3x3::IDENTITY;

        for (std::size_t i = 0; i < count; i++) 
        {
            result = result * matrices[i];
        }

        return result;
    }

    bool Matrix3x3::operator==(Matrix3x3 const& rhs) const
    {
        for (int i = 0; i < 9; i++)
            if (m[i] != rhs.m[i]) return false;

        return true;
    }

    Matrix3x3 Matrix3x3::Inverse() const
    {
        // Compute the determinant
        float det =
            m[0] * (m[4] * m[8] - m[5] * m[7]) -
            m[1] * (m[3] * m[8] - m[5] * m[6]) +
            m[2] * (m[3] * m[7] - m[4] * m[6]);

        if (std::fabs(det) < 1e-6f) // Not invertible
            return Matrix3x3::IDENTITY;

        float invDet = 1.0f / det;

        Matrix3x3 inv;
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

    Matrix3x3 Matrix3x3::Transpose(Matrix3x3 const& mat)
    {
        return Matrix3x3(
            mat.m[0], mat.m[3], mat.m[6],
            mat.m[1], mat.m[4], mat.m[7],
            mat.m[2], mat.m[5], mat.m[8]
        );
    }

    // static constants for Vector3D
    Vector3D const Vector3D::ZERO{ 0.0f, 0.0f, 0.0f };
    Vector3D const Vector3D::ONE{ 1.0f, 1.0f, 1.0f };

    // Vector3D implementation
    Vector3D::Vector3D(float const x, float const y, float const z) : x(x), y(y), z(z) {}

    float Vector3D::Length() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    Vector3D Vector3D::Normalized() const
    {
        float len = Length();
        return (len > 0.0f) ? Vector3D(x / len, y / len, z / len) : Vector3D::ZERO;
    }

    float Vector3D::Dot(Vector3D const& other) const
    {
        return x * other.x + y * other.y + z * other.z;
    }

    Vector3D Vector3D::Cross(Vector3D const& other) const
    {
        return Vector3D(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    Vector3D Vector3D::operator+(Vector3D const& rhs) const
    {
        return Vector3D(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    Vector3D Vector3D::operator-(Vector3D const& rhs) const
    {
        return Vector3D(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    Vector3D Vector3D::operator*(float const scalar) const
    {
        return Vector3D(x * scalar, y * scalar, z * scalar);
    }

    bool Vector3D::operator==(Vector3D const& rhs) const
    {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }

    // static constant for Matrix4x4
    Matrix4x4 const Matrix4x4::IDENTITY
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    // Matrix4x4 implementation
    Matrix4x4::Matrix4x4(
        float const m00, float const m01, float const m02, float const m03,
        float const m10, float const m11, float const m12, float const m13,
        float const m20, float const m21, float const m22, float const m23,
        float const m30, float const m31, float const m32, float const m33)
        : m{ m00, m01, m02, m03,
             m10, m11, m12, m13,
             m20, m21, m22, m23,
             m30, m31, m32, m33 } {
    }

    // Non-member functions
    float ToRadians(float const degrees) 
    {
        return degrees * static_cast<float>(M_PI) / 180.0f;
    }

    float ToDegrees(float const radians) 
    {
        return radians * 180.0f / static_cast<float>(M_PI);
    }

	// Vector2D utility functions
    Vector2D Transform2D(Matrix3x3 const& matrix, Vector2D const& vector)
    {
        return matrix.TransformPoint(vector);
    }

    float Distance(Vector2D const& a, Vector2D const& b)
    {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    Vector2D Lerp(Vector2D const& a, Vector2D const& b, float t)
    {
        return Vector2D(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
    }

    bool AlmostEqual(Vector2D const& a, Vector2D const& b, float epsilon)
    {
        return std::fabs(a.x - b.x) < epsilon && std::fabs(a.y - b.y) < epsilon;
    }

	// Vector3D utility functions
    Vector3D Transform3D(Matrix4x4 const& matrix, Vector3D const& vector)
    {
        return matrix.TransformPoint(vector);
    }

    float Distance(Vector3D const& a, Vector3D const& b)
    {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        float dz = a.z - b.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    Vector3D Lerp(Vector3D const& a, Vector3D const& b, float t)
    {
        return Vector3D(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
    }

    bool AlmostEqual(Vector3D const& a, Vector3D const& b, float epsilon)
    {
        return std::fabs(a.x - b.x) < epsilon && std::fabs(a.y - b.y) < epsilon && std::fabs(a.z - b.z) < epsilon;
    }

    // General utility functions
    float Clamp(float value, float min, float max)
    {
        return std::max(min, std::min(value, max));
    }

    bool AlmostEqual(float a, float b, float epsilon)
    {
        return std::fabs(a - b) < epsilon;
    }

    // Conversion between screen, world, and normalized coordinates
    Vector2D WorldToScreen(Vector2D const& worldPos, Matrix3x3 const& viewMatrix, Vector2D const& screenSize)
    {
        // Transform world position to view space
        Vector2D viewPos = viewMatrix.TransformPoint(worldPos);

        // If viewPos is normalized (0..1), scale to screen size
        // Otherwise, if viewPos is already in pixel coordinates, just return it
        return Vector2D(viewPos.x * screenSize.x, viewPos.y * screenSize.y);
    }

    Vector2D ScreenToWorld(Vector2D const& screenPos, Matrix3x3 const& invViewMatrix, Vector2D const& screenSize) 
    {
        // Convert screen position to normalized coordinates
        Vector2D normPos(screenPos.x / screenSize.x, screenPos.y / screenSize.y);

        // Transform normalized position to world space
        return invViewMatrix.TransformPoint(normPos);
    }

    Vector2D ScreenToNormalized(Vector2D const& screenPos, Vector2D const& screenSize) 
    {
        // Map pixel coordinates to [0,1] range
        return Vector2D(screenPos.x / screenSize.x, screenPos.y / screenSize.y);
    }

    Vector2D NormalizedToScreen(Vector2D const& normPos, Vector2D const& screenSize) 
    {
        // Map [0,1] range to pixel coordinates
        return Vector2D(normPos.x * screenSize.x, normPos.y * screenSize.y);
    }
}
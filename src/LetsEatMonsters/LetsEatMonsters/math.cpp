#include "math.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

namespace Math
{
    // Vector2D implementations
    const Vector2D Vector2D::ZERO{ 0.0f, 0.0f };
    const Vector2D Vector2D::ONE{ 1.0f, 1.0f };

    Vector2D::Vector2D(const float x, const float y) : x(x), y(y) {}

    float Vector2D::Length() const {
        return std::sqrt(x * x + y * y);
    }

    Vector2D Vector2D::Normalized() const {
        float length = Length();
        return (length > 0.0f) ? Vector2D(x / length, y / length) : Vector2D::ZERO;
    }

    float Vector2D::Dot(const Vector2D& other) const {
        return x * other.x + y * other.y;
    }

    Vector2D Vector2D::operator+(const Vector2D& rhs) const {
        return Vector2D(x + rhs.x, y + rhs.y);
    }

    Vector2D Vector2D::operator-(const Vector2D& rhs) const {
        return Vector2D(x - rhs.x, y - rhs.y);
    }

    Vector2D Vector2D::operator*(const float scalar) const {
        return Vector2D(x * scalar, y * scalar);
    }

    bool Vector2D::operator==(const Vector2D& rhs) const {
        return x == rhs.x && y == rhs.y;
    }

    // Matrix3x3 implementations
    const Matrix3x3 Matrix3x3::IDENTITY{
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f};

    Matrix3x3::Matrix3x3(
        const float m00, const float m01, const float m02,
        const float m10, const float m11, const float m12,
        const float m20, const float m21, const float m22)
        : m{ m00, m01, m02, m10, m11, m12, m20, m21, m22 } {}

    Matrix3x3 Matrix3x3::operator*(const Matrix3x3& rhs) const {
        Matrix3x3 result;
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                result.m[row * 3 + col] =
                    m[row * 3 + 0] * rhs.m[0 * 3 + col] +
                    m[row * 3 + 1] * rhs.m[1 * 3 + col] +
                    m[row * 3 + 2] * rhs.m[2 * 3 + col];
            }
        }
        return result;
    }

    Vector2D Matrix3x3::TransformPoint(const Vector2D& vector) const {
        return Vector2D(
            m[0] * vector.x + m[1] * vector.y + m[2],
            m[3] * vector.x + m[4] * vector.y + m[5]);
    }

    Matrix3x3 Matrix3x3::Translate(const Vector2D& offset) {
        // Translation matrix
        return Matrix3x3(
            1.0f, 0.0f, offset.x,
            0.0f, 1.0f, offset.y,
            0.0f, 0.0f, 1.0f);
    }

    Matrix3x3 Matrix3x3::Scale(const Vector2D& factors) {
        // Scaling matrix
        return Matrix3x3(
            factors.x, 0.0f, 0.0f,
            0.0f, factors.y, 0.0f,
            0.0f, 0.0f, 1.0f);
    }

    Matrix3x3 Matrix3x3::Rotate(const float degrees) {
        // Rotation matrix (counter-clockwise)
        float radians = ToRadians(degrees);
        float cosTheta = std::cos(radians);
        float sinTheta = std::sin(radians);
        return Matrix3x3(
            cosTheta, -sinTheta, 0.0f,
            sinTheta, cosTheta, 0.0f,
            0.0f, 0.0f, 1.0f);
    }

    Matrix3x3 Matrix3x3::Concatenate(const Matrix3x3& other) const {
        // Equivalent to multiplying this * other
        return (*this) * other;
    }

    Matrix3x3 Matrix3x3::Concatenate(const Matrix3x3* matrices, std::size_t count) {
        Matrix3x3 result = Matrix3x3::IDENTITY;
        for (std::size_t i = 0; i < count; ++i) {
            result = result * matrices[i];
        }
        return result;
    }

    bool Matrix3x3::operator==(const Matrix3x3& rhs) const {
        for (int i = 0; i < 9; ++i)
            if (m[i] != rhs.m[i]) return false;
        return true;
    }

    // Non-member functions
    float ToRadians(const float degrees) {
        return degrees * static_cast<float>(M_PI) / 180.0f;
    }

    float ToDegrees(const float radians) {
        return radians * 180.0f / static_cast<float>(M_PI);
    }

    Vector2D Transform(const Matrix3x3& matrix, const Vector2D& vector) {
        return matrix.TransformPoint(vector);
    }

    Matrix3x3 Matrix3x3::Inverse() const {
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

    // Conversion between screen, world, and normalized coordinates
    Vector2D WorldToScreen(const Vector2D& worldPos, const Matrix3x3& viewMatrix, const Vector2D& screenSize) {
        // Transform world position to view space
        Vector2D viewPos = viewMatrix.TransformPoint(worldPos);

        // If viewPos is normalized (0..1), scale to screen size
        // Otherwise, if viewPos is already in pixel coordinates, just return it
        return Vector2D(viewPos.x * screenSize.x, viewPos.y * screenSize.y);
    }

    Vector2D ScreenToWorld(const Vector2D& screenPos, const Matrix3x3& invViewMatrix, const Vector2D& screenSize) {
        // Convert screen position to normalized coordinates
        Vector2D normPos(screenPos.x / screenSize.x, screenPos.y / screenSize.y);

        // Transform normalized position to world space
        return invViewMatrix.TransformPoint(normPos);
    }

    Vector2D ScreenToNormalized(const Vector2D& screenPos, const Vector2D& screenSize) {
        // Map pixel coordinates to [0,1] range
        return Vector2D(screenPos.x / screenSize.x, screenPos.y / screenSize.y);
    }

    Vector2D NormalizedToScreen(const Vector2D& normPos, const Vector2D& screenSize) {
        // Map [0,1] range to pixel coordinates
        return Vector2D(normPos.x * screenSize.x, normPos.y * screenSize.y);
    }
}
/*
----------------------------------------------------------------------------------------------------
FILE NAME:			Math.hpp
PROJECT NAME:		Project GAM200
AUTHOR:				Ng Juin Herng, juinherng.ng@digipen.edu

DESCRIPTION:		Math library providing basic vector and matrix operations.

        All content © 2025 DigiPen Institute of Technology Singapore. All rights reserved.
----------------------------------------------------------------------------------------------------
*/
#pragma once

#include <cstddef>

namespace Math
{
    class Vector2D {
    public:
        const float x;
        const float y;

        // Static constants
        static const Vector2D ZERO;
        static const Vector2D ONE;

        // Constructors
        Vector2D(const float x = 0.0f, const float y = 0.0f);

        // Member functions
        float    Length()                   const;
        Vector2D Normalized()               const;
        float    Dot(const Vector2D& other) const;

        // Operator overloads
        Vector2D operator+(const Vector2D& rhs)  const;
        Vector2D operator-(const Vector2D& rhs)  const;
        Vector2D operator*(const float scalar)   const;
        bool     operator==(const Vector2D& rhs) const;
    };

    class Matrix3x3 {
    public:
        // Static constant
        static const Matrix3x3 IDENTITY;

        // Constructors
        Matrix3x3(
            const float m00 = 1.0f, const float m01 = 0.0f, const float m02 = 0.0f,
            const float m10 = 0.0f, const float m11 = 1.0f, const float m12 = 0.0f,
            const float m20 = 0.0f, const float m21 = 0.0f, const float m22 = 1.0f);

        // Member functions
        Vector2D TransformPoint(const Vector2D& vector) const;
        static Matrix3x3 Translate(const Vector2D& offset);
        static Matrix3x3 Scale(const Vector2D& factors);
        static Matrix3x3 Rotate(const float degrees);

        // Concatenates this matrix with another and returns the result
        Matrix3x3 Concatenate(const Matrix3x3& other) const;

        // Static function to concatenate multiple matrices in order
        static Matrix3x3 Concatenate(const Matrix3x3* matrices, std::size_t count);

        // Operator overloads
        Matrix3x3 operator*(const Matrix3x3& rhs)  const;
        bool      operator==(const Matrix3x3& rhs) const;

        // Returns the inverse of this matrix. If not invertible, returns IDENTITY.
        Matrix3x3 Inverse() const;

    private:
        float m[9];
    };

    // Non-member utility functions
    float ToRadians(const float degrees);
    float ToDegrees(const float radians);
    Vector2D Transform(const Matrix3x3& matrix, const Vector2D& vector);

    // Converts world coordinates to screen coordinates
    Vector2D WorldToScreen(const Vector2D& worldPos, const Matrix3x3& viewMatrix, const Vector2D& screenSize);

    // Converts screen coordinates to world coordinates
    Vector2D ScreenToWorld(const Vector2D& screenPos, const Matrix3x3& invViewMatrix, const Vector2D& screenSize);

    // Converts screen coordinates to normalized coordinates
    Vector2D ScreenToNormalized(const Vector2D& screenPos, const Vector2D& screenSize);

    // Converts normalized coordinates to screen coordinates
    Vector2D NormalizedToScreen(const Vector2D& normPos, const Vector2D& screenSize);
}
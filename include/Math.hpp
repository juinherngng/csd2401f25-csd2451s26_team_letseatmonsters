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
        Vector2D(float const x = 0.0f, float const y = 0.0f);

        // Member functions
        float    Length()                   const;
        Vector2D Normalized()               const;
        float    Dot(Vector2D const& other) const;

        // Operator overloads
        Vector2D operator+(Vector2D const& rhs)  const;
        Vector2D operator-(Vector2D const& rhs)  const;
        Vector2D operator*(float const scalar)   const;
        bool     operator==(Vector2D const& rhs) const;
    };

    class Matrix3x3 {
    public:
        // Static constant
        static const Matrix3x3 IDENTITY;

        // Constructors
        Matrix3x3(
            float const m00 = 1.0f, float const m01 = 0.0f, float const m02 = 0.0f,
            float const m10 = 0.0f, float const m11 = 1.0f, float const m12 = 0.0f,
            float const m20 = 0.0f, float const m21 = 0.0f, float const m22 = 1.0f);

        // Member functions
        Vector2D TransformPoint(Vector2D const& vector) const;
        static Matrix3x3 Translate(Vector2D const& offset);
        static Matrix3x3 Scale(Vector2D const& factors);
        static Matrix3x3 Rotate(float const degrees);

        // Concatenates this matrix with another and returns the result
        Matrix3x3 Concatenate(Matrix3x3 const& other) const;

        // Static function to concatenate multiple matrices in order
        static Matrix3x3 Concatenate(Matrix3x3 const* matrices, std::size_t count);

        // Operator overloads
        Matrix3x3 operator*(Matrix3x3 const& rhs)  const;
        bool      operator==(Matrix3x3 const& rhs) const;

        // Returns the inverse of this matrix. If not invertible, returns IDENTITY.
        Matrix3x3 Inverse() const;

        Matrix3x3 Transpose(Matrix3x3 const& mat);

    private:
        float m[9];
    };

    class Vector3D {
    public:
        float const x;
        float const y;
        float const z;

        // Static constants
        static const Vector3D ZERO;
        static const Vector3D ONE;

        // Constructors
        Vector3D(float const x = 0.0f, float const y = 0.0f, float const z = 0.0f);

        // Member functions
        float    Length()                     const;
        Vector3D Normalized()                 const;
        float    Dot(Vector3D const& other)   const;
        Vector3D Cross(Vector3D const& other) const;

        // Operator overloads
        Vector3D operator+(Vector3D const& rhs)  const;
        Vector3D operator-(Vector3D const& rhs)  const;
        Vector3D operator*(float const scalar)   const;
        bool     operator==(Vector3D const& rhs) const;
    };

    class Matrix4x4 {
    public:
        // Static constant
        static const Matrix4x4 IDENTITY;

        // Constructors
        Matrix4x4(
            float const m00 = 1.0f, float const m01 = 0.0f, float const m02 = 0.0f, float const m03 = 0.0f,
            float const m10 = 0.0f, float const m11 = 1.0f, float const m12 = 0.0f, float const m13 = 0.0f,
            float const m20 = 0.0f, float const m21 = 0.0f, float const m22 = 1.0f, float const m23 = 0.0f,
            float const m30 = 0.0f, float const m31 = 0.0f, float const m32 = 0.0f, float const m33 = 1.0f);

        // Member functions
        Vector3D TransformPoint(Vector3D const& vector) const;
        static Matrix4x4 Translate(Vector3D const& offset);
        static Matrix4x4 Scale(Vector3D const& factors);
        static Matrix4x4 RotateX(float const degrees);
        static Matrix4x4 RotateY(float const degrees);
        static Matrix4x4 RotateZ(float const degrees);

        // Concatenates this matrix with another and returns the result
        Matrix4x4 Concatenate(Matrix4x4 const& other) const;

        // Static function to concatenate multiple matrices in order
        static Matrix4x4 Concatenate(Matrix4x4 const* matrices, std::size_t count);

        // Operator overloads
        Matrix4x4 operator*(Matrix4x4 const& rhs)  const;
        bool      operator==(Matrix4x4 const& rhs) const;

        // Returns the inverse of this matrix. If not invertible, returns IDENTITY.
        Matrix4x4 Inverse() const;

    private:
        float m[16];
    };

    // Non-member utility functions
    float ToRadians(float const degrees);
    float ToDegrees(float const radians);

    // Vector2D
    Vector2D Transform2D(Matrix3x3 const& matrix, Vector2D const& vector);
    float Distance(Vector2D const& a, Vector2D const& b);
    Vector2D Lerp(Vector2D const& a, Vector2D const& b, float t);
    bool AlmostEqual(Vector2D const& a, Vector2D const& b, float epsilon = 1e-6f);

    // Vector3D
    Vector3D Transform3D(Matrix4x4 const& matrix, Vector3D const& vector);
    float Distance(Vector3D const& a, Vector3D const& b);
    Vector3D Lerp(Vector3D const& a, Vector3D const& b, float t);
    bool AlmostEqual(Vector3D const& a, Vector3D const& b, float epsilon = 1e-6f);

    // Matrix4x4
    Matrix4x4 Perspective(float fovYDegrees, float aspect, float nearZ, float farZ);
    Matrix4x4 Orthographic(float left, float right, float bottom, float top, float nearZ, float farZ);
    Matrix4x4 LookAt(Vector3D const& eye, Vector3D const& target, Vector3D const& up);
    Matrix4x4 Transpose(Matrix4x4 const& mat);

    // General
    float Clamp(float value, float min, float max);
    bool AlmostEqual(float a, float b, float epsilon = 1e-6f);

    // Converts world coordinates to screen coordinates
    Vector2D WorldToScreen(Vector2D const& worldPos, Matrix3x3 const& viewMatrix, Vector2D const& screenSize);

    // Converts screen coordinates to world coordinates
    Vector2D ScreenToWorld(Vector2D const& screenPos, Matrix3x3 const& invViewMatrix, Vector2D const& screenSize);

    // Converts screen coordinates to normalized coordinates
    Vector2D ScreenToNormalized(Vector2D const& screenPos, Vector2D const& screenSize);

    // Converts normalized coordinates to screen coordinates
    Vector2D NormalizedToScreen(Vector2D const& normPos, Vector2D const& screenSize);
}
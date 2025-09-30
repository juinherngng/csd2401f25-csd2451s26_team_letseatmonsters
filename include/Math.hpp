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
    /************************************************************************/
    /*!
    \class Vector2D
    \brief
    2D vector with x and y components and basic vector operations.
    */
    /************************************************************************/
    class Vector2D {
    public:
        float x;
        float y;

        // Static constants
        static const Vector2D ZERO;
        static const Vector2D ONE;

        /************************************************************************/
        /*!
        \brief
        Constructs a Vector2D with given x and y values.
        \param x
        X component.
        \param y
        Y component.
        */
        /************************************************************************/
        Vector2D(float const x = 0.0f, float const y = 0.0f);

        // Member functions

        /************************************************************************/
        /*!
        \brief
        Returns the length (magnitude) of the vector.
        \return
        The length of the vector.
        */
        /************************************************************************/
        float    Length()                   const;
        /************************************************************************/
        /*!
        \brief
        Returns a normalized (unit length) version of the vector.
        \return
        Normalized vector.
        */
        /************************************************************************/
        Vector2D Normalized()               const;
        /************************************************************************/
        /*!
        \brief
        Computes the dot product with another vector.
        \param other
        The other vector.
        \return
        The dot product.
        */
        /************************************************************************/
        float    Dot(Vector2D const& other) const;

        // Operator overloads

        /************************************************************************/
        /*!
        \brief
        Adds two vectors.
        \param rhs
        The right-hand side vector.
        \return
        The sum vector.
        */
        /************************************************************************/
        Vector2D operator+(Vector2D const& rhs)  const;
        /************************************************************************/
        /*!
        \brief
        Subtracts two vectors.
        \param rhs
        The right-hand side vector.
        \return
        The difference vector.
        */
        /************************************************************************/
        Vector2D operator-(Vector2D const& rhs)  const;
        /************************************************************************/
        /*!
        \brief
        Multiplies the vector by a scalar.
        \param scalar
        The scalar value.
        \return
        The scaled vector.
        */
        /************************************************************************/
        Vector2D operator*(float const scalar)   const;
        /************************************************************************/
        /*!
        \brief
        Checks if two vectors are equal.
        \param rhs
        The right-hand side vector.
        \return
        True if equal, false otherwise.
        */
        /************************************************************************/
        bool     operator==(Vector2D const& rhs) const;

        /************************************************************************/
        /*!
        \brief
        Assigns the values of another Vector2D to this vector.
        \param rhs
        The right-hand side vector to assign from.
        \return
        Reference to this vector after assignment.
        */
        /************************************************************************/
        Vector2D& operator=(Vector2D const& rhs);

        // getter

        /************************************************************************/
        /*!
        \brief
        Gets the x component of the vector.
        \return
        The x component.
        */
        /************************************************************************/
        float GetX() const { return x; }
        /************************************************************************/
        /*!
        \brief
        Gets the y component of the vector.
        \return
        The y component.
        */
        /************************************************************************/
		float GetY() const { return y; }

        // setter

        /************************************************************************/
        /*!
        \brief
        Sets the x component of the vector.
        \param value
        The new x component value.
        */
        /************************************************************************/
		void SetX(float value) { x = value; }
        /************************************************************************/
        /*!
        \brief
        Sets the y component of the vector.
        \param value
        The new y component value.
        */
        /************************************************************************/
		void SetY(float value) { y = value; }
    };

    /************************************************************************/
    /*!
    \class Matrix3x3
    \brief
    3x3 matrix for 2D transformations.
    */
    /************************************************************************/
    class Matrix3x3 {
    public:
        // Static constant
        static const Matrix3x3 IDENTITY;

        /************************************************************************/
        /*!
        \brief
        Constructs a 3x3 matrix with given elements.
        */
        /************************************************************************/
        Matrix3x3(
            float const m00 = 1.0f, float const m01 = 0.0f, float const m02 = 0.0f,
            float const m10 = 0.0f, float const m11 = 1.0f, float const m12 = 0.0f,
            float const m20 = 0.0f, float const m21 = 0.0f, float const m22 = 1.0f);

        // Member functions

        /************************************************************************/
        /*!
        \brief
        Transforms a 2D vector by this matrix.
        \param vector
        The vector to transform.
        \return
        The transformed vector.
        */
        /************************************************************************/
        Vector2D TransformPoint(Vector2D const& vector) const;
        /************************************************************************/
        /*!
        \brief
        Creates a translation matrix.
        \param offset
        The translation offset.
        \return
        The translation matrix.
        */
        /************************************************************************/
        static Matrix3x3 Translate(Vector2D const& offset);
        /************************************************************************/
        /*!
        \brief
        Creates a scaling matrix.
        \param factors
        The scaling factors.
        \return
        The scaling matrix.
        */
        /************************************************************************/
        static Matrix3x3 Scale(Vector2D const& factors);
        /************************************************************************/
        /*!
        \brief
        Creates a rotation matrix.
        \param degrees
        The rotation angle in degrees.
        \return
        The rotation matrix.
        */
        /************************************************************************/
        static Matrix3x3 Rotate(float const degrees);

        /************************************************************************/
        /*!
        \brief
        Concatenates this matrix with another.
        \param other
        The other matrix.
        \return
        The concatenated matrix.
        */
        /************************************************************************/
        Matrix3x3 Concatenate(Matrix3x3 const& other) const;

        /************************************************************************/
        /*!
        \brief
        Concatenates multiple matrices in order.
        \param matrices
        Array of matrices.
        \param count
        Number of matrices.
        \return
        The concatenated matrix.
        */
        /************************************************************************/
        static Matrix3x3 Concatenate(Matrix3x3 const* matrices, std::size_t count);

        // Operator overloads

        /************************************************************************/
        /*!
        \brief
        Multiplies this matrix by another.
        \param rhs
        The right-hand side matrix.
        \return
        The product matrix.
        */
        /************************************************************************/
        Matrix3x3 operator*(Matrix3x3 const& rhs)  const;
        /************************************************************************/
        /*!
        \brief
        Checks if two matrices are equal.
        \param rhs
        The right-hand side matrix.
        \return
        True if equal, false otherwise.
        */
        /************************************************************************/
        bool      operator==(Matrix3x3 const& rhs) const;

        /************************************************************************/
        /*!
        \brief
        Returns the inverse of this matrix. If not invertible, returns IDENTITY.
        \return
        The inverse matrix.
        */
        /************************************************************************/
        Matrix3x3 Inverse() const;

        /************************************************************************/
        /*!
        \brief
        Returns the transpose of a matrix.
        \param mat
        The matrix to transpose.
        \return
        The transposed matrix.
        */
        /************************************************************************/
        Matrix3x3 Transpose(Matrix3x3 const& mat);

    private:
        float m[9];
    };

    /************************************************************************/
    /*!
    \class Vector3D
    \brief
    3D vector with x, y, z components and basic vector operations.
    */
    /************************************************************************/
    class Vector3D {
    public:
        float const x;
        float const y;
        float const z;

        // Static constants
        static const Vector3D ZERO;
        static const Vector3D ONE;

        /************************************************************************/
        /*!
        \brief
        Constructs a Vector3D with given x, y, z values.
        \param x
        X component.
        \param y
        Y component.
        \param z
        Z component.
        */
        /************************************************************************/
        Vector3D(float const x = 0.0f, float const y = 0.0f, float const z = 0.0f);

        // Member functions

        /************************************************************************/
        /*!
        \brief
        Returns the length (magnitude) of the vector.
        \return
        The length of the vector.
        */
        /************************************************************************/
        float    Length()                     const;
        /************************************************************************/
        /*!
        \brief
        Returns a normalized (unit length) version of the vector.
        \return
        Normalized vector.
        */
        /************************************************************************/
        Vector3D Normalized()                 const;
        /************************************************************************/
        /*!
        \brief
        Computes the dot product with another vector.
        \param other
        The other vector.
        \return
        The dot product.
        */
        /************************************************************************/
        float    Dot(Vector3D const& other)   const;
        /************************************************************************/
        /*!
        \brief
        Computes the cross product with another vector.
        \param other
        The other vector.
        \return
        The cross product vector.
        */
        /************************************************************************/
        Vector3D Cross(Vector3D const& other) const;

        // Operator overloads

        /************************************************************************/
        /*!
        \brief
        Adds two vectors.
        \param rhs
        The right-hand side vector.
        \return
        The sum vector.
        */
        /************************************************************************/
        Vector3D operator+(Vector3D const& rhs)  const;
        /************************************************************************/
        /*!
        \brief
        Subtracts two vectors.
        \param rhs
        The right-hand side vector.
        \return
        The difference vector.
        */
        /************************************************************************/
        Vector3D operator-(Vector3D const& rhs)  const;
        /************************************************************************/
        /*!
        \brief
        Multiplies the vector by a scalar.
        \param scalar
        The scalar value.
        \return
        The scaled vector.
        */
        /************************************************************************/
        Vector3D operator*(float const scalar)   const;
        /************************************************************************/
        /*!
        \brief
        Checks if two vectors are equal.
        \param rhs
        The right-hand side vector.
        \return
        True if equal, false otherwise.
        */
        /************************************************************************/
        bool     operator==(Vector3D const& rhs) const;
    };

    /************************************************************************/
    /*!
    \class Matrix4x4
    \brief
    4x4 matrix for 3D transformations.
    */
    /************************************************************************/
    class Matrix4x4 {
    public:
        // Static constant
        static const Matrix4x4 IDENTITY;

        /************************************************************************/
        /*!
        \brief
        Constructs a 4x4 matrix with given elements.
        */
        /************************************************************************/
        Matrix4x4(
            float const m00 = 1.0f, float const m01 = 0.0f, float const m02 = 0.0f, float const m03 = 0.0f,
            float const m10 = 0.0f, float const m11 = 1.0f, float const m12 = 0.0f, float const m13 = 0.0f,
            float const m20 = 0.0f, float const m21 = 0.0f, float const m22 = 1.0f, float const m23 = 0.0f,
            float const m30 = 0.0f, float const m31 = 0.0f, float const m32 = 0.0f, float const m33 = 1.0f);

        // Member functions

        /************************************************************************/
        /*!
        \brief
        Transforms a 3D vector by this matrix.
        \param vector
        The vector to transform.
        \return
        The transformed vector.
        */
        /************************************************************************/
        Vector3D TransformPoint(Vector3D const& vector) const;
        /************************************************************************/
        /*!
        \brief
        Creates a translation matrix.
        \param offset
        The translation offset.
        \return
        The translation matrix.
        */
        /************************************************************************/
        static Matrix4x4 Translate(Vector3D const& offset);
        /************************************************************************/
        /*!
        \brief
        Creates a scaling matrix.
        \param factors
        The scaling factors.
        \return
        The scaling matrix.
        */
        /************************************************************************/
        static Matrix4x4 Scale(Vector3D const& factors);
        /************************************************************************/
        /*!
        \brief
        Creates a rotation matrix around the X axis.
        \param degrees
        The rotation angle in degrees.
        \return
        The rotation matrix.
        */
        /************************************************************************/
        static Matrix4x4 RotateX(float const degrees);
        /************************************************************************/
        /*!
        \brief
        Creates a rotation matrix around the Y axis.
        \param degrees
        The rotation angle in degrees.
        \return
        The rotation matrix.
        */
        /************************************************************************/
        static Matrix4x4 RotateY(float const degrees);
        /************************************************************************/
        /*!
        \brief
        Creates a rotation matrix around the Z axis.
        \param degrees
        The rotation angle in degrees.
        \return
        The rotation matrix.
        */
        /************************************************************************/
        static Matrix4x4 RotateZ(float const degrees);

        /************************************************************************/
        /*!
        \brief
        Concatenates this matrix with another.
        \param other
        The other matrix.
        \return
        The concatenated matrix.
        */
        /************************************************************************/
        Matrix4x4 Concatenate(Matrix4x4 const& other) const;

        /************************************************************************/
        /*!
        \brief
        Concatenates multiple matrices in order.
        \param matrices
        Array of matrices.
        \param count
        Number of matrices.
        \return
        The concatenated matrix.
        */
        /************************************************************************/
        static Matrix4x4 Concatenate(Matrix4x4 const* matrices, std::size_t count);

        // Operator overloads

        /************************************************************************/
        /*!
        \brief
        Multiplies this matrix by another.
        \param rhs
        The right-hand side matrix.
        \return
        The product matrix.
        */
        /************************************************************************/
        Matrix4x4 operator*(Matrix4x4 const& rhs)  const;
        /************************************************************************/
        /*!
        \brief
        Checks if two matrices are equal.
        \param rhs
        The right-hand side matrix.
        \return
        True if equal, false otherwise.
        */
        /************************************************************************/
        bool      operator==(Matrix4x4 const& rhs) const;

        // Returns the inverse of this matrix. If not invertible, returns IDENTITY.
        //Matrix4x4 Inverse() const;

        /************************************************************************/
        /*!
        \brief
        Returns the transpose of a matrix.
        \param mat
        The matrix to transpose.
        \return
        The transposed matrix.
        */
        /************************************************************************/
        Matrix4x4 Transpose(Matrix4x4 const& mat);

		// Utility functions for common transformations

        /************************************************************************/
        /*!
        \brief
        Creates a perspective projection matrix.
        \param fovYDegrees
        Field of view in the Y direction, in degrees.
        \param aspect
        Aspect ratio.
        \param nearZ
        Near clipping plane.
        \param farZ
        Far clipping plane.
        \return
        The perspective projection matrix.
        */
        /************************************************************************/
        Matrix4x4 Perspective(float fovYDegrees, float aspect, float nearZ, float farZ);
        /************************************************************************/
        /*!
        \brief
        Creates an orthographic projection matrix.
        \param left
        Left plane.
        \param right
        Right plane.
        \param bottom
        Bottom plane.
        \param top
        Top plane.
        \param nearZ
        Near clipping plane.
        \param farZ
        Far clipping plane.
        \return
        The orthographic projection matrix.
        */
        /************************************************************************/
        Matrix4x4 Orthographic(float left, float right, float bottom, float top, float nearZ, float farZ);
        /************************************************************************/
        /*!
        \brief
        Creates a look-at view matrix.
        \param eye
        Camera position.
        \param target
        Target position.
        \param up
        Up direction.
        \return
        The look-at matrix.
        */
        /************************************************************************/
        Matrix4x4 LookAt(Vector3D const& eye, Vector3D const& target, Vector3D const& up);

    private:
        float m[16];
    };

    // Non-member utility functions

    /************************************************************************/
    /*!
    \brief
    Converts degrees to radians.
    \param degrees
    Angle in degrees.
    \return
    Angle in radians.
    */
    /************************************************************************/
    float ToRadians(float const degrees);
    /************************************************************************/
    /*!
    \brief
    Converts radians to degrees.
    \param radians
    Angle in radians.
    \return
    Angle in degrees.
    */
    /************************************************************************/
    float ToDegrees(float const radians);

    // Vector2D

    /************************************************************************/
    /*!
    \brief
    Transforms a 2D vector by a 3x3 matrix.
    \param matrix
    The transformation matrix.
    \param vector
    The vector to transform.
    \return
    The transformed vector.
    */
    /************************************************************************/
    Vector2D Transform2D(Matrix3x3 const& matrix, Vector2D const& vector);
    /************************************************************************/
    /*!
    \brief
    Calculates the distance between two 2D vectors.
    \param a
    First vector.
    \param b
    Second vector.
    \return
    The distance between a and b.
    */
    /************************************************************************/
    float Distance(Vector2D const& a, Vector2D const& b);
    /************************************************************************/
    /*!
    \brief
    Linearly interpolates between two 2D vectors.
    \param a
    Start vector.
    \param b
    End vector.
    \param t
    Interpolation factor (0.0 to 1.0).
    \return
    Interpolated vector.
    */
    /************************************************************************/
    Vector2D Lerp(Vector2D const& a, Vector2D const& b, float t);
    /************************************************************************/
    /*!
    \brief
    Checks if two 2D vectors are almost equal within a given epsilon.
    \param a
    First vector.
    \param b
    Second vector.
    \param epsilon
    Tolerance for equality.
    \return
    True if vectors are almost equal, false otherwise.
    */
    /************************************************************************/
    bool AlmostEqual(Vector2D const& a, Vector2D const& b, float epsilon = 1e-6f);

    // Vector3D

    /************************************************************************/
    /*!
    \brief
    Transforms a 3D vector by a 4x4 matrix.
    \param matrix
    The transformation matrix.
    \param vector
    The vector to transform.
    \return
    The transformed vector.
    */
    /************************************************************************/
    Vector3D Transform3D(Matrix4x4 const& matrix, Vector3D const& vector);
    /************************************************************************/
    /*!
    \brief
    Calculates the distance between two 3D vectors.
    \param a
    First vector.
    \param b
    Second vector.
    \return
    The distance between a and b.
    */
    /************************************************************************/
    float Distance(Vector3D const& a, Vector3D const& b);
    /************************************************************************/
    /*!
    \brief
    Linearly interpolates between two 3D vectors.
    \param a
    Start vector.
    \param b
    End vector.
    \param t
    Interpolation factor (0.0 to 1.0).
    \return
    Interpolated vector.
    */
    /************************************************************************/
    Vector3D Lerp(Vector3D const& a, Vector3D const& b, float t);
    /************************************************************************/
    /*!
    \brief
    Checks if two 3D vectors are almost equal within a given epsilon.
    \param a
    First vector.
    \param b
    Second vector.
    \param epsilon
    Tolerance for equality.
    \return
    True if vectors are almost equal, false otherwise.
    */
    /************************************************************************/
    bool AlmostEqual(Vector3D const& a, Vector3D const& b, float epsilon = 1e-6f);

    // General

    /************************************************************************/
    /*!
    \brief
    Clamps a value between a minimum and maximum.
    \tparam T
    Numeric type.
    \param value
    Value to clamp.
    \param min
    Minimum value.
    \param max
    Maximum value.
    \return
    Clamped value.
    */
    /************************************************************************/
    template <typename T>
    T Clamp(T value, T min, T max);
    /************************************************************************/
    /*!
    \brief
    Checks if two values are almost equal within a given epsilon.
    \tparam T
    Numeric type.
    \param a
    First value.
    \param b
    Second value.
    \param epsilon
    Tolerance for equality.
    \return
    True if values are almost equal, false otherwise.
    */
    /************************************************************************/
    template <typename T>
    bool AlmostEqual(T a, T b, T epsilon = 1e-6f);

    /************************************************************************/
    /*!
    \brief
    Converts world coordinates to screen coordinates.
    \param worldPos
    World position.
    \param viewMatrix
    View transformation matrix.
    \param screenSize
    Size of the screen.
    \return
    Screen coordinates.
    */
    /************************************************************************/
    Vector2D WorldToScreen(Vector2D const& worldPos, Matrix3x3 const& viewMatrix, Vector2D const& screenSize);

    /************************************************************************/
    /*!
    \brief
    Converts screen coordinates to world coordinates.
    \param screenPos
    Screen position.
    \param invViewMatrix
    Inverse view transformation matrix.
    \param screenSize
    Size of the screen.
    \return
    World coordinates.
    */
    /************************************************************************/
    Vector2D ScreenToWorld(Vector2D const& screenPos, Matrix3x3 const& invViewMatrix, Vector2D const& screenSize);

    /************************************************************************/
    /*!
    \brief
    Converts screen coordinates to normalized coordinates ([0,1] range).
    \param screenPos
    Screen position.
    \param screenSize
    Size of the screen.
    \return
    Normalized coordinates.
    */
    /************************************************************************/
    Vector2D ScreenToNormalized(Vector2D const& screenPos, Vector2D const& screenSize);

    /************************************************************************/
    /*!
    \brief
    Converts normalized coordinates ([0,1] range) to screen coordinates.
    \param normPos
    Normalized position.
    \param screenSize
    Size of the screen.
    \return
    Screen coordinates.
    */
    /************************************************************************/
    Vector2D NormalizedToScreen(Vector2D const& normPos, Vector2D const& screenSize);
}

// Example use cases

// Vector2D
/************************************************************************/
/*!
\brief
Example: Constructing and using Vector2D.

\code
#include "Math.hpp"

Math::Vector2D a(1.0f, 2.0f);
Math::Vector2D b = Math::Vector2D::ONE; // (1.0f, 1.0f)
float len = a.Length();
Math::Vector2D n = a.Normalized();
float dot = a.Dot(b);
Math::Vector2D sum = a + b;
Math::Vector2D diff = a - b;
Math::Vector2D scaled = a * 2.0f;
bool equal = (a == b);
\endcode
*/
/************************************************************************/

// Matrix3x3
/************************************************************************/
/*!
\brief
Example: Constructing and using Matrix3x3.

\code
#include "Math.hpp"

Math::Matrix3x3 identity = Math::Matrix3x3::IDENTITY;
Math::Matrix3x3 translation = Math::Matrix3x3::Translate(Math::Vector2D(5.0f, 10.0f));
Math::Matrix3x3 scale = Math::Matrix3x3::Scale(Math::Vector2D(2.0f, 2.0f));
Math::Matrix3x3 rotation = Math::Matrix3x3::Rotate(90.0f); // 90 degrees

Math::Vector2D v(1.0f, 0.0f);
Math::Vector2D transformed = translation.TransformPoint(v);

Math::Matrix3x3 combined = translation * rotation * scale;
\endcode
*/
/************************************************************************/

// Vector3D
/************************************************************************/
/*!
\brief
Example: Constructing and using Vector3D.

\code
#include "Math.hpp"

Math::Vector3D a(1.0f, 2.0f, 3.0f);
Math::Vector3D b = Math::Vector3D::ONE; // (1.0f, 1.0f, 1.0f)
float len = a.Length();
Math::Vector3D n = a.Normalized();
float dot = a.Dot(b);
Math::Vector3D cross = a.Cross(b);
Math::Vector3D sum = a + b;
Math::Vector3D diff = a - b;
Math::Vector3D scaled = a * 2.0f;
bool equal = (a == b);
\endcode
*/
/************************************************************************/

// Matrix4x4
/************************************************************************/
/*!
\brief
Example: Constructing and using Matrix4x4.

\code
#include "Math.hpp"

Math::Matrix4x4 identity = Math::Matrix4x4::IDENTITY;
Math::Matrix4x4 translation = Math::Matrix4x4::Translate(Math::Vector3D(1.0f, 2.0f, 3.0f));
Math::Matrix4x4 scale = Math::Matrix4x4::Scale(Math::Vector3D(2.0f, 2.0f, 2.0f));
Math::Matrix4x4 rotation = Math::Matrix4x4::RotateZ(45.0f);

Math::Vector3D v(1.0f, 0.0f, 0.0f);
Math::Vector3D transformed = translation.TransformPoint(v);

Math::Matrix4x4 combined = translation * rotation * scale;
\endcode
*/
/************************************************************************/

// Utility
/************************************************************************/
/*!
\brief
Example: Using Math utility functions.

\code
#include "Math.hpp"

// Angle conversion
float radians = Math::ToRadians(180.0f);   // PI
float degrees = Math::ToDegrees(3.14159f); // ~180

// Clamp and AlmostEqual
float clamped = Math::Clamp(5.0f, 0.0f, 1.0f); // 1.0f
bool  close   = Math::AlmostEqual(1.0f, 1.000001f);

// Lerp and Distance
Math::Vector2D a(0.0f, 0.0f), b(10.0f, 0.0f);
Math::Vector2D mid = Math::Lerp(a, b, 0.5f); // (5,0)
float dist = Math::Distance(a, b);           // 10.0f

// Coordinate conversions
Math::Vector2D screen = Math::WorldToScreen(a, Math::Matrix3x3::IDENTITY, Math::Vector2D(1920, 1080));
Math::Vector2D world = Math::ScreenToWorld(screen, Math::Matrix3x3::IDENTITY, Math::Vector2D(1920, 1080));
\endcode
*/
/************************************************************************/

// Transform component
/************************************************************************/
/*!
\brief
Example: Using Math types in a Transform component.

\code
#include "Math.hpp"
#include "GameComponent.hpp"

class Transform : public GameComponent 
{
public:
    Math::Vector2D position;
    float rotation; // in degrees
    Math::Vector2D scale;

    Transform()
        : position(Math::Vector2D::ZERO), rotation(0.0f), scale(Math::Vector2D::ONE) {}

    void Translate(const Math::Vector2D& delta) 
    {
        position = position + delta;
    }

    void Rotate(float deltaDegrees) 
    {
        rotation += deltaDegrees;
    }

    void Scale(const Math::Vector2D& factor) 
    {
        scale = Math::Vector2D(scale.x * factor.x, scale.y * factor.y);
    }

    Math::Matrix3x3 GetTransformMatrix() const 
    {
        Math::Matrix3x3 t = Math::Matrix3x3::Translate(position);
        Math::Matrix3x3 r = Math::Matrix3x3::Rotate(rotation);
        Math::Matrix3x3 s = Math::Matrix3x3::Scale(scale);
        return t * r * s;
    }
};
\endcode
*/
/************************************************************************/
/************************************************************************/
/*!
\brief
Example: Using Math 3D types in a 3D Transform component.

\code
#include "Math.hpp"
#include "GameComponent.hpp"

class Transform3D : public GameComponent
{
public:
    Math::Vector3D position;
    Math::Vector3D scale;
    float rotationY; // Yaw in degrees

    Transform3D()
        : position(Math::Vector3D::ZERO), scale(Math::Vector3D::ONE), rotationY(0.0f) {}

    Math::Matrix4x4 GetTransformMatrix() const 
    {
        return Math::Matrix4x4::Translate(position)
             * Math::Matrix4x4::RotateY(rotationY)
             * Math::Matrix4x4::Scale(scale);
    }
};
\endcode
*/
/************************************************************************/

// Physics component
/************************************************************************/
/*!
\brief
Example: Using Math types in a Physics/Velocity component.

\code
#include "Math.hpp"
#include "GameComponent.hpp"

class Velocity : public GameComponent 
{
public:
    Math::Vector2D velocity;

    Velocity() : velocity(Math::Vector2D::ZERO) {}

    void ApplyImpulse(const Math::Vector2D& impulse) 
    {
        velocity = velocity + impulse;
    }

    void Update(Transform& transform, float dt) 
    {
        // Move the transform by velocity * dt
        transform.position = transform.position + velocity * dt;
    }
};
\endcode
*/
/************************************************************************/

// Camera component
/************************************************************************/
/*!
\brief
Example: Using Math types in a Camera component.

\code
#include "Math.hpp"
#include "GameComponent.hpp"

class Camera : public GameComponent 
{
public:
    Math::Vector2D position;
    float zoom;

    Camera() : position(Math::Vector2D::ZERO), zoom(1.0f) {}

    Math::Matrix3x3 GetViewMatrix() const 
    {
        // Simple 2D camera: translate and scale
        return Math::Matrix3x3::Scale(Math::Vector2D(zoom, zoom))
             * Math::Matrix3x3::Translate(-position);
    }
};
\endcode
*/
/************************************************************************/

// Utility component
/************************************************************************/
/*!
\brief
Example: Using Math utility functions in a component.

\code
#include "Math.hpp"
#include "GameComponent.hpp"

class Projectile : public GameComponent 
{
public:
    Math::Vector2D position;
    Math::Vector2D target;

    void Update(float dt) 
    {
        Math::Vector2D direction = (target - position).Normalized();
        float speed = 10.0f;
        position = position + direction * speed * dt;
    }

    bool IsAtTarget() const 
    {
        return Math::AlmostEqual(position, target, 0.01f);
    }
};
\endcode
*/
/************************************************************************/
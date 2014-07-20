#ifndef VECTOR_H_INCLUDED
#define VECTOR_H_INCLUDED

#include <cmath>
#include <new>
#include <stdexcept>

static constexpr float eps = 1e-4;

//#define CAN_ACCESS_VECTOR_AS_ARRAY

struct Vector
{
#ifdef CAN_ACCESS_VECTOR_AS_ARRAY
    union
    {
        struct
        {
            float x, y, z;
        };
        float values[3];
    };
#else
    float x, y, z;
#endif
    constexpr Vector()
        : x(0), y(0), z(0)
    {
    }
    constexpr Vector(float v)
        : x(v), y(v), z(v)
    {
    }
    constexpr Vector(float x, float y, float z)
        : x(x), y(y), z(z)
    {
    }
    constexpr const Vector & operator +() const
    {
        return *this;
    }
    constexpr Vector operator -() const
    {
        return Vector(-x, -y, -z);
    }
    constexpr Vector operator +(const Vector & rt) const
    {
        return Vector(x + rt.x, y + rt.y, z + rt.z);
    }
    constexpr Vector operator -(const Vector & rt) const
    {
        return Vector(x - rt.x, y - rt.y, z - rt.z);
    }
    constexpr Vector operator *(const Vector & rt) const
    {
        return Vector(x * rt.x, y * rt.y, z * rt.z);
    }
    constexpr Vector operator /(const Vector & rt) const
    {
        return Vector(x / rt.x, y / rt.y, z / rt.z);
    }
    friend constexpr float dot(const Vector & a, const Vector & b);
#ifdef CAN_ACCESS_VECTOR_AS_ARRAY
    float * begin()
    {
        return &values[0];
    }
    float * end()
    {
        return &values[3];
    }
    constexpr const float * begin() const
    {
        return &values[0];
    }
    constexpr const float * end() const
    {
        return &values[3];
    }
    constexpr const float * cbegin() const
    {
        return &values[0];
    }
    constexpr const float * cend() const
    {
        return &values[3];
    }
    static constexpr size_t size()
    {
        return 3;
    }
#endif
    friend constexpr Vector cross(const Vector & a, const Vector & b);
    friend constexpr Vector operator *(float a, const Vector & b);
    friend constexpr Vector operator /(float a, const Vector & b);
    constexpr Vector operator *(float rt) const
    {
        return Vector(x * rt, y * rt, z * rt);
    }
    constexpr Vector operator /(float rt) const
    {
        return Vector(x / rt, y / rt, z / rt);
    }
    friend constexpr float absSquared(const Vector & v);
    friend constexpr float abs(const Vector & v);
    friend Vector normalize(const Vector & v);
    friend constexpr Vector normalize(const Vector & v, std::nothrow_t);
    const Vector & operator +=(Vector rt)
    {
        return operator =(operator +(rt));
    }
    const Vector & operator -=(Vector rt)
    {
        return operator =(operator -(rt));
    }
    const Vector & operator *=(Vector rt)
    {
        return operator =(operator *(rt));
    }
    const Vector & operator /=(Vector rt)
    {
        return operator =(operator /(rt));
    }
    const Vector & operator *=(float rt)
    {
        return operator =(operator *(rt));
    }
    const Vector & operator /=(float rt)
    {
        return operator =(operator /(rt));
    }
};

constexpr float dot(const Vector & a, const Vector & b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

constexpr Vector cross(const Vector & a, const Vector & b)
{
    return Vector(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

constexpr Vector operator *(float a, const Vector & b)
{
    return Vector(a * b.x, a * b.y, a * b.z);
}

constexpr Vector operator /(float a, const Vector & b)
{
    return Vector(a / b.x, a / b.y, a / b.z);
}

constexpr float absSquared(const Vector & v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

constexpr float abs(const Vector & v)
{
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

namespace
{
constexpr float normalizeNoThrowHelper(float v)
{
    return (v == 0) ? 1 : v;
}
}

inline Vector normalize(const Vector & v)
{
    float r = abs(v);
    if(r == 0)
        throw std::range_error("got <0, 0, 0> in normalize(Vector)");
    return v / r;
}

constexpr Vector normalizeNoThrow(const Vector & v)
{
    return v / normalizeNoThrowHelper(abs(v));
}

constexpr bool operator ==(Vector a, Vector b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

constexpr bool operator !=(Vector a, Vector b)
{
    return a.x != b.x || a.y != b.y || a.z != b.z;
}

#endif // VECTOR_H_INCLUDED

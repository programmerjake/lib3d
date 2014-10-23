#ifndef VECTOR_H_INCLUDED
#define VECTOR_H_INCLUDED

#include <cmath>
#include <new>
#include <stdexcept>

static constexpr float eps = 1e-4;

//#define CAN_ACCESS_VECTOR_AS_ARRAY

struct VectorF
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
    constexpr VectorF()
        : x(0), y(0), z(0)
    {
    }
    constexpr VectorF(float v)
        : x(v), y(v), z(v)
    {
    }
    constexpr VectorF(float x, float y, float z)
        : x(x), y(y), z(z)
    {
    }
    constexpr const VectorF & operator +() const
    {
        return *this;
    }
    constexpr VectorF operator -() const
    {
        return VectorF(-x, -y, -z);
    }
    constexpr VectorF operator +(const VectorF & rt) const
    {
        return VectorF(x + rt.x, y + rt.y, z + rt.z);
    }
    constexpr VectorF operator -(const VectorF & rt) const
    {
        return VectorF(x - rt.x, y - rt.y, z - rt.z);
    }
    constexpr VectorF operator *(const VectorF & rt) const
    {
        return VectorF(x * rt.x, y * rt.y, z * rt.z);
    }
    constexpr VectorF operator /(const VectorF & rt) const
    {
        return VectorF(x / rt.x, y / rt.y, z / rt.z);
    }
    friend constexpr float dot(const VectorF & a, const VectorF & b);
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
    friend constexpr VectorF cross(const VectorF & a, const VectorF & b);
    friend constexpr VectorF operator *(float a, const VectorF & b);
    friend constexpr VectorF operator /(float a, const VectorF & b);
    constexpr VectorF operator *(float rt) const
    {
        return VectorF(x * rt, y * rt, z * rt);
    }
    constexpr VectorF operator /(float rt) const
    {
        return VectorF(x / rt, y / rt, z / rt);
    }
    friend constexpr float absSquared(const VectorF & v);
    friend float abs(const VectorF & v);
    friend VectorF normalize(const VectorF & v);
    friend VectorF normalize(const VectorF & v, std::nothrow_t);
    const VectorF & operator +=(VectorF rt)
    {
        return operator =(operator +(rt));
    }
    const VectorF & operator -=(VectorF rt)
    {
        return operator =(operator -(rt));
    }
    const VectorF & operator *=(VectorF rt)
    {
        return operator =(operator *(rt));
    }
    const VectorF & operator /=(VectorF rt)
    {
        return operator =(operator /(rt));
    }
    const VectorF & operator *=(float rt)
    {
        return operator =(operator *(rt));
    }
    const VectorF & operator /=(float rt)
    {
        return operator =(operator /(rt));
    }
};

constexpr float dot(const VectorF & a, const VectorF & b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

constexpr VectorF cross(const VectorF & a, const VectorF & b)
{
    return VectorF(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

constexpr VectorF operator *(float a, const VectorF & b)
{
    return VectorF(a * b.x, a * b.y, a * b.z);
}

constexpr VectorF operator /(float a, const VectorF & b)
{
    return VectorF(a / b.x, a / b.y, a / b.z);
}

constexpr float absSquared(const VectorF & v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

inline float abs(const VectorF & v)
{
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

namespace
{
inline float normalizeNoThrowHelper(float v)
{
    return (v == 0) ? 1 : v;
}
}

inline VectorF normalize(const VectorF & v)
{
    float r = abs(v);
    if(r == 0)
        throw std::range_error("got <0, 0, 0> in normalize(VectorF)");
    return v / r;
}

inline VectorF normalizeNoThrow(const VectorF & v)
{
    return v / normalizeNoThrowHelper(abs(v));
}

constexpr bool operator ==(VectorF a, VectorF b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

constexpr bool operator !=(VectorF a, VectorF b)
{
    return a.x != b.x || a.y != b.y || a.z != b.z;
}

#endif // VECTOR_H_INCLUDED

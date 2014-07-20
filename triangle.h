#ifndef TRIANGLE_H_INCLUDED
#define TRIANGLE_H_INCLUDED

#include "vector.h"
#include "image.h"
#include "matrix.h"
#include <algorithm>

using namespace std;

struct TextureCoord
{
    float u, v;
    constexpr TextureCoord(float u, float v)
        : u(u), v(v)
    {
    }
    constexpr TextureCoord()
        : u(0), v(0)
    {
    }
};

struct Triangle
{
    TextureCoord t1, t2, t3;
    Vector p1, p2, p3;
    ColorF c1, c2, c3;
    Vector n1, n2, n3;
    constexpr Triangle(Vector p1, Vector p2, Vector p3, ColorF color = RGBAF(1, 1, 1, 1))
        : t1(0, 0), t2(0, 0), t3(0, 0), p1(p1), p2(p2), p3(p3), c1(color), c2(color), c3(color), n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    constexpr Triangle(Vector p1, TextureCoord t1, Vector p2, TextureCoord t2, Vector p3, TextureCoord t3, ColorF color = RGBAF(1, 1, 1, 1))
        : t1(t1), t2(t2), t3(t3), p1(p1), p2(p2), p3(p3), c1(color), c2(color), c3(color), n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    constexpr Triangle(Vector p1, ColorF c1, Vector p2, ColorF c2, Vector p3, ColorF c3)
        : t1(0, 0), t2(0, 0), t3(0, 0), p1(p1), p2(p2), p3(p3), c1(c1), c2(c2), c3(c3), n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    constexpr Triangle(Vector p1, TextureCoord t1, ColorF c1, Vector p2, TextureCoord t2, ColorF c2, Vector p3, TextureCoord t3, ColorF c3)
        : t1(t1), t2(t2), t3(t3), p1(p1), p2(p2), p3(p3), c1(c1), c2(c2), c3(c3), n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    constexpr Triangle(Vector p1, ColorF c1, TextureCoord t1, Vector p2, ColorF c2, TextureCoord t2, Vector p3, ColorF c3, TextureCoord t3)
        : t1(t1), t2(t2), t3(t3), p1(p1), p2(p2), p3(p3), c1(c1), c2(c2), c3(c3), n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    constexpr Triangle(Vector p1, Vector n1, Vector p2, Vector n2, Vector p3, Vector n3, ColorF color = RGBAF(1, 1, 1, 1))
        : t1(0, 0), t2(0, 0), t3(0, 0), p1(p1), p2(p2), p3(p3), c1(color), c2(color), c3(color), n1(n1), n2(n2), n3(n3)
    {
    }
    constexpr Triangle(Vector p1, TextureCoord t1, Vector n1, Vector p2, TextureCoord t2, Vector n2, Vector p3, TextureCoord t3, Vector n3, ColorF color = RGBAF(1, 1, 1, 1))
        : t1(t1), t2(t2), t3(t3), p1(p1), p2(p2), p3(p3), c1(color), c2(color), c3(color), n1(n1), n2(n2), n3(n3)
    {
    }
    constexpr Triangle(Vector p1, ColorF c1, Vector n1, Vector p2, ColorF c2, Vector n2, Vector p3, ColorF c3, Vector n3)
        : t1(0, 0), t2(0, 0), t3(0, 0), p1(p1), p2(p2), p3(p3), c1(c1), c2(c2), c3(c3), n1(n1), n2(n2), n3(n3)
    {
    }
    constexpr Triangle(Vector p1, TextureCoord t1, ColorF c1, Vector n1, Vector p2, TextureCoord t2, ColorF c2, Vector n2, Vector p3, TextureCoord t3, ColorF c3, Vector n3)
        : t1(t1), t2(t2), t3(t3), p1(p1), p2(p2), p3(p3), c1(c1), c2(c2), c3(c3), n1(n1), n2(n2), n3(n3)
    {
    }
    constexpr Triangle(Vector p1, ColorF c1, TextureCoord t1, Vector n1, Vector p2, ColorF c2, TextureCoord t2, Vector n2, Vector p3, ColorF c3, TextureCoord t3, Vector n3)
        : t1(t1), t2(t2), t3(t3), p1(p1), p2(p2), p3(p3), c1(c1), c2(c2), c3(c3), n1(n1), n2(n2), n3(n3)
    {
    }
    constexpr Triangle()
        : t1(0, 0), t2(0, 0), t3(0, 0), p1(0), p2(0), p3(0), c1(RGBAF(1, 1, 1, 1)), c2(RGBAF(1, 1, 1, 1)), c3(RGBAF(1, 1, 1, 1)), n1(0), n2(0), n3(0)
    {
    }
};

inline Triangle transform(const Matrix & m, const Triangle & t)
{
    return Triangle(transform(m, t.p1), t.t1, t.c1, transformNormal(m, t.n1),
                     transform(m, t.p2), t.t2, t.c2, transformNormal(m, t.n2),
                     transform(m, t.p3), t.t3, t.c3, transformNormal(m, t.n3));
}

constexpr Triangle colorize(ColorF color, const Triangle & t)
{
    return Triangle(t.p1, t.t1, colorize(color, t.c1), t.n1,
                     t.p2, t.t2, colorize(color, t.c2), t.n2,
                     t.p3, t.t3, colorize(color, t.c3), t.n3);
}

constexpr Triangle reverse(const Triangle & t)
{
    return Triangle(t.p1, t.t1, t.c1, t.n1, t.p3, t.t3, t.c3, t.n3, t.p2, t.t2, t.c2, t.n2);
}

#endif // TRIANGLE_H_INCLUDED

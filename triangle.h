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

constexpr TextureCoord interpolate(float v, TextureCoord a, TextureCoord b)
{
    return TextureCoord(interpolate(v, a.u, b.u), interpolate(v, a.v, b.v));
}

struct Vertex
{
    TextureCoord t;
    VectorF p;
    ColorF c;
    VectorF n;
    constexpr Vertex(VectorF p, TextureCoord t, ColorF c, VectorF n)
        : t(t), p(p), c(c), n(n)
    {
    }
};

constexpr Vertex interpolate(float v, Vertex a, Vertex b) // doesn't normalize
{
    return Vertex(interpolate(v, a.p, b.p), interpolate(v, a.t, b.t), interpolate(v, a.c, b.c), interpolate(v, a.n, b.n));
}

struct Triangle
{
    TextureCoord t1, t2, t3;
    VectorF p1, p2, p3;
    ColorF c1, c2, c3;
    VectorF n1, n2, n3;
    Triangle(VectorF p1, VectorF p2, VectorF p3, ColorF color = RGBAF(1, 1, 1, 1))
        : t1(0, 0), t2(0, 0), t3(0, 0), p1(p1), p2(p2), p3(p3), c1(color), c2(color), c3(color), n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    Triangle(VectorF p1, TextureCoord t1, VectorF p2, TextureCoord t2, VectorF p3, TextureCoord t3, ColorF color = RGBAF(1, 1, 1, 1))
        : t1(t1), t2(t2), t3(t3), p1(p1), p2(p2), p3(p3), c1(color), c2(color), c3(color), n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    Triangle(VectorF p1, ColorF c1, VectorF p2, ColorF c2, VectorF p3, ColorF c3)
        : t1(0, 0), t2(0, 0), t3(0, 0), p1(p1), p2(p2), p3(p3), c1(c1), c2(c2), c3(c3), n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    Triangle(VectorF p1, TextureCoord t1, ColorF c1, VectorF p2, TextureCoord t2, ColorF c2, VectorF p3, TextureCoord t3, ColorF c3)
        : t1(t1), t2(t2), t3(t3), p1(p1), p2(p2), p3(p3), c1(c1), c2(c2), c3(c3), n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    Triangle(VectorF p1, ColorF c1, TextureCoord t1, VectorF p2, ColorF c2, TextureCoord t2, VectorF p3, ColorF c3, TextureCoord t3)
        : t1(t1), t2(t2), t3(t3), p1(p1), p2(p2), p3(p3), c1(c1), c2(c2), c3(c3), n1(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n2(normalizeNoThrow(cross(p1 - p2, p1 - p3))), n3(normalizeNoThrow(cross(p1 - p2, p1 - p3)))
    {
    }
    constexpr Triangle(VectorF p1, VectorF n1, VectorF p2, VectorF n2, VectorF p3, VectorF n3, ColorF color = RGBAF(1, 1, 1, 1))
        : t1(0, 0), t2(0, 0), t3(0, 0), p1(p1), p2(p2), p3(p3), c1(color), c2(color), c3(color), n1(n1), n2(n2), n3(n3)
    {
    }
    constexpr Triangle(VectorF p1, TextureCoord t1, VectorF n1, VectorF p2, TextureCoord t2, VectorF n2, VectorF p3, TextureCoord t3, VectorF n3, ColorF color = RGBAF(1, 1, 1, 1))
        : t1(t1), t2(t2), t3(t3), p1(p1), p2(p2), p3(p3), c1(color), c2(color), c3(color), n1(n1), n2(n2), n3(n3)
    {
    }
    constexpr Triangle(VectorF p1, ColorF c1, VectorF n1, VectorF p2, ColorF c2, VectorF n2, VectorF p3, ColorF c3, VectorF n3)
        : t1(0, 0), t2(0, 0), t3(0, 0), p1(p1), p2(p2), p3(p3), c1(c1), c2(c2), c3(c3), n1(n1), n2(n2), n3(n3)
    {
    }
    constexpr Triangle(VectorF p1, TextureCoord t1, ColorF c1, VectorF n1, VectorF p2, TextureCoord t2, ColorF c2, VectorF n2, VectorF p3, TextureCoord t3, ColorF c3, VectorF n3)
        : t1(t1), t2(t2), t3(t3), p1(p1), p2(p2), p3(p3), c1(c1), c2(c2), c3(c3), n1(n1), n2(n2), n3(n3)
    {
    }
    constexpr Triangle(VectorF p1, ColorF c1, TextureCoord t1, VectorF n1, VectorF p2, ColorF c2, TextureCoord t2, VectorF n2, VectorF p3, ColorF c3, TextureCoord t3, VectorF n3)
        : t1(t1), t2(t2), t3(t3), p1(p1), p2(p2), p3(p3), c1(c1), c2(c2), c3(c3), n1(n1), n2(n2), n3(n3)
    {
    }
    constexpr Triangle()
        : t1(0, 0), t2(0, 0), t3(0, 0), p1(0), p2(0), p3(0), c1(RGBAF(1, 1, 1, 1)), c2(RGBAF(1, 1, 1, 1)), c3(RGBAF(1, 1, 1, 1)), n1(0), n2(0), n3(0)
    {
    }
    constexpr Triangle(Vertex v1_, Vertex v2_, Vertex v3_)
        : t1(v1_.t), t2(v2_.t), t3(v3_.t), p1(v1_.p), p2(v2_.p), p3(v3_.p), c1(v1_.c), c2(v2_.c), c3(v3_.c), n1(v1_.n), n2(v2_.n), n3(v3_.n)
    VectorF normal() const
    {
        return normalizeNoThrow(cross(p1 - p2, p1 - p3));
    }
    constexpr Vertex v1() const
    {
        return Vertex(p1, t1, c1, n1);
    }
    constexpr Vertex v2() const
    {
        return Vertex(p2, t2, c2, n2);
    }
    constexpr Vertex v3() const
    {
        return Vertex(p3, t3, c3, n3);
    }
};

inline Triangle transform(const Transform & m, const Triangle & t)
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
    return Triangle(t.p1, t.t1, t.c1, -t.n1, t.p3, t.t3, t.c3, -t.n3, t.p2, t.t2, t.c2, -t.n2);
}

struct CutTriangle final
{
    Triangle frontTriangles[2];
    size_t frontTriangleCount = 0;
    Triangle backTriangles[2];
    size_t backTriangleCount = 0;
private:
    static void triangulate(Triangle triangles[], size_t &triangleCount, const Vertex vertices[], size_t vertexCount)
    {
        triangleCount = 0;
        if(vertexCount >= 3)
        {
            triangleCount = vertexCount - 2;
            for(size_t i = 0, j = 1, k = 2; k < vertexCount; i++, j++, k++)
            {
                triangles[i] = Triangle(vertices[0], vertices[j], vertices[k]);
            }
        }
    }
public:
    CutTriangle(Triangle tri, VectorF planeNormal, float planeD)
    {
        const size_t triangleVertexCount = 3, triangleEdgeCount = 3, resultMaxVertexCount = triangleVertexCount + 1;
        Vertex triVertices[triangleVertexCount] = {tri.v1(), tri.v2(), tri.v3()};
        bool isFront[triangleVertexCount];
        for(size_t i = 0; i < triangleVertexCount; i++)
            isFront[i] = (dot(triVertices[i].p, planeNormal) >= -planeD);
        Vertex frontVertices[resultMaxVertexCount];
        size_t frontVertexCount = 0;
        Vertex backVertices[resultMaxVertexCount];
        size_t backVertexCount = 0;
        for(size_t i = 0, j = 1; i < triangleVertexCount; i++, j++, j %= triangleVertexCount)
        {
            if(isFront[i])
            {
                if(isFront[j])
                {
                    frontVertices[frontVertexCount++] = triVertices[i];
                }
                else
                {
                    frontVertices[frontVertexCount++] = triVertices[i];
                    float divisor = dot(triVertices[j].p - triVertices[i].p, planeNormal);
                    if(abs(divisor) >= eps * eps)
                    {
                        float t = (-planeD - dot(triVertices[i].p, planeNormal)) / divisor;
                        Vertex v = interpolate(t, triVertices[i], triVertices[j]);
                        frontVertices[frontVertexCount++] = v;
                        backVertices[backVertexCount++] = v;
                    }
                }
            }
            else
            {
                if(isFront[j])
                {
                    backVertices[backVertexCount++] = triVertices[i];
                    float divisor = dot(triVertices[j].p - triVertices[i].p, planeNormal);
                    if(abs(divisor) >= eps * eps)
                    {
                        float t = (-planeD - dot(triVertices[i].p, planeNormal)) / divisor;
                        Vertex v = interpolate(t, triVertices[i], triVertices[j]);
                        frontVertices[frontVertexCount++] = v;
                        backVertices[backVertexCount++] = v;
                    }
                }
                else
                {
                    backVertices[backVertexCount++] = triVertices[i];
                }
            }
        }
        triangulate(frontTriangles, frontTriangleCount, frontVertices, frontVertexCount);
        triangulate(backTriangles, backTriangleCount, backVertices, backVertexCount);
    }
};

inline CutTriangle cut(Triangle tri, VectorF planeNormal, float planeD)
{
    return CutTriangle(tri, planeNormal, planeD);
}

#endif // TRIANGLE_H_INCLUDED

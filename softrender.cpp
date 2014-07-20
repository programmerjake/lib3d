#include "softrender.h"
#include <utility>

using namespace std;

namespace
{
inline Vector gridify(Vector v)
{
    constexpr float gridSize = 1 / 32.0;
    constexpr float invGridSize = 1 / gridSize;
    return Vector(std::floor(v.x * invGridSize + 0.5) * gridSize, std::floor(v.y * invGridSize + 0.5) * gridSize, std::floor(v.z * invGridSize + 0.5) * gridSize);
}

inline Triangle gridify(Triangle tri)
{
    tri.p1 = gridify(tri.p1);
    tri.p2 = gridify(tri.p2);
    tri.p3 = gridify(tri.p3);
    return tri;
}
}

void SoftwareRenderer::renderTriangle(Triangle triangleIn, shared_ptr<Image> texture)
{
    triangles.push_back(TriangleDescriptor());
    TriangleDescriptor &triangle = triangles.back();
    triangle.texture = texture;
    Triangle &tri = triangle.tri;
    size_t w = image->w, h = image->h;
    float scale;
    if(w < h)
        scale = w * 0.5;
    else
        scale = h * 0.5;
    float centerX = 0.5 * w;
    float centerY = 0.5 * h;
    Matrix transformToScreen(scale, 0, -centerX, -0.5,
                             0, -scale, -centerY, -0.5,
                             0, 0, 1, 0);
    tri = transform(transformToScreen, gridify(triangleIn));
    if(tri.p1.z >= 0 && tri.p2.z >= 0 && tri.p3.z >= 0)
        return;
    PlaneEq plane = PlaneEq(tri);
    if(plane.d <= eps)
        return;
    plane.normal /= -plane.d;
    plane.d = -1;
    PlaneEq edge1 = PlaneEq(Vector(0), tri.p1, tri.p2);
    PlaneEq edge2 = PlaneEq(Vector(0), tri.p2, tri.p3);
    PlaneEq edge3 = PlaneEq(Vector(0), tri.p3, tri.p1);
    if(edge1.eval(tri.p3) < 0)
    {
        edge1.normal = -edge1.normal;
        //edge1.d = -edge1.d; // do nothing because d == 0
    }
    if(edge2.eval(tri.p1) < 0)
    {
        edge2.normal = -edge2.normal;
        //edge2.d = -edge2.d; // do nothing because d == 0
    }
    if(edge3.eval(tri.p2) < 0)
    {
        edge3.normal = -edge3.normal;
        //edge3.d = -edge3.d; // do nothing because d == 0
    }

    PlaneEq uEquation = PlaneEq(Vector(0), tri.p1, tri.p2);
    {
        float divisor = uEquation.eval(tri.p3);
        uEquation.normal /= divisor;
        // uEquation.d == 0
    }
    PlaneEq vEquation = PlaneEq(Vector(0), tri.p3, tri.p1);
    {
        float divisor = vEquation.eval(tri.p2);
        vEquation.normal /= divisor;
        // vEquation.d == 0
    }

    ColorI * texturePixels = texture->pixels;
    size_t textureW = texture->w;
    size_t textureH = texture->h;

    size_t startY = 0, endY = h - 1;
    if(tri.p1.z < -eps && tri.p2.z < -eps && tri.p3.z < -eps)
    {
        float y[3] = {-tri.p1.y / tri.p1.z, -tri.p2.y / tri.p2.z, -tri.p3.y / tri.p3.z};
        float minY, maxY;
        if(y[0] < y[1])
        {
            if(y[0] < y[2])
            {
                minY = y[0];
                if(y[1] < y[2])
                    maxY = y[2];
                else // y[1] >= y[2]
                    maxY = y[1];
            }
            else // y[0] >= y[2];
            {
                minY = y[2];
                maxY = y[1];
            }
        }
        else // y[0] >= y[1]
        {
            if(y[1] < y[2])
            {
                minY = y[1];
                if(y[0] < y[2])
                    maxY = y[2];
                else // y[0] >= y[2]
                    maxY = y[0];
            }
            else // y[1] >= y[2]
            {
                minY = y[2];
                maxY = y[0];
            }
        }
        if(maxY < startY || minY > endY)
            return;
        if(startY < minY)
        {
            startY = (size_t)std::ceil(minY);
        }
        if(endY > maxY)
        {
            endY = (size_t)std::floor(maxY);
        }
    }

    for(size_t y = startY; y <= endY; y++)
    {
        ColorI * imageLine = image->getLineAddress(y);
        Vector startPixelCoords = Vector(0, y, -1);
        Vector stepPixelCoords = Vector(1, 0, 0);
        float startInvZ = dot(plane.normal, startPixelCoords);
        float stepInvZ = dot(plane.normal, stepPixelCoords);
        float startEdge1V = dot(edge1.normal, startPixelCoords) + edge1.d * startInvZ;
        float stepEdge1V = dot(edge1.normal, stepPixelCoords) + edge1.d * stepInvZ;
        float startEdge2V = dot(edge2.normal, startPixelCoords) + edge2.d * startInvZ;
        float stepEdge2V = dot(edge2.normal, stepPixelCoords) + edge2.d * stepInvZ;
        float startEdge3V = dot(edge3.normal, startPixelCoords) + edge3.d * startInvZ;
        float stepEdge3V = dot(edge3.normal, stepPixelCoords) + edge3.d * stepInvZ;

        size_t startX = 0, endX = w - 1;

        if(stepEdge1V > 0) // startEdge
        {
            if(-startEdge1V >= (endX + 1) * stepEdge1V)
                continue;
            if(startX * stepEdge1V < -startEdge1V)
            {
                startX = (size_t)std::ceil(-startEdge1V / stepEdge1V);
            }
        }
        else // endEdge
        {
            if(startEdge1V < 0)
                continue;
            if(endX * stepEdge1V < -startEdge1V)
            {
                endX = (size_t)std::ceil(-startEdge1V / stepEdge1V) - 1;
            }
        }
        if(stepEdge2V > 0) // startEdge
        {
            if(-startEdge2V >= (endX + 1) * stepEdge2V)
                continue;
            if(startX * stepEdge2V < -startEdge2V)
            {
                startX = (size_t)std::ceil(-startEdge2V / stepEdge2V);
            }
        }
        else // endEdge
        {
            if(startEdge2V < 0)
                continue;
            if(endX * stepEdge2V < -startEdge2V)
            {
                endX = (size_t)std::ceil(-startEdge2V / stepEdge2V) - 1;
            }
        }
        if(stepEdge3V > 0) // startEdge
        {
            if(-startEdge3V >= (endX + 1) * stepEdge3V)
                continue;
            if(startX * stepEdge3V < -startEdge3V)
            {
                startX = (size_t)std::ceil(-startEdge3V / stepEdge3V);
            }
        }
        else // endEdge
        {
            if(startEdge3V < 0)
                continue;
            if(endX * stepEdge3V < -startEdge3V)
            {
                endX = (size_t)std::ceil(-startEdge3V / stepEdge3V) - 1;
            }
        }

        startPixelCoords += stepPixelCoords * startX;
        startInvZ += stepInvZ * startX;
        startEdge1V += stepEdge1V * startX;
        startEdge2V += stepEdge2V * startX;
        startEdge3V += stepEdge3V * startX;

        Vector pixelCoords = startPixelCoords;
        float invZ = startInvZ;
        for(size_t x = startX; x <= endX; x++, pixelCoords += stepPixelCoords, invZ += stepInvZ)
        {
            if(invZ < zBuffer[x + y * w])
                continue;

            Vector p = pixelCoords / invZ;

            Vector triPos = Vector(uEquation.eval(p), vEquation.eval(p), 1);
            Vector t1 = Vector(tri.t1.u, tri.t1.v, 1);
            Vector t2 = Vector(tri.t2.u, tri.t2.v, 1);
            Vector t3 = Vector(tri.t3.u, tri.t3.v, 1);
            float c1r = tri.c1.r;
            float c1g = tri.c1.g;
            float c1b = tri.c1.b;
            float c1a = tri.c1.a;
            float c2r = tri.c2.r;
            float c2g = tri.c2.g;
            float c2b = tri.c2.b;
            float c2a = tri.c2.a;
            float c3r = tri.c3.r;
            float c3g = tri.c3.g;
            float c3b = tri.c3.b;
            float c3a = tri.c3.a;
            Vector texturePos = triPos.x * (t3 - t1) + triPos.y * (t2 - t1) + t1;
            ColorF c = RGBAF(triPos.x * (c3r - c1r) + triPos.y * (c2r - c1r) + c1r,
                             triPos.x * (c3g - c1g) + triPos.y * (c2g - c1g) + c1g,
                             triPos.x * (c3b - c1b) + triPos.y * (c2b - c1b) + c1b,
                             triPos.x * (c3a - c1a) + triPos.y * (c2a - c1a) + c1a);

            texturePos.x -= std::floor(texturePos.x);
            texturePos.y -= std::floor(texturePos.y);

            texturePos.x *= textureW;
            texturePos.y *= textureH;

            size_t u = limit<size_t>((size_t)texturePos.x, 0, textureW);
            size_t v = limit<size_t>((size_t)texturePos.y, 0, textureH);

            ColorI fragmentColor = colorize(c, texturePixels[u + textureW * (textureH - v - 1)]);

            if(fragmentColor.a == 0)
                continue;

            if(writeDepth)
                zBuffer[x + y * w] = invZ;

            ColorI & pixel = imageLine[x];
            pixel = compose(fragmentColor, pixel);
        }
    }

#warning finish
}

void SoftwareRenderer::render(const Mesh &m)
{
    shared_ptr<Image> texture = m.image;

    if(texture == nullptr)
    {
        texture = whiteTexture;
    }

    for(Triangle tri : m.triangles)
    {
        renderTriangle(tri, texture);
    }
}

shared_ptr<Image> SoftwareRenderer::finish()
{
#warning finish
    return image;
}


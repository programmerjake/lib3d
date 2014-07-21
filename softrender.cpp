#include "softrender.h"
#include <utility>
#include <iostream>

using namespace std;

namespace
{
inline VectorF gridify(VectorF v)
{
    constexpr float gridSize = 1 / 128.0;
    constexpr float invGridSize = 1 / gridSize;
    return VectorF(std::floor(v.x * invGridSize + 0.5) * gridSize, std::floor(v.y * invGridSize + 0.5) * gridSize, std::floor(v.z * invGridSize + 0.5) * gridSize);
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
    if(plane.d >= -eps)
        return;
    plane.normal /= -plane.d;
    plane.d = -1;
    PlaneEq edge1 = PlaneEq(VectorF(0), tri.p1, tri.p2);
    PlaneEq edge2 = PlaneEq(VectorF(0), tri.p2, tri.p3);
    PlaneEq edge3 = PlaneEq(VectorF(0), tri.p3, tri.p1);
    edge1.d = 0; // assign to 0 because it helps optimization and it should already be 0
    edge2.d = 0; // assign to 0 because it helps optimization and it should already be 0
    edge3.d = 0; // assign to 0 because it helps optimization and it should already be 0

    PlaneEq uEquation = PlaneEq(VectorF(0), tri.p1, tri.p2);
    {
        float divisor = uEquation.eval(tri.p3);
        uEquation.normal /= divisor;
        // uEquation.d == 0
    }
    PlaneEq vEquation = PlaneEq(VectorF(0), tri.p3, tri.p1);
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
        VectorF startPixelCoords = VectorF(0, y, -1);
        VectorF stepPixelCoords = VectorF(1, 0, 0);
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
        else if(stepEdge1V == 0)
        {
            if(startEdge1V < 0)
                continue;
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
        else if(stepEdge2V == 0)
        {
            if(startEdge2V < 0)
                continue;
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
        else if(stepEdge3V == 0)
        {
            if(startEdge3V < 0)
                continue;
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

        if(startX == 0 || endX == w - 1)
        {
            cout << "\x1b[s\x1b[H\n\n\n\n" << w << "x" << h << " : Triangle(\x1b[K\n";
            cout << "VectorF(" << triangleIn.p1.x << ", " << triangleIn.p1.y << ", " << triangleIn.p1.z << "), TextureCoord(" << triangleIn.t1.u << ", " << triangleIn.t1.v << "), RGBAF(" << triangleIn.c1.r << ", " << triangleIn.c1.g << ", " << triangleIn.c1.b << ", " << triangleIn.c1.a << "), VectorF(" << triangleIn.n1.x << ", " << triangleIn.n1.y << ", " << triangleIn.n1.z << "),\x1b[K\n";
            cout << "VectorF(" << triangleIn.p2.x << ", " << triangleIn.p2.y << ", " << triangleIn.p2.z << "), TextureCoord(" << triangleIn.t2.u << ", " << triangleIn.t2.v << "), RGBAF(" << triangleIn.c2.r << ", " << triangleIn.c2.g << ", " << triangleIn.c2.b << ", " << triangleIn.c2.a << "), VectorF(" << triangleIn.n2.x << ", " << triangleIn.n2.y << ", " << triangleIn.n2.z << "),\x1b[K\n";
            cout << "VectorF(" << triangleIn.p3.x << ", " << triangleIn.p3.y << ", " << triangleIn.p3.z << "), TextureCoord(" << triangleIn.t3.u << ", " << triangleIn.t3.v << "), RGBAF(" << triangleIn.c3.r << ", " << triangleIn.c3.g << ", " << triangleIn.c3.b << ", " << triangleIn.c3.a << "), VectorF(" << triangleIn.n3.x << ", " << triangleIn.n3.y << ", " << triangleIn.n3.z << "))\x1b[K\x1b[u" << flush;
        }

        startPixelCoords += stepPixelCoords * startX;
        startInvZ += stepInvZ * startX;
        startEdge1V += stepEdge1V * startX;
        startEdge2V += stepEdge2V * startX;
        startEdge3V += stepEdge3V * startX;

        VectorF pixelCoords = startPixelCoords;
        float invZ = startInvZ;
        for(size_t x = startX; x <= endX; x++, pixelCoords += stepPixelCoords, invZ += stepInvZ)
        {
            if(invZ < zBuffer[x + y * w])
                continue;

            VectorF p = pixelCoords / invZ;

            VectorF triPos = VectorF(uEquation.eval(p), vEquation.eval(p), 1);
            VectorF t1 = VectorF(tri.t1.u, tri.t1.v, 1);
            VectorF t2 = VectorF(tri.t2.u, tri.t2.v, 1);
            VectorF t3 = VectorF(tri.t3.u, tri.t3.v, 1);
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
            VectorF texturePos = triPos.x * (t3 - t1) + triPos.y * (t2 - t1) + t1;
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


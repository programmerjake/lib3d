#ifndef SOFTRENDER_H_INCLUDED
#define SOFTRENDER_H_INCLUDED

#include "renderer.h"
#include <vector>
#include <thread>

using namespace std;

class SoftwareRenderer : public ImageRenderer
{
private:
    shared_ptr<Image> image;
    shared_ptr<const Image> whiteTexture;
    shared_ptr<ImageTexture> imageTexture;
    struct PlaneEq
    {
        VectorF normal;
        float d;
        PlaneEq(VectorF p1, VectorF p2, VectorF p3)
        {
            normal = cross(p1 - p2, p1 - p3);
            d = -dot(normal, p1);
        }
        PlaneEq(VectorF normal, float d)
        {
            this->normal = normal;
            this->d = d;
        }
        PlaneEq(VectorF normal, VectorF p)
        {
            this->normal = normal;
            this->d = -dot(normal, p);
        }
        PlaneEq(Triangle tri)
            : PlaneEq(tri.p1, tri.p2, tri.p3)
        {
        }
        float eval(VectorF p) const
        {
            return dot(p, normal) + d;
        }
    };
    struct TriangleDescriptor
    {
        Triangle tri;
        shared_ptr<const Image> texture;
    };
    vector<TriangleDescriptor> triangles;
    vector<float> zBuffer;
    vector<size_t> tBuffer;
    bool writeDepth = true;
    enum {NoTexture = ~(size_t)0};
    void renderTriangle(Triangle triangleIn, size_t sectionTop, size_t sectionBottom, shared_ptr<const Image> texture);
    float aspectRatio;
public:
    SoftwareRenderer(size_t w, size_t h, float aspectRatio = -1)
        : image(make_shared<Image>(w, h)), whiteTexture(make_shared<Image>(RGBI(0xFF, 0xFF, 0xFF))), aspectRatio(aspectRatio)
    {
        zBuffer.assign(w * h, (const float &)(float)0);
        tBuffer.assign(w * h, (const size_t &)NoTexture);
        imageTexture = make_shared<ImageTexture>(image);
    }
    virtual void render(const Mesh & m) override;
    virtual void calcScales() override
    {
        Renderer::calcScales(image->w, image->h, aspectRatio);
    }
protected:
    virtual void clearInternal(ColorF bg)
    {
        triangles.resize(0);
        image->clear((ColorI)bg);
        zBuffer.assign(image->w * image->h, (float)0);
        tBuffer.assign(image->w * image->h, NoTexture);
    }
public:
    virtual shared_ptr<Texture> finish() override;
    virtual void enableWriteDepth(bool v) override
    {
        writeDepth = v;
    }
    virtual void resize(size_t newW, size_t newH, float newAspectRatio = -1) override
    {
        image = make_shared<Image>(newW, newH);
        imageTexture = make_shared<ImageTexture>(image);
        zBuffer.assign(newW * newH, (const float &)(float)0);
        tBuffer.assign(newW * newH, (const size_t &)NoTexture);
        aspectRatio = newAspectRatio;
    }
};

#endif // SOFTRENDER_H_INCLUDED

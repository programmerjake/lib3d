#ifndef SOFTRENDER_H_INCLUDED
#define SOFTRENDER_H_INCLUDED

#include "renderer.h"
#include <vector>

using namespace std;

class SoftwareRenderer : public ImageRenderer
{
private:
    shared_ptr<Image> image;
    shared_ptr<Image> whiteTexture;
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
        shared_ptr<Image> texture;
    };
    vector<TriangleDescriptor> triangles;
    vector<float> zBuffer;
    vector<size_t> tBuffer;
    bool writeDepth = true;
    enum {NoTexture = ~(size_t)0};
    void renderTriangle(Triangle triangleIn, shared_ptr<Image> texture);
public:
    SoftwareRenderer(size_t w, size_t h)
        : image(make_shared<Image>(w, h)), whiteTexture(make_shared<Image>(RGBI(0xFF, 0xFF, 0xFF)))
    {
        zBuffer.assign(w * h, (const float &)(float)0);
        tBuffer.assign(w * h, (const size_t &)NoTexture);
    }
    virtual void render(const Mesh & m) override;
    virtual void calcScales() override
    {
        Renderer::calcScales(image->w, image->h);
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
    virtual shared_ptr<Image> finish() override;
    virtual void enableWriteDepth(bool v) override
    {
        writeDepth = v;
    }
    virtual void resize(size_t w, size_t h) override
    {
        image = make_shared<Image>(w, h);
        zBuffer.assign(w * h, (const float &)(float)0);
        tBuffer.assign(w * h, (const size_t &)NoTexture);
    }
};

#endif // SOFTRENDER_H_INCLUDED

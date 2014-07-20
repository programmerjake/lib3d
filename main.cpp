#include "renderer.h"
#include "generate.h"
#include <cmath>
#include <random>

using namespace std;

template <typename T>
shared_ptr<Mesh> makeRandomMesh(T & randomGenerator, size_t count)
{
    std::uniform_real_distribution<float> pointDistribution(-10, 10), colorDistribution(0, 1);
    vector<Triangle> triangles;
    triangles.reserve(count);
    for(size_t i = 0; i < count; i++)
    {
        float x1 = pointDistribution(randomGenerator);
        float y1 = pointDistribution(randomGenerator);
        float z1 = pointDistribution(randomGenerator);
        float x2 = pointDistribution(randomGenerator);
        float y2 = pointDistribution(randomGenerator);
        float z2 = pointDistribution(randomGenerator);
        float x3 = pointDistribution(randomGenerator);
        float y3 = pointDistribution(randomGenerator);
        float z3 = pointDistribution(randomGenerator);
        VectorF p1 = VectorF(x1, y1, z1);
        VectorF p2 = VectorF(x2, y2, z2);
        VectorF p3 = VectorF(x3, y3, z3);
        ColorF c1 = HSBF(colorDistribution(randomGenerator), 1, 0.5);
        ColorF c2 = HSBF(colorDistribution(randomGenerator), 1, 0.5);
        ColorF c3 = HSBF(colorDistribution(randomGenerator), 1, 0.5);
        triangles.push_back(Triangle(p1, c1, p2, c2, p3, c3));
    }
    return make_shared<Mesh>(std::move(triangles));
}

VectorF sphericalCoordinates(float r, float theta, float phi)
{
    return Matrix::rotateZ(-phi).concat(Matrix::rotateY(theta)).apply(VectorF(r, 0, 0));
}

shared_ptr<Mesh> makeSphereMesh(size_t uCount, size_t vCount, float r, shared_ptr<Image> image = nullptr, ColorF color = RGBAF(1, 1, 1, 1))
{
    vector<Triangle> triangles;
    triangles.reserve(uCount * vCount * 2);
    for(size_t ui = 0; ui < uCount; ui++)
    {
        for(size_t vi = 0; vi < vCount; vi++)
        {
            VectorF p00 = sphericalCoordinates(1, (float)(ui + 0) / uCount * 2 * M_PI, (float)(vi + 0) / vCount * M_PI - M_PI / 2);
            VectorF p10 = sphericalCoordinates(1, (float)(ui + 1) / uCount * 2 * M_PI, (float)(vi + 0) / vCount * M_PI - M_PI / 2);
            VectorF p01 = sphericalCoordinates(1, (float)(ui + 0) / uCount * 2 * M_PI, (float)(vi + 1) / vCount * M_PI - M_PI / 2);
            VectorF p11 = sphericalCoordinates(1, (float)(ui + 1) / uCount * 2 * M_PI, (float)(vi + 1) / vCount * M_PI - M_PI / 2);
            TextureCoord t00 = TextureCoord((float)(ui + 0) / uCount, (float)(vi + 0) / vCount);
            TextureCoord t10 = TextureCoord((float)(ui + 1) / uCount, (float)(vi + 0) / vCount);
            TextureCoord t01 = TextureCoord((float)(ui + 0) / uCount, (float)(vi + 1) / vCount);
            TextureCoord t11 = TextureCoord((float)(ui + 1) / uCount, (float)(vi + 1) / vCount);
            triangles.push_back(Triangle(p00 * r, t00, p00, p01 * r, t01, p01, p11 * r, t11, p11, color));
            triangles.push_back(Triangle(p00 * r, t00, p00, p11 * r, t11, p11, p10 * r, t10, p10, color));
        }
    }
    return make_shared<Mesh>(triangles, image);
}

inline Mesh shadeMesh(const Mesh & meshIn, VectorF lightDir)
{
    lightDir = normalize(lightDir);
    vector<Triangle> triangles;
    triangles.reserve(meshIn.triangles.size());
    for(Triangle tri : meshIn.triangles)
    {
        if(tri.n1 == VectorF(0) || tri.n2 == VectorF(0) || tri.n3 == VectorF(0))
        {
            triangles.push_back(tri);
            continue;
        }
        tri.c1 = scaleF(tri.c1, std::max<float>(0, dot(tri.n1, lightDir)) * 0.65 + 0.35);
        tri.c2 = scaleF(tri.c2, std::max<float>(0, dot(tri.n2, lightDir)) * 0.65 + 0.35);
        tri.c3 = scaleF(tri.c3, std::max<float>(0, dot(tri.n3, lightDir)) * 0.65 + 0.35);
        triangles.push_back(tri);
    }
    return Mesh(std::move(triangles), meshIn.image);
}

int main()
{
    shared_ptr<WindowRenderer> renderer = getWindowRenderer();
    minstd_rand0 randomGenerator;
    shared_ptr<Mesh> m = makeRandomMesh(randomGenerator, 5);
    m->append(reverse(*m));
    shared_ptr<Mesh> m2 = makeSphereMesh(32, 32, 5);
    TextureDescriptor td(make_shared<Image>());
    if(false)
        m2 = (shared_ptr<Mesh>)transform(Matrix::translate(VectorF(-0.5)).concat(Matrix::scale(2 * 3.5)), Generate::unitBox(td, td, td, td, td, td));
    double startTime = timer();
    shared_ptr<ImageRenderer> imageRenderer = makeImageRenderer(256, 256);
    while(true)
    {
        double time = timer();
        Matrix tform = (Matrix::rotateY((time - startTime) / 2 * M_PI)).concat(Matrix::translate(0, 0, -19));
        renderer->clear();
#if 1
        imageRenderer->clear(RGBF(1, 1, 1));
        imageRenderer->render((Mesh)transform(tform, m));
        shared_ptr<Image> image = imageRenderer->finish();
        tform = (Matrix::rotateY((time - startTime) / 5 * M_PI)).concat(Matrix::rotateX((time - startTime) / 15 * M_PI)).concat(Matrix::translate(0, 0, -10));
        m2->image = image;
        renderer->render(shadeMesh(transform(tform, m2), VectorF(1, 1, 1)));
#else
        renderer->render(shadeMesh(transform(tform, m), VectorF(1, 1, 1)));
#endif
        renderer->flip();
    }
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "renderer.h"
#include "generate.h"
#include <cmath>
#include <random>
#include <iostream>
#include <sstream>
#include <string>
#include <cwchar>
#include "model.h"
#include "bsp_tree.h"

using namespace std;

template <typename T>
shared_ptr<Mesh> makeRandomMesh(T &randomGenerator, size_t count)
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
    return Matrix::rotateZ(phi).concat(Matrix::rotateY(theta)).apply(VectorF(r, 0, 0));
}

Mesh makeSphereMesh(size_t uCount, size_t vCount, float r, shared_ptr<Texture> image = nullptr, ColorF color = RGBAF(1, 1, 1, 1))
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
            if(vi > 0)
                triangles.push_back(Triangle(p00 * r, t00, p00, p10 * r, t10, p10, p11 * r, t11, p11, color));
            if(vi + 1 < vCount)
                triangles.push_back(Triangle(p00 * r, t00, p00, p11 * r, t11, p11, p01 * r, t01, p01, color));
        }
    }

    return Mesh(std::move(triangles), image);
}

Mesh makeCylinderMesh(size_t sideCount, float r, float length, ColorF color = RGBAF(1, 1, 1, 1))
{
    Mesh retval;
    retval.reserve(sideCount * 4);
    Vertex end0 = Vertex(VectorF(-r, -length, 0), TextureCoord(0, 0), color, VectorF(0, -1, 0));
    Vertex end1 = Vertex(VectorF(-r, length, 0), TextureCoord(0, 0), color, VectorF(0, 1, 0));
    Vertex side0 = Vertex(VectorF(-r, -length, 0), TextureCoord(0, 0), color, VectorF(-1, 0, 0));
    Vertex side1 = Vertex(VectorF(-r, length, 0), TextureCoord(0, 0), color, VectorF(-1, 0, 0));
    vector<Vertex> end0Vertices, end1Vertices;
    end0Vertices.reserve(sideCount);
    end1Vertices.reserve(sideCount);
    for(size_t i = 0; i < sideCount; i++)
    {
        Transform tform0 = Matrix::rotateY((float)(i + 0) / sideCount * 2 * M_PI);
        Transform tform1 = Matrix::rotateY((float)(i + 1) / sideCount * 2 * M_PI);
        Vertex vs00 = transform(tform0, side0);
        Vertex vs01 = transform(tform0, side1);
        Vertex vs10 = transform(tform1, side0);
        Vertex vs11 = transform(tform1, side1);
        retval.append(Triangle(vs00, vs11, vs01));
        retval.append(Triangle(vs00, vs10, vs11));
        end0Vertices.push_back(transform(tform0, end0));
        end1Vertices.push_back(transform(tform0, end1));
    }
    reverse(end0Vertices.begin(), end0Vertices.end());
    retval.append(Generate::convexPolygon(nullptr, end0Vertices));
    retval.append(Generate::convexPolygon(nullptr, end1Vertices));
    return std::move(retval);
}

inline ColorF shadeFn(ColorF vColor, VectorF vNormal, VectorF vPosition)
{
    const VectorF lightVector = normalizeNoThrow(VectorF(1, 1, 1));
    const VectorF specularLightVector = normalizeNoThrow(lightVector + VectorF(0, 0, 1));
    float v = 0.3;
    v += max<float>(0, dot(vNormal, lightVector)) * 0.4;
    float s = max<float>(0, dot(vNormal, specularLightVector));
    s *= s;
    s *= s;
    s *= s;
    s *= s;
    v += s * 0;
    return scaleF(min<float>(1, v), vColor);
}

struct SetColorShadeFn
{
    ColorF color;
    constexpr SetColorShadeFn(ColorF color)
        : color(color)
    {
    }
    ColorF operator ()(ColorF, VectorF, VectorF) const
    {
        return color;
    }
};

int main(int argc, char **argv)
{
    static shared_ptr<Model> model;
    try
    {
        string fileName = argc > 1 ? argv[1] : "";
        if(fileName != "")
        {
            shared_ptr<ModelLoader> loader = ModelLoader::load(fileName, [](string msg){cout << "model load : warning : " << msg << endl;});
            model = loader->loadAll();
            cout << "model has " << model->triangleCount() << " triangles." << endl;
        }
        static shared_ptr<WindowRenderer> renderer = getWindowRenderer();
        minstd_rand0 randomGenerator;
        static shared_ptr<Mesh> m = makeRandomMesh(randomGenerator, 20);
        m->append(reverse(*m));
        static Mesh m2;
        static shared_ptr<ImageRenderer> imageRenderer;
        static shared_ptr<Texture> testTexture = renderer->preloadTexture(make_shared<ImageTexture>(Image::loadImage("test2.png")));

        if(true)
        {
            TextureDescriptor td(testTexture);
            m2 = transform(Matrix::translate(VectorF(-0.5)).concat(Matrix::scale(2 * 10)), Generate::unitBox(td, td, td, td, td, td));
            imageRenderer = makeImageRenderer(1024, 1024);
        }
        else
        {
            m2 = makeSphereMesh(50, 25, 16, testTexture);
            imageRenderer = makeImageRenderer(2048, 1024);
        }
        pair<VectorF, VectorF> modelExtents;
        static float modelContainingSphereRadius = 0;
        if(model)
        {
            model->preloadTextures(renderer);
            modelExtents = model->getExtents();
            modelContainingSphereRadius = max(abs(get<0>(modelExtents)), abs(get<1>(modelExtents)));
            for(auto &m : model->meshes)
            {
                get<1>(m).append(reverse(get<1>(m)));
            }
        }
        static Mesh m3;
        if(!model)
        {
            m3 = makeSphereMesh(10, 5, 6, nullptr, RGBF(1, 0, 1));
            TextureDescriptor td(testTexture);
            BSPTree csgObject = BSPTree(((Mesh)transform(Matrix::translate(VectorF(-0.5)).concat(Matrix::scale(2 * 5)), Generate::unitBox(td, td, td, td, td, td))).triangles);
            BSPTree t = BSPTree(m3.triangles);
            BSPTree cylinder = BSPTree(makeCylinderMesh(5, 2, 10, RGBF(1, 1, 0)).triangles);
            t = csgDifference(csgIntersection(std::move(t), std::move(csgObject)), cylinder);
            t = csgDifference(std::move(t), transform(Matrix::rotateZ(M_PI / 2), cylinder));
            t = csgDifference(std::move(t), transform(Matrix::rotateX(M_PI / 2), std::move(cylinder)));
            m3 = colorize(RGBAF(1, 1, 1, 0.95), Mesh(t.getTriangles(std::move(m3.triangles)), m3.image));
            cout << "model has " << m3.triangleCount() << " triangles." << endl;
        }

        static shared_ptr<Texture> fontTexture = renderer->preloadTexture(loadTextFontTexture());

        static double startTime = timer();
        static double lastTime = -1;
        static float fps = 30;

#ifdef __EMSCRIPTEN__
        emscripten_set_main_loop([]()
        {
#else
        while(true)
        {
#endif
            double time = timer();
            if(lastTime != -1)
                fps = 1 / max<float>(1e-9f, time - lastTime);
            lastTime = time;
            Transform tform = (Matrix::rotateY((time - startTime) / 2 * M_PI)).concat(Matrix::translate(0, 0, -30));
            renderer->clear();
#if 0
            imageRenderer->clear(RGBF(1, 1, 1));
            imageRenderer->render((Mesh)transform(tform, m));
            shared_ptr<Texture> image = imageRenderer->finish();
            tform = (Matrix::rotateY((time - startTime) / 5 * M_PI)).concat(Matrix::rotateX((time - startTime) / 15 * M_PI)).concat(Matrix::translate(0, 0, -30));
            m2->image = image;
            renderer->render(shadeMesh(transform(tform, m2), shadeFn));
#else
            if(model)
            {
                float scaleFactor = 30 / max(0.001f, modelContainingSphereRadius);
                model->render(renderer, Matrix::translate(0, 0, -1.2 * scaleFactor * modelContainingSphereRadius), (Matrix::rotateY((time - startTime) / 5 * M_PI)).concat(Matrix::rotateX((time - startTime) / 15 * M_PI)).concat(Matrix::scale(scaleFactor)), Light(VectorF(1, 1, 1)));
            }
            else
            {
                tform = (Matrix::rotateY((time - startTime) / 5 * M_PI)).concat(Matrix::rotateX((time - startTime) / 15 * M_PI));
                Matrix tform2 = Matrix::translate(0, 0, -30);
                VectorF viewPoint = inverse(tform2).apply(VectorF(0));
                Mesh containerMesh = shadeMesh(colorize(RGBAF(1, 1, 1, 0.5), transform(tform.concat(tform2), m2)), shadeFn);
                renderer->render(reverse(containerMesh));
                Mesh preCutMesh = transform(tform, m3);
                renderer->render(shadeMesh(transform(tform2, preCutMesh), shadeFn));
                //CutMesh cutMesh = cut(preCutMesh, (Matrix::rotateY(-M_PI / 16 * (sin((time - startTime) / 1 * M_PI)))).concat(Matrix::rotateZ((time - startTime) / 4 * M_PI)).apply(VectorF(-1, 0, 0)), 0);
                //cutMesh.front.append(cutMesh.coplanar);
                //renderer->render(shadeMesh(transform(tform2, cutMesh.front), shadeFn));
                //renderer->render(shadeMesh(shadeMesh(transform(tform2, cutMesh.back), SetColorShadeFn(HSBAF((time - startTime) / 3, 1, 0.5, 0.25))), shadeFn));
                renderer->render(containerMesh);
            }
#endif
            wostringstream ss;
            ss << L"FPS: " << fps;
            //float textWidth = Generate::Text::width(ss.str());
            float textHeight = Generate::Text::height(ss.str());
            float textScale = 1 / 6.0;
            renderer->render(transform(Matrix::scale(textScale).concat(Matrix::translate(-renderer->scaleX(), renderer->scaleY() - textHeight * textScale, -1)), Generate::Text::mesh(ss.str(), fontTexture, RGBF(1, 0.5, 1))));
            renderer->flip();
        }
#ifdef __EMSCRIPTEN__
        , 0, true);
#endif
    }
    catch(exception &e)
    {
        cerr << "Error : " << e.what() << endl;
        return 1;
    }
    return 0;
}

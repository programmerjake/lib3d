#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED

#include "triangle.h"
#include "matrix.h"
#include <vector>
#include <cassert>
#include <utility>
#include <memory>

using namespace std;

struct Mesh;

struct TransformedMesh
{
    Matrix tform;
    shared_ptr<Mesh> mesh;
    TransformedMesh(Matrix tform, shared_ptr<Mesh> mesh)
        : tform(tform), mesh(mesh)
    {
        assert(mesh != nullptr);
    }
//    operator Mesh() const;
    operator shared_ptr<Mesh>() const;
};

struct ColorizedMesh
{
    ColorF color;
    shared_ptr<Mesh> mesh;
    ColorizedMesh(ColorF color, shared_ptr<Mesh> mesh)
        : color(color), mesh(mesh)
    {
        assert(mesh != nullptr);
    }
//    operator Mesh() const;
    operator shared_ptr<Mesh>() const;
};

struct ColorizedTransformedMesh
{
    ColorF color;
    Matrix tform;
    shared_ptr<Mesh> mesh;
    ColorizedTransformedMesh(ColorF color, Matrix tform, shared_ptr<Mesh> mesh)
        : color(color), tform(tform), mesh(mesh)
    {
        assert(mesh != nullptr);
    }
//    operator Mesh() const;
    operator shared_ptr<Mesh>() const;
};

struct Mesh
{
    vector<Triangle> triangles;
    shared_ptr<Texture> image;
    Mesh(vector<Triangle> triangles = vector<Triangle>(), shared_ptr<Texture> image = nullptr)
        : triangles(std::move(triangles)), image(image)
    {
    }
    Mesh(shared_ptr<Mesh> rt)
        : Mesh(*rt)
    {
    }
    Mesh(const Mesh & rt, Matrix tform)
        : image(rt.image)
    {
        triangles.reserve(rt.triangles.size());
        std::transform(rt.triangles.begin(), rt.triangles.end(), back_inserter(triangles), [&tform](const Triangle & t)->Triangle
        {
            return transform(tform, t);
        });
    }
    Mesh(const Mesh & rt, ColorF color)
        : image(rt.image)
    {
        triangles.reserve(rt.triangles.size());
        std::transform(rt.triangles.begin(), rt.triangles.end(), back_inserter(triangles), [&color](const Triangle & t)->Triangle
        {
            return colorize(color, t);
        });
    }
    Mesh(const Mesh & rt, ColorF color, Matrix tform)
        : image(rt.image)
    {
        triangles.reserve(rt.triangles.size());
        std::transform(rt.triangles.begin(), rt.triangles.end(), back_inserter(triangles), [&color, &tform](const Triangle & t)->Triangle
        {
            return colorize(color, transform(tform, t));
        });
    }
    Mesh(TransformedMesh mesh)
        : Mesh(*mesh.mesh, mesh.tform)
    {
    }
    Mesh(ColorizedMesh mesh)
        : Mesh(*mesh.mesh, mesh.color)
    {
    }
    Mesh(ColorizedTransformedMesh mesh)
        : Mesh(*mesh.mesh, mesh.color, mesh.tform)
    {
    }
    void append(const Mesh & rt)
    {
        assert(rt.image == nullptr || image == nullptr || image == rt.image);
        if(rt.image != nullptr)
            image = rt.image;
        triangles.insert(triangles.end(), rt.triangles.begin(), rt.triangles.end());
    }
    void append(const Mesh & rt, Matrix tform)
    {
        assert(rt.image == nullptr || image == nullptr || image == rt.image);
        if(rt.image != nullptr)
            image = rt.image;
        triangles.reserve(triangles.size() + rt.triangles.size());
        std::transform(rt.triangles.begin(), rt.triangles.end(), back_inserter(triangles), [&tform](const Triangle & t)->Triangle
        {
            return transform(tform, t);
        });
    }
    void append(const Mesh & rt, ColorF color)
    {
        assert(rt.image == nullptr || image == nullptr || image == rt.image);
        if(rt.image != nullptr)
            image = rt.image;
        triangles.reserve(triangles.size() + rt.triangles.size());
        std::transform(rt.triangles.begin(), rt.triangles.end(), back_inserter(triangles), [&color](const Triangle & t)->Triangle
        {
            return colorize(color, t);
        });
    }
    void append(const Mesh & rt, ColorF color, Matrix tform)
    {
        assert(rt.image == nullptr || image == nullptr || image == rt.image);
        if(rt.image != nullptr)
            image = rt.image;
        triangles.reserve(triangles.size() + rt.triangles.size());
        std::transform(rt.triangles.begin(), rt.triangles.end(), back_inserter(triangles), [&color, &tform](const Triangle & t)->Triangle
        {
            return colorize(color, transform(tform, t));
        });
    }
    void append(shared_ptr<Mesh> rt)
    {
        append(*rt);
    }
    void append(Triangle triangle)
    {
        triangles.push_back(triangle);
    }
    void append(TransformedMesh mesh)
    {
        append(*mesh.mesh, mesh.tform);
    }
    void append(ColorizedMesh mesh)
    {
        append(*mesh.mesh, mesh.color);
    }
    void append(ColorizedTransformedMesh mesh)
    {
        append(*mesh.mesh, mesh.color, mesh.tform);
    }
    pair<VectorF, VectorF> getExtents() const
    {
        VectorF minP = VectorF(0), maxP = VectorF(0);
        bool isFirst = true;
        for(Triangle tri : triangles)
        {
            if(isFirst)
            {
                minP = tri.p1;
                maxP = tri.p1;
                isFirst = false;
            }
            else
            {
                minP.x = min(minP.x, tri.p1.x);
                minP.y = min(minP.y, tri.p1.y);
                minP.z = min(minP.z, tri.p1.z);
                maxP.x = max(maxP.x, tri.p1.x);
                maxP.y = max(maxP.y, tri.p1.y);
                maxP.z = max(maxP.z, tri.p1.z);
            }
            minP.x = min(minP.x, tri.p2.x);
            minP.y = min(minP.y, tri.p2.y);
            minP.z = min(minP.z, tri.p2.z);
            maxP.x = max(maxP.x, tri.p2.x);
            maxP.y = max(maxP.y, tri.p2.y);
            maxP.z = max(maxP.z, tri.p2.z);

            minP.x = min(minP.x, tri.p3.x);
            minP.y = min(minP.y, tri.p3.y);
            minP.z = min(minP.z, tri.p3.z);
            maxP.x = max(maxP.x, tri.p3.x);
            maxP.y = max(maxP.y, tri.p3.y);
            maxP.z = max(maxP.z, tri.p3.z);
        }
        return make_pair(minP, maxP);
    }
};

//inline TransformedMesh::operator Mesh() const
//{
//    return Mesh(*this);
//}

inline TransformedMesh::operator shared_ptr<Mesh>() const
{
    return shared_ptr<Mesh>(new Mesh(*this));
}

//inline ColorizedMesh::operator Mesh() const
//{
//    return Mesh(*this);
//}

inline ColorizedMesh::operator shared_ptr<Mesh>() const
{
    return shared_ptr<Mesh>(new Mesh(*this));
}

//inline ColorizedTransformedMesh::operator Mesh() const
//{
//    return Mesh(*this);
//}

inline ColorizedTransformedMesh::operator shared_ptr<Mesh>() const
{
    return shared_ptr<Mesh>(new Mesh(*this));
}

inline TransformedMesh transform(Matrix tform, Mesh mesh)
{
    return TransformedMesh(tform, make_shared<Mesh>(std::move(mesh)));
}

inline TransformedMesh transform(Matrix tform, shared_ptr<Mesh> mesh)
{
    assert(mesh != nullptr);
    return TransformedMesh(tform, mesh);
}

inline TransformedMesh transform(Matrix tform, TransformedMesh mesh)
{
    return TransformedMesh(transform(tform, mesh.tform), mesh.mesh);
}

inline ColorizedTransformedMesh transform(Matrix tform, ColorizedMesh mesh)
{
    return ColorizedTransformedMesh(mesh.color, tform, mesh.mesh);
}

inline ColorizedTransformedMesh transform(Matrix tform, ColorizedTransformedMesh mesh)
{
    return ColorizedTransformedMesh(mesh.color, transform(tform, mesh.tform), mesh.mesh);
}

inline ColorizedMesh colorize(ColorF color, Mesh mesh)
{
    return ColorizedMesh(color, make_shared<Mesh>(std::move(mesh)));
}

inline ColorizedMesh colorize(ColorF color, shared_ptr<Mesh> mesh)
{
    assert(mesh != nullptr);
    return ColorizedMesh(color, mesh);
}

inline ColorizedMesh colorize(ColorF color, ColorizedMesh mesh)
{
    return ColorizedMesh(colorize(color, mesh.color), mesh.mesh);
}

inline ColorizedTransformedMesh colorize(ColorF color, TransformedMesh mesh)
{
    return ColorizedTransformedMesh(color, mesh.tform, mesh.mesh);
}

inline ColorizedTransformedMesh colorize(ColorF color, ColorizedTransformedMesh mesh)
{
    return ColorizedTransformedMesh(colorize(color, mesh.color), mesh.tform, mesh.mesh);
}

#endif // MESH_H_INCLUDED

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
    Transform tform;
    shared_ptr<Mesh> mesh;
    TransformedMesh(Transform tform, shared_ptr<Mesh> mesh)
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
    Transform tform;
    shared_ptr<Mesh> mesh;
    ColorizedTransformedMesh(ColorF color, Transform tform, shared_ptr<Mesh> mesh)
        : color(color), tform(tform), mesh(mesh)
    {
        assert(mesh != nullptr);
    }
//    operator Mesh() const;
    operator shared_ptr<Mesh>() const;
};

struct TransformedMeshRef
{
    Transform tform;
    const Mesh &mesh;
    TransformedMeshRef(Transform tform, const Mesh &mesh)
        : tform(tform), mesh(mesh)
    {
    }
//    operator Mesh() const;
    operator shared_ptr<Mesh>() const;
};

struct ColorizedMeshRef
{
    ColorF color;
    const Mesh &mesh;
    ColorizedMeshRef(ColorF color, const Mesh &mesh)
        : color(color), mesh(mesh)
    {
    }
//    operator Mesh() const;
    operator shared_ptr<Mesh>() const;
};

struct ColorizedTransformedMeshRef
{
    ColorF color;
    Transform tform;
    const Mesh &mesh;
    ColorizedTransformedMeshRef(ColorF color, Transform tform, const Mesh &mesh)
        : color(color), tform(tform), mesh(mesh)
    {
    }
//    operator Mesh() const;
    operator shared_ptr<Mesh>() const;
};

struct TransformedMeshRRef
{
    Transform tform;
    Mesh &mesh;
    TransformedMeshRRef(Transform tform, Mesh &&mesh)
        : tform(tform), mesh(mesh)
    {
    }
    operator shared_ptr<Mesh>() &&;
};

struct ColorizedMeshRRef
{
    ColorF color;
    Mesh &mesh;
    ColorizedMeshRRef(ColorF color, Mesh &&mesh)
        : color(color), mesh(mesh)
    {
    }
    operator shared_ptr<Mesh>() &&;
};

struct ColorizedTransformedMeshRRef
{
    ColorF color;
    Transform tform;
    Mesh &mesh;
    ColorizedTransformedMeshRRef(ColorF color, Transform tform, Mesh &&mesh)
        : color(color), tform(tform), mesh(mesh)
    {
    }
    operator shared_ptr<Mesh>() &&;
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
    Mesh(const Mesh & rt, Transform tform)
        : image(rt.image)
    {
        triangles.reserve(rt.triangles.size());
        std::transform(rt.triangles.begin(), rt.triangles.end(), back_inserter(triangles), [&tform](const Triangle & t)->Triangle
        {
            return transform(tform, t);
        });
    }
    Mesh(Mesh && rt, Transform tform)
        : image(rt.image)
    {
        triangles = std::move(rt.triangles);
        for(Triangle &tri : triangles)
        {
            tri = transform(tform, tri);
        }
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
    Mesh(Mesh && rt, ColorF color)
        : image(rt.image)
    {
        triangles = std::move(rt.triangles);
        for(Triangle &tri : triangles)
        {
            tri = colorize(color, tri);
        }
    }
    Mesh(const Mesh & rt, ColorF color, Transform tform)
        : image(rt.image)
    {
        triangles.reserve(rt.triangles.size());
        std::transform(rt.triangles.begin(), rt.triangles.end(), back_inserter(triangles), [&color, &tform](const Triangle & t)->Triangle
        {
            return colorize(color, transform(tform, t));
        });
    }
    Mesh(Mesh && rt, ColorF color, Transform tform)
        : image(rt.image)
    {
        triangles = std::move(rt.triangles);
        for(Triangle &tri : triangles)
        {
            tri = colorize(color, transform(tform, tri));
        }
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
    Mesh(TransformedMeshRef mesh)
        : Mesh(mesh.mesh, mesh.tform)
    {
    }
    Mesh(ColorizedMeshRef mesh)
        : Mesh(mesh.mesh, mesh.color)
    {
    }
    Mesh(ColorizedTransformedMeshRef mesh)
        : Mesh(mesh.mesh, mesh.color, mesh.tform)
    {
    }
    Mesh(TransformedMeshRRef &&mesh)
        : Mesh(std::move(mesh.mesh), mesh.tform)
    {
    }
    Mesh(ColorizedMeshRRef &&mesh)
        : Mesh(std::move(mesh.mesh), mesh.color)
    {
    }
    Mesh(ColorizedTransformedMeshRRef &&mesh)
        : Mesh(std::move(mesh.mesh), mesh.color, mesh.tform)
    {
    }
    void clear()
    {
        image = nullptr;
        triangles.clear();
    }
    void append(const Mesh & rt)
    {
        assert(rt.image == nullptr || image == nullptr || image == rt.image);
        if(rt.image != nullptr)
            image = rt.image;
        triangles.insert(triangles.end(), rt.triangles.begin(), rt.triangles.end());
    }
    void append(const Mesh & rt, Transform tform)
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
    void append(const Mesh & rt, ColorF color, Transform tform)
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
    void append(TransformedMeshRef mesh)
    {
        append(mesh.mesh, mesh.tform);
    }
    void append(ColorizedMeshRef mesh)
    {
        append(mesh.mesh, mesh.color);
    }
    void append(ColorizedTransformedMeshRef mesh)
    {
        append(mesh.mesh, mesh.color, mesh.tform);
    }
    void append(TransformedMeshRRef &&mesh)
    {
        append(mesh.mesh, mesh.tform);
    }
    void append(ColorizedMeshRRef &&mesh)
    {
        append(mesh.mesh, mesh.color);
    }
    void append(ColorizedTransformedMeshRRef &&mesh)
    {
        append(mesh.mesh, mesh.color, mesh.tform);
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
    void assign(const Mesh & mesh)
    {
        clear();
        append(mesh);
    }
    void assign(const Mesh & mesh, Transform tform)
    {
        clear();
        append(mesh, tform);
    }
    void assign(const Mesh & mesh, ColorF color)
    {
        clear();
        append(mesh, color);
    }
    void assign(const Mesh & mesh, ColorF color, Transform tform)
    {
        clear();
        append(mesh, color, tform);
    }
    void assign(shared_ptr<Mesh> mesh)
    {
        clear();
        append(mesh);
    }
    void assign(TransformedMesh mesh)
    {
        clear();
        append(mesh);
    }
    void assign(ColorizedMesh mesh)
    {
        clear();
        append(mesh);
    }
    void assign(ColorizedTransformedMesh mesh)
    {
        clear();
        append(mesh);
    }
    void assign(TransformedMeshRef mesh)
    {
        clear();
        append(mesh);
    }
    void assign(ColorizedMeshRef mesh)
    {
        clear();
        append(mesh);
    }
    void assign(ColorizedTransformedMeshRef mesh)
    {
        clear();
        append(mesh);
    }
    void assign(TransformedMeshRRef &&mesh)
    {
        *this = Mesh(std::move(mesh));
    }
    void assign(ColorizedMeshRRef &&mesh)
    {
        *this = Mesh(std::move(mesh));
    }
    void assign(ColorizedTransformedMeshRRef &&mesh)
    {
        *this = Mesh(std::move(mesh));
    }
    size_t triangleCount() const
    {
        return triangles.size();
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

inline TransformedMeshRef::operator shared_ptr<Mesh>() const
{
    return shared_ptr<Mesh>(new Mesh(*this));
}

inline TransformedMeshRRef::operator shared_ptr<Mesh>() &&
{
    return shared_ptr<Mesh>(new Mesh(std::move(*this)));
}

//inline ColorizedMesh::operator Mesh() const
//{
//    return Mesh(*this);
//}

inline ColorizedMesh::operator shared_ptr<Mesh>() const
{
    return shared_ptr<Mesh>(new Mesh(*this));
}

inline ColorizedMeshRef::operator shared_ptr<Mesh>() const
{
    return shared_ptr<Mesh>(new Mesh(*this));
}

inline ColorizedMeshRRef::operator shared_ptr<Mesh>() &&
{
    return shared_ptr<Mesh>(new Mesh(std::move(*this)));
}

//inline ColorizedTransformedMesh::operator Mesh() const
//{
//    return Mesh(*this);
//}

inline ColorizedTransformedMesh::operator shared_ptr<Mesh>() const
{
    return shared_ptr<Mesh>(new Mesh(*this));
}

inline ColorizedTransformedMeshRef::operator shared_ptr<Mesh>() const
{
    return shared_ptr<Mesh>(new Mesh(*this));
}

inline ColorizedTransformedMeshRRef::operator shared_ptr<Mesh>() &&
{
    return shared_ptr<Mesh>(new Mesh(std::move(*this)));
}

inline TransformedMeshRef transform(Transform tform, const Mesh &mesh)
{
    return TransformedMeshRef(tform, mesh);
}

inline TransformedMeshRRef transform(Transform tform, Mesh &&mesh)
{
    return TransformedMeshRRef(tform, std::move(mesh));
}

inline TransformedMesh transform(Transform tform, shared_ptr<Mesh> mesh)
{
    assert(mesh != nullptr);
    return TransformedMesh(tform, mesh);
}

inline TransformedMesh transform(Transform tform, TransformedMesh mesh)
{
    return TransformedMesh(transform(tform, mesh.tform), mesh.mesh);
}

inline ColorizedTransformedMesh transform(Transform tform, ColorizedMesh mesh)
{
    return ColorizedTransformedMesh(mesh.color, tform, mesh.mesh);
}

inline ColorizedTransformedMesh transform(Transform tform, ColorizedTransformedMesh mesh)
{
    return ColorizedTransformedMesh(mesh.color, transform(tform, mesh.tform), mesh.mesh);
}

inline TransformedMeshRef transform(Transform tform, TransformedMeshRef mesh)
{
    return TransformedMeshRef(transform(tform, mesh.tform), mesh.mesh);
}

inline ColorizedTransformedMeshRef transform(Transform tform, ColorizedMeshRef mesh)
{
    return ColorizedTransformedMeshRef(mesh.color, tform, mesh.mesh);
}

inline ColorizedTransformedMeshRef transform(Transform tform, ColorizedTransformedMeshRef mesh)
{
    return ColorizedTransformedMeshRef(mesh.color, transform(tform, mesh.tform), mesh.mesh);
}

inline TransformedMeshRRef transform(Transform tform, TransformedMeshRRef &&mesh)
{
    return TransformedMeshRRef(transform(tform, mesh.tform), std::move(mesh.mesh));
}

inline ColorizedTransformedMeshRRef transform(Transform tform, ColorizedMeshRRef &&mesh)
{
    return ColorizedTransformedMeshRRef(mesh.color, tform, std::move(mesh.mesh));
}

inline ColorizedTransformedMeshRRef transform(Transform tform, ColorizedTransformedMeshRRef &&mesh)
{
    return ColorizedTransformedMeshRRef(mesh.color, transform(tform, mesh.tform), std::move(mesh.mesh));
}

inline ColorizedMeshRef colorize(ColorF color, const Mesh &mesh)
{
    return ColorizedMeshRef(color, mesh);
}

inline ColorizedMeshRRef colorize(ColorF color, Mesh &&mesh)
{
    return ColorizedMeshRRef(color, std::move(mesh));
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

inline ColorizedMeshRef colorize(ColorF color, ColorizedMeshRef mesh)
{
    return ColorizedMeshRef(colorize(color, mesh.color), mesh.mesh);
}

inline ColorizedTransformedMeshRef colorize(ColorF color, TransformedMeshRef mesh)
{
    return ColorizedTransformedMeshRef(color, mesh.tform, mesh.mesh);
}

inline ColorizedTransformedMeshRef colorize(ColorF color, ColorizedTransformedMeshRef mesh)
{
    return ColorizedTransformedMeshRef(colorize(color, mesh.color), mesh.tform, mesh.mesh);
}

inline ColorizedMeshRRef colorize(ColorF color, ColorizedMeshRRef &&mesh)
{
    return ColorizedMeshRRef(colorize(color, mesh.color), std::move(mesh.mesh));
}

inline ColorizedTransformedMeshRRef colorize(ColorF color, TransformedMeshRRef &&mesh)
{
    return ColorizedTransformedMeshRRef(color, mesh.tform, std::move(mesh.mesh));
}

inline ColorizedTransformedMeshRRef colorize(ColorF color, ColorizedTransformedMeshRRef &&mesh)
{
    return ColorizedTransformedMeshRRef(colorize(color, mesh.color), mesh.tform, std::move(mesh.mesh));
}

#endif // MESH_H_INCLUDED

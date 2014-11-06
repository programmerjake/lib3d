#ifndef BSP_TREE_H_INCLUDED
#define BSP_TREE_H_INCLUDED

#include "triangle.h"
#include <utility>

struct BSPNode final
{
    vector<Triangle> triangles;
    VectorF normal;
    float d;
    BSPNode *front;
    BSPNode *back;
    BSPNode(Triangle tri)
        : triangles(1, tri), normal(tri.normal()), front(nullptr), back(nullptr)
    {
        d = -dot(normal, tri.p1);
    }
    BSPNode(Triangle tri, VectorF normal, float d)
        : triangles(1, tri), normal(tri.empty() ? VectorF(0) : normal), d(d), front(nullptr), back(nullptr)
    {
    }
    BSPNode(vector<Triangle> triangles, VectorF normal, float d)
        : triangles(std::move(triangles)), normal(normal), d(d), front(nullptr), back(nullptr)
    {
    }
    bool good() const
    {
        return normal != VectorF(0);
    }
};

class BSPTree final
{
    BSPNode *root = nullptr;
    static void freeNode(BSPNode *node)
    {
        if(node != nullptr)
        {
            freeNode(node->front);
            freeNode(node->back);
            delete node;
        }
    }
    static BSPNode *dupNode(const BSPNode *node)
    {
        if(node == nullptr)
            return nullptr;
        BSPNode *retval = new BSPNode(*node);
        retval->front = dupNode(node->front);
        retval->back = dupNode(node->back);
        return retval;
    }
    static void insertTriangle(BSPNode *&tree, Triangle tri)
    {
        if(tri.empty())
        {
            return;
        }
        if(tree == nullptr)
        {
            tree = new BSPNode(tri);
            return;
        }
        CutTriangle ct = cut(tri, tree->normal, tree->d);
        for(size_t i = 0; i < ct.coplanarTriangleCount; i++)
        {
            tree->triangles.push_back(ct.coplanarTriangles[i]);
        }
        for(size_t i = 0; i < ct.backTriangleCount; i++)
        {
            insertTriangle(tree->back, ct.backTriangles[i]);
        }
        for(size_t i = 0; i < ct.frontTriangleCount; i++)
        {
            insertTriangle(tree->front, ct.frontTriangles[i]);
        }
    }
    static void getTriangles(vector<Triangle> &triangles, const BSPNode *tree)
    {
        if(tree == nullptr)
            return;
        getTriangles(triangles, tree->front);
        triangles.insert(triangles.end(), tree->triangles.begin(), tree->triangles.end());
        getTriangles(triangles, tree->back);
    }
    static void getTriangles(vector<Triangle> &triangles, const BSPNode *tree, VectorF viewPoint)
    {
        if(tree == nullptr)
            return;
        if(dot(viewPoint, tree->normal) > -tree->d)
        {
            getTriangles(triangles, tree->back, viewPoint);
            triangles.insert(triangles.end(), tree->triangles.begin(), tree->triangles.end());
            getTriangles(triangles, tree->front, viewPoint);
        }
        else
        {
            getTriangles(triangles, tree->front, viewPoint);
            triangles.insert(triangles.end(), tree->triangles.begin(), tree->triangles.end());
            getTriangles(triangles, tree->back, viewPoint);
        }
    }
public:
    BSPTree()
    {
    }
    BSPTree(Triangle tri)
    {
        insert(tri);
    }
    BSPTree(const vector<Triangle> &triangles)
    {
        insert(triangles);
    }
    BSPTree(const BSPTree &rt)
        : root(dupNode(rt.root))
    {
    }
    BSPTree(BSPTree &&rt)
        : root(rt.root)
    {
        rt.root = nullptr;
    }
    void swap(BSPTree &rt)
    {
        auto temp = root;
        root = rt.root;
        rt.root = temp;
    }
    const BSPTree &operator =(const BSPTree &rt)
    {
        if(rt.root == root)
            return *this;
        freeNode(root);
        root = dupNode(rt.root);
        return *this;
    }
    const BSPTree &operator =(BSPTree &&rt)
    {
        swap(rt);
        return *this;
    }
    ~BSPTree()
    {
        freeNode(root);
    }
    void clear()
    {
        freeNode(root);
        root = nullptr;
    }
    void insert(Triangle tri)
    {
        insertTriangle(root, tri);
    }
    void insert(const vector<Triangle> &triangles)
    {
        for(Triangle tri : triangles)
        {
            insert(tri);
        }
    }
    vector<Triangle> getTriangles(vector<Triangle> buffer = vector<Triangle>()) const
    {
        buffer.clear();
        getTriangles(buffer, root);
        return std::move(buffer);
    }
    vector<Triangle> getTriangles(VectorF viewPoint, vector<Triangle> buffer = vector<Triangle>()) const
    {
        buffer.clear();
        getTriangles(buffer, root, viewPoint);
        return std::move(buffer);
    }
};

#endif // BSP_TREE_H_INCLUDED

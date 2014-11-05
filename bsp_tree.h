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

#error finish
#warning finish

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
    static void insertTriangle(BSPNode *&tree, Triangle tri, VectorF tNormal, float tD)
    {
        if(!node->good())
        {
            delete node;
            return;
        }
        if(tree == nullptr)
        {
            tree = node;
            return;
        }
        #error finish
        if(absSquared(tree->normal - nodePlaneNormal) < eps * eps && abs(tree->d - nodePlaneD) < eps)
        {
            insertNode(tree->front, node);
            return;
        }
        CutTriangle ct = cut(node->tri, tree->normal, tree->d);
        if(ct.backTriangleCount + ct.frontTriangleCount <= 1)
        {
            if(ct.backTriangleCount > 0)
            {
                insertNode(tree->back, node);
            }
            else
            {
                insertNode(tree->front, node);
            }
            return;
        }
        bool usedPassedInNode = false;
        for(size_t i = 0; i < ct.backTriangleCount; i++)
        {
            if(ct.backTriangles[i].empty())
                continue;
            if(usedPassedInNode)
                insertNode(tree->back, new BSPNode(ct.backTriangles[i], nodePlaneNormal, nodePlaneD));
            else
            {
                usedPassedInNode = true;
                *node = BSPNode(ct.backTriangles[i], nodePlaneNormal, nodePlaneD);
                insertNode(tree->back, node);
            }
        }
        for(size_t i = 0; i < ct.frontTriangleCount; i++)
        {
            if(ct.frontTriangles[i].empty())
                continue;
            if(usedPassedInNode)
                insertNode(tree->front, new BSPNode(ct.frontTriangles[i], nodePlaneNormal, nodePlaneD));
            else
            {
                usedPassedInNode = true;
                *node = BSPNode(ct.frontTriangles[i], nodePlaneNormal, nodePlaneD);
                insertNode(tree->front, node);
            }
        }
        if(!usedPassedInNode)
            delete node;
    }
    static void getTriangles(vector<Triangle> &triangles, const BSPNode *tree)
    {
        if(tree == nullptr)
            return;
        getTriangles(triangles, tree->front);
        triangles.push_back(tree->tri);
        getTriangles(triangles, tree->back);
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
        if(!tri.empty())
            insertNode(root, new BSPNode(tri));
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
};

#endif // BSP_TREE_H_INCLUDED

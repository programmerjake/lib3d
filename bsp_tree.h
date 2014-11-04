#ifndef BSP_TREE_H_INCLUDED
#define BSP_TREE_H_INCLUDED

#include "triangle.h"
#include <utility>

struct BSPNode final
{
    Triangle tri;
    VectorF normal;
    float d;
    BSPNode *front;
    BSPNode *back;
    BSPNode(Triangle tri)
        : tri(tri), normal(tri.normal()), front(nullptr), back(nullptr)
    {
        d = -dot(normal, tri.p1);
    }
    bool good() const
    {
        return absSquared(normal) >= eps * eps;
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
    static void insertNode(BSPNode *&tree, BSPNode *node)
    {

    }
public:
    void clear()
    {
        freeNode(root);
        root = nullptr;
    }
    void insert(Triangle tri)
    {
        BSPNode *node = new BSPNode(tri);
        if(!node->good())
        {
            delete node;
            return;
        }
        insertNode(root, node);
    }
};

#endif // BSP_TREE_H_INCLUDED

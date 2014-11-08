#ifndef BSP_TREE_H_INCLUDED
#define BSP_TREE_H_INCLUDED

#include "triangle.h"
#include <utility>
#include <random>

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
    void invert()
    {
        for(Triangle &tri : triangles)
        {
            tri = reverse(tri);
        }
        normal = -normal;
        d = -d;
        if(front)
            front->invert();
        if(back)
            back->invert();
        auto temp = front;
        front = back;
        back = temp;
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
    static vector<Triangle> clipTriangles(vector<Triangle> triangles, const BSPNode *tree)
    {
        if(tree == nullptr)
            return std::move(triangles);
        vector<Triangle> front, back;
        VectorF tn = tree->normal;
        float td = tree->d;
        for(Triangle tri : triangles)
        {
            CutTriangle ct = cut(tri, tn, td);
            for(size_t i = 0; i < ct.coplanarTriangleCount; i++)
            {
                if(dot(ct.coplanarTriangles[i].point_cross(), tn) >= 0)
                {
                    front.push_back(ct.coplanarTriangles[i]);
                }
                else if(tree->back != nullptr)
                {
                    back.push_back(ct.coplanarTriangles[i]);
                }
            }
            for(size_t i = 0; i < ct.frontTriangleCount; i++)
            {
                front.push_back(ct.frontTriangles[i]);
            }
            if(tree->back != nullptr)
            {
                for(size_t i = 0; i < ct.backTriangleCount; i++)
                {
                    back.push_back(ct.backTriangles[i]);
                }
            }
        }
        triangles = vector<Triangle>();
        front = clipTriangles(std::move(front), tree->front);
        if(tree->back != nullptr)
            back = clipTriangles(std::move(back), tree->back);
        else
            back.clear();
        front.insert(front.begin(), back.begin(), back.end());
        return std::move(front);
    }
    static void clipTo(BSPNode *clipTree, const BSPNode *clipToTree)
    {
        if(clipTree == nullptr)
            return;
        clipTree->triangles = clipTriangles(std::move(clipTree->triangles), clipToTree);
        clipTo(clipTree->front, clipToTree);
        clipTo(clipTree->back, clipToTree);
    }
    void merge(const BSPNode *tree)
    {
        if(tree == nullptr)
            return;
        merge(tree->front);
        insert(tree->triangles);
        merge(tree->back);
    }
    static void transform_helper(Transform tform, BSPNode *tree)
    {
        if(tree == nullptr)
            return;
        transform_helper(tform, tree->front);
        transform_helper(tform, tree->back);
        for(Triangle &tri : tree->triangles)
        {
            tri = transform(tform, tri);
        }
        VectorF p = transform(tform, tree->normal * -tree->d);
        tree->normal = transformNormal(tform, tree->normal);
        tree->d = -dot(tree->normal, p);
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
        vector<size_t> order;
        order.resize(triangles.size());
        for(size_t i = 0; i < order.size(); i++)
            order[i] = i;
        default_random_engine rg;
        uniform_int_distribution<size_t> d(0, order.size() - 1);
        for(size_t &v : order)
        {
            std::swap(v, order[d(rg)]);
        }
        for(size_t index : order)
        {
            insert(triangles[index]);
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
    void clipTo(const BSPTree &tree)
    {
        clipTo(root, tree.root);
    }
    void invert()
    {
        if(root)
            root->invert();
    }
    void insert(const BSPTree &tree)
    {
        merge(tree.root);
    }
    void transform_helper(Transform tform)
    {
        transform_helper(tform, root);
    }
    vector<Triangle> clipTriangles(vector<Triangle> triangles) const
    {
        return clipTriangles(std::move(triangles), root);
    }
};

inline BSPTree csgUnion(BSPTree a, BSPTree b)
{
    a.clipTo(b);
    b.clipTo(a);
    b.invert();
    b.clipTo(a);
    b.invert();
    a.insert(b);
    return std::move(a);
}

inline BSPTree csgIntersection(BSPTree a, BSPTree b)
{
    a.invert();
    b.clipTo(a);
    b.invert();
    a.clipTo(b);
    b.clipTo(a);
    a.insert(b);
    a.invert();
    return std::move(a);
}

inline BSPTree csgDifference(BSPTree a, BSPTree b)
{
    a.invert();
    a.clipTo(b);
    b.clipTo(a);
    b.invert();
    b.clipTo(a);
    b.invert();
    a.insert(b);
    a.invert();
    return std::move(a);
}

inline BSPTree transform(Transform tform, BSPTree tree)
{
    tree.transform_helper(tform);
    return std::move(tree);
}

#endif // BSP_TREE_H_INCLUDED

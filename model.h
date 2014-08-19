#ifndef MODEL_H_INCLUDED
#define MODEL_H_INCLUDED

#include "material.h"
#include "mesh.h"
#include "renderer.h"
#include "generate.h"
#include <vector>
#include <tuple>
#include <stdexcept>

using namespace std;

struct Model
{
    vector<pair<Material, Mesh>> meshes;
    template <typename ...Lights>
    void render(shared_ptr<Renderer> renderer, Matrix globalToCameraTransform, Matrix localToGlobalTransform, Lights ...lights) const
    {
        for(const pair<Material, Mesh> &mesh : meshes)
        {
            Mesh m = transform(localToGlobalTransform, get<1>(mesh));
            m = shadeMesh(m, light_material(get<0>(mesh), lights...));
            renderer->render(transform(globalToCameraTransform, m));
        }
    }
    Model(Mesh mesh, Material material = Material())
        : meshes{make_pair(material, mesh)}
    {
    }
    Model()
    {
    }
    void preloadTextures(shared_ptr<Renderer> renderer)
    {
        for(pair<Material, Mesh> &mesh : meshes)
        {
            get<0>(mesh).texture = renderer->preloadTexture(get<0>(mesh).texture);
            get<1>(mesh).image = renderer->preloadTexture(get<1>(mesh).image);
        }
    }
    pair<VectorF, VectorF> getExtents() const
    {
        VectorF minP = VectorF(0);
        VectorF maxP = VectorF(0);
        bool isFirst = true;
        for(const pair<Material, Mesh> &mesh : meshes)
        {
            auto extents = get<1>(mesh).getExtents();
            if(isFirst)
            {
                minP = get<0>(extents);
                maxP = get<1>(extents);
                isFirst = false;
            }
            else
            {
                minP.x = min(minP.x, get<0>(extents).x);
                minP.y = min(minP.y, get<0>(extents).y);
                minP.z = min(minP.z, get<0>(extents).z);
                maxP.x = max(maxP.x, get<1>(extents).x);
                maxP.y = max(maxP.y, get<1>(extents).y);
                maxP.z = max(maxP.z, get<1>(extents).z);
            }
        }
        return make_pair(minP, maxP);
    }
};

struct ModelLoadException : public runtime_error
{
    explicit ModelLoadException(const string &msg)
        : runtime_error(msg)
    {
    }
};

struct ModelLoader
{
    ModelLoader(const ModelLoader &) = delete;
    void operator =(const ModelLoader &) = delete;
    ModelLoader()
    {
    }
    virtual ~ModelLoader()
    {
    }
    virtual pair<shared_ptr<Model>, string> load() = 0;
    shared_ptr<Model> loadAll()
    {
        Model retval;
        for(shared_ptr<Model> model = get<0>(load()); model != nullptr; model = get<0>(load()))
        {
            for(auto mesh : model->meshes)
            {
                retval.meshes.push_back(mesh);
            }
        }
        return make_shared<Model>(std::move(retval));
    }

    static shared_ptr<ModelLoader> loadOBJ(string fileName, function<void(string)> warningFunction = nullptr);
};

#endif // MODEL_H_INCLUDED

#ifndef RENDERER_H_INCLUDED
#define RENDERER_H_INCLUDED

#include "mesh.h"
#include <chrono>

using namespace std;

inline double realTimer()
{
    auto timePointInNanoseconds = chrono::time_point_cast<chrono::nanoseconds>(chrono::steady_clock::now());
    return 1e-9 * timePointInNanoseconds.time_since_epoch().count();
}

struct Renderer
{
    Renderer(const Renderer & rt) = delete;
    const Renderer & operator =(const Renderer & rt) = delete;
    virtual ~Renderer()
    {
    }
    virtual void render(const Mesh & m) = 0;
    void render(shared_ptr<Mesh> m)
    {
        assert(m != nullptr);
        render(*m);
    }
    void render(TransformedMesh m)
    {
        render((Mesh)m);
    }
    void render(ColorizedTransformedMesh m)
    {
        render((Mesh)m);
    }
    void render(ColorizedMesh m)
    {
        render((Mesh)m);
    }
    void render(TransformedMeshRef m)
    {
        render((Mesh)m);
    }
    void render(ColorizedTransformedMeshRef m)
    {
        render((Mesh)m);
    }
    void render(ColorizedMeshRef m)
    {
        render((Mesh)m);
    }
    void render(TransformedMeshRRef &&m)
    {
        render(Mesh(std::move(m)));
    }
    void render(ColorizedTransformedMeshRRef &&m)
    {
        render(Mesh(std::move(m)));
    }
    void render(ColorizedMeshRRef &&m)
    {
        render(Mesh(std::move(m)));
    }
    virtual void calcScales() = 0;
protected:
    virtual void clearInternal(ColorF bg) = 0;
    float scaleXValue = 1;
    float scaleYValue = 1;
    double lastFlipTime = -1;
    float fps = 60.0;
    size_t frameCount = 0;
    virtual void onSetFPS()
    {
    }
    void copyScales(const Renderer &otherRenderer)
    {
        scaleXValue = otherRenderer.scaleXValue;
        scaleYValue = otherRenderer.scaleYValue;
    }
    void calcFPS()
    {
        if(lastFlipTime == -1)
        {
            lastFlipTime = realTimer();
            frameCount = 0;
            return;
        }
        frameCount++;
        double time = realTimer();
        if(time - lastFlipTime > 0.01)
        {
            fps = frameCount / (time - lastFlipTime);
            lastFlipTime = time;
            frameCount = 0;
            onSetFPS();
        }
    }
    void calcScales(size_t w, size_t h, float aspectRatio = -1)
    {
        if(aspectRatio > 0)
        {
            if(aspectRatio > 1)
            {
                scaleXValue = aspectRatio;
                scaleYValue = 1;
            }
            else
            {
                scaleXValue = 1;
                scaleYValue = 1 / aspectRatio;
            }
        }
        else if(w > h)
        {
            scaleXValue = (float)w / h;
            scaleYValue = 1;
        }
        else
        {
            scaleXValue = 1;
            scaleYValue = (float)h / w;
        }
    }
public:
    Renderer()
        : scaleXValue(1), scaleYValue(1)
    {
    }
    void clear(ColorF backgroundColor = RGBF(0, 0, 0))
    {
        calcScales();
        clearInternal(backgroundColor);
    }
    float scaleX() const
    {
        return scaleXValue;
    }
    float scaleY() const
    {
        return scaleYValue;
    }
    virtual void enableWriteDepth(bool v) = 0;
    virtual shared_ptr<Texture> preloadTexture(shared_ptr<Texture> texture)
    {
        return texture;
    }
};

struct WindowRenderer : public Renderer
{
    virtual void flip() = 0;
    virtual double timer()
    {
        return realTimer();
    }
};

struct ImageRenderer : public Renderer
{
    virtual shared_ptr<Texture> finish() = 0;
    virtual void resize(size_t newW, size_t newH, float newAspectRatio = -1) = 0;
};

void setDefaultRendererSize(int w, int h, float aspectRatio = -1);
int getDefaultRendererWidth();
int getDefaultRendererHeight();
float getDefaultRendererAspectRatio();

shared_ptr<WindowRenderer> getWindowRenderer();
shared_ptr<ImageRenderer> makeImageRenderer(size_t w, size_t h, float aspectRatio = -1);

inline double timer()
{
    return getWindowRenderer()->timer();
}

#endif // RENDERER_H_INCLUDED

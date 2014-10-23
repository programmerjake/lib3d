#include "softrender.h"
#include <utility>
#include <iostream>
#ifndef __EMSCRIPTEN__
#include <thread>
#include <mutex>
#include <condition_variable>
#endif
#include <functional>
#include <memory>

using namespace std;

namespace
{
inline VectorF gridify(VectorF v)
{
    constexpr float gridSize = 1 / 128.0;
    constexpr float invGridSize = 1 / gridSize;
    return VectorF(std::floor(v.x * invGridSize + 0.5) * gridSize, std::floor(v.y * invGridSize + 0.5) * gridSize, std::floor(v.z * invGridSize + 0.5) * gridSize);
}

inline Triangle gridify(Triangle tri)
{
    tri.p1 = gridify(tri.p1);
    tri.p2 = gridify(tri.p2);
    tri.p3 = gridify(tri.p3);
    return tri;
}
}

void SoftwareRenderer::renderTriangle(Triangle triangleIn, size_t sectionTop, size_t sectionBottom, shared_ptr<const Image> texture)
{
    TriangleDescriptor triangle;
    triangle.texture = texture;
    Triangle &tri = triangle.tri;
    size_t w = image->w, h = image->h;
    float centerX = 0.5 * w;
    float centerY = 0.5 * h;
    Matrix transformToScreen(w * 0.5 / scaleX(), 0, -centerX, -0.5,
                             0, h * -0.5 / scaleY(), -centerY, -0.5,
                             0, 0, 1, 0);
    tri = transform(transformToScreen, gridify(triangleIn));
    if(tri.p1.z >= 0 && tri.p2.z >= 0 && tri.p3.z >= 0)
        return;
    PlaneEq plane = PlaneEq(tri);
    if(plane.d >= -eps)
        return;
    plane.normal /= -plane.d;
    plane.d = -1;
    PlaneEq edge1 = PlaneEq(VectorF(0), tri.p1, tri.p2);
    PlaneEq edge2 = PlaneEq(VectorF(0), tri.p2, tri.p3);
    PlaneEq edge3 = PlaneEq(VectorF(0), tri.p3, tri.p1);
    edge1.d = 0; // assign to 0 because it helps optimization and it should already be 0
    edge2.d = 0; // assign to 0 because it helps optimization and it should already be 0
    edge3.d = 0; // assign to 0 because it helps optimization and it should already be 0

    PlaneEq uEquation = PlaneEq(VectorF(0), tri.p1, tri.p2);
    {
        float divisor = uEquation.eval(tri.p3);
        uEquation.normal /= divisor;
        // uEquation.d == 0
    }
    PlaneEq vEquation = PlaneEq(VectorF(0), tri.p3, tri.p1);
    {
        float divisor = vEquation.eval(tri.p2);
        vEquation.normal /= divisor;
        // vEquation.d == 0
    }

    const ColorI * texturePixels = texture->getPixels();
    size_t textureW = texture->w;
    size_t textureH = texture->h;

    int_fast32_t startY = sectionTop, endY = (int_fast32_t)sectionBottom - 1;
    if(tri.p1.z < -eps && tri.p2.z < -eps && tri.p3.z < -eps)
    {
        float y[3] = {-tri.p1.y / tri.p1.z, -tri.p2.y / tri.p2.z, -tri.p3.y / tri.p3.z};
        float minY, maxY;
        if(y[0] < y[1])
        {
            if(y[0] < y[2])
            {
                minY = y[0];
                if(y[1] < y[2])
                    maxY = y[2];
                else // y[1] >= y[2]
                    maxY = y[1];
            }
            else // y[0] >= y[2];
            {
                minY = y[2];
                maxY = y[1];
            }
        }
        else // y[0] >= y[1]
        {
            if(y[1] < y[2])
            {
                minY = y[1];
                if(y[0] < y[2])
                    maxY = y[2];
                else // y[0] >= y[2]
                    maxY = y[0];
            }
            else // y[1] >= y[2]
            {
                minY = y[2];
                maxY = y[0];
            }
        }
        if(maxY < startY || minY > endY)
            return;
        if(startY < minY)
        {
            startY = (int_fast32_t)std::ceil(minY);
        }
        if(endY > maxY)
        {
            endY = (int_fast32_t)std::floor(maxY);
        }
    }

    for(int_fast32_t y = startY; y <= endY; y++)
    {
        ColorI * imageLine = image->getLineAddress(y);
        VectorF startPixelCoords = VectorF(0, y, -1);
        VectorF stepPixelCoords = VectorF(1, 0, 0);
        float startInvZ = dot(plane.normal, startPixelCoords);
        float stepInvZ = dot(plane.normal, stepPixelCoords);
        float startEdge1V = dot(edge1.normal, startPixelCoords) + edge1.d * startInvZ;
        float stepEdge1V = dot(edge1.normal, stepPixelCoords) + edge1.d * stepInvZ;
        float startEdge2V = dot(edge2.normal, startPixelCoords) + edge2.d * startInvZ;
        float stepEdge2V = dot(edge2.normal, stepPixelCoords) + edge2.d * stepInvZ;
        float startEdge3V = dot(edge3.normal, startPixelCoords) + edge3.d * startInvZ;
        float stepEdge3V = dot(edge3.normal, stepPixelCoords) + edge3.d * stepInvZ;

        int_fast32_t startX = 0, endX = (int_fast32_t)w - 1;

        if(stepEdge1V > 0) // startEdge
        {
            if(-startEdge1V >= (endX + 1) * stepEdge1V)
                continue;
            if(startX * stepEdge1V < -startEdge1V)
            {
                startX = (int_fast32_t)std::ceil(-startEdge1V / stepEdge1V);
            }
        }
        else if(stepEdge1V == 0)
        {
            if(startEdge1V < 0)
                continue;
        }
        else // endEdge
        {
            if(startEdge1V < 0)
                continue;
            if(endX * stepEdge1V < -startEdge1V)
            {
                endX = (int_fast32_t)std::ceil(-startEdge1V / stepEdge1V) - 1;
            }
        }
        if(stepEdge2V > 0) // startEdge
        {
            if(-startEdge2V >= (endX + 1) * stepEdge2V)
                continue;
            if(startX * stepEdge2V < -startEdge2V)
            {
                startX = (int_fast32_t)std::ceil(-startEdge2V / stepEdge2V);
            }
        }
        else if(stepEdge2V == 0)
        {
            if(startEdge2V < 0)
                continue;
        }
        else // endEdge
        {
            if(startEdge2V < 0)
                continue;
            if(endX * stepEdge2V < -startEdge2V)
            {
                endX = (int_fast32_t)std::ceil(-startEdge2V / stepEdge2V) - 1;
            }
        }
        if(stepEdge3V > 0) // startEdge
        {
            if(-startEdge3V >= (endX + 1) * stepEdge3V)
                continue;
            if(startX * stepEdge3V < -startEdge3V)
            {
                startX = (int_fast32_t)std::ceil(-startEdge3V / stepEdge3V);
            }
        }
        else if(stepEdge3V == 0)
        {
            if(startEdge3V < 0)
                continue;
        }
        else // endEdge
        {
            if(startEdge3V < 0)
                continue;
            if(endX * stepEdge3V < -startEdge3V)
            {
                endX = (int_fast32_t)std::ceil(-startEdge3V / stepEdge3V) - 1;
            }
        }

        startPixelCoords += stepPixelCoords * startX;
        startInvZ += stepInvZ * startX;
        startEdge1V += stepEdge1V * startX;
        startEdge2V += stepEdge2V * startX;
        startEdge3V += stepEdge3V * startX;

        VectorF pixelCoords = startPixelCoords;
        float invZ = startInvZ;
        for(int_fast32_t x = startX; x <= endX; x++, pixelCoords += stepPixelCoords, invZ += stepInvZ)
        {
            if(invZ < zBuffer[x + y * w])
                continue;

            VectorF p = pixelCoords / invZ;

            VectorF triPos = VectorF(uEquation.eval(p), vEquation.eval(p), 1);
            VectorF t1 = VectorF(tri.t1.u, tri.t1.v, 1);
            VectorF t2 = VectorF(tri.t2.u, tri.t2.v, 1);
            VectorF t3 = VectorF(tri.t3.u, tri.t3.v, 1);
            float c1r = tri.c1.r;
            float c1g = tri.c1.g;
            float c1b = tri.c1.b;
            float c1a = tri.c1.a;
            float c2r = tri.c2.r;
            float c2g = tri.c2.g;
            float c2b = tri.c2.b;
            float c2a = tri.c2.a;
            float c3r = tri.c3.r;
            float c3g = tri.c3.g;
            float c3b = tri.c3.b;
            float c3a = tri.c3.a;
            VectorF texturePos = triPos.x * (t3 - t1) + triPos.y * (t2 - t1) + t1;
            ColorF c = RGBAF(triPos.x * (c3r - c1r) + triPos.y * (c2r - c1r) + c1r,
                             triPos.x * (c3g - c1g) + triPos.y * (c2g - c1g) + c1g,
                             triPos.x * (c3b - c1b) + triPos.y * (c2b - c1b) + c1b,
                             triPos.x * (c3a - c1a) + triPos.y * (c2a - c1a) + c1a);

            texturePos.x -= std::floor(texturePos.x);
            texturePos.y -= std::floor(texturePos.y);

            texturePos.x *= textureW;
            texturePos.y *= textureH;

            size_t u = limit<size_t>((size_t)texturePos.x, 0, textureW);
            size_t v = limit<size_t>((size_t)texturePos.y, 0, textureH);

            ColorI fragmentColor = colorize(c, texturePixels[u + textureW * (textureH - v - 1)]);

            if(fragmentColor.a == 0)
                continue;

            if(writeDepth)
                zBuffer[x + y * w] = invZ;

            ColorI & pixel = imageLine[x];
            pixel = compose(fragmentColor, pixel);
        }
    }
}

#ifndef __EMSCRIPTEN__
namespace
{
struct RenderThreadState final
{
    mutex lock;
    condition_variable cond;
    thread theThread;
    enum class State
    {
        Stopped,
        Waiting,
        Starting,
        Stopping,
        Running
    };
    State state = State::Stopped;
    function<void()> currentRunFunction = nullptr;
    void run_fn()
    {
        unique_lock<mutex> lockIt(lock);
        state = State::Waiting;
        cond.notify_all();
        while(state != State::Stopping)
        {
            while(state == State::Waiting)
                cond.wait(lockIt);
            if(state == State::Starting)
            {
                state = State::Running;
                cond.notify_all();
                currentRunFunction();
                state = State::Waiting;
                cond.notify_all();
            }
            else
                break;
        }
        state = State::Stopped;
        cond.notify_all();
    }
    void wait()
    {
        unique_lock<mutex> lockIt(lock);
        while(state == State::Running || state == State::Starting || state == State::Stopping)
            cond.wait(lockIt);
    }
    void stop()
    {
        {
            unique_lock<mutex> lockIt(lock);
            while(state == State::Running || state == State::Starting || state == State::Stopping)
                cond.wait(lockIt);
            if(state == State::Stopped)
                return;
            state = State::Stopping;
            cond.notify_all();
        }
        theThread.join();
    }
    void start(function<void()> fn)
    {
        assert(fn != nullptr);
        unique_lock<mutex> lockIt(lock);
        while(state == State::Running || state == State::Starting || state == State::Stopping)
            cond.wait(lockIt);
        if(state == State::Stopped)
        {
            theThread = thread([this](){run_fn();});
            while(state == State::Stopped)
                cond.wait(lockIt);
        }
        assert(state == State::Waiting);
        state = State::Starting;
        currentRunFunction = fn;
        cond.notify_all();
    }
    bool tryStart(function<void()> fn)
    {
        assert(fn != nullptr);
        unique_lock<mutex> lockIt(lock);
        if(state == State::Running || state == State::Starting)
            return false;
        while(state == State::Stopping)
            cond.wait(lockIt);
        if(state == State::Stopped)
        {
            theThread = thread([this](){run_fn();});
            while(state == State::Stopped)
                cond.wait(lockIt);
        }
        assert(state == State::Waiting);
        state = State::Starting;
        currentRunFunction = fn;
        cond.notify_all();
        return true;
    }
    bool available()
    {
        unique_lock<mutex> lockIt(lock);
        switch(state)
        {
        case State::Running:
        case State::Starting:
            return false;
        case State::Stopped:
        case State::Waiting:
        case State::Stopping:
            return true;
        }
        assert(false);
        return false;
    }
    bool stopped()
    {
        unique_lock<mutex> lockIt(lock);
        switch(state)
        {
        case State::Running:
        case State::Starting:
        case State::Waiting:
            return false;
        case State::Stopped:
        case State::Stopping:
            return true;
        }
        assert(false);
        return true;
    }
};
struct RenderThreadPool final
{
    vector<unique_ptr<RenderThreadState>> threads;
    RenderThreadPool()
    {
        size_t threadCount = thread::hardware_concurrency();
        if(threadCount == 0)
            threadCount = 1;
        threads.reserve(threadCount);
        for(size_t i = 0; i < threadCount; i++)
            threads.push_back(unique_ptr<RenderThreadState>(new RenderThreadState()));
    }
    ~RenderThreadPool()
    {
        for(unique_ptr<RenderThreadState> &t : threads)
        {
            t->stop();
        }
    }
    size_t nextThreadIndex = 0;
    void start(function<void()> fn)
    {
        for(size_t i = 0; i < threads.size(); i++)
        {
            size_t index = nextThreadIndex;
            nextThreadIndex = (nextThreadIndex + 1) % threads.size();
            if(threads[index]->tryStart(fn))
                return;
        }
        size_t index = nextThreadIndex;
        nextThreadIndex = (nextThreadIndex + 1) % threads.size();
        threads[index]->start(fn);
        return;
    }
};
RenderThreadPool &getRenderThreadPool()
{
    static RenderThreadPool retval;
    return retval;
}
}
#endif

void SoftwareRenderer::render(const Mesh &m)
{
    shared_ptr<const Image> texture = ((m.image != nullptr) ? m.image->getImage() : whiteTexture);

#ifndef __EMSCRIPTEN__
    size_t threadCount = thread::hardware_concurrency();
    if(threadCount == 0)
        threadCount = 1;
    size_t threadsLeft = threadCount;
    mutex threadsLeftLock;
    condition_variable threadsLeftCond;
#else
    const size_t threadCount = 1;
#endif

    size_t h = image->h;

    for(size_t i = 0; i < threadCount; i++)
    {
        function<void()> fn = [i, threadCount, h, texture,
#ifndef __EMSCRIPTEN__
                                &threadsLeft, &threadsLeftLock, &threadsLeftCond,
#endif // __EMSCRIPTEN__
                                &m, this]()
        {
            size_t startY = i * h / threadCount;
            size_t endY = (i + 1) * h / threadCount;
            if(startY == endY)
                return;
            for(Triangle tri : m.triangles)
            {
                renderTriangle(tri, startY, endY, texture);
            }
#ifndef __EMSCRIPTEN__
            unique_lock<mutex> lockIt(threadsLeftLock);
            threadsLeft--;
            threadsLeftCond.notify_all();
#endif // __EMSCRIPTEN__
        };
#ifndef __EMSCRIPTEN__
        getRenderThreadPool().start(fn);
#else
        fn();
#endif
    }
#ifndef __EMSCRIPTEN__
    unique_lock<mutex> lockIt(threadsLeftLock);
    while(threadsLeft > 0)
        threadsLeftCond.wait(lockIt);
#endif
}

shared_ptr<Texture> SoftwareRenderer::finish()
{
    return imageTexture;
}


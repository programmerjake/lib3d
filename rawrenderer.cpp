#ifndef __EMSCRIPTEN__
#include "rawrenderer.h"
#include <memory>
#include <fstream>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <cstring>

using namespace std;

namespace
{
shared_ptr<string> rawRendererFileName;
float rawRendererFrameRate = -1;
}

string getRawRendererOutputFile()
{
    if(rawRendererFileName)
        return *rawRendererFileName;
    const char * retval = getenv("LIB3D_OUTPUT_FILE");
    if(retval != nullptr && strlen(retval) != 0)
        return retval;
    return "lib3d-output.dat";
}

void setRawRendererOutputFile(string fileName)
{
    rawRendererFileName = make_shared<string>(fileName);
}

void setRawRendererFrameRate(float fps)
{
    if(fps < 1)
        throw logic_error("frame rate too small");
    rawRendererFrameRate = fps;
}

float getRawRendererFrameRate()
{
    if(rawRendererFrameRate > 0)
        return rawRendererFrameRate;
    const char * retval = getenv("LIB3D_FPS");
    if(retval != nullptr)
    {
        float fps;
        if(1 == sscanf(retval, " %g", &fps) && isfinite(fps) && fps > 1 && fps < 1000)
            return fps;
    }
    return 24;
}

namespace
{
class KeyGetter
{
    termios originalTermios;
    KeyGetter(const KeyGetter &) = delete;
    void operator =(const KeyGetter &) = delete;
public:
    KeyGetter()
    {
        termios newTermios;
        tcgetattr(0, &newTermios);
        originalTermios = newTermios;
        cfmakeraw(&newTermios);
        tcsetattr(0, TCSANOW, &newTermios);
    }
    ~KeyGetter()
    {
        tcsetattr(0, TCSANOW, &originalTermios);
    }
    bool kbhit()
    {
        timeval tv = {0};
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(0, &fds);
        return select(1, &fds, nullptr, nullptr, &tv) > 0;
    }
    int getch()
    {
        uint8_t retval;
        if(read(0, (void *)&retval, sizeof(retval)) > 0)
            return retval;
        return EOF;
    }
};

class RawRenderer : public WindowRenderer
{
    void encodeImageData()
    {
        if(!outputStream)
            return;
        ostream &os = *outputStream;
        os.write((const char *)&imageData[0], imageData.size());
    }
    void encoderThreadFn()
    {
        encoderLock.lock();
        while(!done)
        {
            if(!hasImage)
            {
                encoderCond.wait(encoderLock);
                continue;
            }
            encoderLock.unlock();
            encodeImageData();
            encoderLock.lock();
            hasImage = false;
            encoderCond.notify_all();
        }
        encoderLock.unlock();
    }
    shared_ptr<ImageRenderer> imageRenderer;
    size_t w, h;
    float aspectRatio, frameRate;
    vector<uint8_t> imageData;
    shared_ptr<KeyGetter> keyGetter;
    double deltaTime, currentTime = 0;
    thread encoderThread;
    bool done = false, hasImage = false;
    mutex encoderLock;
    condition_variable_any encoderCond;
    shared_ptr<ofstream> outputStream;
public:
    RawRenderer(string fileName = getRawRendererOutputFile(), float frameRate = getRawRendererFrameRate())
        : frameRate(frameRate), keyGetter(make_shared<KeyGetter>()), deltaTime(1 / frameRate)
    {
        w = getDefaultRendererWidth();
        h = getDefaultRendererHeight();
        imageData.resize(4 * w * h); // rgba
        aspectRatio = getDefaultRendererAspectRatio();
        imageRenderer = makeImageRenderer(w, h, aspectRatio);
        if(fileName != "")
        {
            outputStream = make_shared<ofstream>(fileName.c_str(), ios::binary);
            if(!outputStream || !*outputStream)
                throw runtime_error("can't open '" + fileName + "'");
        }
        encoderThread = thread(&RawRenderer::encoderThreadFn, this);
    }
    virtual ~RawRenderer()
    {
        encoderLock.lock();
        done = true;
        encoderCond.notify_all();
        encoderLock.unlock();
        encoderThread.join();
    }
protected:
    virtual void clearInternal(ColorF bg) override
    {
        imageRenderer->clear(bg);
    }
public:
    virtual void calcScales() override
    {
        Renderer::calcScales(w, h, aspectRatio);
    }
    virtual void render(const Mesh &m) override
    {
        imageRenderer->render(m);
    }
    virtual void enableWriteDepth(bool v) override
    {
        imageRenderer->enableWriteDepth(v);
    }
    virtual void flip() override
    {
        shared_ptr<const Image> pimage = imageRenderer->finish()->getImage();
        currentTime += deltaTime;
        assert(pimage != nullptr);
        const Image & image = *pimage;
        assert(image.w == w);
        assert(image.h == h);
        encoderLock.lock();
        while(hasImage)
        {
            encoderCond.wait(encoderLock);
        }
        uint32_t *imageDataPtr = (uint32_t *)&imageData[0];
        for(size_t y = 0; y < h; y++)
        {
            for(size_t x = 0; x < w; x++)
            {
                *imageDataPtr++ = (uint32_t)image.getPixel(x, y);
            }
        }
        hasImage = true;
        encoderCond.notify_all();
        encoderLock.unlock();

        while(keyGetter->kbhit())
        {
            int ch = keyGetter->getch();
            if(ch == 0x1B || ch == 'q' || ch == 'Q' || ch == 0x3)
                exit(0);
        }
        cout << timer() << "\x1b[K\r" << flush;
    }
    virtual double timer() override
    {
        return currentTime;
    }
    virtual shared_ptr<Texture> preloadTexture(shared_ptr<Texture> texture) override
    {
        return imageRenderer->preloadTexture(texture);
    }
};
}

shared_ptr<WindowRenderer> makeRawRenderer()
{
    return make_shared<RawRenderer>();
}
#endif
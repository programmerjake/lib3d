#include "svgarenderer.h"
#include <vga.h>
#include <vgagl.h>
#include <mutex>
#include <condition_variable>
#include <climits>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <thread>

using namespace std;

#if 0
#define vga_init() (0)
#define vga_setmode(a) (0)
#define gl_setpixelrgb(x, y, r, g, b) do {} while(false)
#define gl_copyscreen(gc) do {} while(false)
namespace
{
vga_modeinfo *my_vga_getmodeinfo(int mode)
{
    static vga_modeinfo retval = {0};
    retval.colors = 1 << 24;
    retval.width = 640;
    retval.height = 480;
    return &retval;
}
}
#define vga_getmodeinfo my_vga_getmodeinfo
#define vga_hasmode(mode) (1)
#define gl_allocatecontext() (new GraphicsContext)
#define gl_freecontext(gc) do {delete gc;} while(0)
#define gl_setcontextvga(m) (0)
#define gl_getcontext(gc) do {} while(0)
#define gl_setrgbpalette() do {} while(0)
#define gl_setcontextvgavirtual(m) (0)
#define vga_lastmodenumber() (1)
#endif

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

class SVGARenderer : public WindowRenderer
{
private:
    shared_ptr<ImageRenderer> imageRenderer;
    shared_ptr<GraphicsContext> physicalContext;
    vector<uint32_t> pixels;
    size_t w, h;
    float aspectRatio, defaultAspectRatio;
    enum class Status
    {
        Initializing,
        Error,
        Running,
        WritingFrame,
        Closing,
    };
    Status status;
    string errorMessage;
    mutex statusLock;
    condition_variable_any statusCond;
    thread renderThread;
    shared_ptr<KeyGetter> keyGetter;
    static void init()
    {
        static bool didInit = false;
        if(didInit)
            return;
        didInit = true;
        vga_init();
        atexit([](){vga_setmode(TEXT);});
    }
    void writeFrame()
    {
        for(size_t y = 0; y < h; y++)
        {
            for(size_t x = 0; x < w; x++)
            {
                ColorI color = (ColorI)pixels[x + y * w];
                gl_setpixelrgb(x, y, color.r, color.g, color.b);
            }
        }
        gl_copyscreen(physicalContext.get());
    }

    static int log2i(unsigned v)
    {
        int retval = -1;
        while(v != 0)
        {
            retval++;
            v >>= 1;
        }
        return retval;
    }
    float modeDistance(const vga_modeinfo *minfo)
    {
        return abs(minfo->width - w) + abs(minfo->height - h) - 0.1f * log2i(minfo->colors);
    }
    void renderThreadFn()
    {
        lock_guard<mutex> lockIt(statusLock);
        try
        {
            int bestMode = 0;
            float bestModeDistance = 0;
            int bestModeW = 0;
            int bestModeH = 0;
            bool needSetPalette = false;
            for(int mode = 1; mode <= vga_lastmodenumber(); mode++)
            {
                if(!vga_hasmode(mode))
                    continue;
                const vga_modeinfo *minfo = vga_getmodeinfo(mode);
                if(minfo->colors != 1 << 8 && minfo->colors != 1 << 15 && minfo->colors != 1 << 16 && minfo->colors != 1 << 24)
                    continue;
                float currentModeDistance = modeDistance(minfo);
                if(bestMode == 0 || currentModeDistance < bestModeDistance)
                {
                    bestMode = mode;
                    bestModeDistance = currentModeDistance;
                    bestModeW = minfo->width;
                    bestModeH = minfo->height;
                    needSetPalette = (minfo->colors == 1 << 8);
                }
            }
            if(bestMode == 0)
            {
                throw runtime_error("no valid video modes");
            }
            w = bestModeW;
            h = bestModeH;
            vga_setmode(bestMode);
            physicalContext = shared_ptr<GraphicsContext>(gl_allocatecontext(), [](GraphicsContext *v){gl_freecontext(v);});
            gl_setcontextvga(bestMode);
            if(needSetPalette)
                gl_setrgbpalette();
            gl_getcontext(physicalContext.get());
            gl_setcontextvgavirtual(bestMode);
            status = Status::Running;
            statusCond.notify_all();
            while(status != Status::Closing)
            {
                while(status == Status::Running)
                    statusCond.wait(statusLock);
                if(status == Status::WritingFrame)
                {
                    statusLock.unlock();
                    try
                    {
                        writeFrame();
                    }
                    catch(...)
                    {
                        statusLock.lock();
                        throw;
                    }
                    statusLock.lock();
                    if(status == Status::WritingFrame)
                    {
                        status = Status::Running;
                        statusCond.notify_all();
                    }
                }
            }
            vga_setmode(TEXT);
        }
        catch(exception &e)
        {
            errorMessage = e.what();
            status = Status::Error;
            statusCond.notify_all();
        }
    }
    void stopRenderThread()
    {
        statusLock.lock();
        status = Status::Closing;
        statusCond.notify_all();
        statusLock.unlock();
        renderThread.join();
    }
public:
    SVGARenderer()
        : defaultAspectRatio(getDefaultRendererAspectRatio())
    {
        init();
        w = getDefaultRendererWidth();
        h = getDefaultRendererHeight();
        {
            lock_guard<mutex> lockIt(statusLock);
            status = Status::Initializing;
            renderThread = thread(&SVGARenderer::renderThreadFn, this);
            while(status == Status::Initializing)
            {
                statusCond.wait(statusLock);
            }
            if(status == Status::Error)
            {
                renderThread.join();
                throw runtime_error(errorMessage);
            }
        }
        try
        {
            pixels.resize(w * h, 0);
            aspectRatio = defaultAspectRatio;
            imageRenderer = makeImageRenderer(w, h, aspectRatio);
            keyGetter = make_shared<KeyGetter>();
        }
        catch(...)
        {
            stopRenderThread();
            throw;
        }
    }
    virtual ~SVGARenderer()
    {
        keyGetter = nullptr;
        stopRenderThread();
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
        assert(pimage != nullptr);
        const Image & image = *pimage;
        assert(image.w == w);
        assert(image.h == h);
        {
            lock_guard<mutex> lockIt(statusLock);
            while(status == Status::WritingFrame)
                statusCond.wait(statusLock);
            if(status == Status::Error)
            {
                throw runtime_error(errorMessage);
            }
            for(size_t y = 0; y < h; y++)
            {
                for(size_t x = 0; x < w; x++)
                {
                    pixels[x + y * w] = (uint32_t)image.getPixel(x, y);
                }
            }
            status = Status::WritingFrame;
            statusCond.notify_all();
        }
        calcFPS();
        while(keyGetter->kbhit())
        {
            int ch = keyGetter->getch();
            if(ch == 0x1B || ch == 'q' || ch == 'Q' || ch == 0x3)
                exit(0);
        }
    }
    virtual shared_ptr<Texture> preloadTexture(shared_ptr<Texture> texture) override
    {
        return imageRenderer->preloadTexture(texture);
    }
};
}

shared_ptr<WindowRenderer> makeSVGARenderer()
{
    return make_shared<SVGARenderer>();
}

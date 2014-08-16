#include "ffmpeg_renderer.h"
#include <memory>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <fstream>

using namespace std;

namespace
{
shared_ptr<string> ffmpegFileName;
shared_ptr<string> ffmpegFormat;
shared_ptr<string> ffmpegCodec;
float ffmpegFrameRate = 30;
}

string getFFmpegOutputFile()
{
    if(ffmpegFileName)
        return *ffmpegFileName;
    return "lib3d-output.mp4";
}

void setFFmpegOutputFile(string fileName)
{
    ffmpegFileName = make_shared<string>(fileName);
}

string getFFmpegOutputFormat()
{
    if(ffmpegFormat)
        return *ffmpegFormat;
    return "mp4";
}

void setFFmpegOutputFormat(string format)
{
    ffmpegFormat = make_shared<string>(format);
}

string getFFmpegOutputCodec()
{
    if(ffmpegCodec)
        return *ffmpegCodec;
    return "mpeg4";
}

void setFFmpegOutputCodec(string codec)
{
    ffmpegCodec = make_shared<string>(codec);
}

void setFFmpegFrameRate(float fps)
{
    if(fps < 1)
        throw logic_error("ffmpeg frame rate too small");
    ffmpegFrameRate = fps;
}

float getFFmpegFrameRate()
{
    return ffmpegFrameRate;
}

#if 0

namespace
{
struct Writer
{
    Writer(const Writer &) = delete;
    void operator =(const Writer &) = delete;
    virtual void write(const void *buffer, size_t size) = 0;
    virtual void flush() = 0;
    virtual void close() = 0;
    virtual ~Writer()
    {
    }
};

class FileWriter : public Writer
{
private:
    ofstream os;
public:
    FileWriter(string fileName)
        : os(fileName.c_str(), ios::binary)
    {
        if(!os)
            throw runtime_error("can't open " + fileName);
    }

    virtual void write(const void *buffer, size_t size) override
    {
        os.write((const char *)buffer, size);
        if(!os)
            throw runtime_error("can't write to file");
    }

    virtual void flush() override
    {
        os.flush();
    }

    virtual void close() override
    {
        os.close();
    }
};

class FFmpegRenderer : public WindowRenderer
{
    static void init()
    {
        static bool didInit = false;
        if(didInit)
            return;
        didInit = true;
        avcodec_register_all();
    }
    static AVCodec *findCodec(string name)
    {
        for(AVCodec *retval = av_codec_next(nullptr); retval != nullptr; retval = av_codec_next(retval))
        {
            if(retval->type != AVMEDIA_TYPE_VIDEO)
                continue;
            if(!av_codec_is_encoder(retval))
                continue;
            if(retval->name == name || name == "")
                return retval;
        }
        return nullptr;
    }
    void calcTimeBase()
    {
        if(theCodec->supported_framerates == nullptr)
        {
            if(abs(frameRate - floor(frameRate)) < 1e-4)
            {
                theCodecContext->time_base = (AVRational){1, (int)floor(frameRate + 0.5)};
            }
            else if(abs(frameRate - 24.0 * 1000 / 1001) < 1e-4)
            {
                theCodecContext->time_base = (AVRational){1001, 24000};
            }
            else
            {
                theCodecContext->time_base = (AVRational){0, 0};
                for(int denom = 1; denom <= 1001; denom++)
                {
                    if(abs(frameRate * denom - floor(frameRate * denom)) < 1e-4)
                    {
                        theCodecContext->time_base = (AVRational){denom, (int)floor(frameRate * denom + 0.5)};
                        break;
                    }
                }
                if(theCodecContext->time_base.num == 0)
                {
                    int num = 1 << 14;
                    int denom = (int)floor(frameRate * (1 << 14) + 0.5);
                    int v = num | denom; // or together so smallest set bit is divisor of both
                    int powerOf2Factor = v - (v & (v - 1)); // find smallest set bit
                    num /= powerOf2Factor;
                    denom /= powerOf2Factor;
                    theCodecContext->time_base = (AVRational){num, denom};
                }
            }
        }
        else
        {
            theCodecContext->time_base = (AVRational){0, 0};
            float closestDistance = -1;
            for(const AVRational *currentTimeBase = theCodec->supported_framerates; currentTimeBase->num != 0 && currentTimeBase->den != 0; currentTimeBase++)
            {
                float fpsValue = (float)currentTimeBase->den / currentTimeBase->num;
                float distance = abs(fpsValue - frameRate);
                if(closestDistance < 0 || distance < closestDistance)
                {
                    theCodecContext->time_base = *currentTimeBase;
                    closestDistance = distance;
                }
            }
            if(closestDistance == -1)
                throw logic_error("ffmpeg codec has no valid time bases");
        }
    }
    AVCodec *theCodec;
    AVCodecContext *theCodecContext;
    AVFrame *picture;
    SwsContext *swscaleContext;
    SwsFilter *defaultFilter;
    shared_ptr<Writer> theWriter;
    shared_ptr<ImageRenderer> imageRenderer;
    size_t w, h;
    float aspectRatio, frameRate;
public:
    FFmpegRenderer(shared_ptr<Writer> theWriter, string codec = getFFmpegOutputCodec(), string format = getFFmpegOutputFormat(), float frameRate = getFFmpegFrameRate())
        : theWriter(theWriter), frameRate(frameRate)
    {
        w = getDefaultRendererWidth();
        h = getDefaultRendererHeight();
        aspectRatio = getDefaultRendererAspectRatio();
        imageRenderer = makeImageRenderer(w, h, aspectRatio);
        theCodec = findCodec(codec);
        if(theCodec == nullptr)
            throw runtime_error("can't find codec " + codec);
        theCodecContext = avcodec_alloc_context3(theCodec);
        if(theCodecContext == nullptr)
        {
            throw runtime_error("can't create codec context");
        }
        theCodecContext->width = w;
        theCodecContext->height = h;
        calcTimeBase();
        assert(theCodec->pix_fmts);
        theCodecContext->pix_fmt = *theCodec->pix_fmts;
        picture = avcodec_alloc_frame();
        if(picture == nullptr)
        {
            avcodec_close(theCodecContext);
            av_free(theCodecContext);
            throw runtime_error("can't allocate frame");
        }
        SwsFilter *defaultFilter = sws_getDefaultFilter(0, 0, 0, 0, 0, 0, 0);
        swscaleContext = sws_getCachedContext(nullptr, w, h, PIX_FMT_RGB32, w, h, theCodecContext->pix_fmt, 0, defaultFilter, defaultFilter, nullptr);
    }
    virtual ~FFmpegRenderer()
    {
        #error add new objects to destructor
        avcodec_close(theCodecContext);
        av_free(theCodecContext);
        av_free(picture);
    }
    FFmpegRenderer(string fileName = getFFmpegOutputFile(), string codec = getFFmpegOutputCodec(), string format = getFFmpegOutputFormat(), float frameRate = getFFmpegFrameRate())
        : FFmpegRenderer(make_shared<FileWriter>(fileName), codec, format, frameRate)
    {
    }
    virtual ~LibAARenderer()
    {
        aa_uninitkbd(context);
        aa_close(context);
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
        #error finish
        #error add termination code to check for keyboard hit
    }
};
}
#else
#warning finish implementing ffmpeg renderer
#endif

shared_ptr<WindowRenderer> makeFFmpegRenderer()
{
    throw runtime_error("ffmpeg renderer not implemented");
}

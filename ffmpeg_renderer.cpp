#include "ffmpeg_renderer.h"
#include <memory>
#include <libavcodec/avcodec.h>

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
    virtual void close() = 0;
    virtual ~Writer()
    {
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
    static AVCodec *findCodec()
    {
        AVCodec *retval = findCodec("mpeg4");
        if(retval)
            return retval;
        return findCodec("");
    }
    AVCodec *theCodec;
    AVCodecContext *theCodecContext;
    shared_ptr<Writer> theWriter;
    shared_ptr<ImageRenderer> imageRenderer;
    size_t w, h;
    float aspectRatio;
public:
    FFmpegRenderer(shared_ptr<Writer> theWriter)
        : theWriter(theWriter)
    {
        w = getDefaultRendererWidth();
        h = getDefaultRendererHeight();
        aspectRatio = getDefaultRendererAspectRatio();
        imageRenderer = makeImageRenderer(w, h, aspectRatio);
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

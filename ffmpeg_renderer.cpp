#include "ffmpeg_renderer.h"
#include <memory>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#include <fstream>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <iostream>

using namespace std;

namespace
{
shared_ptr<string> ffmpegFileName;
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

class FFmpegRenderer : public WindowRenderer
{
    static void init()
    {
        static bool didInit = false;
        if(didInit)
            return;
        didInit = true;
        avcodec_register_all();
        av_register_all();
    }
    static AVCodec *findCodec(string name)
    {
        return avcodec_find_encoder_by_name(name.c_str());
    }
    static AVOutputFormat *findFormat(string name)
    {
        for(AVOutputFormat *retval = av_oformat_next(nullptr); retval; retval = av_oformat_next(retval))
        {
            if(retval->name == name)
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
    void writeEncodedChunk(const void *buffer, size_t chunkSize)
    {
        theWriter->write(buffer, chunkSize);
    }
    AVCodec *theCodec;
    AVCodecContext *theCodecContext;
    AVFrame *picture;
    SwsContext *swscaleContext;
    SwsFilter *defaultFilter;
    shared_ptr<ImageRenderer> imageRenderer;
    size_t w, h;
    float aspectRatio, frameRate;
    AVDictionary *optionsDictionary;
    vector<uint8_t> imageData, packetBuffer;
    shared_ptr<KeyGetter> keyGetter;
    AVOutputFormat *outputFormat;
    AVFormatContext *theFormatContext;
    AVStream *videoStream;
public:
    FFmpegRenderer(string fileName = getFFmpegOutputFile(), string codec = getFFmpegOutputCodec(), string format = getFFmpegOutputFormat(), float frameRate = getFFmpegFrameRate())
        : theWriter(theWriter), frameRate(frameRate), keyGetter(make_shared<KeyGetter>())
    {
        init();
        w = getDefaultRendererWidth();
        h = getDefaultRendererHeight();
        imageData.resize(4 * w * h); // rgba
        packetBuffer.resize(1 << 20);
        aspectRatio = getDefaultRendererAspectRatio();
        imageRenderer = makeImageRenderer(w, h, aspectRatio);
        theCodec = findCodec(codec);
        if(theCodec == nullptr)
            throw runtime_error("can't find codec " + codec);
        outputFormat = findFormat(format);
        if(outputFormat == nullptr)
            throw runtime_error("can't find format " + format);
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
            av_free(theCodecContext);
            throw runtime_error("can't allocate frame");
        }
        if(avpicture_alloc((AVPicture *)picture, theCodecContext->pix_fmt, w, h) < 0)
        {
            av_free(theCodecContext);
            av_free(picture);
            throw runtime_error("can't allocate picture");
        }
        optionsDictionary = nullptr;
        if(avcodec_open2(theCodecContext, theCodec, &optionsDictionary) < 0)
        {
            av_dict_free(&optionsDictionary);
            av_free(theCodecContext);
            avpicture_free((AVPicture *)picture);
            av_free(picture);
            throw runtime_error((string)"can't open codec " + theCodec->name);
        }
        defaultFilter = sws_getDefaultFilter(0, 0, 0, 0, 0, 0, 0);
        if(defaultFilter == nullptr)
        {
            avcodec_close(theCodecContext);
            av_dict_free(&optionsDictionary);
            av_free(theCodecContext);
            avpicture_free((AVPicture *)picture);
            av_free(picture);
            throw runtime_error("can't create default filter");
        }
        swscaleContext = sws_getCachedContext(nullptr, w, h, PIX_FMT_RGB32, w, h, theCodecContext->pix_fmt, SWS_BILINEAR, defaultFilter, defaultFilter, nullptr);
        if(defaultFilter == nullptr)
        {
            sws_freeFilter(defaultFilter);
            avcodec_close(theCodecContext);
            av_dict_free(&optionsDictionary);
            av_free(theCodecContext);
            avpicture_free((AVPicture *)picture);
            av_free(picture);
            throw runtime_error("can't create swscale context");
        }
        theFormatContext = avformat_alloc_context();
        if(theFormatContext == nullptr)
        {
            sws_freeContext(swscaleContext);
            sws_freeFilter(defaultFilter);
            avcodec_close(theCodecContext);
            av_dict_free(&optionsDictionary);
            av_free(theCodecContext);
            avpicture_free((AVPicture *)picture);
            av_free(picture);
        }
        theFormatContext->oformat = outputFormat;
        videoStream = avformat_new_stream(theFormatContext, theCodec);
    }
    virtual ~FFmpegRenderer()
    {
        for(;;)
        {
            int size = avcodec_encode_video(theCodecContext, &packetBuffer[0], packetBuffer.size(), nullptr);
            if(size == 0)
                break;
            writeEncodedChunk((const void *)&packetBuffer[0], size);
        }
        av_free(theFormatContext);
        sws_freeContext(swscaleContext);
        sws_freeFilter(defaultFilter);
        avcodec_close(theCodecContext);
        av_dict_free(&optionsDictionary);
        av_free(theCodecContext);
        avpicture_free((AVPicture *)picture);
        av_free(picture);
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
        uint32_t *imageDataPtr = (uint32_t *)&imageData[0];
        for(size_t y = 0; y < h; y++)
        {
            for(size_t x = 0; x < w; x++)
            {
                *imageDataPtr++ = (uint32_t)image.getPixel(x, y);
            }
        }
        AVPicture srcPicture;
        avpicture_fill(&srcPicture, &imageData[0], PIX_FMT_RGB32, w, h);
        sws_scale(swscaleContext, srcPicture.data, srcPicture.linesize, 0, h, picture->data, picture->linesize);
        int size = avcodec_encode_video(theCodecContext, &packetBuffer[0], packetBuffer.size(), picture);
        if(size > 0)
            writeEncodedChunk((const void *)&packetBuffer[0], size);
        while(keyGetter->kbhit())
        {
            int ch = keyGetter->getch();
            if(ch == 0x1B || ch == 'q' || ch == 'Q' || ch == 0x3)
                exit(0);
        }
    }
};
}
#else
#warning finish implementing ffmpeg renderer
#endif

shared_ptr<WindowRenderer> makeFFmpegRenderer()
{
    return make_shared<FFmpegRenderer>();
}

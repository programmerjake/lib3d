#include "ffmpeg_renderer.h"
#include <memory>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavformat/avio.h>
#include <libavutil/mathematics.h>
}
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

using namespace std;

namespace
{
shared_ptr<string> ffmpegFileName;
float ffmpegFrameRate = -1;
size_t ffmpegBitRate = 0;
function<void(void)> ffmpegRenderStatusCallback = nullptr;
}

string getFFmpegOutputFile()
{
    if(ffmpegFileName)
        return *ffmpegFileName;
    const char * retval = getenv("LIB3D_OUTPUT_FILE");
    if(retval != nullptr && strlen(retval) != 0)
        return retval;
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
    if(ffmpegFrameRate > 0)
        return ffmpegFrameRate;
    const char * retval = getenv("LIB3D_FFMPEG_FPS");
    if(retval != nullptr)
    {
        float fps;
        if(1 == sscanf(retval, " %g", &fps) && isfinite(fps) && fps > 1 && fps < 1000)
            return fps;
    }
    return 24;
}

void setFFmpegBitRate(size_t bitRate)
{
    bitRate = max((size_t)20000, bitRate);
    ffmpegBitRate = bitRate;
}

void registerFFmpegRenderStatusCallback(function<void(void)> fn)
{
    if(ffmpegRenderStatusCallback == nullptr)
        ffmpegRenderStatusCallback = fn;
    else
    {
        function<void(void)> prevFn = ffmpegRenderStatusCallback;
        ffmpegRenderStatusCallback = [fn, prevFn](){prevFn(); fn();};
    }
}

namespace
{
size_t getFFmpegBitRate(size_t pixelCount)
{
    if(ffmpegBitRate > 0)
        return ffmpegBitRate;
    const char * retval = getenv("LIB3D_FFMPEG_BITRATE");
    if(retval != nullptr)
    {
        int bitRate;
        if(1 == sscanf(retval, " %d", &bitRate))
            return max((size_t)20000, (size_t)bitRate);
    }
    return (200000 + pixelCount) * getFFmpegFrameRate() / 24;
}
}

size_t getFFmpegBitRate()
{
    return getFFmpegBitRate(300000);
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

class FFmpegRenderer : public WindowRenderer
{
    static void init()
    {
        static bool didInit = false;
        if(didInit)
            return;
        didInit = true;
        av_register_all();
    }
    static void calcTimeBase(const AVCodec *theCodec, AVCodecContext *theCodecContext, float &frameRate)
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
        frameRate = (float)theCodecContext->time_base.den / theCodecContext->time_base.num;
    }
    void writePacket()
    {
        size_t size = packetBuffer.size();
        if(size > 0)
        {
            AVPacket thePacket;
            av_init_packet(&thePacket);
            if(theCodecContext->coded_frame->pts != (decltype(theCodecContext->coded_frame->pts))AV_NOPTS_VALUE)
                thePacket.pts = av_rescale_q(theCodecContext->coded_frame->pts, theCodecContext->time_base, theStream->time_base);
            if(theCodecContext->coded_frame->key_frame)
                thePacket.flags |= AV_PKT_FLAG_KEY;
            thePacket.stream_index = theStream->index;
            thePacket.data = &packetBuffer[0];
            thePacket.size = size;
            if(av_interleaved_write_frame(theFormatContext, &thePacket) != 0)
            {
                throw runtime_error("can't write to output video");
            }
        }
    }
    void encodeImageData()
    {
        AVPicture srcPicture;
        avpicture_fill(&srcPicture, &imageData[0], PIX_FMT_RGB32, w, h);
        sws_scale(swscaleContext, srcPicture.data, srcPicture.linesize, 0, h, picture->data, picture->linesize);
        packetBuffer.resize(packetBuffer.capacity());
        int size = avcodec_encode_video(theCodecContext, &packetBuffer[0], packetBuffer.size(), picture);
        packetBuffer.resize(size);
        writePacket();
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
    AVFrame *picture;
    SwsContext *swscaleContext;
    SwsFilter *defaultFilter;
    shared_ptr<ImageRenderer> imageRenderer;
    size_t w, h;
    float aspectRatio, frameRate;
    vector<uint8_t> imageData, packetBuffer;
    shared_ptr<KeyGetter> keyGetter;
    AVFormatContext *theFormatContext;
    AVStream *theStream;
    AVOutputFormat *theFormat;
    AVCodec *theCodec;
    AVCodecContext *theCodecContext;
    double deltaTime, currentTime = 0;
    thread encoderThread;
    bool done = false, hasImage = false;
    mutex encoderLock;
    condition_variable_any encoderCond;
public:
    FFmpegRenderer(function<shared_ptr<ImageRenderer>(size_t w, size_t h, float aspectRatio)> imageRendererMaker, string fileName = getFFmpegOutputFile(), float frameRate = getFFmpegFrameRate())
        : frameRate(frameRate), keyGetter(make_shared<KeyGetter>())
    {
        init();
        w = getDefaultRendererWidth();
        h = getDefaultRendererHeight();
        imageData.resize(4 * w * h); // rgba
        packetBuffer.reserve(1 << 18);
        aspectRatio = getDefaultRendererAspectRatio();
        imageRenderer = imageRendererMaker(w, h, aspectRatio);

        theFormat = av_guess_format(nullptr, fileName.c_str(), nullptr);
        if(theFormat == nullptr)
        {
            const char * const defaultFormat = "ogg";
            cout << "can't guess format from filename : trying " << defaultFormat << endl;
            theFormat = av_guess_format(defaultFormat, nullptr, nullptr);
        }
        if(theFormat == nullptr)
        {
            throw runtime_error("can't find valid format");
        }
        else if(theFormat->video_codec == CODEC_ID_NONE)
        {
            throw runtime_error("format doesn't support video");
        }
        theFormatContext = avformat_alloc_context();
        if(theFormatContext == nullptr)
        {
            throw runtime_error("can't allocate format context");
        }
        theFormatContext->oformat = theFormat;
        strncpy(theFormatContext->filename, fileName.c_str(), sizeof(theFormatContext->filename));

        theCodec = avcodec_find_encoder(theFormat->video_codec);
        if(theCodec == nullptr)
        {
            avformat_free_context(theFormatContext);
            throw runtime_error("can't find valid codec");
        }

        theStream = avformat_new_stream(theFormatContext, theCodec);
        if(theStream == nullptr)
        {
            avformat_free_context(theFormatContext);
            throw runtime_error("can't create stream");
        }
        theCodecContext = theStream->codec;
        theCodecContext->width = w;
        theCodecContext->height = h;
        theCodecContext->bit_rate = getFFmpegBitRate(w * h);
        calcTimeBase(theCodec, theCodecContext, frameRate);
        theStream->time_base = theCodecContext->time_base;
        deltaTime = 1 / (double)frameRate;
        this->frameRate = frameRate;
        assert(theCodec->pix_fmts);
        theCodecContext->pix_fmt = *theCodec->pix_fmts;
        picture = avcodec_alloc_frame();
        if(picture == nullptr)
        {
            avformat_free_context(theFormatContext);
            throw runtime_error("can't allocate frame");
        }
        if(avpicture_alloc((AVPicture *)picture, theCodecContext->pix_fmt, w, h) < 0)
        {
            av_free(picture);
            avformat_free_context(theFormatContext);
            throw runtime_error("can't allocate picture");
        }
        if(theFormat->flags & AVFMT_GLOBALHEADER)
            theCodecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;

        AVDictionary *options = nullptr;
        av_dict_set(&options, "threads", "auto", 0);

        if(avcodec_open2(theCodecContext, theCodec, &options) < 0)
        {
            avpicture_free((AVPicture *)picture);
            av_free(picture);
            avformat_free_context(theFormatContext);
            throw runtime_error((string)"can't open codec " + theCodec->name);
        }
        defaultFilter = sws_getDefaultFilter(0, 0, 0, 0, 0, 0, 0);
        if(defaultFilter == nullptr)
        {
            avcodec_close(theCodecContext);
            avpicture_free((AVPicture *)picture);
            av_free(picture);
            avformat_free_context(theFormatContext);
            throw runtime_error("can't create default filter");
        }
        swscaleContext = sws_getCachedContext(nullptr, w, h, PIX_FMT_RGB32, w, h, theCodecContext->pix_fmt, SWS_BILINEAR, defaultFilter, defaultFilter, nullptr);
        if(defaultFilter == nullptr)
        {
            sws_freeFilter(defaultFilter);
            avcodec_close(theCodecContext);
            avpicture_free((AVPicture *)picture);
            av_free(picture);
            avformat_free_context(theFormatContext);
            throw runtime_error("can't create swscale context");
        }
        if((theFormat->flags & AVFMT_NOFILE) == 0 && avio_open(&theFormatContext->pb, fileName.c_str(), AVIO_FLAG_WRITE) < 0)
        {
            sws_freeContext(swscaleContext);
            sws_freeFilter(defaultFilter);
            avcodec_close(theCodecContext);
            avpicture_free((AVPicture *)picture);
            av_free(picture);
            avformat_free_context(theFormatContext);
            throw runtime_error("can't write to '" + fileName + "'");
        }

        avformat_write_header(theFormatContext, nullptr);
        encoderThread = thread(&FFmpegRenderer::encoderThreadFn, this);
    }
    virtual ~FFmpegRenderer()
    {
        encoderLock.lock();
        done = true;
        encoderCond.notify_all();
        encoderLock.unlock();
        encoderThread.join();
        for(;;)
        {
            packetBuffer.resize(packetBuffer.capacity());
            int size = avcodec_encode_video(theCodecContext, &packetBuffer[0], packetBuffer.size(), nullptr);
            packetBuffer.resize(size);
            if(size == 0)
                break;
            writePacket();
        }
        av_write_trailer(theFormatContext);
        if((theFormat->flags & AVFMT_NOFILE) == 0)
        {
            avio_close(theFormatContext->pb);
        }
        sws_freeContext(swscaleContext);
        sws_freeFilter(defaultFilter);
        avcodec_close(theCodecContext);
        avpicture_free((AVPicture *)picture);
        av_free(picture);
        avformat_free_context(theFormatContext);
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
        if(ffmpegRenderStatusCallback)
            ffmpegRenderStatusCallback();
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

shared_ptr<WindowRenderer> makeFFmpegRenderer(function<shared_ptr<ImageRenderer>(size_t w, size_t h, float aspectRatio)> imageRendererMaker)
{
    return make_shared<FFmpegRenderer>(imageRendererMaker);
}

#include "cacarenderer.h"
#include "softrender.h"
#include <caca.h>

namespace
{
class CacaRenderer : public WindowRenderer
{
private:
    caca_canvas_t * canvas;
    caca_display_t * display;
    caca_dither_t * dither;
    shared_ptr<ImageRenderer> imageRenderer;
    vector<uint32_t> pixels;
    size_t w, h;
    float aspectRatio, defaultAspectRatio;
    size_t scaleFactor = 2;
    void makeDither()
    {
        dither = caca_create_dither(32, w, h, w * sizeof(uint32_t), (uint32_t)RGBAI(0xFF, 0, 0, 0), (uint32_t)RGBAI(0, 0xFF, 0, 0), (uint32_t)RGBAI(0, 0, 0xFF, 0), (uint32_t)RGBAI(0, 0, 0, 0xFF));
        if(dither == nullptr)
            return;
        caca_set_dither_algorithm(dither, "ordered8");
    }
public:
    CacaRenderer()
        : defaultAspectRatio(getDefaultRendererAspectRatio())
    {
        display = caca_create_display(nullptr);
        if(display == nullptr)
        {
            throw runtime_error("caca_create_display failed");
        }
        canvas = caca_get_canvas(display);
        w = caca_get_canvas_width(canvas) * scaleFactor;
        h = caca_get_canvas_height(canvas) * scaleFactor;
        pixels.resize(w * h, 0);
        makeDither();
        if(dither == nullptr)
        {
            caca_free_display(display);
            throw runtime_error("caca_create_dither failed");
        }
        aspectRatio = defaultAspectRatio;
        if(aspectRatio == -1)
            aspectRatio = 0.5 * w / h;
        imageRenderer = makeImageRenderer(w, h, aspectRatio);
    }
    virtual ~CacaRenderer()
    {
        caca_free_dither(dither);
        caca_free_display(display);
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
        for(size_t y = 0; y < h; y++)
        {
            for(size_t x = 0; x < w; x++)
            {
                pixels[x + y * w] = (uint32_t)image.getPixel(x, y);
            }
        }
        caca_dither_bitmap(canvas, 0, 0, caca_get_canvas_width(canvas), caca_get_canvas_height(canvas), dither, (const void *)&pixels[0]);
        caca_refresh_display(display);
        calcFPS();
        caca_event_t event;
        while(caca_get_event(display, CACA_EVENT_ANY, &event, 0))
        {
            switch(caca_get_event_type(&event))
            {
            case CACA_EVENT_KEY_PRESS:
            {
                switch(caca_get_event_key_ch(&event))
                {
                case CACA_KEY_ESCAPE:
                    exit(0);
                    break;
                default:
                    break;
                }
                break;
            }
            case CACA_EVENT_RESIZE:
            {
                break;
            }
            case CACA_EVENT_QUIT:
            {
                exit(0);
                break;
            }
            default:
                break;
            }
        }
        int wi = caca_get_canvas_width(canvas) * scaleFactor;
        int hi = caca_get_canvas_height(canvas) * scaleFactor;
        if(wi <= 0) wi = 1;
        if(hi <= 0) hi = 1;
        if((int)w != wi || (int)h != hi)
        {
            w = wi;
            h = hi;
            aspectRatio = defaultAspectRatio;
            if(aspectRatio == -1)
                aspectRatio = 0.5 * w / h;
            imageRenderer->resize(w, h, aspectRatio);
            pixels.resize(w * h, 0);
            caca_free_dither(dither);
            makeDither();
            assert(dither != nullptr);
        }
    }
};
}

shared_ptr<WindowRenderer> makeCacaRenderer()
{
    return make_shared<CacaRenderer>();
}

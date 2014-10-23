#ifndef __EMSCRIPTEN__
#include "libaarenderer.h"
#include "softrender.h"
#include <aalib.h>

namespace
{
class LibAARenderer : public WindowRenderer
{
private:
    aa_context * context;
    aa_renderparams * renderParams;
    shared_ptr<ImageRenderer> imageRenderer;
    size_t w, h;
    float aspectRatio, defaultAspectRatio;
    static void resizeHandler(aa_context * context)
    {
        aa_resize(context);
    }
public:
    LibAARenderer()
        : defaultAspectRatio(getDefaultRendererAspectRatio())
    {
        if(!aa_parseoptions(nullptr, nullptr, nullptr, nullptr))
            throw runtime_error("aalib option parsing error");
        context = aa_autoinit(&aa_defparams);
        if(context == nullptr)
            throw runtime_error("aalib initialization error");
        w = aa_imgwidth(context);
        h = aa_imgheight(context);
        int physWidth = aa_mmwidth(context);
        int physHeight = aa_mmheight(context);
        aspectRatio = defaultAspectRatio;
        if(physWidth > 0 && physHeight > 0 && aspectRatio == -1)
            aspectRatio = (float)physWidth / physHeight;
        if(!aa_autoinitkbd(context, AA_SENDRELEASE))
        {
            aa_close(context);
            throw runtime_error("aalib keyboard initialization error");
        }
        aa_resizehandler(context, resizeHandler);
        renderParams = aa_getrenderparams();
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
        size_t aaW = aa_imgwidth(context);
        size_t aaH = aa_imgheight(context);
        unsigned char * aaImage = (unsigned char *)aa_image(context);
        for(size_t y = 0; y < h && y < aaH; y++)
        {
            for(size_t x = 0; x < w && x < aaW; x++)
            {
                aaImage[x + y * aaW] = (unsigned char)getLuminanceValueI(image.getPixel(x, y));
            }
        }
        aa_render(context, renderParams, 0, 0, aa_scrwidth(context), aa_scrheight(context));
        aa_flush(context);
        calcFPS();
        for(int event = aa_getevent(context, 0); event != AA_NONE; event = aa_getevent(context, 0))
        {
            switch(event)
            {
            case AA_ESC:
                exit(0);
                break;
            case AA_MOUSE:
            case AA_RESIZE: // resize handler already called
                break;
            default:
                break;
            }
        }
        int wi = aa_imgwidth(context), hi = aa_imgheight(context);
        if(wi <= 0) wi = 1;
        if(hi <= 0) hi = 1;
        int physWidth = aa_mmwidth(context);
        int physHeight = aa_mmheight(context);
        float aspectRatio = defaultAspectRatio;
        if(physWidth > 0 && physHeight > 0 && aspectRatio != -1)
            aspectRatio = (float)physWidth / physHeight;
        if((int)w != wi || (int)h != hi || aspectRatio != this->aspectRatio)
        {
            w = wi;
            h = hi;
            this->aspectRatio = aspectRatio;
            imageRenderer->resize(w, h, aspectRatio);
        }
    }
    virtual shared_ptr<Texture> preloadTexture(shared_ptr<Texture> texture) override
    {
        return imageRenderer->preloadTexture(texture);
    }
};
}

shared_ptr<WindowRenderer> makeLibAARenderer()
{
    return make_shared<LibAARenderer>();
}
#endif
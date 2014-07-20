#include "renderer.h"
#include "softrender.h"
#include <SDL.h>
#include <stdexcept>
#include <cstdlib>
#include <cassert>
#include <iostream>

using namespace std;

namespace
{
#ifdef NDEBUG
#define verify(v) do {if(v) {}} while(0)
#else
#define verify(v) assert(v)
#endif

class SDLWindowRenderer : public WindowRenderer
{
private:
    SDL_Window * window;
    SDL_Texture * texture;
    SDL_Renderer * sdlRenderer;
    shared_ptr<ImageRenderer> imageRenderer;
    size_t w, h;
public:
    SDLWindowRenderer()
    {
        if(SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            throw runtime_error(string("SDL_Init Error : ") + SDL_GetError());
        }
        atexit(SDL_Quit);
        if(SDL_CreateWindowAndRenderer(640, 480, SDL_WINDOW_RESIZABLE, &window, &sdlRenderer) != 0)
        {
            throw runtime_error(string("SDL_CreateWindowAndRenderer Error : ") + SDL_GetError());
        }
        int wi, hi;
        SDL_GetWindowSize(window, &wi, &hi);
        w = wi;
        h = hi;
        if(w <= 0) w = 1;
        if(h <= 0) h = 1;
        texture = SDL_CreateTexture(sdlRenderer, SDL_MasksToPixelFormatEnum(32, RGBAI(0xFF, 0, 0, 0), RGBAI(0, 0xFF, 0, 0), RGBAI(0, 0, 0xFF, 0), RGBAI(0, 0, 0, 0xFF)), SDL_TEXTUREACCESS_STREAMING, w, h);
        if(texture == nullptr)
        {
            SDL_DestroyRenderer(sdlRenderer);
            SDL_DestroyWindow(window);
            throw runtime_error(string("SDL_CreateTexture Error : ") + SDL_GetError());
        }
        imageRenderer = makeImageRenderer(w, h);
    }
    virtual ~SDLWindowRenderer()
    {
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyWindow(window);
    }
protected:
    virtual void clearInternal(ColorF bg) override
    {
        imageRenderer->clear(bg);
    }
    virtual void onSetFPS() override
    {
        cout << "Size : " << w << "x" << h << "                 FPS : " << fps << "\x1b[K\r" << flush;
    }
public:
    virtual void calcScales() override
    {
        Renderer::calcScales(w, h);
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
        shared_ptr<Image> pimage = imageRenderer->finish();
        assert(pimage != nullptr);
        Image & image = *pimage;
        assert(image.w == w);
        assert(image.h == h);
        void * pixels;
        int pitch;
        verify(0 == SDL_LockTexture(texture, nullptr, &pixels, &pitch));
        for(size_t y = 0; y < h; y++)
        {
            for(size_t x = 0; x < w; x++)
            {
                *(uint32_t *)((char *)pixels + pitch * y + sizeof(uint32_t) * x) = image.getPixel(x, y);
            }
        }
        SDL_UnlockTexture(texture);
        SDL_RenderClear(sdlRenderer);
        SDL_RenderCopy(sdlRenderer, texture, nullptr, nullptr);
        SDL_RenderPresent(sdlRenderer);
        calcFPS();
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
            case SDL_QUIT:
                exit(0);
                break;
            case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_ESCAPE || (event.key.keysym.sym == SDLK_F4 && (event.key.keysym.mod & (KMOD_CTRL | KMOD_SHIFT)) == 0 && (event.key.keysym.mod & KMOD_ALT) != 0))
                    exit(0);
                break;
            }
        }
        int wi, hi;
        SDL_GetWindowSize(window, &wi, &hi);
        if(wi <= 0) wi = 1;
        if(hi <= 0) hi = 1;
        if((int)w != wi || (int)h != hi)
        {
            w = wi;
            h = hi;
            SDL_DestroyTexture(texture);
            texture = SDL_CreateTexture(sdlRenderer, SDL_MasksToPixelFormatEnum(32, RGBAI(0xFF, 0, 0, 0), RGBAI(0, 0xFF, 0, 0), RGBAI(0, 0, 0xFF, 0), RGBAI(0, 0, 0, 0xFF)), SDL_TEXTUREACCESS_STREAMING, w, h);
            if(texture == nullptr)
            {
                throw runtime_error(string("SDL_CreateTexture Error : ") + SDL_GetError());
            }
            imageRenderer->resize(w, h);
        }
    }
};

#if 0
class OpenGLWindowRenderer : public WindowRenderer
{
private:
    SDL_Window * window;
    SDL_Texture * texture;
    SDL_Renderer * sdlRenderer;
    shared_ptr<ImageRenderer> imageRenderer;
    size_t w, h;
public:
    OpenGLWindowRenderer()
    {
        if(SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            throw runtime_error(string("SDL_Init Error : ") + SDL_GetError());
        }
        atexit(SDL_Quit);
        if(SDL_CreateWindowAndRenderer(640, 480, SDL_WINDOW_RESIZABLE, &window, &sdlRenderer) != 0)
        {
            throw runtime_error(string("SDL_CreateWindowAndRenderer Error : ") + SDL_GetError());
        }
        int wi, hi;
        SDL_GetWindowSize(window, &wi, &hi);
        w = wi;
        h = hi;
        if(w <= 0) w = 1;
        if(h <= 0) h = 1;
        texture = SDL_CreateTexture(sdlRenderer, SDL_MasksToPixelFormatEnum(32, RGBAI(0xFF, 0, 0, 0), RGBAI(0, 0xFF, 0, 0), RGBAI(0, 0, 0xFF, 0), RGBAI(0, 0, 0, 0xFF)), SDL_TEXTUREACCESS_STREAMING, w, h);
        if(texture == nullptr)
        {
            throw runtime_error(string("SDL_CreateTexture Error : ") + SDL_GetError());
        }
        imageRenderer = makeImageRenderer(w, h);
    }
protected:
    virtual void clearInternal(ColorF bg) override
    {
        imageRenderer->clear(bg);
    }
    virtual void onSetFPS() override
    {
        cout << "Size : " << w << "x" << h << "                 FPS : " << fps << "\x1b[K\r" << flush;
    }
public:
    virtual void calcScales() override
    {
        Renderer::calcScales(w, h);
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
        shared_ptr<Image> pimage = imageRenderer->finish();
        assert(pimage != nullptr);
        Image & image = *pimage;
        assert(image.w == w);
        assert(image.h == h);
        void * pixels;
        int pitch;
        verify(0 == SDL_LockTexture(texture, nullptr, &pixels, &pitch));
        for(size_t y = 0; y < h; y++)
        {
            for(size_t x = 0; x < w; x++)
            {
                *(uint32_t *)((char *)pixels + pitch * y + sizeof(uint32_t) * x) = image.getPixel(x, y);
            }
        }
        SDL_UnlockTexture(texture);
        SDL_RenderClear(sdlRenderer);
        SDL_RenderCopy(sdlRenderer, texture, nullptr, nullptr);
        SDL_RenderPresent(sdlRenderer);
        calcFPS();
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
            case SDL_QUIT:
                exit(0);
                break;
            case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_ESCAPE || (event.key.keysym.sym == SDLK_F4 && (event.key.keysym.mod & (KMOD_CTRL | KMOD_SHIFT)) == 0 && (event.key.keysym.mod & KMOD_ALT) != 0))
                    exit(0);
                break;
            }
        }
        int wi, hi;
        SDL_GetWindowSize(window, &wi, &hi);
        if(wi <= 0) wi = 1;
        if(hi <= 0) hi = 1;
        if((int)w != wi || (int)h != hi)
        {
            w = wi;
            h = hi;
            SDL_DestroyTexture(texture);
            texture = SDL_CreateTexture(sdlRenderer, SDL_MasksToPixelFormatEnum(32, RGBAI(0xFF, 0, 0, 0), RGBAI(0, 0xFF, 0, 0), RGBAI(0, 0, 0xFF, 0), RGBAI(0, 0, 0, 0xFF)), SDL_TEXTUREACCESS_STREAMING, w, h);
            if(texture == nullptr)
            {
                throw runtime_error(string("SDL_CreateTexture Error : ") + SDL_GetError());
            }
            imageRenderer->resize(w, h);
        }
    }
};
#endif

shared_ptr<WindowRenderer> makeWindowRenderer()
{
    #warning finish adding OpenGL
    return make_shared<SDLWindowRenderer>();
}
}

shared_ptr<WindowRenderer> getWindowRenderer()
{
    static shared_ptr<WindowRenderer> theWindowRenderer = nullptr;
    if(theWindowRenderer == nullptr)
        theWindowRenderer = makeWindowRenderer();
    return theWindowRenderer;
}

shared_ptr<ImageRenderer> makeImageRenderer(size_t w, size_t h)
{
    #warning finish adding OpenGL
    return make_shared<SoftwareRenderer>(w, h);
}

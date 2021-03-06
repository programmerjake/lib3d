#include "renderer.h"
#include "softrender.h"
#include "libaarenderer.h"
#include "cacarenderer.h"
#include "ffmpeg_renderer.h"
#include "rawrenderer.h"
#include "svgarenderer.h"
#include <SDL.h>
#include <stdexcept>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <string>
#ifndef __EMSCRIPTEN__
#include <GL/gl.h>
#endif // __EMSCRIPTEN__
#include <functional>
#include <sstream>
#include "generate.h"

using namespace std;

namespace
{
int defaultRendererWidth = -1, defaultRendererHeight = -1;
float defaultRendererAspectRatio = -1;

void getDefaultRendererSize(int &w, int &h)
{
    if(defaultRendererWidth < 0)
    {
        const char * str = getenv("LIB3D_RESOLUTION");
        if(str == nullptr)
            str = "";
        if(2 == sscanf(str, " %ix%i", &w, &h) && w > 0 && h > 0)
        {
            w += w & 1; // round up to even width
            return;
        }
        w = 640;
        h = 480;
        return;
    }
    w = defaultRendererWidth;
    h = defaultRendererHeight;
}
}

void setDefaultRendererSize(int w, int h, float aspectRatio)
{
    assert(w > 0 && h > 0);
    assert(aspectRatio == -1 || aspectRatio > 0);
    defaultRendererWidth = w;
    defaultRendererHeight = h;
    defaultRendererAspectRatio = aspectRatio;
}

int getDefaultRendererWidth()
{
    int w, h;
    getDefaultRendererSize(w, h);
    return w;
}

int getDefaultRendererHeight()
{
    int w, h;
    getDefaultRendererSize(w, h);
    return h;
}

float getDefaultRendererAspectRatio()
{
    return defaultRendererAspectRatio;
}

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
#ifdef __EMSCRIPTEN__
    SDL_Surface *screen;
    SDL_Surface *texture;
#else
    SDL_Window * window;
    SDL_Texture * texture;
    SDL_Renderer * sdlRenderer;
#endif // __EMSCRIPTEN__
    shared_ptr<ImageRenderer> imageRenderer;
    size_t w, h;
    float aspectRatio;
public:
    SDLWindowRenderer()
    {
        if(SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            throw runtime_error(string("SDL_Init Error : ") + SDL_GetError());
        }
#ifdef __EMSCRIPTEN__
        screen = SDL_SetVideoMode(getDefaultRendererWidth(), getDefaultRendererHeight(), 32, SDL_ANYFORMAT | SDL_RESIZABLE);
        if(screen == nullptr)
        {
            string msg = string("SDL_SetVideoMode Error : ") + SDL_GetError();
            SDL_Quit();
            throw runtime_error(msg);
        }
#else
        if(SDL_CreateWindowAndRenderer(getDefaultRendererWidth(), getDefaultRendererHeight(), SDL_WINDOW_RESIZABLE, &window, &sdlRenderer) != 0)
        {
            string msg = string("SDL_CreateWindowAndRenderer Error : ") + SDL_GetError();
            SDL_Quit();
            throw runtime_error(msg);
        }
#endif
        int wi, hi;
#ifdef __EMSCRIPTEN__
        wi = screen->w;
        hi = screen->h;
#else
        SDL_GetWindowSize(window, &wi, &hi);
#endif
        w = wi;
        h = hi;
        aspectRatio = getDefaultRendererAspectRatio();
        if(w <= 0) w = 1;
        if(h <= 0) h = 1;
#ifdef __EMSCRIPTEN__
        texture = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, RGBAI(0xFF, 0, 0, 0), RGBAI(0, 0xFF, 0, 0), RGBAI(0, 0, 0xFF, 0), RGBAI(0, 0, 0, 0xFF));
#else
        texture = SDL_CreateTexture(sdlRenderer, SDL_MasksToPixelFormatEnum(32, RGBAI(0xFF, 0, 0, 0), RGBAI(0, 0xFF, 0, 0), RGBAI(0, 0, 0xFF, 0), RGBAI(0, 0, 0, 0xFF)), SDL_TEXTUREACCESS_STREAMING, w, h);
#endif
        if(texture == nullptr)
        {
            string msg = string("SDL_CreateTexture Error : ") + SDL_GetError();
#ifndef __EMSCRIPTEN__
            SDL_DestroyRenderer(sdlRenderer);
            SDL_DestroyWindow(window);
#endif
            SDL_Quit();
            throw runtime_error(msg);
        }
        imageRenderer = makeImageRenderer(w, h, aspectRatio);
    }
    virtual ~SDLWindowRenderer()
    {
#ifdef __EMSCRIPTEN__
        SDL_FreeSurface(texture);
#else
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(sdlRenderer);
        SDL_DestroyWindow(window);
#endif
        SDL_Quit();
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
        void * pixels;
        int pitch;
#ifdef __EMSCRIPTEN__
        SDL_LockSurface(texture);
        pixels = texture->pixels;
        pitch = texture->pitch;
#else
        verify(0 == SDL_LockTexture(texture, nullptr, &pixels, &pitch));
#endif
        for(size_t y = 0; y < h; y++)
        {
            for(size_t x = 0; x < w; x++)
            {
                *(uint32_t *)((char *)pixels + pitch * y + sizeof(uint32_t) * x) = image.getPixel(x, y);
            }
        }
#ifdef __EMSCRIPTEN__
        SDL_UnlockSurface(texture);
        SDL_BlitSurface(texture, nullptr, screen, nullptr);
        SDL_Flip(screen);
#else
        SDL_UnlockTexture(texture);
        SDL_RenderClear(sdlRenderer);
        SDL_RenderCopy(sdlRenderer, texture, nullptr, nullptr);
        SDL_RenderPresent(sdlRenderer);
#endif // __EMSCRIPTEN__
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
#ifdef __EMSCRIPTEN__
            case SDL_VIDEORESIZE:
                screen = SDL_SetVideoMode(event.resize.w, event.resize.h, 32, SDL_ANYFORMAT | SDL_RESIZABLE);
                break;
#endif // __EMSCRIPTEN__
            }
        }
        int wi, hi;
#ifdef __EMSCRIPTEN__
        wi = screen->w;
        hi = screen->h;
#else
        SDL_GetWindowSize(window, &wi, &hi);
#endif
        if(wi <= 0) wi = 1;
        if(hi <= 0) hi = 1;
        if((int)w != wi || (int)h != hi)
        {
            w = wi;
            h = hi;
#ifdef __EMSCRIPTEN__
            SDL_FreeSurface(texture);
            texture = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, RGBAI(0xFF, 0, 0, 0), RGBAI(0, 0xFF, 0, 0), RGBAI(0, 0, 0xFF, 0), RGBAI(0, 0, 0, 0xFF));
#else
            SDL_DestroyTexture(texture);
            texture = SDL_CreateTexture(sdlRenderer, SDL_MasksToPixelFormatEnum(32, RGBAI(0xFF, 0, 0, 0), RGBAI(0, 0xFF, 0, 0), RGBAI(0, 0, 0xFF, 0), RGBAI(0, 0, 0, 0xFF)), SDL_TEXTUREACCESS_STREAMING, w, h);
#endif
            if(texture == nullptr)
            {
                throw runtime_error(string("SDL_CreateTexture Error : ") + SDL_GetError());
            }
            imageRenderer->resize(w, h, aspectRatio);
        }
    }
};
#ifndef __EMSCRIPTEN__
class OpenGLWindowRenderer : public WindowRenderer
{
    friend class OpenGLImageRenderer;
private:
    SDL_Window * window;
    SDL_GLContext glContext;
    size_t w, h;
    bool supportsExtFrameBufferObjects;
    vector<float> vertexArray, textureCoordArray, colorArray;

    template <typename... Args>
    static void debugDumpGLInternal(Args... args);

    template <typename T, typename... Args>
    static void debugDumpGLInternal(T arg1, Args ... args)
    {
#if 0
        cout << arg1;
#endif
        debugDumpGLInternal(args...);
    }

    static void debugDumpGLInternal()
    {
    }

    template <typename... Args>
    static void debugDumpGL(Args... args)
    {
        debugDumpGLInternal(args..., "\n");
    }

    template <typename T>
    static void dumpGLArg(size_t, T arg)
    {
        debugDumpGL(arg);
    }

    static void dumpGLArg(size_t, int arg)
    {
        debugDumpGL(arg, " 0x", hex, arg, dec);
    }

    static void dumpGLArg(size_t, unsigned arg)
    {
        debugDumpGL(arg, " 0x", hex, arg, dec);
    }

    static void dumpGLArg(size_t, short arg)
    {
        debugDumpGL(arg, " 0x", hex, arg, dec);
    }

    static void dumpGLArg(size_t, unsigned short arg)
    {
        debugDumpGL(arg, " 0x", hex, arg, dec);
    }

    static void dumpGLArg(size_t, signed char arg)
    {
        debugDumpGL((int)arg, " 0x", hex, (int)arg, dec);
    }

    static void dumpGLArg(size_t, unsigned char arg)
    {
        debugDumpGL((unsigned)arg, " 0x", hex, (unsigned)arg, dec);
    }

    static void dumpGLArg(size_t, char arg)
    {
        debugDumpGL((unsigned)arg, " 0x", hex, (unsigned)arg, dec);
    }

    template <typename... Args>
    static void dumpGLArgs(size_t n, Args... args);

    template <typename Arg1, typename... Args>
    static void dumpGLArgs(size_t n, Arg1 arg1, Args... args)
    {
        dumpGLArg(n, arg1);
        dumpGLArgs(n + 1, args...);
    }

    static void dumpGLArgs(size_t n)
    {
    }

    template <typename T>
    static T dumpGLReturnValue(T v)
    {
        debugDumpGL(" -> ", v);
        return v;
    }

    template <typename ReturnType, typename... Args>
    struct DumpGLStruct
    {
        const char * name;
        ReturnType (APIENTRY * fn)(Args...);
        constexpr DumpGLStruct(const char * name, ReturnType (APIENTRY * fn)(Args...))
            : name(name), fn(fn)
        {
        }
        void init(ReturnType (APIENTRY * fn)(Args...))
        {
            this->fn = fn;
        }
        constexpr DumpGLStruct()
            : name(""), fn(nullptr)
        {
        }
        ReturnType operator ()(Args... args) const
        {
            debugDumpGL(name, " : ");
            dumpGLArgs(1, args...);
            return dumpGLReturnValue(fn(args...));
        }
    };
    template <typename... Args>
    struct DumpGLStruct<void, Args...>
    {
        const char * name;
        void (APIENTRY * fn)(Args...);
        constexpr DumpGLStruct(const char * name, void (APIENTRY * fn)(Args...))
            : name(name), fn(fn)
        {
        }
        void init(void (APIENTRY * fn)(Args...))
        {
            this->fn = fn;
        }
        constexpr DumpGLStruct()
            : name(""), fn(nullptr)
        {
        }
        void operator ()(Args... args) const
        {
            debugDumpGL(name, " : ");
            dumpGLArgs(1, args...);
            fn(args...);
        }
    };
    template <typename ReturnType, typename... Args>
    static constexpr DumpGLStruct<ReturnType, Args...> makeDumpGLStruct(const char * name, ReturnType (APIENTRY * fn)(Args...))
    {
        return DumpGLStruct<ReturnType, Args...>(name, fn);
    }
#define DECLARE_GL_FUNCTION(retType, fn, args) \
typedef retType (APIENTRY * fn ## Type)args; \
fn ## Type fn ## Ptr = nullptr; \
static const char * fn ## NameString() \
{ \
    return #retType " " #fn #args; \
} \
decltype(makeDumpGLStruct(#retType " " #fn #args, fn ## Ptr)) fn

#define LOAD_GL_FUNCTION(fn) \
do \
{ \
    fn ## Ptr = (fn ## Type)SDL_GL_GetProcAddress(#fn); \
    if(!fn ## Ptr) \
        throw runtime_error("can't load" #fn); \
    fn = makeDumpGLStruct(fn ## NameString(), fn ## Ptr);\
} \
while(0)

    DECLARE_GL_FUNCTION(void, glEnable, (GLenum cap));
    DECLARE_GL_FUNCTION(void, glEnableClientState, (GLenum cap));
    DECLARE_GL_FUNCTION(void, glDepthFunc, (GLenum func));
    DECLARE_GL_FUNCTION(void, glCullFace, (GLenum mode));
    DECLARE_GL_FUNCTION(void, glFrontFace, (GLenum mode));
    DECLARE_GL_FUNCTION(void, glAlphaFunc, (GLenum func, GLclampf ref));
    DECLARE_GL_FUNCTION(void, glBlendFunc, (GLenum sfactor, GLenum dfactor));
    DECLARE_GL_FUNCTION(void, glTexEnvi, (GLenum target, GLenum pname, GLint param));
    DECLARE_GL_FUNCTION(void, glHint, (GLenum target, GLenum mode));
    DECLARE_GL_FUNCTION(void, glViewport, (GLint x, GLint y, GLsizei width, GLsizei height));
    DECLARE_GL_FUNCTION(void, glMatrixMode, (GLenum mode));
    DECLARE_GL_FUNCTION(void, glLoadIdentity, ());
    DECLARE_GL_FUNCTION(void, glFrustum, (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val));
    DECLARE_GL_FUNCTION(void, glClearColor, (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha));
    DECLARE_GL_FUNCTION(void, glClear, (GLbitfield mask));
    DECLARE_GL_FUNCTION(void, glDepthMask, (GLboolean flag));
    DECLARE_GL_FUNCTION(void, glVertexPointer, (GLint size, GLenum type, GLsizei stride, const GLvoid *ptr));
    DECLARE_GL_FUNCTION(void, glTexCoordPointer, (GLint size, GLenum type, GLsizei stride, const GLvoid *ptr));
    DECLARE_GL_FUNCTION(void, glColorPointer, (GLint size, GLenum type, GLsizei stride, const GLvoid *ptr));
    DECLARE_GL_FUNCTION(void, glDrawArrays, (GLenum mode, GLint first, GLsizei count));
    DECLARE_GL_FUNCTION(void, glBindTexture, (GLenum target, GLuint texture));
    DECLARE_GL_FUNCTION(void, glScalef, (GLfloat x, GLfloat y, GLfloat z));
    DECLARE_GL_FUNCTION(void, glTranslatef, (GLfloat x, GLfloat y, GLfloat z));
    DECLARE_GL_FUNCTION(void, glGenTextures, (GLsizei n, GLuint *textures));
    DECLARE_GL_FUNCTION(void, glDeleteTextures, (GLsizei n, GLuint *textures));
    DECLARE_GL_FUNCTION(void, glTexParameteri, (GLenum target, GLenum pname, GLint param));
    DECLARE_GL_FUNCTION(void, glPixelStorei, (GLenum pname, GLint param));
    DECLARE_GL_FUNCTION(void, glTexImage2D, (GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels));
    DECLARE_GL_FUNCTION(void, glTexSubImage2D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels));
    DECLARE_GL_FUNCTION(void, glGetTexImage, (GLenum target, GLint level, GLenum format, GLenum type, GLvoid * img));
    DECLARE_GL_FUNCTION(void, glCopyTexSubImage2D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height));

    // in GL_EXT_framebuffer_object
    DECLARE_GL_FUNCTION(void, glGenFramebuffersEXT, (GLsizei n, GLuint *framebuffers));
    DECLARE_GL_FUNCTION(void, glDeleteFramebuffersEXT, (GLsizei n, const GLuint *framebuffers));
    DECLARE_GL_FUNCTION(void, glGenRenderbuffersEXT, (GLsizei n, GLuint *renderbuffers));
    DECLARE_GL_FUNCTION(void, glDeleteRenderbuffersEXT, (GLsizei n, const GLuint *renderbuffers));
    DECLARE_GL_FUNCTION(void, glBindFramebufferEXT, (GLenum target, GLuint framebuffer));
    DECLARE_GL_FUNCTION(void, glBindRenderbufferEXT, (GLenum target, GLuint renderbuffer));
    DECLARE_GL_FUNCTION(void, glFramebufferTexture2DEXT, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level));
    DECLARE_GL_FUNCTION(void, glRenderbufferStorageEXT, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height));
    DECLARE_GL_FUNCTION(void, glFramebufferRenderbufferEXT, (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer));
    DECLARE_GL_FUNCTION(GLenum, glCheckFramebufferStatusEXT, (GLenum target));

    void loadFunctions()
    {
        LOAD_GL_FUNCTION(glEnable);
        LOAD_GL_FUNCTION(glEnableClientState);
        LOAD_GL_FUNCTION(glDepthFunc);
        LOAD_GL_FUNCTION(glCullFace);
        LOAD_GL_FUNCTION(glFrontFace);
        LOAD_GL_FUNCTION(glAlphaFunc);
        LOAD_GL_FUNCTION(glBlendFunc);
        LOAD_GL_FUNCTION(glTexEnvi);
        LOAD_GL_FUNCTION(glHint);
        LOAD_GL_FUNCTION(glViewport);
        LOAD_GL_FUNCTION(glMatrixMode);
        LOAD_GL_FUNCTION(glLoadIdentity);
        LOAD_GL_FUNCTION(glFrustum);
        LOAD_GL_FUNCTION(glClearColor);
        LOAD_GL_FUNCTION(glClear);
        LOAD_GL_FUNCTION(glDepthMask);
        LOAD_GL_FUNCTION(glVertexPointer);
        LOAD_GL_FUNCTION(glTexCoordPointer);
        LOAD_GL_FUNCTION(glColorPointer);
        LOAD_GL_FUNCTION(glDrawArrays);
        LOAD_GL_FUNCTION(glBindTexture);
        LOAD_GL_FUNCTION(glScalef);
        LOAD_GL_FUNCTION(glTranslatef);
        LOAD_GL_FUNCTION(glGenTextures);
        LOAD_GL_FUNCTION(glDeleteTextures);
        LOAD_GL_FUNCTION(glTexParameteri);
        LOAD_GL_FUNCTION(glPixelStorei);
        LOAD_GL_FUNCTION(glTexImage2D);
        LOAD_GL_FUNCTION(glTexSubImage2D);
        LOAD_GL_FUNCTION(glGetTexImage);
        LOAD_GL_FUNCTION(glCopyTexSubImage2D);
        supportsExtFrameBufferObjects = SDL_GL_ExtensionSupported("GL_EXT_framebuffer_object");
        if(supportsExtFrameBufferObjects)
        {
            LOAD_GL_FUNCTION(glGenFramebuffersEXT);
            LOAD_GL_FUNCTION(glDeleteFramebuffersEXT);
            LOAD_GL_FUNCTION(glGenRenderbuffersEXT);
            LOAD_GL_FUNCTION(glDeleteRenderbuffersEXT);
            LOAD_GL_FUNCTION(glBindFramebufferEXT);
            LOAD_GL_FUNCTION(glBindRenderbufferEXT);
            LOAD_GL_FUNCTION(glFramebufferTexture2DEXT);
            LOAD_GL_FUNCTION(glRenderbufferStorageEXT);
            LOAD_GL_FUNCTION(glFramebufferRenderbufferEXT);
            LOAD_GL_FUNCTION(glCheckFramebufferStatusEXT);
        }
    }

    friend struct GLTexture;

    struct GLTexture : public Texture
    {
        GLuint texture = 0;
        size_t w = 0, h = 0;
        OpenGLWindowRenderer * renderer;
        vector<GLubyte> imageData;
        shared_ptr<Image> simage = nullptr;
        weak_ptr<const Image> wimage;
        bool imageValid = false;
        GLTexture(OpenGLWindowRenderer * renderer)
            : renderer(renderer)
        {
        }
        virtual ~GLTexture()
        {
            renderer->destroyGLTexture(*this);
        }
        virtual shared_ptr<const Image> getImage() override
        {
            if(simage == nullptr)
            {
                shared_ptr<const Image> image = wimage.lock();
                if(image != nullptr)
                    return image;
                simage = make_shared<Image>(w, h);
                imageValid = false;
            }
            if(simage != nullptr && !imageValid)
            {
                constexpr size_t bytesPerPixel = 4;
                imageValid = true;
                imageData.resize(w * h * bytesPerPixel);
                renderer->glPixelStorei(GL_PACK_ALIGNMENT, 1);
                renderer->glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *)&imageData[0]);
                ColorI * pixels = simage->getPixels();
                for(size_t y = 0; y < h; y++)
                {
                    for(size_t x = 0; x < w; x++)
                    {
                        ColorI & color = pixels[x + (h - y - 1) * w];
                        color.r = imageData[x * bytesPerPixel + y * bytesPerPixel * w + 0];
                        color.g = imageData[x * bytesPerPixel + y * bytesPerPixel * w + 1];
                        color.b = imageData[x * bytesPerPixel + y * bytesPerPixel * w + 2];
                        color.a = imageData[x * bytesPerPixel + y * bytesPerPixel * w + 3];
                    }
                }
            }
            return static_pointer_cast<const Image>(simage);
        }
        void invalidate()
        {
            imageValid = false;
        }
    };

    void destroyGLTexture(GLTexture & params)
    {
        if(glContext != nullptr && params.texture != 0)
        {
            glDeleteTextures(1, &params.texture);
            params.texture = 0;
        }
    }

    shared_ptr<GLTexture> createTexture(size_t w, size_t h)
    {
        shared_ptr<GLTexture> retval = make_shared<GLTexture>(this);
        glGenTextures(1, &retval->texture);
        retval->w = w;
        retval->h = h;
        glBindTexture(GL_TEXTURE_2D, retval->texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        return retval;
    }

    shared_ptr<GLTexture> bindImage(shared_ptr<Texture> textureIn)
    {
        if(textureIn == nullptr)
        {
            glBindTexture(GL_TEXTURE_2D, 0);
            return nullptr;
        }
        shared_ptr<GLTexture> texture = dynamic_pointer_cast<GLTexture>(textureIn);
        if(texture != nullptr)
        {
            glBindTexture(GL_TEXTURE_2D, texture->texture);
            return texture;
        }
        shared_ptr<const Image> image = textureIn->getImage();
        size_t w = image->w, h = image->h;
        texture = static_pointer_cast<GLTexture>(image->glProperties);
        bool isNew = false;
        if(texture == nullptr)
        {
            isNew = true;
            texture = make_shared<GLTexture>(this);
            image->glProperties = static_pointer_cast<void>(texture);
        }
        texture->wimage = image;
        GLTexture & params = *texture;
        if(!isNew)
        {
            if(params.w != w || params.w != h)
            {
                glDeleteTextures(1, &params.texture);
                isNew = true;
            }
        }
        if(isNew)
        {
            glGenTextures(1, &params.texture);
            params.w = w;
            params.w = h;
        }
        constexpr size_t bytesPerPixel = 4;
        params.imageData.resize(w * h * bytesPerPixel);
        const ColorI * pixels = image->getPixels();
        for(size_t y = 0; y < h; y++)
        {
            for(size_t x = 0; x < w; x++)
            {
                ColorI color = pixels[x + y * w];
                params.imageData[x * bytesPerPixel + (h - y - 1) * bytesPerPixel * w + 0] = color.r;
                params.imageData[x * bytesPerPixel + (h - y - 1) * bytesPerPixel * w + 1] = color.g;
                params.imageData[x * bytesPerPixel + (h - y - 1) * bytesPerPixel * w + 2] = color.b;
                params.imageData[x * bytesPerPixel + (h - y - 1) * bytesPerPixel * w + 3] = color.a;
            }
        }
        glBindTexture(GL_TEXTURE_2D, params.texture);
        if(isNew)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (const GLvoid *)&params.imageData[0]);
        }
        else
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, (const GLvoid *)&params.imageData[0]);
        return texture;
    }

    void setupContext(size_t w, size_t h, float scaleXValue, float scaleYValue, GLuint framebuffer)
    {
        if(supportsExtFrameBufferObjects)
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer);
        else
        {
            assert(framebuffer == 0);
        }
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_ALPHA_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glDepthFunc(GL_LEQUAL);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glAlphaFunc(GL_GREATER, 0.0f);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
        glViewport(0, 0, w, h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        const float minDistance = 5e-2f, maxDistance = 100.0f;
        glFrustum(-minDistance * scaleXValue, minDistance * scaleXValue, -minDistance * scaleYValue, minDistance * scaleYValue, minDistance, maxDistance);
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_VERTEX_ARRAY);
    }

    float aspectRatio;

    void setupContext()
    {
        Renderer::calcScales(w, h, aspectRatio);
        setupContext(w, h, scaleX(), scaleY(), 0);
    }
    bool writeDepth = true;
public:
    static OpenGLWindowRenderer * windowRenderer;
    OpenGLWindowRenderer(int defaultWidth = getDefaultRendererWidth(), int defaultHeight = getDefaultRendererHeight(), float defaultAspectRatio = getDefaultRendererAspectRatio())
    {
        if(SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            throw runtime_error(string("SDL_Init Error : ") + SDL_GetError());
        }
        if(0 != SDL_GL_LoadLibrary(nullptr))
        {
            string msg = string("SDL_GL_LoadLibrary Error : ") + SDL_GetError();
            SDL_Quit();
            throw runtime_error(msg);
        }
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        window = SDL_CreateWindow("lib3d", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, defaultWidth, defaultHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
        if(window == nullptr)
        {
            string msg = string("SDL_CreateWindow Error : ") + SDL_GetError();
            SDL_GL_UnloadLibrary();
            SDL_Quit();
            throw runtime_error(msg);
        }
        int wi, hi;
        SDL_GetWindowSize(window, &wi, &hi);
        w = wi;
        h = hi;
        if(w <= 0) w = 1;
        if(h <= 0) h = 1;
        aspectRatio = defaultAspectRatio;
        glContext = SDL_GL_CreateContext(window);
        if(glContext == nullptr)
        {
            string msg = string("SDL_GL_CreateContext Error : ") + SDL_GetError();
            SDL_DestroyWindow(window);
            SDL_GL_UnloadLibrary();
            SDL_Quit();
            throw runtime_error(msg);
        }
        try
        {
            loadFunctions();
        }
        catch(exception & e)
        {
            SDL_GL_DeleteContext(glContext);
            glContext = nullptr;
            SDL_DestroyWindow(window);
            SDL_GL_UnloadLibrary();
            SDL_Quit();
            throw e;
        }
        setupContext();
        windowRenderer = this;
    }
    virtual ~OpenGLWindowRenderer()
    {
        windowRenderer = nullptr;
        SDL_GL_DeleteContext(glContext);
        glContext = nullptr;
        SDL_DestroyWindow(window);
        SDL_GL_UnloadLibrary();
        SDL_Quit();
    }
protected:
    virtual void clearInternal(ColorF bg) override
    {
        if(supportsExtFrameBufferObjects)
            setupContext();
        glDepthMask(GL_TRUE);
        glClearColor(bg.r, bg.g, bg.b, bg.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
public:
    virtual void calcScales() override
    {
        Renderer::calcScales(w, h, aspectRatio);
    }
    virtual void render(const Mesh &m) override
    {
        if(m.triangles.size() == 0)
            return;
        if(supportsExtFrameBufferObjects)
            setupContext();
        glDepthMask(writeDepth ? GL_TRUE : GL_FALSE);
        bindImage(m.image);
        vertexArray.resize(m.triangles.size() * 3 * 3);
        textureCoordArray.resize(m.triangles.size() * 3 * 2);
        colorArray.resize(m.triangles.size() * 3 * 4);
        for(size_t i = 0; i < m.triangles.size(); i++)
        {
            Triangle tri = m.triangles[i];
            vertexArray[i * 3 * 3 + 0 * 3 + 0] = tri.p1.x;
            vertexArray[i * 3 * 3 + 0 * 3 + 1] = tri.p1.y;
            vertexArray[i * 3 * 3 + 0 * 3 + 2] = tri.p1.z;
            vertexArray[i * 3 * 3 + 1 * 3 + 0] = tri.p2.x;
            vertexArray[i * 3 * 3 + 1 * 3 + 1] = tri.p2.y;
            vertexArray[i * 3 * 3 + 1 * 3 + 2] = tri.p2.z;
            vertexArray[i * 3 * 3 + 2 * 3 + 0] = tri.p3.x;
            vertexArray[i * 3 * 3 + 2 * 3 + 1] = tri.p3.y;
            vertexArray[i * 3 * 3 + 2 * 3 + 2] = tri.p3.z;
            textureCoordArray[i * 3 * 2 + 0 * 2 + 0] = tri.t1.u;
            textureCoordArray[i * 3 * 2 + 0 * 2 + 1] = tri.t1.v;
            textureCoordArray[i * 3 * 2 + 1 * 2 + 0] = tri.t2.u;
            textureCoordArray[i * 3 * 2 + 1 * 2 + 1] = tri.t2.v;
            textureCoordArray[i * 3 * 2 + 2 * 2 + 0] = tri.t3.u;
            textureCoordArray[i * 3 * 2 + 2 * 2 + 1] = tri.t3.v;
            colorArray[i * 3 * 4 + 0 * 4 + 0] = tri.c1.r;
            colorArray[i * 3 * 4 + 0 * 4 + 1] = tri.c1.g;
            colorArray[i * 3 * 4 + 0 * 4 + 2] = tri.c1.b;
            colorArray[i * 3 * 4 + 0 * 4 + 3] = tri.c1.a;
            colorArray[i * 3 * 4 + 1 * 4 + 0] = tri.c2.r;
            colorArray[i * 3 * 4 + 1 * 4 + 1] = tri.c2.g;
            colorArray[i * 3 * 4 + 1 * 4 + 2] = tri.c2.b;
            colorArray[i * 3 * 4 + 1 * 4 + 3] = tri.c2.a;
            colorArray[i * 3 * 4 + 2 * 4 + 0] = tri.c3.r;
            colorArray[i * 3 * 4 + 2 * 4 + 1] = tri.c3.g;
            colorArray[i * 3 * 4 + 2 * 4 + 2] = tri.c3.b;
            colorArray[i * 3 * 4 + 2 * 4 + 3] = tri.c3.a;
        }
        glVertexPointer(3, GL_FLOAT, 0, (const void *)&vertexArray[0]);
        glTexCoordPointer(2, GL_FLOAT, 0, (const void *)&textureCoordArray[0]);
        glColorPointer(4, GL_FLOAT, 0, (const void *)&colorArray[0]);
        glDrawArrays(GL_TRIANGLES, 0, (GLint)m.triangles.size() * 3);
    }
    virtual void enableWriteDepth(bool v) override
    {
        writeDepth = v;
    }
    virtual void flip() override
    {
        if(supportsExtFrameBufferObjects)
            setupContext();
        SDL_GL_SwapWindow(window);
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
            setupContext();
        }
    }
    virtual shared_ptr<Texture> preloadTexture(shared_ptr<Texture> texture) override
    {
        if(texture == nullptr)
            return texture;
        if(dynamic_cast<const GLTexture *>(texture.get()) != nullptr)
            return texture;
        if(supportsExtFrameBufferObjects)
            setupContext();
        return bindImage(texture);
    }
};

OpenGLWindowRenderer * OpenGLWindowRenderer::windowRenderer = nullptr;

class OpenGLImageRenderer : public ImageRenderer
{
private:
    shared_ptr<OpenGLWindowRenderer::GLTexture> image;
    size_t w, h;
    float aspectRatio;
    OpenGLWindowRenderer * renderer;
    vector<float> vertexArray, textureCoordArray, colorArray;
    GLuint framebuffer, renderbuffer, texture;
    void bindImage(shared_ptr<Texture> textureIn)
    {
        renderer->bindImage(textureIn);
    }

    void setupContext()
    {
        Renderer::calcScales(w, h, aspectRatio);
        renderer->setupContext(w, h, scaleX(), scaleY(), framebuffer);
    }

    void teardown()
    {
        if(image == nullptr)
            return;
        renderer->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
        renderer->glBindTexture(GL_TEXTURE_2D, 0);
        renderer->glDeleteTextures(1, &texture);
        renderer->glDeleteRenderbuffersEXT(1, &renderbuffer);
        renderer->glDeleteFramebuffersEXT(1, &framebuffer);
        image = nullptr;
    }

    void setup()
    {
        if(!renderer->supportsExtFrameBufferObjects)
            throw runtime_error("FBOs not supported");
        image = renderer->createTexture(w, h);
        renderer->glGenFramebuffersEXT(1, &framebuffer);
        renderer->glGenRenderbuffersEXT(1, &renderbuffer);
        renderer->glGenTextures(1, &texture);
        renderer->glBindTexture(GL_TEXTURE_2D, texture);
        renderer->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        renderer->glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer);
        renderer->glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, texture, 0);
        renderer->glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbuffer);
        renderer->glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, w, h);
        renderer->glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, renderbuffer);
        if(renderer->glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
        {
            teardown();
            throw runtime_error("can't create valid framebuffer in OpenGLImageRenderer constructor");
        }
    }

    bool writeDepth = true;
public:
    OpenGLImageRenderer(size_t w, size_t h, float aspectRatio, OpenGLWindowRenderer * renderer = OpenGLWindowRenderer::windowRenderer)
        : w(w), h(h), aspectRatio(aspectRatio), renderer(renderer)
    {
        setup();
    }
    virtual ~OpenGLImageRenderer()
    {
        teardown();
    }
protected:
    virtual void clearInternal(ColorF bg) override
    {
        setupContext();
        renderer->glDepthMask(GL_TRUE);
        renderer->glClearColor(bg.r, bg.g, bg.b, bg.a);
        renderer->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
public:
    virtual void calcScales() override
    {
        Renderer::calcScales(w, h, aspectRatio);
    }
    virtual void render(const Mesh &m) override
    {
        if(m.triangles.size() == 0)
            return;
        setupContext();
        renderer->glDepthMask(writeDepth ? GL_TRUE : GL_FALSE);
        bindImage(m.image);
        vertexArray.resize(m.triangles.size() * 3 * 3);
        textureCoordArray.resize(m.triangles.size() * 3 * 2);
        colorArray.resize(m.triangles.size() * 3 * 4);
        for(size_t i = 0; i < m.triangles.size(); i++)
        {
            Triangle tri = m.triangles[i];
            vertexArray[i * 3 * 3 + 0 * 3 + 0] = tri.p1.x;
            vertexArray[i * 3 * 3 + 0 * 3 + 1] = tri.p1.y;
            vertexArray[i * 3 * 3 + 0 * 3 + 2] = tri.p1.z;
            vertexArray[i * 3 * 3 + 1 * 3 + 0] = tri.p2.x;
            vertexArray[i * 3 * 3 + 1 * 3 + 1] = tri.p2.y;
            vertexArray[i * 3 * 3 + 1 * 3 + 2] = tri.p2.z;
            vertexArray[i * 3 * 3 + 2 * 3 + 0] = tri.p3.x;
            vertexArray[i * 3 * 3 + 2 * 3 + 1] = tri.p3.y;
            vertexArray[i * 3 * 3 + 2 * 3 + 2] = tri.p3.z;
            textureCoordArray[i * 3 * 2 + 0 * 2 + 0] = tri.t1.u;
            textureCoordArray[i * 3 * 2 + 0 * 2 + 1] = tri.t1.v;
            textureCoordArray[i * 3 * 2 + 1 * 2 + 0] = tri.t2.u;
            textureCoordArray[i * 3 * 2 + 1 * 2 + 1] = tri.t2.v;
            textureCoordArray[i * 3 * 2 + 2 * 2 + 0] = tri.t3.u;
            textureCoordArray[i * 3 * 2 + 2 * 2 + 1] = tri.t3.v;
            colorArray[i * 3 * 4 + 0 * 4 + 0] = tri.c1.r;
            colorArray[i * 3 * 4 + 0 * 4 + 1] = tri.c1.g;
            colorArray[i * 3 * 4 + 0 * 4 + 2] = tri.c1.b;
            colorArray[i * 3 * 4 + 0 * 4 + 3] = tri.c1.a;
            colorArray[i * 3 * 4 + 1 * 4 + 0] = tri.c2.r;
            colorArray[i * 3 * 4 + 1 * 4 + 1] = tri.c2.g;
            colorArray[i * 3 * 4 + 1 * 4 + 2] = tri.c2.b;
            colorArray[i * 3 * 4 + 1 * 4 + 3] = tri.c2.a;
            colorArray[i * 3 * 4 + 2 * 4 + 0] = tri.c3.r;
            colorArray[i * 3 * 4 + 2 * 4 + 1] = tri.c3.g;
            colorArray[i * 3 * 4 + 2 * 4 + 2] = tri.c3.b;
            colorArray[i * 3 * 4 + 2 * 4 + 3] = tri.c3.a;
        }
        renderer->glVertexPointer(3, GL_FLOAT, 0, (const void *)&vertexArray[0]);
        renderer->glTexCoordPointer(2, GL_FLOAT, 0, (const void *)&textureCoordArray[0]);
        renderer->glColorPointer(4, GL_FLOAT, 0, (const void *)&colorArray[0]);
        renderer->glDrawArrays(GL_TRIANGLES, 0, (GLint)m.triangles.size() * 3);
    }
    virtual void enableWriteDepth(bool v) override
    {
        writeDepth = v;
    }
    virtual shared_ptr<Texture> finish() override
    {
        setupContext();
        bindImage(image);
        renderer->glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w, h);
        image->invalidate();
        return image;
    }
    virtual shared_ptr<Texture> preloadTexture(shared_ptr<Texture> texture) override
    {
        if(texture == nullptr)
            return texture;
        if(dynamic_cast<const OpenGLWindowRenderer::GLTexture *>(texture.get()) != nullptr)
            return texture;
        setupContext();
        return renderer->bindImage(texture);
    }
    virtual void resize(size_t newW, size_t newH, float newAspectRatio = -1) override
    {
        teardown();
        w = newW;
        h = newH;
        aspectRatio = newAspectRatio;
        setup();
    }
};
#endif
class NullWindowRenderer : public WindowRenderer
{
private:
    size_t w, h;
    float aspectRatio;
public:
    NullWindowRenderer()
        : w(getDefaultRendererWidth()), h(getDefaultRendererHeight()), aspectRatio(getDefaultRendererAspectRatio())
    {
    }
protected:
    virtual void clearInternal(ColorF bg) override
    {
    }
    virtual void onSetFPS() override
    {
        cout << "FPS: " << fps << "\x1b[K\r" << flush;
    }
public:
    virtual void calcScales() override
    {
        Renderer::calcScales(w, h, aspectRatio);
    }
    virtual void render(const Mesh &m) override
    {
    }
    virtual void enableWriteDepth(bool v) override
    {
    }
    virtual void flip() override
    {
        calcFPS();
    }
};

shared_ptr<ImageRenderer> makeSoftwareImageRenderer(size_t w, size_t h, float aspectRatio)
{
    return make_shared<SoftwareRenderer>(w, h, aspectRatio);
}

shared_ptr<ImageRenderer> makeOpenGLImageRenderer(size_t w, size_t h, float aspectRatio)
{
#ifndef __EMSCRIPTEN__
    try
    {
        return make_shared<OpenGLImageRenderer>(w, h, aspectRatio);
    }
    catch(exception & e)
    {
    }
#endif
    return makeSoftwareImageRenderer(w, h, aspectRatio);
}

struct Driver
{
    string name;
    function<shared_ptr<WindowRenderer>()> makeWindowRenderer;
    function<shared_ptr<ImageRenderer>(size_t w, size_t h, float aspectRatio)> makeImageRenderer;
    Driver(string name, function<shared_ptr<WindowRenderer>()> makeWindowRenderer, function<shared_ptr<ImageRenderer>(size_t w, size_t h, float aspectRatio)> makeImageRenderer)
        : name(name), makeWindowRenderer(makeWindowRenderer), makeImageRenderer(makeImageRenderer)
    {
    }
    shared_ptr<WindowRenderer> start() const
    {
        try
        {
            return makeWindowRenderer();
        }
        catch(exception & e)
        {
            cout << "lib3d : can't start " << name << " : " << e.what() << endl;
        }
        return nullptr;
    }
};

#ifndef __EMSCRIPTEN__
shared_ptr<WindowRenderer> makeFFmpegNoOpenGLRenderer()
{
    shared_ptr<WindowRenderer> retval = makeFFmpegRenderer();
    registerFFmpegRenderStatusCallback([]()
    {
        cout << timer() << "\x1b[K\r" << flush;
    });
    return retval;
}

class FFmpegOpenGLRenderer : public WindowRenderer
{
    shared_ptr<WindowRenderer> ffmpegRenderer;
    shared_ptr<WindowRenderer> openGLRenderer;
    shared_ptr<Texture> fontTexture;
    void renderCharacter(Mesh &dest, int x, int y, char ch)
    {
        TextureDescriptor td = getTextFontCharacterTextureDescriptor(ch, fontTexture);
        if(!td)
            return;
        ColorF c = RGBF(0, 0, 0);
        dest.append(Generate::quadrilateral(td,
                                            VectorF(0 + x, 0 + y, 0), c,
                                            VectorF(1 + x, 0 + y, 0), c,
                                            VectorF(1 + x, 1 + y, 0), c,
                                            VectorF(0 + x, 1 + y, 0), c
                                            ));
    }
    void renderText(Mesh &dest, int x, int y, string str)
    {
        for(char ch : str)
        {
            renderCharacter(dest, x++, y, ch);
        }
    }
    double lastDisplayStatsTime = -1;
public:
    FFmpegOpenGLRenderer()
        : openGLRenderer(make_shared<OpenGLWindowRenderer>(128, 96))
    {
        ffmpegRenderer = makeFFmpegRenderer(makeOpenGLImageRenderer);
        fontTexture = openGLRenderer->preloadTexture(loadTextFontTexture());
    }
protected:
    virtual void clearInternal(ColorF bg) override
    {
        ffmpegRenderer->clear(bg);
    }
public:
    virtual void calcScales() override
    {
        ffmpegRenderer->calcScales();
        copyScales(*ffmpegRenderer);
    }
    virtual void render(const Mesh &m) override
    {
        ffmpegRenderer->render(m);
    }
    virtual void enableWriteDepth(bool v) override
    {
        ffmpegRenderer->enableWriteDepth(v);
    }
    virtual void flip() override
    {
        ffmpegRenderer->flip();
        calcFPS();
        double currentTime = realTimer();
        if(currentTime - 0.1 < lastDisplayStatsTime)
            return;
        lastDisplayStatsTime = currentTime;
        openGLRenderer->clear(GrayscaleF(0.75));
        ostringstream os;
        os << timer();
        string str = os.str();
        str.resize(10, ' ');
        if(str != "")
        {
            Mesh mesh;
            renderText(mesh, 0, 0, str);
            openGLRenderer->render((Mesh)transform(Matrix::translate(-0.5 * str.size(), -0.5, 0).concat(Matrix::scale(2.0 / str.size())).concat(Matrix::translate(0, 0, -1)), mesh));
        }
        openGLRenderer->flip();
    }
    virtual double timer() override
    {
        return ffmpegRenderer->timer();
    }
    virtual shared_ptr<Texture> preloadTexture(shared_ptr<Texture> texture) override
    {
        return ffmpegRenderer->preloadTexture(texture);
    }
};

shared_ptr<WindowRenderer> makeFFmpegOpenGLRenderer()
{
    return make_shared<FFmpegOpenGLRenderer>();
}
#endif

const Driver drivers[] =
{
#ifndef __EMSCRIPTEN__
    Driver("opengl", []()->shared_ptr<WindowRenderer>{return make_shared<OpenGLWindowRenderer>();}, makeOpenGLImageRenderer),
    Driver("opengl-no-fbo", []()->shared_ptr<WindowRenderer>{return make_shared<OpenGLWindowRenderer>();}, makeSoftwareImageRenderer),
#endif
    Driver("sdl", []()->shared_ptr<WindowRenderer>{return make_shared<SDLWindowRenderer>();}, makeSoftwareImageRenderer),
#ifndef __EMSCRIPTEN__
    Driver("svga", makeSVGARenderer, makeSoftwareImageRenderer),
    Driver("caca", makeCacaRenderer, makeSoftwareImageRenderer),
    Driver("aalib", makeLibAARenderer, makeSoftwareImageRenderer),
#endif
    Driver("null", []()->shared_ptr<WindowRenderer>{return make_shared<NullWindowRenderer>();}, makeSoftwareImageRenderer),
#ifndef __EMSCRIPTEN__
    Driver("ffmpeg", makeFFmpegOpenGLRenderer, makeOpenGLImageRenderer),
    Driver("ffmpeg-no-opengl", makeFFmpegNoOpenGLRenderer, makeSoftwareImageRenderer),
    Driver("raw", makeRawRenderer, makeSoftwareImageRenderer),
#endif
};

constexpr int driverCount = sizeof(drivers) / sizeof(drivers[0]);

int driverIndex = -1;

string getDriverEnvVar()
{
    const char * retval = getenv("LIB3D_DRIVER");
    if(retval == nullptr)
        return "";
    return retval;
}

shared_ptr<WindowRenderer> makeWindowRenderer(string driver = getDriverEnvVar())
{
    shared_ptr<WindowRenderer> retval = nullptr;
    if(driver == "list")
    {
        cout << "lib3d drivers :";
        for(int i = 0; i < driverCount; i++)
            cout << " " << drivers[i].name;
        cout << endl;
        exit(0);
    }
    if(driver != "")
    {
        bool found = false;
        for(int i = 0; i < driverCount; i++)
        {
            if(drivers[i].name == driver)
            {
                found = true;
                retval = drivers[i].start();
                if(retval != nullptr)
                {
                    driverIndex = i;
                    return retval;
                }
                if(driver == "ffmpeg")
                {
                    return makeWindowRenderer("ffmpeg-no-opengl");
                }
                exit(1);
                break;
            }
        }
        if(!found)
        {
            cout << "lib3d : can't find driver : " << driver << endl;
            exit(1);
        }
    }
    for(int i = 0; i < driverCount; i++)
    {
        retval = drivers[i].start();
        if(retval != nullptr)
        {
            driverIndex = i;
            return retval;
        }
    }
    cout << "lib3d : can't start any drivers\n";
    exit(1);
    return nullptr;
}

shared_ptr<WindowRenderer> getWindowRendererInternal()
{
    static shared_ptr<WindowRenderer> theWindowRenderer = nullptr;
    static shared_ptr<WindowRenderer> theWindowRendererReference = nullptr;
    static bool makingIt = false;
    if(theWindowRenderer == nullptr && !makingIt)
    {
        makingIt = true;
        try
        {
            theWindowRenderer = makeWindowRenderer();
        }
        catch(...)
        {
            makingIt = false;
            throw;
        }
        makingIt = false;
        theWindowRendererReference = theWindowRenderer;
        theWindowRenderer = shared_ptr<WindowRenderer>(theWindowRendererReference.get(), [](WindowRenderer *){});
        atexit([](){theWindowRendererReference = nullptr;});
    }
    return theWindowRenderer;
}
}

shared_ptr<WindowRenderer> getWindowRenderer()
{
    return getWindowRendererInternal();
}

shared_ptr<ImageRenderer> makeImageRenderer(size_t w, size_t h, float aspectRatio)
{
    if(driverIndex == -1)
        getWindowRendererInternal();
    if(driverIndex == -1)
        return make_shared<SoftwareRenderer>(w, h, aspectRatio);
    return drivers[driverIndex].makeImageRenderer(w, h, aspectRatio);
}

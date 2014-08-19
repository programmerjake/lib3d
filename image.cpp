#include "image.h"
#include <cstdlib>
#include "image_load_internal.h"

using namespace std;

namespace
{
struct StaticImage
{
    const char * const name;
    const size_t w, h;
    const uint8_t * const pixels;
    const bool isMask;
    Image makeImage() const
    {
        Image retval(w, h);
        ColorI * destPixels = retval.getPixels();
        const uint8_t * srcPixels = pixels;
        for(size_t i = 0; i < w * h; i++)
        {
            if(isMask)
            {
                *destPixels = GrayscaleAI(0xFF, *srcPixels++ ? 0xFF : 0);
            }
            else
            {
                destPixels->r = *srcPixels++;
                destPixels->g = *srcPixels++;
                destPixels->b = *srcPixels++;
                destPixels->a = *srcPixels++;
            }
            destPixels++;
        }
        return std::move(retval);
    }
};

const uint8_t testImagePixels[] =
{
    0, 0xFF, 0, 0xFF,  0, 0xFF, 0, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF,
    0, 0xFF, 0, 0xFF,  0, 0xFF, 0, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF,
    0, 0, 0xFF, 0xFF,  0, 0, 0xFF, 0xFF,  0xFF, 0, 0, 0xFF,  0xFF, 0, 0, 0xFF,
    0, 0, 0xFF, 0xFF,  0, 0, 0xFF, 0xFF,  0xFF, 0, 0, 0xFF,  0xFF, 0, 0, 0xFF,
};

const StaticImage testImage =
{
    "test.png",
    4, 4, &testImagePixels[0], false
};

const uint8_t ffmpegOpenGLRendererFontPixels[] =
{
    0, 1, 1, 1, 1, 0, 0, 0, // 0: '?'
    1, 1, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 1, 1, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, // 1: '.'
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, // 2: '-'
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 0, 0,
    1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, // 3: '+'
    0, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 0, 0,
    1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, // 4: 'e'
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 0, 0, 0,
    1, 1, 0, 0, 1, 1, 0, 0,
    1, 1, 1, 1, 1, 1, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 1, 1, 1, 1, 1, 0, 0, // 5: '0'
    1, 1, 0, 0, 0, 1, 1, 0,
    1, 1, 0, 0, 1, 1, 1, 0,
    1, 1, 0, 1, 1, 1, 1, 0,
    1, 1, 1, 1, 0, 1, 1, 0,
    1, 1, 1, 0, 0, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 1, 1, 0, 0, 0, 0, // 6: '1'
    0, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 1, 1, 1, 1, 0, 0, 0, // 7: '2'
    1, 1, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0,
    0, 0, 1, 1, 1, 0, 0, 0,
    0, 1, 1, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 1, 1, 0, 0,
    1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 1, 1, 1, 1, 0, 0, 0, // 8: '3'
    1, 1, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0,
    0, 0, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0,
    1, 1, 0, 0, 1, 1, 0, 0,
    0, 1, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 1, 1, 1, 0, 0, // 9: '4'
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 1, 1, 0, 1, 1, 0, 0,
    1, 1, 0, 0, 1, 1, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 1, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    1, 1, 1, 1, 1, 1, 0, 0, // 10: '5'
    1, 1, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0,
    1, 1, 0, 0, 1, 1, 0, 0,
    0, 1, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 1, 1, 1, 0, 0, 0, // 11: '6'
    0, 1, 1, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 0, 0, 0,
    1, 1, 0, 0, 1, 1, 0, 0,
    1, 1, 0, 0, 1, 1, 0, 0,
    0, 1, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    1, 1, 1, 1, 1, 1, 0, 0, // 12: '7'
    1, 1, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 1, 1, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 1, 1, 1, 1, 0, 0, 0, // 13: '8'
    1, 1, 0, 0, 1, 1, 0, 0,
    1, 1, 0, 0, 1, 1, 0, 0,
    0, 1, 1, 1, 1, 0, 0, 0,
    1, 1, 0, 0, 1, 1, 0, 0,
    1, 1, 0, 0, 1, 1, 0, 0,
    0, 1, 1, 1, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 1, 1, 1, 1, 0, 0, 0, // 14: '9'
    1, 1, 0, 0, 1, 1, 0, 0,
    1, 1, 0, 0, 1, 1, 0, 0,
    0, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 1, 1, 0, 0, 0,
    0, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, // 15: ' '
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

};

const StaticImage ffmpegOpenGLRendererFont =
{
    "ffmpeg-opengl-renderer-font.png",
    8, 8 * 16/* 16 pictures */, &ffmpegOpenGLRendererFontPixels[0], true
};
}

TextureDescriptor getFFmpegOpenGLRendererFontCharacterTextureDescriptor(char ch, shared_ptr<Texture> texture)
{
    int yPos = 0;
    switch(ch)
    {
    case '.':
        yPos = 8;
        break;
    case '-':
        yPos = 16;
        break;
    case '+':
        yPos = 24;
        break;
    case 'e':
        yPos = 32;
        break;
    case '0':
        yPos = 40;
        break;
    case '1':
        yPos = 48;
        break;
    case '2':
        yPos = 56;
        break;
    case '3':
        yPos = 64;
        break;
    case '4':
        yPos = 72;
        break;
    case '5':
        yPos = 80;
        break;
    case '6':
        yPos = 88;
        break;
    case '7':
        yPos = 96;
        break;
    case '8':
        yPos = 104;
        break;
    case '9':
        yPos = 112;
        break;
    case ' ':
        yPos = 120;
        break;
    default: // '?'
        yPos = 0;
        break;
    }
    float maxV = 1 - (yPos + 0.1) / ffmpegOpenGLRendererFont.h;
    float minV = 1 - (yPos + 8 - 0.1) / ffmpegOpenGLRendererFont.h;
    return TextureDescriptor(texture, 0, 1, minV, maxV);
}

namespace
{
struct StaticImageRef
{
    const StaticImage * const simage;
    shared_ptr<const Image> pimage;
    StaticImageRef(const StaticImage * simage)
        : simage(simage), pimage(nullptr)
    {
    }
};

StaticImageRef images[] =
{
    StaticImageRef(&testImage),
    StaticImageRef(&ffmpegOpenGLRendererFont)
};
}

shared_ptr<const Image> Image::loadImage(string fileName)
{
    for(StaticImageRef & i : images)
    {
        if(i.simage->name == fileName)
        {
            shared_ptr<const Image> & retval = i.pimage;
            if(retval == nullptr)
                retval = make_shared<Image>(i.simage->makeImage());
            return retval;
        }
    }
    ImageLoadInternal::ImageLoaderResult result = ImageLoadInternal::load(fileName, [](string msg)
    {
        throw ImageLoadError(msg);
    });
    shared_ptr<Image> retval = make_shared<Image>(result.w, result.h);
    for(size_t y = 0; y < result.h; y++)
    {
        for(size_t x = 0; x < result.w; x++)
        {
            const uint8_t *pixel = result.getPixelPtr(x, y);
            retval->setPixel(x, y, RGBAI(pixel[0], pixel[1], pixel[2], pixel[3]));
        }
    }
    return retval;
}

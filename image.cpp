#include "image.h"
#include <cstdlib>
#include "image_load_internal.h"

using namespace std;

namespace
{
struct StaticImage
{
    enum class Type
    {
        RGBA,
        Mask,
        RLEMask,
        BitwiseMask,
    };
    const char * const name;
    const size_t w, h;
    const uint8_t * const pixels;
    const Type type;
    Image makeImage() const
    {
        Image retval(w, h);
        ColorI * destPixels = retval.getPixels();
        const uint8_t * srcPixels = pixels;
        switch(type)
        {
        case Type::RGBA:
            for(size_t i = 0; i < w * h; i++)
            {
                destPixels->r = *srcPixels++;
                destPixels->g = *srcPixels++;
                destPixels->b = *srcPixels++;
                destPixels->a = *srcPixels++;
                destPixels++;
            }
            break;
        case Type::Mask:
            for(size_t i = 0; i < w * h; i++)
            {
                *destPixels = GrayscaleAI(0xFF, *srcPixels++ ? 0xFF : 0);
                destPixels++;
            }
            break;
        case Type::RLEMask:
        {
            unsigned count = 0;
            bool currentValue = false;
            for(size_t i = 0; i < w * h; i++)
            {
                if(count == 0)
                {
                    count = *srcPixels++;
                    currentValue = (count & 0x80) != 0;
                    count &= ~0x80;
                    if(count == 0)
                        count = 0x80;
                }
                *destPixels++ = GrayscaleAI(0xFF, currentValue ? 0xFF : 0);
                count--;
            }
            break;
        }
        case Type::BitwiseMask:
        {
            uint8_t bitMask = 0, v = 0;
            for(size_t i = 0; i < w * h; i++)
            {
                if(bitMask == 0)
                {
                    v = *srcPixels++;
                    bitMask = 1;
                }
                *destPixels++ = GrayscaleAI(0xFF, (v & bitMask) == 0 ? 0xFF : 0);
                bitMask <<= 1;
            }
            break;
        }
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
    4, 4, &testImagePixels[0], StaticImage::Type::RGBA
};

const uint8_t ffmpegOpenGLRendererFontPixels[] =
{
    0xff, 0x81, 0x81, 0xc9, 0xf7, 0xe3, 0xf7, 0xff, 0x00, 0xff, 0x00, 0x0f,
    0xc3, 0x03, 0x01, 0x66, 0xff, 0x7e, 0x00, 0x80, 0xe3, 0xc1, 0xf7, 0xff,
    0x00, 0xc3, 0x3c, 0x1f, 0x99, 0x33, 0x39, 0xa5, 0xff, 0x5a, 0x24, 0x80,
    0xc1, 0xe3, 0xe3, 0xe7, 0x18, 0x99, 0x66, 0x0f, 0x99, 0x03, 0x01, 0xc3,
    0xff, 0x7e, 0x00, 0x80, 0x80, 0x80, 0xc1, 0xc3, 0x3c, 0xbd, 0x42, 0x41,
    0x99, 0xf3, 0x39, 0x18, 0xff, 0x42, 0x3c, 0xc1, 0xc1, 0x80, 0x80, 0xc3,
    0x3c, 0xbd, 0x42, 0xcc, 0xc3, 0xf3, 0x39, 0x18, 0xff, 0x66, 0x18, 0xe3,
    0xe3, 0xc1, 0xc1, 0xe7, 0x18, 0x99, 0x66, 0xcc, 0xe7, 0xf1, 0x19, 0xc3,
    0xff, 0x7e, 0x00, 0xf7, 0xf7, 0xe3, 0xe3, 0xff, 0x00, 0xc3, 0x3c, 0xcc,
    0x81, 0xf0, 0x98, 0xa5, 0xff, 0x81, 0x81, 0xff, 0xff, 0xc1, 0xc1, 0xff,
    0x00, 0xff, 0x00, 0xe1, 0xe7, 0xf8, 0xfc, 0x66, 0xfe, 0xbf, 0xe7, 0x99,
    0x01, 0x83, 0xff, 0xe7, 0xe7, 0xe7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xf8, 0x8f, 0xc3, 0x99, 0x24, 0x39, 0xff, 0xc3, 0xc3, 0xe7, 0xe7, 0xf3,
    0xff, 0xdb, 0xe7, 0x00, 0xe0, 0x83, 0x81, 0x99, 0x24, 0xe3, 0xff, 0x81,
    0x81, 0xe7, 0xcf, 0xf9, 0xfc, 0x99, 0xc3, 0x00, 0x80, 0x80, 0xe7, 0x99,
    0x21, 0xc9, 0xff, 0xe7, 0xe7, 0xe7, 0x80, 0x80, 0xfc, 0x00, 0x81, 0x81,
    0xe0, 0x83, 0xe7, 0x99, 0x27, 0xc9, 0x81, 0x81, 0xe7, 0x81, 0xcf, 0xf9,
    0xfc, 0x99, 0x00, 0xc3, 0xf8, 0x8f, 0x81, 0xff, 0x27, 0xe3, 0x81, 0xc3,
    0xe7, 0xc3, 0xe7, 0xf3, 0x80, 0xdb, 0x00, 0xe7, 0xfe, 0xbf, 0xc3, 0x99,
    0x27, 0xcc, 0x81, 0xe7, 0xe7, 0xe7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xe7, 0xff, 0xff, 0xe1, 0xff, 0x00, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf3, 0xc9, 0xc9, 0xf3, 0xff, 0xe3, 0xf9,
    0xe7, 0xf9, 0xff, 0xff, 0xff, 0xff, 0xff, 0x9f, 0xff, 0xe1, 0xc9, 0xc9,
    0xc1, 0x9c, 0xc9, 0xf9, 0xf3, 0xf3, 0x99, 0xf3, 0xff, 0xff, 0xff, 0xcf,
    0xff, 0xe1, 0xc9, 0x80, 0xfc, 0xcc, 0xe3, 0xfc, 0xf9, 0xe7, 0xc3, 0xf3,
    0xff, 0xff, 0xff, 0xe7, 0xff, 0xf3, 0xff, 0xc9, 0xe1, 0xe7, 0x91, 0xff,
    0xf9, 0xe7, 0x00, 0xc0, 0xff, 0xc0, 0xff, 0xf3, 0xff, 0xf3, 0xff, 0x80,
    0xcf, 0xf3, 0xc4, 0xff, 0xf9, 0xe7, 0xc3, 0xf3, 0xff, 0xff, 0xff, 0xf9,
    0xff, 0xff, 0xff, 0xc9, 0xe0, 0x99, 0xcc, 0xff, 0xf3, 0xf3, 0x99, 0xf3,
    0xf3, 0xff, 0xf3, 0xfc, 0xff, 0xf3, 0xff, 0xc9, 0xf3, 0x9c, 0x91, 0xff,
    0xe7, 0xf9, 0xff, 0xff, 0xf3, 0xff, 0xf3, 0xfe, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xff, 0xff, 0xff,
    0xc1, 0xf3, 0xe1, 0xe1, 0xc7, 0xc0, 0xe3, 0xc0, 0xe1, 0xe1, 0xff, 0xff,
    0xe7, 0xff, 0xf9, 0xe1, 0x9c, 0xf1, 0xcc, 0xcc, 0xc3, 0xfc, 0xf9, 0xcc,
    0xcc, 0xcc, 0xf3, 0xf3, 0xf3, 0xff, 0xf3, 0xcc, 0x8c, 0xf3, 0xcf, 0xcf,
    0xc9, 0xe0, 0xfc, 0xcf, 0xcc, 0xcc, 0xf3, 0xf3, 0xf9, 0xc0, 0xe7, 0xcf,
    0x84, 0xf3, 0xe3, 0xe3, 0xcc, 0xcf, 0xe0, 0xe7, 0xe1, 0xc1, 0xff, 0xff,
    0xfc, 0xff, 0xcf, 0xe7, 0x90, 0xf3, 0xf9, 0xcf, 0x80, 0xcf, 0xcc, 0xf3,
    0xcc, 0xcf, 0xff, 0xff, 0xf9, 0xff, 0xe7, 0xf3, 0x98, 0xf3, 0xcc, 0xcc,
    0xcf, 0xcc, 0xcc, 0xf3, 0xcc, 0xe7, 0xf3, 0xf3, 0xf3, 0xc0, 0xf3, 0xff,
    0xc1, 0xc0, 0xc0, 0xe1, 0x87, 0xe1, 0xe1, 0xf3, 0xe1, 0xf1, 0xf3, 0xf3,
    0xe7, 0xff, 0xf9, 0xf3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xf9, 0xff, 0xff, 0xff, 0xff, 0xc1, 0xf3, 0xc0, 0xc3,
    0xe0, 0x80, 0x80, 0xc3, 0xcc, 0xe1, 0x87, 0x98, 0xf0, 0x9c, 0x9c, 0xe3,
    0x9c, 0xe1, 0x99, 0x99, 0xc9, 0xb9, 0xb9, 0x99, 0xcc, 0xf3, 0xcf, 0x99,
    0xf9, 0x88, 0x98, 0xc9, 0x84, 0xcc, 0x99, 0xfc, 0x99, 0xe9, 0xe9, 0xfc,
    0xcc, 0xf3, 0xcf, 0xc9, 0xf9, 0x80, 0x90, 0x9c, 0x84, 0xcc, 0xc1, 0xfc,
    0x99, 0xe1, 0xe1, 0xfc, 0xc0, 0xf3, 0xcf, 0xe1, 0xf9, 0x80, 0x84, 0x9c,
    0x84, 0xc0, 0x99, 0xfc, 0x99, 0xe9, 0xe9, 0x8c, 0xcc, 0xf3, 0xcc, 0xc9,
    0xb9, 0x94, 0x8c, 0x9c, 0xfc, 0xcc, 0x99, 0x99, 0xc9, 0xb9, 0xf9, 0x99,
    0xcc, 0xf3, 0xcc, 0x99, 0x99, 0x9c, 0x9c, 0xc9, 0xe1, 0xcc, 0xc0, 0xc3,
    0xe0, 0x80, 0xf0, 0x83, 0xcc, 0xe1, 0xe1, 0x98, 0x80, 0x9c, 0x9c, 0xe3,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xc0, 0xe1, 0xc0, 0xe1, 0xc0, 0xcc, 0xcc, 0x9c,
    0x9c, 0xcc, 0x80, 0xe1, 0xfc, 0xe1, 0xf7, 0xff, 0x99, 0xcc, 0x99, 0xcc,
    0xd2, 0xcc, 0xcc, 0x9c, 0x9c, 0xcc, 0x9c, 0xf9, 0xf9, 0xe7, 0xe3, 0xff,
    0x99, 0xcc, 0x99, 0xf8, 0xf3, 0xcc, 0xcc, 0x9c, 0xc9, 0xcc, 0xce, 0xf9,
    0xf3, 0xe7, 0xc9, 0xff, 0xc1, 0xcc, 0xc1, 0xf1, 0xf3, 0xcc, 0xcc, 0x94,
    0xe3, 0xe1, 0xe7, 0xf9, 0xe7, 0xe7, 0x9c, 0xff, 0xf9, 0xc4, 0xc9, 0xc7,
    0xf3, 0xcc, 0xcc, 0x80, 0xe3, 0xf3, 0xb3, 0xf9, 0xcf, 0xe7, 0xff, 0xff,
    0xf9, 0xe1, 0x99, 0xcc, 0xf3, 0xcc, 0xe1, 0x88, 0xc9, 0xf3, 0x99, 0xf9,
    0x9f, 0xe7, 0xff, 0xff, 0xf0, 0xc7, 0x98, 0xe1, 0xe1, 0xc0, 0xf3, 0x9c,
    0x9c, 0xe1, 0x80, 0xe1, 0xbf, 0xe1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
    0xf3, 0xff, 0xf8, 0xff, 0xc7, 0xff, 0xe3, 0xff, 0xf8, 0xf3, 0xcf, 0xf8,
    0xf1, 0xff, 0xff, 0xff, 0xf3, 0xff, 0xf9, 0xff, 0xcf, 0xff, 0xc9, 0xff,
    0xf9, 0xff, 0xff, 0xf9, 0xf3, 0xff, 0xff, 0xff, 0xe7, 0xe1, 0xf9, 0xe1,
    0xcf, 0xe1, 0xf9, 0x91, 0xc9, 0xf1, 0xcf, 0x99, 0xf3, 0xcc, 0xe0, 0xe1,
    0xff, 0xcf, 0xc1, 0xcc, 0xc1, 0xcc, 0xf0, 0xcc, 0x91, 0xf3, 0xcf, 0xc9,
    0xf3, 0x80, 0xcc, 0xcc, 0xff, 0xc1, 0x99, 0xfc, 0xcc, 0xc0, 0xf9, 0xcc,
    0x99, 0xf3, 0xcf, 0xe1, 0xf3, 0x80, 0xcc, 0xcc, 0xff, 0xcc, 0x99, 0xcc,
    0xcc, 0xfc, 0xf9, 0xc1, 0x99, 0xf3, 0xcc, 0xc9, 0xf3, 0x94, 0xcc, 0xcc,
    0xff, 0x91, 0xc4, 0xe1, 0x91, 0xe1, 0xf0, 0xcf, 0x98, 0xe1, 0xcc, 0x98,
    0xe1, 0x9c, 0xcc, 0xe1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0,
    0xff, 0xff, 0xe1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc7, 0xe7, 0xf8, 0x91, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xf3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf3,
    0xe7, 0xf3, 0xc4, 0xf7, 0xc4, 0x91, 0xc4, 0xc1, 0xc1, 0xcc, 0xcc, 0x9c,
    0x9c, 0xcc, 0xc0, 0xf3, 0xe7, 0xf3, 0xff, 0xe3, 0x99, 0xcc, 0x91, 0xfc,
    0xf3, 0xcc, 0xcc, 0x94, 0xc9, 0xcc, 0xe6, 0xf8, 0xff, 0xc7, 0xff, 0xc9,
    0x99, 0xcc, 0x99, 0xe1, 0xf3, 0xcc, 0xcc, 0x80, 0xe3, 0xcc, 0xf3, 0xf3,
    0xe7, 0xf3, 0xff, 0x9c, 0xc1, 0xc1, 0xf9, 0xcf, 0xd3, 0xcc, 0xe1, 0x80,
    0xc9, 0xc1, 0xd9, 0xf3, 0xe7, 0xf3, 0xff, 0x9c, 0xf9, 0xcf, 0xf0, 0xe0,
    0xe7, 0x91, 0xf3, 0xc9, 0x9c, 0xcf, 0xc0, 0xc7, 0xe7, 0xf8, 0xff, 0x80,
    0xf0, 0x87, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xe1, 0xff, 0xc7, 0x81, 0xcc, 0xf8, 0xf3, 0xff,
    0x81, 0xcc, 0xf8, 0xcc, 0xc1, 0xf8, 0x9c, 0xf3, 0xcc, 0xcc, 0xff, 0x3c,
    0xff, 0xff, 0xf3, 0xff, 0x3c, 0xff, 0xff, 0xff, 0x9c, 0xff, 0xe3, 0xf3,
    0xfc, 0xff, 0xe1, 0xc3, 0xe1, 0xe1, 0xe1, 0xe1, 0xc3, 0xe1, 0xe1, 0xf1,
    0xe3, 0xf1, 0xc9, 0xff, 0xcc, 0xcc, 0xcc, 0x9f, 0xcf, 0xcf, 0xcf, 0xfc,
    0x99, 0xcc, 0xcc, 0xf3, 0xe7, 0xf3, 0x9c, 0xe1, 0xe1, 0xcc, 0xc0, 0x83,
    0xc1, 0xc1, 0xc1, 0xfc, 0x81, 0xc0, 0xc0, 0xf3, 0xe7, 0xf3, 0x80, 0xcc,
    0xe7, 0xcc, 0xfc, 0x99, 0xcc, 0xcc, 0xcc, 0xe1, 0xf9, 0xfc, 0xfc, 0xf3,
    0xe7, 0xf3, 0x9c, 0xc0, 0xcf, 0x81, 0xe1, 0x03, 0x81, 0x81, 0x81, 0xcf,
    0xc3, 0xe1, 0xe1, 0xe1, 0xc3, 0xe1, 0x9c, 0xcc, 0xe1, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xe3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xc7, 0xff, 0x83, 0xe1, 0xff, 0xff, 0xe1, 0xff, 0xff, 0x3c, 0xcc, 0xe7,
    0xe3, 0xcc, 0xe0, 0x8f, 0xff, 0xff, 0xc9, 0xcc, 0xcc, 0xf8, 0xcc, 0xf8,
    0xcc, 0xe7, 0xff, 0xe7, 0xc9, 0xcc, 0xcc, 0x27, 0xc0, 0x01, 0xcc, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xc3, 0xcc, 0x81, 0xd9, 0xe1, 0xcc, 0xe7,
    0xf9, 0xcf, 0x80, 0xe1, 0xe1, 0xe1, 0xcc, 0xcc, 0xcc, 0x99, 0xcc, 0xfc,
    0xf0, 0xc0, 0xa0, 0xc3, 0xe1, 0x01, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
    0xcc, 0x99, 0xcc, 0xfc, 0xf9, 0xf3, 0x9c, 0xe7, 0xf9, 0xcc, 0xcc, 0xcc,
    0xcc, 0xcc, 0xcc, 0xcc, 0xc1, 0xc3, 0xcc, 0x81, 0x98, 0xc0, 0x0c, 0xe7,
    0xc0, 0x01, 0x8c, 0xe1, 0xe1, 0xe1, 0x81, 0x81, 0xcf, 0xe7, 0xe1, 0xe7,
    0xc0, 0xf3, 0x9c, 0xe4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xe0, 0xff, 0xff, 0xe7, 0xff, 0xf3, 0x1c, 0xf1, 0xc7, 0xe3, 0xff, 0xff,
    0xff, 0xc0, 0xc3, 0xe3, 0xf3, 0xff, 0xff, 0x3c, 0x3c, 0xe7, 0xff, 0xff,
    0xff, 0xff, 0xc7, 0xc7, 0xe0, 0xff, 0xc9, 0xc9, 0xff, 0xff, 0xff, 0x9c,
    0x9c, 0xe7, 0x33, 0xcc, 0xe1, 0xf1, 0xff, 0xff, 0xff, 0xcc, 0xc9, 0xc9,
    0xf3, 0xff, 0xff, 0xcc, 0xcc, 0xff, 0x99, 0x99, 0xcf, 0xf3, 0xe1, 0xcc,
    0xe0, 0xc8, 0x83, 0xe3, 0xf9, 0xc0, 0xc0, 0x84, 0x24, 0xe7, 0xcc, 0x33,
    0xc1, 0xf3, 0xcc, 0xcc, 0xcc, 0xc0, 0xff, 0xff, 0xfc, 0xfc, 0xcf, 0x33,
    0x13, 0xe7, 0x99, 0x99, 0xcc, 0xf3, 0xcc, 0xcc, 0xcc, 0xc4, 0x81, 0xc1,
    0xcc, 0xfc, 0xcf, 0x99, 0x09, 0xe7, 0x33, 0xcc, 0x81, 0xe1, 0xe1, 0x81,
    0xcc, 0xcc, 0xff, 0xff, 0xe1, 0xff, 0xff, 0xcc, 0x0c, 0xe7, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0f,
    0x3f, 0xff, 0xff, 0xff, 0xbb, 0x55, 0x24, 0xe7, 0xe7, 0xe7, 0x93, 0xff,
    0xff, 0x93, 0x93, 0xff, 0x93, 0x93, 0xe7, 0xff, 0xee, 0xaa, 0x11, 0xe7,
    0xe7, 0xe7, 0x93, 0xff, 0xff, 0x93, 0x93, 0xff, 0x93, 0x93, 0xe7, 0xff,
    0xbb, 0x55, 0x24, 0xe7, 0xe7, 0xe0, 0x93, 0xff, 0xe0, 0x90, 0x93, 0x80,
    0x90, 0x93, 0xe0, 0xff, 0xee, 0xaa, 0x88, 0xe7, 0xe7, 0xe7, 0x93, 0xff,
    0xe7, 0x9f, 0x93, 0x9f, 0x9f, 0x93, 0xe7, 0xff, 0xbb, 0x55, 0x24, 0xe7,
    0xe0, 0xe0, 0x90, 0x80, 0xe0, 0x90, 0x93, 0x90, 0x80, 0x80, 0xe0, 0xe0,
    0xee, 0xaa, 0x11, 0xe7, 0xe7, 0xe7, 0x93, 0x93, 0xe7, 0x93, 0x93, 0x93,
    0xff, 0xff, 0xff, 0xe7, 0xbb, 0x55, 0x24, 0xe7, 0xe7, 0xe7, 0x93, 0x93,
    0xe7, 0x93, 0x93, 0x93, 0xff, 0xff, 0xff, 0xe7, 0xee, 0xaa, 0x88, 0xe7,
    0xe7, 0xe7, 0x93, 0x93, 0xe7, 0x93, 0x93, 0x93, 0xff, 0xff, 0xff, 0xe7,
    0xe7, 0xe7, 0xff, 0xe7, 0xff, 0xe7, 0xe7, 0x93, 0x93, 0xff, 0x93, 0xff,
    0x93, 0xff, 0x93, 0xe7, 0xe7, 0xe7, 0xff, 0xe7, 0xff, 0xe7, 0xe7, 0x93,
    0x93, 0xff, 0x93, 0xff, 0x93, 0xff, 0x93, 0xe7, 0xe7, 0xe7, 0xff, 0xe7,
    0xff, 0xe7, 0x07, 0x93, 0x13, 0x03, 0x10, 0x00, 0x13, 0x00, 0x10, 0x00,
    0xe7, 0xe7, 0xff, 0xe7, 0xff, 0xe7, 0xe7, 0x93, 0xf3, 0xf3, 0xff, 0xff,
    0xf3, 0xff, 0xff, 0xff, 0x07, 0x00, 0x00, 0x07, 0x00, 0x00, 0x07, 0x13,
    0x03, 0x13, 0x00, 0x10, 0x13, 0x00, 0x10, 0x00, 0xff, 0xff, 0xe7, 0xe7,
    0xff, 0xe7, 0xe7, 0x93, 0xff, 0x93, 0xff, 0x93, 0x93, 0xff, 0x93, 0xff,
    0xff, 0xff, 0xe7, 0xe7, 0xff, 0xe7, 0xe7, 0x93, 0xff, 0x93, 0xff, 0x93,
    0x93, 0xff, 0x93, 0xff, 0xff, 0xff, 0xe7, 0xe7, 0xff, 0xe7, 0xe7, 0x93,
    0xff, 0x93, 0xff, 0x93, 0x93, 0xff, 0x93, 0xff, 0x93, 0xff, 0xff, 0x93,
    0xe7, 0xff, 0xff, 0x93, 0xe7, 0xe7, 0xff, 0x00, 0xff, 0xf0, 0x0f, 0x00,
    0x93, 0xff, 0xff, 0x93, 0xe7, 0xff, 0xff, 0x93, 0xe7, 0xe7, 0xff, 0x00,
    0xff, 0xf0, 0x0f, 0x00, 0x93, 0x00, 0xff, 0x93, 0x07, 0x07, 0xff, 0x93,
    0x00, 0xe7, 0xff, 0x00, 0xff, 0xf0, 0x0f, 0x00, 0x93, 0xff, 0xff, 0x93,
    0xe7, 0xe7, 0xff, 0x93, 0xe7, 0xe7, 0xff, 0x00, 0xff, 0xf0, 0x0f, 0x00,
    0x00, 0x00, 0x00, 0x03, 0x07, 0x07, 0x03, 0x00, 0x00, 0xe0, 0x07, 0x00,
    0x00, 0xf0, 0x0f, 0xff, 0xff, 0xe7, 0x93, 0xff, 0xff, 0xe7, 0x93, 0x93,
    0xe7, 0xff, 0xe7, 0x00, 0x00, 0xf0, 0x0f, 0xff, 0xff, 0xe7, 0x93, 0xff,
    0xff, 0xe7, 0x93, 0x93, 0xe7, 0xff, 0xe7, 0x00, 0x00, 0xf0, 0x0f, 0xff,
    0xff, 0xe7, 0x93, 0xff, 0xff, 0xe7, 0x93, 0x93, 0xe7, 0xff, 0xe7, 0x00,
    0x00, 0xf0, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff,
    0xc0, 0xe3, 0xe3, 0xc7, 0xff, 0x9f, 0xe3, 0xe1, 0xff, 0xe1, 0xc0, 0x80,
    0xcc, 0xff, 0x99, 0x91, 0xf3, 0xc9, 0xc9, 0xf3, 0xff, 0xcf, 0xf9, 0xcc,
    0x91, 0xcc, 0xcc, 0xc9, 0xf9, 0x81, 0x99, 0xc4, 0xe1, 0x9c, 0x9c, 0xe7,
    0x81, 0x81, 0xfc, 0xcc, 0xc4, 0xe0, 0xfc, 0xc9, 0xf3, 0xe4, 0x99, 0xe7,
    0xcc, 0x80, 0x9c, 0xc1, 0x24, 0x24, 0xe0, 0xcc, 0xec, 0xcc, 0xfc, 0xc9,
    0xf9, 0xe4, 0x99, 0xe7, 0xcc, 0x9c, 0xc9, 0xcc, 0x24, 0x24, 0xfc, 0xcc,
    0xc4, 0xe0, 0xfc, 0xc9, 0xcc, 0xe4, 0xc1, 0xe7, 0xe1, 0xc9, 0xc9, 0xcc,
    0x81, 0x81, 0xf9, 0xcc, 0x91, 0xfc, 0xfc, 0xc9, 0xc0, 0xf1, 0xf9, 0xe7,
    0xf3, 0xe3, 0x88, 0xe1, 0xff, 0xf9, 0xe3, 0xcc, 0xff, 0xfc, 0xff, 0xff,
    0xff, 0xff, 0xfc, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xff,
    0xff, 0xf3, 0xf9, 0xe7, 0x8f, 0xe7, 0xf3, 0xff, 0xe3, 0xff, 0xff, 0x0f,
    0xe1, 0xf1, 0xff, 0xff, 0xc0, 0xf3, 0xf3, 0xf3, 0x27, 0xe7, 0xf3, 0x91,
    0xc9, 0xff, 0xff, 0xcf, 0xc9, 0xe7, 0xff, 0xff, 0xff, 0xc0, 0xe7, 0xf9,
    0x27, 0xe7, 0xff, 0xc4, 0xc9, 0xff, 0xff, 0xcf, 0xc9, 0xf3, 0xc3, 0xff,
    0xc0, 0xf3, 0xf3, 0xf3, 0xe7, 0xe7, 0xc0, 0xff, 0xe3, 0xe7, 0xff, 0xcf,
    0xc9, 0xf9, 0xc3, 0xff, 0xff, 0xf3, 0xf9, 0xe7, 0xe7, 0xe7, 0xff, 0x91,
    0xff, 0xe7, 0xe7, 0xc8, 0xc9, 0xe1, 0xc3, 0xff, 0xc0, 0xff, 0xff, 0xff,
    0xe7, 0xe4, 0xf3, 0xc4, 0xff, 0xff, 0xff, 0xc9, 0xff, 0xff, 0xc3, 0xff,
    0xff, 0xc0, 0xc0, 0xc0, 0xe7, 0xe4, 0xf3, 0xff, 0xff, 0xff, 0xff, 0xc3,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe7, 0xf1, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xc7, 0xff, 0xff, 0xff, 0xff
};

const StaticImage ffmpegOpenGLRendererFont =
{
    "text-font.png",
    128, 128, &ffmpegOpenGLRendererFontPixels[0], StaticImage::Type::BitwiseMask
};

uint8_t convertToCP437(unsigned ch)
{
    static const unsigned table[0x100] =
    {
        0x0000, // 0x00 - NULL
        0x263A, // 0x01 - START OF HEADING
        0x263B, // 0x02 - START OF TEXT
        0x2665, // 0x03 - END OF TEXTSMISSION
        0x2666, // 0x04 - END OF TRANSMISSION
        0x2663, // 0x05 - ENQUIRYEDGE
        0x2660, // 0x06 - ACKNOWLEDGE
        0x2022, // 0x07 - BELLSPACE
        0x25D8, // 0x08 - BACKSPACEL TABULATION
        0x25CB, // 0x09 - HORIZONTAL TABULATION
        0x25D9, // 0x0A - LINE FEEDTABULATION
        0x2642, // 0x0B - VERTICAL TABULATION
        0x2640, // 0x0C - FORM FEEDRETURN
        0x266A, // 0x0D - CARRIAGE RETURN
        0x266B, // 0x0E - SHIFT OUT
        0x263C, // 0x0F - SHIFT INK ESCAPE
        0x25BA, // 0x10 - DATA LINK ESCAPENE
        0x25C4, // 0x11 - DEVICE CONTROL ONE
        0x2195, // 0x12 - DEVICE CONTROL TWOEE
        0x203C, // 0x13 - DEVICE CONTROL THREE
        0x00B6, // 0x14 - DEVICE CONTROL FOURE
        0x00A7, // 0x15 - NEGATIVE ACKNOWLEDGE
        0x25AC, // 0x16 - SYNCHRONOUS IDLEION BLOCK
        0x21A8, // 0x17 - END OF TRANSMISSION BLOCK
        0x2191, // 0x18 - CANCEL MEDIUM
        0x2193, // 0x19 - END OF MEDIUM
        0x2192, // 0x1A - SUBSTITUTE
        0x2190, // 0x1B - ESCAPEEPARATOR
        0x221F, // 0x1C - FILE SEPARATORR
        0x2194, // 0x1D - GROUP SEPARATORR
        0x25B2, // 0x1E - RECORD SEPARATOR
        0x25BC, // 0x1F - UNIT SEPARATOR
        0x0020, // 0x20 - SPACEMATION MARK
        0x0021, // 0x21 - EXCLAMATION MARK
        0x0022, // 0x22 - QUOTATION MARK
        0x0023, // 0x23 - NUMBER SIGN
        0x0024, // 0x24 - DOLLAR SIGNN
        0x0025, // 0x25 - PERCENT SIGN
        0x0026, // 0x26 - AMPERSANDE
        0x0027, // 0x27 - APOSTROPHETHESIS
        0x0028, // 0x28 - LEFT PARENTHESISS
        0x0029, // 0x29 - RIGHT PARENTHESIS
        0x002A, // 0x2A - ASTERISKN
        0x002B, // 0x2B - PLUS SIGN
        0x002C, // 0x2C - COMMAN-MINUS
        0x002D, // 0x2D - HYPHEN-MINUS
        0x002E, // 0x2E - FULL STOP
        0x002F, // 0x2F - SOLIDUSERO
        0x0030, // 0x30 - DIGIT ZERO
        0x0031, // 0x31 - DIGIT ONE
        0x0032, // 0x32 - DIGIT TWOEE
        0x0033, // 0x33 - DIGIT THREE
        0x0034, // 0x34 - DIGIT FOUR
        0x0035, // 0x35 - DIGIT FIVE
        0x0036, // 0x36 - DIGIT SIXEN
        0x0037, // 0x37 - DIGIT SEVEN
        0x0038, // 0x38 - DIGIT EIGHTACEOIN SMALL LETTER NTHORIZONTAL DOUBLE
        0x0039, // 0x39 - DIGIT NINE
        0x003A, // 0x3A - COLON
        0x003B, // 0x3B - SEMICOLON
        0x003C, // 0x3C - LESS-THAN SIGN
        0x003D, // 0x3D - EQUALS SIGN
        0x003E, // 0x3E - GREATER-THAN SIGN
        0x003F, // 0x3F - QUESTION MARK
        0x0040, // 0x40 - COMMERCIAL AT
        0x0041, // 0x41 - LATIN CAPITAL LETTER A
        0x0042, // 0x42 - LATIN CAPITAL LETTER B
        0x0043, // 0x43 - LATIN CAPITAL LETTER C
        0x0044, // 0x44 - LATIN CAPITAL LETTER D
        0x0045, // 0x45 - LATIN CAPITAL LETTER E
        0x0046, // 0x46 - LATIN CAPITAL LETTER F
        0x0047, // 0x47 - LATIN CAPITAL LETTER G
        0x0048, // 0x48 - LATIN CAPITAL LETTER H
        0x0049, // 0x49 - LATIN CAPITAL LETTER I
        0x004A, // 0x4A - LATIN CAPITAL LETTER J
        0x004B, // 0x4B - LATIN CAPITAL LETTER K
        0x004C, // 0x4C - LATIN CAPITAL LETTER L
        0x004D, // 0x4D - LATIN CAPITAL LETTER M
        0x004E, // 0x4E - LATIN CAPITAL LETTER N
        0x004F, // 0x4F - LATIN CAPITAL LETTER O
        0x0050, // 0x50 - LATIN CAPITAL LETTER P
        0x0051, // 0x51 - LATIN CAPITAL LETTER Q
        0x0052, // 0x52 - LATIN CAPITAL LETTER R
        0x0053, // 0x53 - LATIN CAPITAL LETTER S
        0x0054, // 0x54 - LATIN CAPITAL LETTER T
        0x0055, // 0x55 - LATIN CAPITAL LETTER U
        0x0056, // 0x56 - LATIN CAPITAL LETTER V
        0x0057, // 0x57 - LATIN CAPITAL LETTER W
        0x0058, // 0x58 - LATIN CAPITAL LETTER X
        0x0059, // 0x59 - LATIN CAPITAL LETTER Y
        0x005A, // 0x5A - LATIN CAPITAL LETTER Z
        0x005B, // 0x5B - LEFT SQUARE BRACKET
        0x005C, // 0x5C - REVERSE SOLIDUS
        0x005D, // 0x5D - RIGHT SQUARE BRACKET
        0x005E, // 0x5E - CIRCUMFLEX ACCENT
        0x005F, // 0x5F - LOW LINE
        0x0060, // 0x60 - GRAVE ACCENT
        0x0061, // 0x61 - LATIN SMALL LETTER A
        0x0062, // 0x62 - LATIN SMALL LETTER B
        0x0063, // 0x63 - LATIN SMALL LETTER C
        0x0064, // 0x64 - LATIN SMALL LETTER D
        0x0065, // 0x65 - LATIN SMALL LETTER E
        0x0066, // 0x66 - LATIN SMALL LETTER F
        0x0067, // 0x67 - LATIN SMALL LETTER G
        0x0068, // 0x68 - LATIN SMALL LETTER H
        0x0069, // 0x69 - LATIN SMALL LETTER I
        0x006A, // 0x6A - LATIN SMALL LETTER J
        0x006B, // 0x6B - LATIN SMALL LETTER K
        0x006C, // 0x6C - LATIN SMALL LETTER L
        0x006D, // 0x6D - LATIN SMALL LETTER M
        0x006E, // 0x6E - LATIN SMALL LETTER N
        0x006F, // 0x6F - LATIN SMALL LETTER O
        0x0070, // 0x70 - LATIN SMALL LETTER P
        0x0071, // 0x71 - LATIN SMALL LETTER Q
        0x0072, // 0x72 - LATIN SMALL LETTER R
        0x0073, // 0x73 - LATIN SMALL LETTER S
        0x0074, // 0x74 - LATIN SMALL LETTER T
        0x0075, // 0x75 - LATIN SMALL LETTER U
        0x0076, // 0x76 - LATIN SMALL LETTER V
        0x0077, // 0x77 - LATIN SMALL LETTER W
        0x0078, // 0x78 - LATIN SMALL LETTER X
        0x0079, // 0x79 - LATIN SMALL LETTER Y
        0x007A, // 0x7A - LATIN SMALL LETTER Z
        0x007B, // 0x7B - LEFT CURLY BRACKET
        0x007C, // 0x7C - VERTICAL LINE
        0x007D, // 0x7D - RIGHT CURLY BRACKET
        0x007E, // 0x7E - TILDE
        0x2302, // 0x7F - DELETE
        0x00C7, // 0x80 - LATIN CAPITAL LETTER C WITH CEDILLA
        0x00FC, // 0x81 - LATIN SMALL LETTER U WITH DIAERESIS
        0x00E9, // 0x82 - LATIN SMALL LETTER E WITH ACUTE
        0x00E2, // 0x83 - LATIN SMALL LETTER A WITH CIRCUMFLEX
        0x00E4, // 0x84 - LATIN SMALL LETTER A WITH DIAERESIS
        0x00E0, // 0x85 - LATIN SMALL LETTER A WITH GRAVE
        0x00E5, // 0x86 - LATIN SMALL LETTER A WITH RING ABOVE
        0x00E7, // 0x87 - LATIN SMALL LETTER C WITH CEDILLA
        0x00EA, // 0x88 - LATIN SMALL LETTER E WITH CIRCUMFLEX
        0x00EB, // 0x89 - LATIN SMALL LETTER E WITH DIAERESIS
        0x00E8, // 0x8A - LATIN SMALL LETTER E WITH GRAVE
        0x00EF, // 0x8B - LATIN SMALL LETTER I WITH DIAERESIS
        0x00EE, // 0x8C - LATIN SMALL LETTER I WITH CIRCUMFLEX
        0x00EC, // 0x8D - LATIN SMALL LETTER I WITH GRAVE
        0x00C4, // 0x8E - LATIN CAPITAL LETTER A WITH DIAERESIS
        0x00C5, // 0x8F - LATIN CAPITAL LETTER A WITH RING ABOVE
        0x00C9, // 0x90 - LATIN CAPITAL LETTER E WITH ACUTE
        0x00E6, // 0x91 - LATIN SMALL LIGATURE AE
        0x00C6, // 0x92 - LATIN CAPITAL LIGATURE AE
        0x00F4, // 0x93 - LATIN SMALL LETTER O WITH CIRCUMFLEX
        0x00F6, // 0x94 - LATIN SMALL LETTER O WITH DIAERESIS
        0x00F2, // 0x95 - LATIN SMALL LETTER O WITH GRAVE
        0x00FB, // 0x96 - LATIN SMALL LETTER U WITH CIRCUMFLEX
        0x00F9, // 0x97 - LATIN SMALL LETTER U WITH GRAVE
        0x00FF, // 0x98 - LATIN SMALL LETTER Y WITH DIAERESIS
        0x00D6, // 0x99 - LATIN CAPITAL LETTER O WITH DIAERESIS
        0x00DC, // 0x9A - LATIN CAPITAL LETTER U WITH DIAERESIS
        0x00A2, // 0x9B - CENT SIGN
        0x00A3, // 0x9C - POUND SIGN
        0x00A5, // 0x9D - YEN SIGN
        0x20A7, // 0x9E - PESETA SIGN
        0x0192, // 0x9F - LATIN SMALL LETTER F WITH HOOK
        0x00E1, // 0xA0 - LATIN SMALL LETTER A WITH ACUTE
        0x00ED, // 0xA1 - LATIN SMALL LETTER I WITH ACUTE
        0x00F3, // 0xA2 - LATIN SMALL LETTER O WITH ACUTE
        0x00FA, // 0xA3 - LATIN SMALL LETTER U WITH ACUTE
        0x00F1, // 0xA4 - LATIN SMALL LETTER N WITH TILDE
        0x00D1, // 0xA5 - LATIN CAPITAL LETTER N WITH TILDE
        0x00AA, // 0xA6 - FEMININE ORDINAL INDICATOR
        0x00BA, // 0xA7 - MASCULINE ORDINAL INDICATOR
        0x00BF, // 0xA8 - INVERTED QUESTION MARK
        0x2310, // 0xA9 - REVERSED NOT SIGN
        0x00AC, // 0xAA - NOT SIGN
        0x00BD, // 0xAB - VULGAR FRACTION ONE HALF
        0x00BC, // 0xAC - VULGAR FRACTION ONE QUARTER
        0x00A1, // 0xAD - INVERTED EXCLAMATION MARK
        0x00AB, // 0xAE - LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
        0x00BB, // 0xAF - RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
        0x2591, // 0xB0 - LIGHT SHADE
        0x2592, // 0xB1 - MEDIUM SHADE
        0x2593, // 0xB2 - DARK SHADE
        0x2502, // 0xB3 - BOX DRAWINGS LIGHT VERTICAL
        0x2524, // 0xB4 - BOX DRAWINGS LIGHT VERTICAL AND LEFT
        0x2561, // 0xB5 - BOX DRAWINGS VERTICAL SINGLE AND LEFT DOUBLE
        0x2562, // 0xB6 - BOX DRAWINGS VERTICAL DOUBLE AND LEFT SINGLE
        0x2556, // 0xB7 - BOX DRAWINGS DOWN DOUBLE AND LEFT SINGLE
        0x2555, // 0xB8 - BOX DRAWINGS DOWN SINGLE AND LEFT DOUBLE
        0x2563, // 0xB9 - BOX DRAWINGS DOUBLE VERTICAL AND LEFT
        0x2551, // 0xBA - BOX DRAWINGS DOUBLE VERTICAL
        0x2557, // 0xBB - BOX DRAWINGS DOUBLE DOWN AND LEFT
        0x255D, // 0xBC - BOX DRAWINGS DOUBLE UP AND LEFT
        0x255C, // 0xBD - BOX DRAWINGS UP DOUBLE AND LEFT SINGLE
        0x255B, // 0xBE - BOX DRAWINGS UP SINGLE AND LEFT DOUBLE
        0x2510, // 0xBF - BOX DRAWINGS LIGHT DOWN AND LEFT
        0x2514, // 0xC0 - BOX DRAWINGS LIGHT UP AND RIGHT
        0x2534, // 0xC1 - BOX DRAWINGS LIGHT UP AND HORIZONTAL
        0x252C, // 0xC2 - BOX DRAWINGS LIGHT DOWN AND HORIZONTAL
        0x251C, // 0xC3 - BOX DRAWINGS LIGHT VERTICAL AND RIGHT
        0x2500, // 0xC4 - BOX DRAWINGS LIGHT HORIZONTAL
        0x253C, // 0xC5 - BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL
        0x255E, // 0xC6 - BOX DRAWINGS VERTICAL SINGLE AND RIGHT DOUBLE
        0x255F, // 0xC7 - BOX DRAWINGS VERTICAL DOUBLE AND RIGHT SINGLE
        0x255A, // 0xC8 - BOX DRAWINGS DOUBLE UP AND RIGHT
        0x2554, // 0xC9 - BOX DRAWINGS DOUBLE DOWN AND RIGHT
        0x2569, // 0xCA - BOX DRAWINGS DOUBLE UP AND HORIZONTAL
        0x2566, // 0xCB - BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL
        0x2560, // 0xCC - BOX DRAWINGS DOUBLE VERTICAL AND RIGHT
        0x2550, // 0xCD - BOX DRAWINGS DOUBLE HORIZONTAL
        0x256C, // 0xCE - BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL
        0x2567, // 0xCF - BOX DRAWINGS UP SINGLE AND HORIZONTAL DOUBLE
        0x2568, // 0xD0 - BOX DRAWINGS UP DOUBLE AND HORIZONTAL SINGLE
        0x2564, // 0xD1 - BOX DRAWINGS DOWN SINGLE AND HORIZONTAL DOUBLE
        0x2565, // 0xD2 - BOX DRAWINGS DOWN DOUBLE AND HORIZONTAL SINGLE
        0x2559, // 0xD3 - BOX DRAWINGS UP DOUBLE AND RIGHT SINGLE
        0x2558, // 0xD4 - BOX DRAWINGS UP SINGLE AND RIGHT DOUBLE
        0x2552, // 0xD5 - BOX DRAWINGS DOWN SINGLE AND RIGHT DOUBLE
        0x2553, // 0xD6 - BOX DRAWINGS DOWN DOUBLE AND RIGHT SINGLE
        0x256B, // 0xD7 - BOX DRAWINGS VERTICAL DOUBLE AND HORIZONTAL SINGLE
        0x256A, // 0xD8 - BOX DRAWINGS VERTICAL SINGLE AND HORIZONTAL DOUBLE
        0x2518, // 0xD9 - BOX DRAWINGS LIGHT UP AND LEFT
        0x250C, // 0xDA - BOX DRAWINGS LIGHT DOWN AND RIGHT
        0x2588, // 0xDB - FULL BLOCK
        0x2584, // 0xDC - LOWER HALF BLOCK
        0x258C, // 0xDD - LEFT HALF BLOCK
        0x2590, // 0xDE - RIGHT HALF BLOCK
        0x2580, // 0xDF - UPPER HALF BLOCK
        0x03B1, // 0xE0 - GREEK SMALL LETTER ALPHA
        0x00DF, // 0xE1 - LATIN SMALL LETTER SHARP S
        0x0393, // 0xE2 - GREEK CAPITAL LETTER GAMMA
        0x03C0, // 0xE3 - GREEK SMALL LETTER PI
        0x03A3, // 0xE4 - GREEK CAPITAL LETTER SIGMA
        0x03C3, // 0xE5 - GREEK SMALL LETTER SIGMA
        0x00B5, // 0xE6 - MICRO SIGN
        0x03C4, // 0xE7 - GREEK SMALL LETTER TAU
        0x03A6, // 0xE8 - GREEK CAPITAL LETTER PHI
        0x0398, // 0xE9 - GREEK CAPITAL LETTER THETA
        0x03A9, // 0xEA - GREEK CAPITAL LETTER OMEGA
        0x03B4, // 0xEB - GREEK SMALL LETTER DELTA
        0x221E, // 0xEC - INFINITY
        0x03C6, // 0xED - GREEK SMALL LETTER PHI
        0x03B5, // 0xEE - GREEK SMALL LETTER EPSILON
        0x2229, // 0xEF - INTERSECTION
        0x2261, // 0xF0 - IDENTICAL TO
        0x00B1, // 0xF1 - PLUS-MINUS SIGN
        0x2265, // 0xF2 - GREATER-THAN OR EQUAL TO
        0x2264, // 0xF3 - LESS-THAN OR EQUAL TO
        0x2320, // 0xF4 - TOP HALF INTEGRAL
        0x2321, // 0xF5 - BOTTOM HALF INTEGRAL
        0x00F7, // 0xF6 - DIVISION SIGN
        0x2248, // 0xF7 - ALMOST EQUAL TO
        0x00B0, // 0xF8 - DEGREE SIGN
        0x2219, // 0xF9 - BULLET OPERATOR
        0x00B7, // 0xFA - MIDDLE DOT
        0x221A, // 0xFB - SQUARE ROOT
        0x207F, // 0xFC - SUPERSCRIPT LATIN SMALL LETTER N
        0x00B2, // 0xFD - SUPERSCRIPT TWO
        0x25A0, // 0xFE - BLACK SQUARE
        0x00A0, // 0xFF - NO-BREAK SPACE
    };
    if(ch < 0x7F)
        return (uint8_t)ch;
    for(unsigned i = 0; i < sizeof(table) / sizeof(table[0]); i++)
    {
        if(table[i] == ch)
            return i;
    }
    return 4; // '♦'
}
}

TextureDescriptor getTextFontCharacterTextureDescriptor(unsigned ch, shared_ptr<Texture> texture)
{
    unsigned position = convertToCP437(ch);
    int xPos = position % 16, yPos = position / 16;
    constexpr int charSize = 8;
    xPos *= charSize;
    yPos *= charSize;
    constexpr float edgeSpacing = 0.1;
    float minU = (xPos + edgeSpacing) / ffmpegOpenGLRendererFont.w;
    float maxU = (xPos + charSize - edgeSpacing) / ffmpegOpenGLRendererFont.w;
    float maxV = 1 - (yPos + edgeSpacing) / ffmpegOpenGLRendererFont.h;
    float minV = 1 - (yPos + charSize - edgeSpacing) / ffmpegOpenGLRendererFont.h;
    return TextureDescriptor(texture, minU, maxU, minV, maxV);
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

shared_ptr<Texture> loadTextFontTexture()
{
    return make_shared<ImageTexture>(Image::loadImage("text-font.png"));
}
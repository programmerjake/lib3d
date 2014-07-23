#include "image.h"

namespace
{
struct StaticImage
{
    const char * const name;
    const size_t w, h;
    const uint8_t * const pixels;
    Image makeImage() const
    {
        Image retval(w, h);
        ColorI * destPixels = retval.getPixels();
        const uint8_t * srcPixels = &pixels[0];
        for(size_t i = 0; i < w * h; i++)
        {
            destPixels->r = *srcPixels++;
            destPixels->g = *srcPixels++;
            destPixels->b = *srcPixels++;
            destPixels->a = *srcPixels++;
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
    "test",
    4, 4, &testImagePixels[0]
};

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
    throw runtime_error("can't load image : " + fileName);
}

#include "image_load_internal.h"
#include <wand/MagickWand.h>
#include <cstdlib>

using namespace std;

namespace
{
void init()
{
    static bool didIt = false;
    if(didIt)
        return;
    didIt = true;
    MagickWandGenesis();
    atexit([]()
    {
        MagickWandTerminus();
    });
}
}

ImageLoadInternal::ImageLoaderResult ImageLoadInternal::load(string fileName, void (*throwErrorFn)(string msg))
{
    init();
    MagickWand *mw = NewMagickWand();
    MagickBooleanType status = MagickReadImage(mw, fileName.c_str());
    if(status == MagickFalse)
    {
        string msg;
        ExceptionType severity;
        char *description = MagickGetException(mw, &severity);
        msg = description;
        MagickRelinquishMemory(description);
        DestroyMagickWand(mw);
        throwErrorFn(msg);
    }
    PixelIterator *iterator = NewPixelIterator(mw);
    if(iterator == nullptr)
    {
        string msg;
        ExceptionType severity;
        char *description = MagickGetException(mw, &severity);
        msg = description;
        MagickRelinquishMemory(description);
        DestroyMagickWand(mw);
        throwErrorFn(msg);
    }
    uint8_t *data = nullptr;
    size_t w = MagickGetImageWidth(mw), h = MagickGetImageHeight(mw);
    try
    {
        data = new uint8_t[4 * w * h];
        if(!data)
        {
            throwErrorFn("can't allocate image memory");
        }
        for(size_t y = 0; y < h; y++)
        {
            size_t currentW;
            PixelWand **pixels = PixelGetNextIteratorRow(iterator, &currentW);
            if(pixels == nullptr)
            {
                throwErrorFn("can't get image pixels");
            }
            if(currentW != w)
            {
                throwErrorFn("current row width != image width");
            }
            for(size_t x = 0; x < w; x++)
            {
                uint8_t *pixel = data + x * 4 + y * w * 4;
                Quantum r, g, b, a;
                r = PixelGetRedQuantum(pixels[x]);
                g = PixelGetGreenQuantum(pixels[x]);
                b = PixelGetBlueQuantum(pixels[x]);
                a = PixelGetAlphaQuantum(pixels[x]);
                pixel[0] = (r * 0xFF + 0x7F) / QuantumRange;
                pixel[1] = (g * 0xFF + 0x7F) / QuantumRange;
                pixel[2] = (b * 0xFF + 0x7F) / QuantumRange;
                pixel[3] = (a * 0xFF + 0x7F) / QuantumRange;
            }
        }
    }
    catch(...)
    {
        DestroyPixelIterator(iterator);
        DestroyMagickWand(mw);
        delete []data;
        throw;
    }
    DestroyPixelIterator(iterator);
    DestroyMagickWand(mw);
    return ImageLoaderResult(data, w, h);
}

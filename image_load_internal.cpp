#include "image_load_internal.h"
#ifndef __EMSCRIPTEN__
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
#else
#include <png.h>
#include <cstdio>
#include <cstdlib>
#include <cerrno>

using namespace std;

namespace
{

void pngloaderror(png_structp png_ptr, png_const_charp msg)
{
    *(string *)png_get_error_ptr(png_ptr) = msg;
    longjmp(png_jmpbuf(png_ptr), 1);
}

void pngloadwarning(png_structp, png_const_charp)
{
    // do nothing
}

inline bool LoadPNG(const char *filename, uint8_t *&pixels, unsigned &width, unsigned &height, string &errorMsg)
{
    FILE *f = fopen(filename, "rb");
    if(!f)
    {
        errorMsg = string("can't open file : ") + strerror(errno) + " : \"" + filename + "\"";
        return false;
    }
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (void *)&errorMsg, pngloaderror, pngloadwarning);
    if(!png_ptr)
    {
        fclose(f);
        errorMsg = "can't create png read struct";
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(!info_ptr)
    {
        png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
        fclose(f);
        errorMsg = "can't create png info struct";
        return false;
    }

    uint8_t *volatile retval = NULL;  // so that it isn't messed up by longjmp
    volatile png_bytepp rows = NULL;

    if(setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
        fclose(f);
        delete []retval;
        delete []rows;
        return false;
    }

    png_init_io(png_ptr, f);

    png_read_info(png_ptr, info_ptr);

    png_uint_32 XRes, YRes;
    int bit_depth, color_type, interlace_type;
    png_get_IHDR(png_ptr, info_ptr, &XRes, &YRes, &bit_depth, &color_type, &interlace_type, int_p_NULL, int_p_NULL);

    png_set_strip_16(png_ptr);

    png_set_packing(png_ptr);

    if(color_type == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_palette_to_rgb(png_ptr);
    }

    if((color_type & PNG_COLOR_MASK_ALPHA) == 0)
    {
        png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);
    }

    width = XRes;
    height = YRes;
    retval = new uint8_t[width * height * 4];
    if(!retval)
    {
        errorMsg = "can't allocate image data array";
        longjmp(png_jmpbuf(png_ptr), 1);
    }
    rows = (png_bytepp)new png_bytep[YRes];
    if(!rows)
    {
        errorMsg = "can't allocate image row indirection array";
        longjmp(png_jmpbuf(png_ptr), 1);
    }
    for(unsigned y = 0; y < YRes; y++)
    {
        rows[y] = (png_bytep)&retval[y * XRes * 4];
    }

    png_read_image(png_ptr, rows);

    png_read_end(png_ptr, info_ptr);

    delete []rows;

    png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
    fclose(f);
    pixels = (uint8_t *)retval;
    return true;
}

}

ImageLoadInternal::ImageLoaderResult ImageLoadInternal::load(string fileName, void (*throwErrorFn)(string msg))
{
    string errorMsg, str = fileName;
    uint8_t *data;
    unsigned w, h;
    if(!LoadPNG(str.c_str(), data, w, h, errorMsg))
    {
        throwErrorFn(errorMsg);
    }
    return ImageLoaderResult(data, w, h);
}
#endif
#ifndef IMAGE_LOAD_INTERNAL_H_INCLUDED
#define IMAGE_LOAD_INTERNAL_H_INCLUDED

#include <string>
#include <cstdint>

using namespace std;

namespace ImageLoadInternal
{
    struct ImageLoaderResult
    {
        uint8_t *data;
        size_t w, h;
        ImageLoaderResult(uint8_t *data, size_t w, size_t h)
            : data(data), w(w), h(h)
        {
        }
        ImageLoaderResult(ImageLoaderResult &&rt)
            : data(rt.data), w(rt.w), h(rt.h)
        {
            rt.data = nullptr;
        }
        ImageLoaderResult(const ImageLoaderResult &) = delete;
        void operator =(const ImageLoaderResult &) = delete;
        ~ImageLoaderResult()
        {
            delete []data;
        }
        const uint8_t *getPixelPtr(size_t x, size_t y) const
        {
            return data + 4 * x + 4 * w * y;
        }
    };
    ImageLoaderResult load(string fileName, void (*throwErrorFn)(string msg));
}

#endif // IMAGE_LOAD_INTERNAL_H_INCLUDED

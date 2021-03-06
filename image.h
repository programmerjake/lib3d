#ifndef IMAGE_H_INCLUDED
#define IMAGE_H_INCLUDED

#include <cstdint>
#include <memory>
#include <utility>
#include <iterator>
#include <cassert>
#include <cmath>
#include <string>
#include <stdexcept>

using namespace std;

struct ColorI
{
    uint8_t b, g, r, a;
    constexpr operator uint32_t() const
    {
        return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }
    constexpr ColorI()
        : b(0), g(0), r(0), a(0)
    {
    }
    constexpr ColorI(uint32_t v)
        : b((uint8_t)v), g((uint8_t)(v >> 8)), r((uint8_t)(v >> 16)), a((uint8_t)(v >> 24))
    {
    }
private:
    constexpr ColorI(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        : b(b), g(g), r(r), a(a)
    {
    }
public:
    friend constexpr ColorI RGBAI(int R, int G, int B, int A);
    friend constexpr bool operator ==(ColorI a, ColorI b)
    {
        return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
    }
    friend constexpr bool operator !=(ColorI a, ColorI b)
    {
        return a.r != b.r || a.g != b.g || a.b != b.b || a.a != b.a;
    }
};

namespace std
{
template <>
struct hash<ColorI>
{
    hash<uint8_t> uint8Hasher;
    size_t operator ()(ColorI c) const
    {
        return uint8Hasher(c.r) + 2 * uint8Hasher(c.g) + 3 * uint8Hasher(c.b) + 5 * uint8Hasher(c.a);
    }
};
}

constexpr ColorI RGBAI(int R, int G, int B, int A)
{
    return ColorI((uint8_t)R, (uint8_t)G, (uint8_t)B, (uint8_t)A);
}

constexpr ColorI RGBI(int R, int G, int B)
{
    return RGBAI(R, G, B, 0xFF);
}

constexpr ColorI GrayscaleAI(int v, int A)
{
    return RGBAI(v, v, v, A);
}

constexpr ColorI GrayscaleI(int v)
{
    return RGBI(v, v, v);
}

constexpr int getAValueI(ColorI c)
{
    return c.a;
}

constexpr int getRValueI(ColorI c)
{
    return c.r;
}

constexpr int getGValueI(ColorI c)
{
    return c.g;
}

constexpr int getBValueI(ColorI c)
{
    return c.b;
}

constexpr int getLuminanceValueI(ColorI c)
{
    return (int)(((uint_fast32_t)c.r * 13988 + (uint_fast32_t)c.g * 47055 + (uint_fast32_t)c.b * 4750) >> 16);
}

template <typename T>
constexpr const T &limit(const T &v, const T &minimum, const T &maximum)
{
    return (v < minimum) ? minimum : ((maximum < v) ? maximum : v);
}

struct ColorF
{
    float r, g, b, a;
    explicit constexpr ColorF(ColorI c)
        : r((float)c.r / 0xFF), g((float)c.g / 0xFF), b((float)c.b / 0xFF), a((float)c.a / 0xFF)
    {
    }
    constexpr ColorF()
        : r(0), g(0), b(0), a(0)
    {
    }
private:
    constexpr ColorF(float r, float g, float b, float a)
        : r(r), g(g), b(b), a(a)
    {
    }
public:
    explicit constexpr operator ColorI() const
    {
        return RGBAI(limit((int)(r * 0x100), 0, 0xFF), limit((int)(g * 0x100), 0, 0xFF), limit((int)(b * 0x100), 0, 0xFF), limit((int)(a * 0x100), 0, 0xFF));
    }
    friend constexpr ColorF RGBAF(float r, float g, float b, float a);
    friend constexpr bool operator ==(ColorF a, ColorF b)
    {
        return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
    }
    friend constexpr bool operator !=(ColorF a, ColorF b)
    {
        return a.r != b.r || a.g != b.g || a.b != b.b || a.a != b.a;
    }
};

namespace std
{
template <>
struct hash<ColorF>
{
    hash<float> floatHasher;
    size_t operator ()(ColorF c) const
    {
        return floatHasher(c.r) + 2 * floatHasher(c.g) + 3 * floatHasher(c.b) + 5 * floatHasher(c.a);
    }
};
}

constexpr ColorF RGBAF(float r, float g, float b, float a)
{
    return ColorF(r, g, b, a);
}

constexpr ColorF RGBF(float r, float g, float b)
{
    return RGBAF(r, g, b, 1);
}

constexpr ColorF XYZAF(float x, float y, float z, float a)
{
    return RGBAF(0.41847 * x - 0.15866 * y - 0.082835 * z,
                 -0.091169 * x + 0.25243 * y + 0.015708 * z,
                 0.0009209 * x - 0.0025498 * y + 0.1786 * z, a);
}

constexpr ColorF XYZF(float x, float y, float z)
{
    return XYZAF(x, y, z, 1);
}

constexpr ColorF GrayscaleAF(float v, float A)
{
    return RGBAF(v, v, v, A);
}

constexpr ColorF GrayscaleF(float v)
{
    return RGBF(v, v, v);
}

constexpr float getAValueF(ColorF c)
{
    return c.a;
}

constexpr float getRValueF(ColorF c)
{
    return c.r;
}

constexpr float getGValueF(ColorF c)
{
    return c.g;
}

constexpr float getBValueF(ColorF c)
{
    return c.b;
}

constexpr float getLuminanceValueF(ColorF c)
{
    return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b;
}

constexpr ColorF compose(ColorF fg, ColorF bg)
{
    return RGBAF(fg.r * fg.a + bg.r * (1 - fg.a),
                 fg.g * fg.a + bg.g * (1 - fg.a),
                 fg.b * fg.a + bg.b * (1 - fg.a),
                 fg.a + bg.a * (1 - fg.a));
}

constexpr ColorF addHelper(ColorF a, ColorF b, float alphaValue)
{
    return RGBAF(a.r * (a.a / alphaValue) + b.r * (b.a / alphaValue), a.g * (a.a / alphaValue) + b.g * (b.a / alphaValue), a.b * (a.a / alphaValue) + b.b * (b.a / alphaValue), alphaValue);
}

constexpr ColorF add(ColorF a, ColorF b)
{
    return (a.a <= 0 && b.a <= 0) ? RGBAF(a.r + b.r, a.g + b.g, a.b + b.b, 0) : addHelper(a, b, 1 - (1 - a.a) * (1 - b.a));
}

constexpr ColorI compose(ColorI fg, ColorI bg)
{
    return RGBAI(((unsigned)fg.r * fg.a + (unsigned)bg.r * (0xFF - fg.a)) / 0xFF,
                 ((unsigned)fg.g * fg.a + (unsigned)bg.g * (0xFF - fg.a)) / 0xFF,
                 ((unsigned)fg.b * fg.a + (unsigned)bg.b * (0xFF - fg.a)) / 0xFF,
                 fg.a + (unsigned)bg.a * (0xFF - fg.a) / 0xFF);
}

constexpr ColorF scale(ColorF a, ColorF b)
{
    return RGBAF(a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a);
}

constexpr ColorF scaleF(float a, ColorF b)
{
    return RGBAF(a * b.r, a * b.g, a * b.b, b.a);
}

constexpr ColorF scaleF(ColorF b, float a)
{
    return RGBAF(a * b.r, a * b.g, a * b.b, b.a);
}

constexpr ColorF colorize(ColorF a, ColorF b)
{
    return scale(a, b);
}

constexpr ColorI colorize(ColorF a, ColorI b)
{
    return RGBAI((int)(a.r * b.r), (int)(a.g * b.g), (int)(a.b * b.b), (int)(a.a * b.a));
}

template <typename T>
constexpr T interpolate(float v, T a, T b)
{
    return a + v * (b - a);
}

template <>
constexpr ColorF interpolate<ColorF>(float v, ColorF a, ColorF b)
{
    return RGBAF(interpolate(v, a.r, b.r), interpolate(v, a.g, b.g), interpolate(v, a.b, b.b), interpolate(v, a.a, b.a));
}

inline ColorF HSBAF(float H, float S, float Br, float A)
{
    H -= std::floor(H);
    S = limit<float>(S, 0, 1);
    Br = limit<float>(Br, 0, 1);
    float R = 0, G = 0, B = 0;
    H *= 6;

    switch((int)std::floor(H))
    {
    case 0:
        R = 1;
        G = H;
        break;

    case 1:
        R = 2 - H;
        G = 1;
        break;

    case 2:
        G = 1;
        B = H - 2;
        break;

    case 3:
        G = 4 - H;
        B = 1;
        break;

    case 4:
        B = 1;
        R = H - 4;
        break;

    default:
        B = 6 - H;
        R = 1;
        break;
    }

    ColorF grayscaleValue = RGBAF(Br, Br, Br, A);
    ColorF colorValue = RGBAF(R, G, B, A);

    if(Br < 0.5)
    {
        colorValue = interpolate(Br * 2, RGBAF(0, 0, 0, A), colorValue);
    }
    else
    {
        colorValue = interpolate(2 - Br * 2, RGBAF(1, 1, 1, A), colorValue);
    }

    return interpolate(S, grayscaleValue, colorValue);
}

inline ColorF HSBF(float H, float S, float B)
{
    return HSBAF(H, S, B, 1);
}

constexpr ColorF setAlphaF(ColorF c, float a)
{
    return RGBAF(c.r, c.g, c.b, a);
}

constexpr ColorI setAlphaI(ColorI c, int a)
{
    return RGBAI(c.r, c.g, c.b, a);
}

struct ImageLoadError : public runtime_error
{
    explicit ImageLoadError(const string &msg)
        : runtime_error(msg)
    {
    }
};

struct Image
{
    size_t w, h;
private:
    ColorI *pixels;
public:
    ColorI *getPixels()
    {
        return pixels;
    }
    const ColorI *getPixels() const
    {
        return pixels;
    }
    mutable shared_ptr<void> glProperties;
    Image(size_t w, size_t h, ColorI *pixels = nullptr)
        : w(w), h(h), pixels(pixels)
    {
        if(pixels == nullptr && w > 0 && h > 0)
        {
            this->pixels = new ColorI[w * h];
            clear();
        }
    }
    Image(ColorI color)
        : Image(1, 1)
    {
        clear(color);
    }
    Image()
        : Image(0, 0)
    {
    }
    Image(const Image &rt)
        : w(rt.w), h(rt.h), pixels(nullptr), glProperties(nullptr)
    {
        if(w > 0 && h > 0)
        {
            pixels = new ColorI[w * h];

            for(size_t i = 0; i < w * h; i++)
            {
                pixels[i] = rt.pixels[i];
            }
        }
    }
    Image(Image &&rt)
        : w(rt.w), h(rt.h), pixels(rt.pixels), glProperties(rt.glProperties)
    {
        rt.w = 0;
        rt.h = 0;
        rt.pixels = nullptr;
        rt.glProperties = nullptr;
    }
    ~Image()
    {
        delete []pixels;
    }
    const Image &operator =(const Image &rt)
    {
        if(pixels != nullptr && pixels == rt.pixels)
        {
            return *this;
        }

        if(pixels != nullptr)
        {
            delete []pixels;
        }

        w = rt.w;
        h = rt.h;

        if(w > 0 && h > 0)
        {
            pixels = new ColorI[w * h];

            for(size_t i = 0; i < w * h; i++)
            {
                pixels[i] = rt.pixels[i];
            }
        }

        return *this;
    }
    const Image &operator =(Image && rt)
    {
        std::swap(w, rt.w);
        std::swap(h, rt.h);
        std::swap(pixels, rt.pixels);
        std::swap(glProperties, rt.glProperties);
        return *this;
    }
    operator bool() const
    {
        return pixels != nullptr;
    }
    bool operator !() const
    {
        return pixels == nullptr;
    }
    void setPixel(int x, int y, ColorI color)
    {
        if(x < 0 || y < 0 || (size_t)x >= w || (size_t)y >= h)
        {
            return;
        }

        pixels[x + y * w] = color;
    }
    ColorI getPixel(int x, int y) const
    {
        if(x < 0 || y < 0 || (size_t)x >= w || (size_t)y >= h)
        {
            return RGBAI(0, 0, 0, 0);
        }

        return pixels[x + y * w];
    }
    ColorI *getLineAddress(size_t y)
    {
        return &pixels[y * w];
    }
    const ColorI *getLineAddress(size_t y) const
    {
        return &pixels[y * w];
    }
    void clear(ColorI color = RGBAI(0, 0, 0, 0))
    {
        for(size_t i = 0; i < w * h; i++)
        {
            pixels[i] = color;
        }
    }
    struct HLineRange
    {
        ColorI *beginV;
        ColorI *endV;
        HLineRange(ColorI *beginV, ColorI *endV)
            : beginV(beginV), endV(endV)
        {
        }
        ColorI *begin()
        {
            return beginV;
        }
        ColorI *end()
        {
            return endV;
        }
    };
    struct HLineRangeConst
    {
        const ColorI *beginV;
        const ColorI *endV;
        HLineRangeConst(ColorI *beginV, ColorI *endV)
            : beginV(beginV), endV(endV)
        {
        }
        const ColorI *begin()
        {
            return beginV;
        }
        const ColorI *end()
        {
            return endV;
        }
    };
    HLineRange getHorizontalLine(int startX, int endX, int y)
    {
        assert(startX <= endX);
        assert(startX >= 0 && (size_t)endX < w);
        assert(y >= 0 && (size_t)y < h);
        return HLineRange(&pixels[startX + y * w], &pixels[endX + y * w]);
    }
    HLineRangeConst getHorizontalLine(int startX, int endX, int y) const
    {
        assert(startX <= endX);
        assert(startX >= 0 && (size_t)endX < w);
        assert(y >= 0 && (size_t)y < h);
        return HLineRangeConst(&pixels[startX + y * w], &pixels[endX + y * w]);
    }
    HLineRange getHorizontalLine(int y)
    {
        return getHorizontalLine(0, w, y);
    }
    HLineRangeConst getHorizontalLine(int y) const
    {
        return getHorizontalLine(0, w, y);
    }
    struct ColumnIterator : public std::iterator<std::random_access_iterator_tag, ColorI>
    {
        friend struct ColumnConstIterator;
        ColorI *value;
        size_t w;
    public:
        ColumnIterator()
            : value(nullptr), w(0)
        {
        }
        ColumnIterator(ColorI *value, size_t w)
            : value(value), w(w)
        {
        }
        bool operator ==(const ColumnIterator &rt) const
        {
            return value == rt.value;
        }
        bool operator !=(const ColumnIterator &rt) const
        {
            return value != rt.value;
        }
        bool operator >=(const ColumnIterator &rt) const
        {
            return value >= rt.value;
        }
        bool operator <=(const ColumnIterator &rt) const
        {
            return value <= rt.value;
        }
        bool operator >(const ColumnIterator &rt) const
        {
            return value > rt.value;
        }
        bool operator <(const ColumnIterator &rt) const
        {
            return value < rt.value;
        }
        ColorI &operator *() const
        {
            return *value;
        }
        ColorI &operator [](ptrdiff_t index) const
        {
            return value[(ptrdiff_t)w * index];
        }
        ColorI *operator ->() const
        {
            return value;
        }
        const ColumnIterator &operator ++()
        {
            value += w;
            return *this;
        }
        const ColumnIterator &operator --()
        {
            value -= w;
            return *this;
        }
        ColumnIterator operator ++(int)
        {
            ColumnIterator retval = *this;
            value += w;
            return retval;
        }
        ColumnIterator operator --(int)
        {
            ColumnIterator retval = *this;
            value -= w;
            return retval;
        }
        ColumnIterator operator +(ptrdiff_t rt) const
        {
            return ColumnIterator(value + w * rt, w);
        }
        ColumnIterator operator -(ptrdiff_t rt) const
        {
            return ColumnIterator(value - w * rt, w);
        }
        friend ColumnIterator operator +(ptrdiff_t a, ColumnIterator b)
        {
            return ColumnIterator(b.value + b.w * a, b.w);
        }
        ptrdiff_t operator -(const ColumnIterator &rt) const
        {
            return (value - rt.value) / w;
        }
        const ColumnIterator &operator +=(ptrdiff_t rt)
        {
            value += w * rt;
            return *this;
        }
        const ColumnIterator &operator -=(ptrdiff_t rt)
        {
            value -= w * rt;
            return *this;
        }
    };
    struct ColumnConstIterator : public std::iterator<std::random_access_iterator_tag, const ColorI>
    {
        const ColorI *value;
        size_t w;
    public:
        ColumnConstIterator()
            : value(nullptr), w(0)
        {
        }
        ColumnConstIterator(const ColorI *value, size_t w)
            : value(value), w(w)
        {
        }
        ColumnConstIterator(const ColumnIterator &rt)
            : value(rt.value), w(rt.w)
        {
        }
        bool operator ==(const ColumnConstIterator &rt) const
        {
            return value == rt.value;
        }
        bool operator !=(const ColumnConstIterator &rt) const
        {
            return value != rt.value;
        }
        bool operator >=(const ColumnConstIterator &rt) const
        {
            return value >= rt.value;
        }
        bool operator <=(const ColumnConstIterator &rt) const
        {
            return value <= rt.value;
        }
        bool operator >(const ColumnConstIterator &rt) const
        {
            return value > rt.value;
        }
        bool operator <(const ColumnConstIterator &rt) const
        {
            return value < rt.value;
        }
        const ColorI &operator *() const
        {
            return *value;
        }
        const ColorI &operator [](ptrdiff_t index) const
        {
            return value[(ptrdiff_t)w * index];
        }
        const ColorI *operator ->() const
        {
            return value;
        }
        const ColumnConstIterator &operator ++()
        {
            value += w;
            return *this;
        }
        const ColumnConstIterator &operator --()
        {
            value -= w;
            return *this;
        }
        ColumnConstIterator operator ++(int)
        {
            ColumnConstIterator retval = *this;
            value += w;
            return retval;
        }
        ColumnConstIterator operator --(int)
        {
            ColumnConstIterator retval = *this;
            value -= w;
            return retval;
        }
        ColumnConstIterator operator +(ptrdiff_t rt) const
        {
            return ColumnConstIterator(value + w * rt, w);
        }
        ColumnConstIterator operator -(ptrdiff_t rt) const
        {
            return ColumnConstIterator(value - w * rt, w);
        }
        friend ColumnConstIterator operator +(ptrdiff_t a, ColumnConstIterator b)
        {
            return ColumnConstIterator(b.value + b.w * a, b.w);
        }
        ptrdiff_t operator -(const ColumnConstIterator &rt) const
        {
            return (value - rt.value) / w;
        }
        const ColumnConstIterator &operator +=(ptrdiff_t rt)
        {
            value += w * rt;
            return *this;
        }
        const ColumnConstIterator &operator -=(ptrdiff_t rt)
        {
            value -= w * rt;
            return *this;
        }
    };
    struct VLineRange
    {
        ColorI *beginPtr;
        ColorI *endPtr;
        size_t w;
        VLineRange(ColorI *beginPtr, ColorI *endPtr, size_t w)
            : beginPtr(beginPtr), endPtr(endPtr), w(w)
        {
        }
        ColumnIterator begin() const
        {
            return ColumnIterator(beginPtr, w);
        }
        ColumnIterator end() const
        {
            return ColumnIterator(endPtr, w);
        }
    };
    struct VLineRangeConst
    {
        const ColorI *beginPtr;
        const ColorI *endPtr;
        size_t w;
        VLineRangeConst(const ColorI *beginPtr, const ColorI *endPtr, size_t w)
            : beginPtr(beginPtr), endPtr(endPtr), w(w)
        {
        }
        ColumnConstIterator begin() const
        {
            return ColumnConstIterator(beginPtr, w);
        }
        ColumnConstIterator end() const
        {
            return ColumnConstIterator(endPtr, w);
        }
    };
    VLineRange getVerticalLine(int x, int startY, int endY)
    {
        assert(startY <= endY);
        assert(startY >= 0 && (size_t)endY < h);
        assert(x >= 0 && (size_t)x < w);
        return VLineRange(&pixels[x + startY * w], &pixels[x + endY * w], w);
    }
    VLineRange getVerticalLine(int x)
    {
        return getVerticalLine(x, 0, h);
    }
    VLineRangeConst getVerticalLine(int x, int startY, int endY) const
    {
        assert(startY <= endY);
        assert(startY >= 0 && (size_t)endY < h);
        assert(x >= 0 && (size_t)x < w);
        return VLineRangeConst(&pixels[x + startY * w], &pixels[x + endY * w], w);
    }
    VLineRangeConst getVerticalLine(int x) const
    {
        return getVerticalLine(x, 0, h);
    }
    static shared_ptr<const Image> loadImage(string fName);
};

struct Texture
{
    Texture(const Texture &rt) = delete;
    const Texture &operator =(const Texture &rt) = delete;
    Texture()
    {
    }
    virtual ~Texture()
    {
    }
    virtual shared_ptr<const Image> getImage() = 0;
};

class ImageTexture : public Texture
{
    shared_ptr<const Image> image;
public:
    ImageTexture(shared_ptr<const Image> image)
        : image(image)
    {
        assert(image != nullptr);
    }
    virtual shared_ptr<const Image> getImage() override
    {
        return image;
    }
};

struct TextureDescriptor
{
    shared_ptr<Texture> image;
    float minU, minV, maxU, maxV;
    TextureDescriptor(shared_ptr<Texture> image = nullptr)
        : image(image), minU(0), minV(0), maxU(1), maxV(1)
    {
    }
    TextureDescriptor(shared_ptr<Texture> image, float minU, float maxU, float minV, float maxV)
        : image(image), minU(minU), minV(minV), maxU(maxU), maxV(maxV)
    {
    }
    operator bool() const
    {
        return image != nullptr;
    }
    bool operator !() const
    {
        return image == nullptr;
    }
};

shared_ptr<Texture> loadTextFontTexture();
TextureDescriptor getTextFontCharacterTextureDescriptor(unsigned ch, shared_ptr<Texture> texture);

#endif // IMAGE_H_INCLUDED

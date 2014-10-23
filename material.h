#ifndef MATERIAL_H_INCLUDED
#define MATERIAL_H_INCLUDED

#include "image.h"
#include "vector.h"
#include <vector>

using namespace std;

struct Light
{
    VectorF direction;
    ColorF color;
    Light(VectorF direction = VectorF(0), ColorF color = GrayscaleF(1))
        : direction(normalizeNoThrow(direction)), color(color)
    {
    }
    ColorF evalLight(VectorF vertexNormal, VectorF /*vertexPosition*/) const
    {
        float v = dot(direction, vertexNormal);
        if(v < 0)
            v = 0;
        return scaleF(v, color);
    }
};

//template <typename ...Args>
//struct LightListLiteral;

struct Material
{
    ColorF ambient;
    ColorF diffuse;
    float opacity;
    shared_ptr<Texture> texture;
    ColorF evalGlobal(VectorF /*vertexNormal*/, VectorF /*vertexPosition*/) const
    {
        return ambient;
    }
    ColorF evalCombine(ColorF retval, ColorF lightVal) const
    {
        return add(retval, lightVal);
    }
    ColorF evalFinalize(ColorF retval, ColorF vertexColor) const
    {
        retval = colorize(vertexColor, setAlphaF(retval, opacity));
        retval.r = min(1.0f, retval.r);
        retval.g = min(1.0f, retval.g);
        retval.b = min(1.0f, retval.b);
        return retval;
    }
    Material(ColorF ambient, ColorF diffuse, float opacity, shared_ptr<Texture> texture = nullptr)
        : ambient(ambient), diffuse(diffuse), opacity(opacity), texture(texture)
    {
    }
    Material(ColorF ambient, ColorF diffuse, shared_ptr<Texture> texture = nullptr)
        : ambient(ambient), diffuse(diffuse), opacity(diffuse.a), texture(texture)
    {
    }
//    template <typename ...Args>
//    ColorF eval(const LightListLiteral<Args...> &lights, ColorF vertexColor, VectorF vertexNormal, VectorF vertexPosition) const
//    {
//        return evalFinalize(lights.eval(*this, vertexNormal, vertexPosition), vertexColor);
//    }
    ColorF eval(const vector<Light> &lights, ColorF vertexColor, VectorF vertexNormal, VectorF vertexPosition) const
    {
        ColorF retval = evalGlobal(vertexNormal, vertexPosition);
        for(const Light &light : lights)
        {
            retval = evalCombine(retval, light.evalLight(vertexNormal, vertexPosition));
        }
        return evalFinalize(retval, vertexColor);
    }
    explicit Material(ColorF c, shared_ptr<Texture> texture = nullptr)
        : ambient(scaleF(0.35, c)), diffuse(scaleF(0.65, c)), opacity(c.a), texture(texture)
    {
    }
    explicit Material(shared_ptr<Texture> texture = nullptr)
        : Material(GrayscaleF(1), texture)
    {
    }
    bool operator ==(const Material &rt) const
    {
        return ambient == rt.ambient && diffuse == rt.diffuse && opacity == rt.opacity && texture == rt.texture;
    }
    bool operator !=(const Material &rt) const
    {
        return !operator ==(rt);
    }
};
#if 0
template <>
struct LightListLiteral<>
{
    constexpr LightListLiteral()
    {
    }
    ColorF eval(const Material &material, VectorF vertexNormal, VectorF vertexPosition) const
    {
        return material.evalGlobal(vertexNormal, vertexPosition);
    }
};

template <typename ...Args>
struct LightListLiteral<Light, Args...>
{
    Light first;
    LightListLiteral<Args...> rest;
    constexpr LightListLiteral(Light first, Args ...rest)
        : first(first), rest(rest...)
    {
    }
    ColorF eval(const Material &material, VectorF vertexNormal, VectorF vertexPosition) const
    {
        return material.evalCombine(rest.eval(material, vertexNormal, vertexPosition), first.evalLight(vertexNormal, vertexPosition));
    }
};
#endif
template <typename ...Args>
inline vector<Light> make_light_list(Args ...args)
{
    return vector<Light>{args...};
}

template <typename T>
struct LitMaterial
{
    const Material material;
    const T lights;
    constexpr LitMaterial(const Material &material, const T &lights)
        : material(material), lights(lights)
    {
    }
    ColorF operator ()(ColorF vertexColor, VectorF vertexNormal, VectorF vertexPosition) const
    {
        return material.eval(lights, vertexColor, vertexNormal, vertexPosition);
    }
};

template <typename ...Args>
inline LitMaterial<vector<Light>> light_material(const Material &material, Args ...args)
{
    return LitMaterial<vector<Light>>(material, make_light_list(args...));
}

inline LitMaterial<vector<Light>> light_material(const Material &material, const vector<Light> &lights)
{
    return LitMaterial<vector<Light>>(material, lights);
}

#endif // MATERIAL_H_INCLUDED

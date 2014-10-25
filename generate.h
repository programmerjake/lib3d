/*
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#ifndef GENERATE_H_INCLUDED
#define GENERATE_H_INCLUDED

#include "mesh.h"
#include "image.h"
#include <utility>
#include <functional>
#include <tuple>
#include <vector>
#include <cwchar>
#include <string>
#include <cmath>

using namespace std;

inline Mesh reverse(const Mesh & meshIn)
{
    vector<Triangle> triangles = meshIn.triangles;
    for(Triangle & tri : triangles)
    {
        tri = reverse(tri);
    }
    return Mesh(std::move(triangles), meshIn.image);
}

inline Mesh reverse(Mesh && meshIn)
{
    vector<Triangle> triangles = std::move(meshIn.triangles);
    for(Triangle & tri : triangles)
    {
        tri = reverse(tri);
    }
    return Mesh(std::move(triangles), std::move(meshIn.image));
}

template <typename Fn>
inline Mesh shadeMesh(const Mesh &meshIn, Fn shadeFn)
{
    vector<Triangle> triangles;
    triangles.reserve(meshIn.triangles.size());

    for(Triangle tri : meshIn.triangles)
    {
        if(tri.n1 == VectorF(0) || tri.n2 == VectorF(0) || tri.n3 == VectorF(0))
        {
            triangles.push_back(tri);
            continue;
        }

        tri.c1 = shadeFn(tri.c1, tri.n1, tri.p1);
        tri.c2 = shadeFn(tri.c2, tri.n2, tri.p2);
        tri.c3 = shadeFn(tri.c3, tri.n3, tri.p3);
        triangles.push_back(tri);
    }

    return Mesh(std::move(triangles), meshIn.image);
}

template <typename Fn>
inline Mesh shadeMesh(Mesh &&meshIn, Fn shadeFn)
{
    vector<Triangle> triangles = std::move(meshIn.triangles);

    for(Triangle &tri : triangles)
    {
        if(tri.n1 == VectorF(0) || tri.n2 == VectorF(0) || tri.n3 == VectorF(0))
        {
            continue;
        }

        tri.c1 = shadeFn(tri.c1, tri.n1, tri.p1);
        tri.c2 = shadeFn(tri.c2, tri.n2, tri.p2);
        tri.c3 = shadeFn(tri.c3, tri.n3, tri.p3);
    }

    return Mesh(std::move(triangles), std::move(meshIn.image));
}

namespace Generate
{
	inline Mesh quadrilateral(TextureDescriptor texture, VectorF p1, ColorF c1, VectorF p2, ColorF c2, VectorF p3, ColorF c3, VectorF p4, ColorF c4)
	{
		const TextureCoord t1 = TextureCoord(texture.minU, texture.minV);
		const TextureCoord t2 = TextureCoord(texture.maxU, texture.minV);
		const TextureCoord t3 = TextureCoord(texture.maxU, texture.maxV);
		const TextureCoord t4 = TextureCoord(texture.minU, texture.maxV);
		return Mesh(vector<Triangle>{Triangle(p1, c1, t1, p2, c2, t2, p3, c3, t3), Triangle(p3, c3, t3, p4, c4, t4, p1, c1, t1)}, texture.image);
	}

	inline Mesh convexPolygon(shared_ptr<Texture> texture, vector<tuple<VectorF, ColorF, TextureCoord, VectorF>> vertices)
	{
	    if(vertices.size() < 3)
            return Mesh();
        vector<Triangle> triangles;
        triangles.reserve(vertices.size() - 2);
        tuple<VectorF, ColorF, TextureCoord, VectorF> v1 = vertices[0];
        for(size_t i = 1, j = 2; j < vertices.size(); i++, j++)
        {
            tuple<VectorF, ColorF, TextureCoord, VectorF> v2 = vertices[i], v3 = vertices[j];
            triangles.push_back(Triangle(get<0>(v1), get<1>(v1), get<2>(v1), get<3>(v1),
                                          get<0>(v2), get<1>(v2), get<2>(v2), get<3>(v2),
                                          get<0>(v3), get<1>(v3), get<2>(v3), get<3>(v3)));
        }
        return Mesh(std::move(triangles), texture);
	}

	inline Mesh convexPolygon(shared_ptr<Texture> texture, vector<tuple<VectorF, ColorF, TextureCoord>> vertices)
	{
	    if(vertices.size() < 3)
            return Mesh();
        vector<Triangle> triangles;
        triangles.reserve(vertices.size() - 2);
        tuple<VectorF, ColorF, TextureCoord> v1 = vertices[0];
        for(size_t i = 1, j = 2; j < vertices.size(); i++, j++)
        {
            tuple<VectorF, ColorF, TextureCoord> v2 = vertices[i], v3 = vertices[j];
            triangles.push_back(Triangle(get<0>(v1), get<1>(v1), get<2>(v1),
                                          get<0>(v2), get<1>(v2), get<2>(v2),
                                          get<0>(v3), get<1>(v3), get<2>(v3)));
        }
        return Mesh(std::move(triangles), texture);
	}

	inline Mesh convexPolygon(shared_ptr<Texture> texture, vector<tuple<VectorF, TextureCoord>> vertices)
	{
	    if(vertices.size() < 3)
            return Mesh();
        vector<Triangle> triangles;
        triangles.reserve(vertices.size() - 2);
        tuple<VectorF, TextureCoord> v1 = vertices[0];
        for(size_t i = 1, j = 2; j < vertices.size(); i++, j++)
        {
            tuple<VectorF, TextureCoord> v2 = vertices[i], v3 = vertices[j];
            triangles.push_back(Triangle(get<0>(v1), get<1>(v1),
                                          get<0>(v2), get<1>(v2),
                                          get<0>(v3), get<1>(v3)));
        }
        return Mesh(std::move(triangles), texture);
	}

	/// make a box from <0, 0, 0> to <1, 1, 1>
	inline Mesh unitBox(TextureDescriptor nx, TextureDescriptor px, TextureDescriptor ny, TextureDescriptor py, TextureDescriptor nz, TextureDescriptor pz)
	{
		const VectorF p0 = VectorF(0, 0, 0);
		const VectorF p1 = VectorF(1, 0, 0);
		const VectorF p2 = VectorF(0, 1, 0);
		const VectorF p3 = VectorF(1, 1, 0);
		const VectorF p4 = VectorF(0, 0, 1);
		const VectorF p5 = VectorF(1, 0, 1);
		const VectorF p6 = VectorF(0, 1, 1);
		const VectorF p7 = VectorF(1, 1, 1);
		Mesh retval;
		const ColorF c = RGBAF(1, 1, 1, 1);
		if(nx)
		{
			retval.append(quadrilateral(nx,
									 p0, c,
									 p4, c,
									 p6, c,
									 p2, c
									 ));
		}
		if(px)
		{
			retval.append(quadrilateral(px,
									 p5, c,
									 p1, c,
									 p3, c,
									 p7, c
									 ));
		}
		if(ny)
		{
			retval.append(quadrilateral(ny,
									 p0, c,
									 p1, c,
									 p5, c,
									 p4, c
									 ));
		}
		if(py)
		{
			retval.append(quadrilateral(py,
									 p6, c,
									 p7, c,
									 p3, c,
									 p2, c
									 ));
		}
		if(nz)
		{
			retval.append(quadrilateral(nz,
									 p1, c,
									 p0, c,
									 p2, c,
									 p3, c
									 ));
		}
		if(pz)
		{
			retval.append(quadrilateral(pz,
									 p4, c,
									 p5, c,
									 p7, c,
									 p6, c
									 ));
		}
		return std::move(retval);
	}

    struct TextProperties final
    {
        float tabWidth = 8;
    };

	class Text final
	{
	    Text() = delete;
	    Text(const Text &) = delete;
	    void operator =(const Text &) = delete;
        static void renderChar(Mesh &dest, float x, float y, unsigned ch, ColorF c, const std::shared_ptr<Texture> &fontTexture)
        {
            TextureDescriptor td = getTextFontCharacterTextureDescriptor(ch, fontTexture);
            dest.append(quadrilateral(td,
                                      VectorF(0 + x, 0 + y, 0), c,
                                      VectorF(1 + x, 0 + y, 0), c,
                                      VectorF(1 + x, 1 + y, 0), c,
                                      VectorF(0 + x, 1 + y, 0), c
                                      ));
        }
        static void renderEngine(Mesh *dest, float &x, float &y, float &w, float &h, std::wstring str, ColorF c, const TextProperties &tp, const std::shared_ptr<Texture> &fontTexture = nullptr)
        {
            float wholeHeight = 0;
            if(dest)
                renderEngine(nullptr, x, y, w, wholeHeight, str, c, tp);
            x = 0;
            y = 0;
            w = 0;
            h = 0;
            for(auto ch : str)
            {
                switch(ch)
                {
                case '\r':
                    x = 0;
                    break;
                case '\n':
                    x = 0;
                    y += 1;
                    break;
                case '\t':
                {
                    float tabWidth = tp.tabWidth;
                    if(!std::isfinite(tabWidth) || tabWidth < 1e-4)
                        tabWidth = 1;
                    if(tabWidth > 1e6)
                        tabWidth = 1e6;
                    x /= tabWidth;
                    x += 1e-4;
                    x = std::floor(x);
                    x += 1;
                    x *= tabWidth;
                    break;
                }
                case '\b':
                    x -= 1;
                    if(x < 0)
                        x = 0;
                    break;
                case '\0':
                case ' ':
                    x += 1;
                    break;
                default:
                    if(dest)
                        renderChar(*dest, x, wholeHeight - y - 1, ch, c, fontTexture);
                    x += 1;
                    break;
                }
                if(y >= h)
                    h = y + 1;
                if(x > w)
                    w = x;
            }
            y = h - y - 1;
        }
	public:
        static float width(std::wstring str, const TextProperties &tp = TextProperties())
        {
            float x, y, w, h;
            renderEngine(nullptr, x, y, w, h, str, ColorF(), tp);
            return w;
        }
        static float height(std::wstring str, const TextProperties &tp = TextProperties())
        {
            float x, y, w, h;
            renderEngine(nullptr, x, y, w, h, str, ColorF(), tp);
            return h;
        }
        static float x(std::wstring str, const TextProperties &tp = TextProperties())
        {
            float x, y, w, h;
            renderEngine(nullptr, x, y, w, h, str, ColorF(), tp);
            return x;
        }
        static float y(std::wstring str, const TextProperties &tp = TextProperties())
        {
            float x, y, w, h;
            renderEngine(nullptr, x, y, w, h, str, ColorF(), tp);
            return y;
        }
        static Mesh &mesh(Mesh &dest, std::wstring str, std::shared_ptr<Texture> fontTexture = nullptr, ColorF c = GrayscaleF(1), const TextProperties &tp = TextProperties())
        {
            float x, y, w, h;
            renderEngine(&dest, x, y, w, h, str, c, tp, fontTexture);
            return dest;
        }
        static Mesh mesh(std::wstring str, std::shared_ptr<Texture> fontTexture = nullptr, ColorF c = GrayscaleF(1), const TextProperties &tp = TextProperties())
        {
            Mesh dest;
            mesh(dest, str, fontTexture, c, tp);
            return std::move(dest);
        }
	};
}

#endif // GENERATE_H_INCLUDED

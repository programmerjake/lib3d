#include "model.h"
#include <stdexcept>
#include <cassert>
#include <fstream>
#include <unordered_map>
#include <climits>
#include <cstdlib>
#include <sstream>

using namespace std;

namespace
{
class OBJModelLoader : public ModelLoader
{
    ifstream is;
    static string readLine(istream &is)
    {
        if(!is)
            return "";
        while(is.peek() != EOF)
        {
            if(isspace(is.peek()))
            {
                is.get();
            }
            else if(is.peek() == '#') // comment
            {
                do
                {
                    is.get();
                }
                while(is.peek() != '\r' && is.peek() != '\n' && is.peek() != EOF);
            }
            else
            {
                break;
            }
        }
        if(is.peek() == EOF)
            return "";
        string retval = "";
        do
        {
            retval += (char)is.get();
        }
        while(is.peek() != '\r' && is.peek() != '\n' && is.peek() != EOF);
        return retval;
    }
    static vector<string> readTokenizedLine(istream &is)
    {
        string line = readLine(is);
        vector<string> retval;
        bool isStartOfToken = true;
        for(char ch : line)
        {
            if(isblank(ch))
            {
                isStartOfToken = true;
            }
            else
            {
                if(isStartOfToken)
                    retval.push_back("");
                isStartOfToken = false;
                retval.back() += ch;
            }
        }
        return std::move(retval);
    }
    static string canonicalizePath(string path)
    {
        char * str = realpath(path.c_str(), nullptr);
        if(str == nullptr)
            throw ModelLoadException("invalid path '" + path + "'");
        string retval = str;
        free(str);
        return retval;
    }
    static bool isAbsolutePath(string path)
    {
        return path.substr(0, 1) == "/";
    }
    static string getNeighborFileName(string originalFile, string neighborFile)
    {
        if(neighborFile == "")
            return "";
        if(originalFile == "")
            return "";
        if(isAbsolutePath(neighborFile))
            return canonicalizePath(neighborFile);
        string file = canonicalizePath(originalFile);
        file.erase(file.find_last_of('/'));
        file += "/" + neighborFile;
        return canonicalizePath(file);
    }
    static float parseFloat(string str)
    {
        istringstream is(str);
        float retval;
        if(!(is >> retval) || !isfinite(retval))
            throw ModelLoadException("invalid floating-point number '" + str + "'");
        return retval;
    }
    static int parseInt(string str)
    {
        istringstream is(str);
        int retval;
        if(!(is >> retval))
            throw ModelLoadException("invalid integer '" + str + "'");
        return retval;
    }
    static size_t parseIndex(string str, size_t size)
    {
        istringstream is(str);
        ptrdiff_t retval;
        if(!(is >> retval) || ((retval < 1 || retval > (ptrdiff_t)size) && (retval < -(ptrdiff_t)size || retval > -1)))
            throw ModelLoadException("invalid index '" + str + "'");
        if(retval < 0)
            return retval + size;
        return retval - 1;
    }
    tuple<VectorF, TextureCoord, bool, VectorF, bool> parseVertex(string str)
    {
        VectorF point, normal = VectorF(0);
        TextureCoord textureCoord = TextureCoord(0, 0);
        bool hasTextureCoord = false, hasNormal = false;
        size_t firstSlashIndex = str.find_first_of('/');
        if(firstSlashIndex == string::npos)
        {
            point = points[parseIndex(str, points.size())];
        }
        else
        {
            point = points[parseIndex(str.substr(0, firstSlashIndex), points.size())];
            string rest = str.substr(firstSlashIndex + 1);
            size_t secondSlashIndex = rest.find_first_of('/');
            if(secondSlashIndex == string::npos)
            {
                hasTextureCoord = true;
                textureCoord = textureCoords[parseIndex(rest, textureCoords.size())];
            }
            else
            {
                hasNormal = true;
                normal = normals[parseIndex(rest.substr(secondSlashIndex + 1), normals.size())];
                if(secondSlashIndex > 0)
                {
                    hasTextureCoord = true;
                    textureCoord = textureCoords[parseIndex(rest.substr(0, secondSlashIndex), textureCoords.size())];
                }
            }
        }
        return tuple<VectorF, TextureCoord, bool, VectorF, bool>(point, textureCoord, hasTextureCoord, normal, hasNormal);
    }
    ColorF validateColor(ColorF color, function<void(string)> warningFunction)
    {
        const float eps = 1e-4;
        if(color.r < -eps || color.r > 1 + eps || color.g < -eps || color.g > 1 + eps || color.b < -eps || color.b > 1 + eps || color.a < -eps || color.a > 1 + eps)
        {
            if(warningFunction)
                warningFunction("color out of range : limiting");
        }
        return RGBAF(limit<float>(color.r, 0, 1), limit<float>(color.g, 0, 1), limit<float>(color.b, 0, 1), limit<float>(color.a, 0, 1));
    }
    ColorF parseColorSpecifier(const vector<string> &line, function<void(string)> warningFunction)
    {
        if(line.size() < 2)
            throw ModelLoadException("too few arguments for " + line[0]);
        if(line[1] == "xyz")
        {
            if(line.size() < 3)
                throw ModelLoadException("too few arguments for " + line[0] + " xyz");
            if(line.size() != 3 && line.size() != 5)
                throw ModelLoadException("invalid argument count for " + line[0] + " xyz");
            if(line.size() > 5)
                throw ModelLoadException("too many arguments for " + line[0]);
            float x = parseFloat(line[2]);
            if(line.size() == 3)
            {
                return validateColor(XYZF(x, x, x), warningFunction);
            }
            float y = parseFloat(line[3]);
            float z = parseFloat(line[4]);
            return validateColor(XYZF(x, y, z), warningFunction);
        }
        if(line.size() > 4)
            throw ModelLoadException("too many arguments for " + line[0]);
        if(line.size() != 2 && line.size() != 4)
            throw ModelLoadException("invalid argument count for " + line[0]);
        if(line.size() == 2)
        {
            return validateColor(GrayscaleF(parseFloat(line[1])), warningFunction);
        }
        assert(line.size() == 4);
        return validateColor(RGBF(parseFloat(line[1]), parseFloat(line[2]), parseFloat(line[3])), warningFunction);
    }
    shared_ptr<Texture> loadImage(string fileName)
    {
        shared_ptr<Texture> &retval = images[fileName];
        if(retval == nullptr)
        {
            try
            {
                retval = make_shared<ImageTexture>(Image::loadImage(fileName));
            }
            catch(exception &e)
            {
                throw ModelLoadException((string)"can not load image : " + e.what());
            }
        }
        return retval;
    }
    void parseMaterialLibrary(string fileName, function<void(string)> warningFunction)
    {
        ifstream is(fileName.c_str());
        if(!is)
            throw ModelLoadException("can't open '" + fileName + "'");
        shared_ptr<Material> currentMaterial = nullptr;
        string currentMaterialName = "";
        bool hasAmbient = false, hasDiffuse = false, hasOpacity = false;
        for(vector<string> line = readTokenizedLine(is); !line.empty(); line = readTokenizedLine(is))
        {
            if(line[0] == "Ks" ||
               line[0] == "Tf" ||
               line[0] == "Ns" ||
               line[0] == "sharpness" ||
               line[0] == "Ni" ||
               line[0] == "map_Ka" ||
               line[0] == "map_Ks" ||
               line[0] == "map_Ns" ||
               line[0] == "map_d" ||
               line[0] == "disp" ||
               line[0] == "decal" ||
               line[0] == "bump" ||
               line[0] == "refl")
            {
                if(warningFunction)
                    warningFunction(line[0] + " not implemented");
            }
            else if(line[0] == "newmtl")
            {
                if(line.size() < 2)
                    throw ModelLoadException("too few arguments for newmtl");
                if(line.size() > 2)
                    throw ModelLoadException("too many arguments for newmtl");
                currentMaterialName = line[1];
                shared_ptr<Material> &m = materials[line[1]];
                if(m != nullptr && warningFunction)
                    warningFunction("material already exists : '" + line[1] + "'");
                m = make_shared<Material>();
                currentMaterial = m;
                hasAmbient = false;
                hasDiffuse = false;
                hasOpacity = false;
            }
            else if(line[0] == "Ka")
            {
                if(currentMaterial == nullptr)
                    throw ModelLoadException("Ka before newmtl");
                if(hasAmbient && warningFunction)
                    warningFunction("Ka for material already specified");
                hasAmbient = true;
                currentMaterial->ambient = parseColorSpecifier(line, warningFunction);
            }
            else if(line[0] == "Kd")
            {
                if(currentMaterial == nullptr)
                    throw ModelLoadException("Kd before newmtl");
                if(hasDiffuse && warningFunction)
                    warningFunction("Kd for material already specified");
                hasDiffuse = true;
                currentMaterial->diffuse = parseColorSpecifier(line, warningFunction);
            }
            else if(line[0] == "d")
            {
                if(currentMaterial == nullptr)
                    throw ModelLoadException("d before newmtl");
                if(hasOpacity && warningFunction)
                    warningFunction("d for material already specified");
                hasOpacity = true;
                currentMaterial->opacity = getLuminanceValueF(parseColorSpecifier(line, warningFunction));
                if(currentMaterial->opacity == 0 && warningFunction)
                    warningFunction("opacity equals 0 for " + currentMaterialName);
            }
            else if(line[0] == "map_Kd")
            {
                if(currentMaterial == nullptr)
                    throw ModelLoadException("map_Kd before newmtl");
                if(currentMaterial->texture != nullptr && warningFunction)
                    warningFunction("map_Kd for material already specified");
                if(line.size() < 2)
                    throw ModelLoadException("too few arguments for map_Kd");
                if(line.size() > 2)
                    throw ModelLoadException("too many arguments for map_Kd");
                currentMaterial->texture = loadImage(getNeighborFileName(fileName, line[1]));
            }
            else if(line[0] == "illum")
            {
                if(currentMaterial == nullptr)
                    throw ModelLoadException("illum before newmtl");
                if(line.size() < 2)
                    throw ModelLoadException("too few arguments for illum");
                if(line.size() > 2)
                    throw ModelLoadException("too many arguments for illum");
                int model = parseInt(line[1]);
                switch(model)
                {
                case 0:
                    currentMaterial->ambient = currentMaterial->diffuse;
                    currentMaterial->diffuse = GrayscaleF(0);
                    break;
                case 1:
                    break;
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                    if(warningFunction)
                    {
                        ostringstream os;
                        os << "illum " << model << " not supported : treated as illum 1";
                        warningFunction(os.str());
                    }
                    break;
                default:
                    throw ModelLoadException("illum model number out of range");
                }
            }
            else
            {
                throw ModelLoadException("unknown command : " + line[0]);
            }
        }
    }
    vector<VectorF> points;
    vector<TextureCoord> textureCoords;
    vector<VectorF> normals;
    unordered_map<string, shared_ptr<Material>> materials;
    shared_ptr<Material> currentMaterial;
    unordered_map<string, shared_ptr<Model>> models;
    vector<string> modelList;
    size_t currentReturnedModel = 0;
    unordered_map<string, shared_ptr<Texture>> images;
public:
    OBJModelLoader(string fileName, function<void(string)> warningFunction)
        : is(fileName.c_str())
    {
        if(!is)
            throw ModelLoadException("can't open '" + fileName + "'");
        currentMaterial = make_shared<Material>();
        materials[""] = currentMaterial;

        shared_ptr<Model> currentModel = nullptr;
        string currentModelName = "";
        for(vector<string> line = readTokenizedLine(is); !line.empty(); line = readTokenizedLine(is))
        {
            if(line[0] == "call" ||
               line[0] == "csh" ||
               line[0] == "vp" ||
               line[0] == "cstype" ||
               line[0] == "deg" ||
               line[0] == "bmat" ||
               line[0] == "step" ||
               line[0] == "p" ||
               line[0] == "l" ||
               line[0] == "curv" ||
               line[0] == "curv2" ||
               line[0] == "surf" ||
               line[0] == "parm" ||
               line[0] == "trim" ||
               line[0] == "hole" ||
               line[0] == "scrv" ||
               line[0] == "sp" ||
               line[0] == "end" ||
               line[0] == "con" ||
               line[0] == "g" ||
               line[0] == "mg" ||
               line[0] == "s" ||
               line[0] == "bevel" ||
               line[0] == "c_interp" ||
               line[0] == "d_interp" ||
               line[0] == "lod" ||
               line[0] == "maplib" ||
               line[0] == "usemap" ||
               line[0] == "shadow_obj" ||
               line[0] == "trace_obj" ||
               line[0] == "ctech" ||
               line[0] == "stech")
            {
                if(warningFunction)
                    warningFunction(line[0] + " not implemented");
            }
            else if(line[0] == "v")
            {
                VectorF v;
                if(line.size() < 4)
                    throw ModelLoadException("too few arguments for v");
                if(line.size() > 5)
                    throw ModelLoadException("too many arguments for v");
                v.x = parseFloat(line[1]);
                v.y = parseFloat(line[2]);
                v.z = parseFloat(line[3]);
                if(line.size() > 4)
                    parseFloat(line[4]);
                points.push_back(v);
            }
            else if(line[0] == "vn")
            {
                VectorF v;
                if(line.size() < 4)
                    throw ModelLoadException("too few arguments for vn");
                if(line.size() > 4)
                    throw ModelLoadException("too many arguments for vn");
                v.x = parseFloat(line[1]);
                v.y = parseFloat(line[2]);
                v.z = parseFloat(line[3]);
                try
                {
                    v = normalize(v);
                }
                catch(exception &)
                {
                    throw ModelLoadException("normal zero for vn");
                }
                normals.push_back(v);
            }
            else if(line[0] == "vt")
            {
                TextureCoord v = TextureCoord(0, 0);
                if(line.size() < 2)
                    throw ModelLoadException("too few arguments for vt");
                if(line.size() > 4)
                    throw ModelLoadException("too many arguments for vt");
                v.u = parseFloat(line[1]);
                if(line.size() > 2)
                    v.v = parseFloat(line[2]);
                if(line.size() > 3)
                    parseFloat(line[3]);
                textureCoords.push_back(v);
            }
            else if(line[0] == "f")
            {
                if(line.size() < 4)
                    throw ModelLoadException("too few arguments for f");
                auto vertex = parseVertex(line[1]);
                bool hasTextureCoord = get<2>(vertex), hasNormal = get<4>(vertex);
                Mesh faceMesh;
                shared_ptr<Texture> texture = currentMaterial->texture;
                if(hasNormal)
                {
                    vector<tuple<VectorF, ColorF, TextureCoord, VectorF>> vertices;
                    vertices.reserve(line.size() - 1);
                    vertices.push_back(tuple<VectorF, ColorF, TextureCoord, VectorF>(get<0>(vertex), GrayscaleF(1), get<1>(vertex), get<3>(vertex)));
                    for(size_t i = 2; i < line.size(); i++)
                    {
                        vertex = parseVertex(line[i]);
                        if(hasTextureCoord != get<2>(vertex) || hasNormal != get<4>(vertex))
                            throw ModelLoadException("vertex parts do not match");
                        vertices.push_back(tuple<VectorF, ColorF, TextureCoord, VectorF>(get<0>(vertex), GrayscaleF(1), get<1>(vertex), get<3>(vertex)));
                    }
                    faceMesh = Generate::convexPolygon(texture, vertices);
                }
                else
                {
                    vector<tuple<VectorF, ColorF, TextureCoord>> vertices;
                    vertices.reserve(line.size() - 1);
                    vertices.push_back(tuple<VectorF, ColorF, TextureCoord>(get<0>(vertex), GrayscaleF(1), get<1>(vertex)));
                    for(size_t i = 2; i < line.size(); i++)
                    {
                        vertex = parseVertex(line[i]);
                        if(hasTextureCoord != get<2>(vertex) || hasNormal != get<4>(vertex))
                            throw ModelLoadException("vertex parts do not match");
                        vertices.push_back(tuple<VectorF, ColorF, TextureCoord>(get<0>(vertex), GrayscaleF(1), get<1>(vertex)));
                    }
                    faceMesh = Generate::convexPolygon(texture, vertices);
                }
                if(currentModel == nullptr)
                {
                    shared_ptr<Model> &m = models[currentModelName];
                    if(m == nullptr)
                    {
                        m = make_shared<Model>();
                        modelList.push_back(currentModelName);
                    }
                    currentModel = m;
                }
                //faceMesh = reverse(faceMesh);
                if(currentModel->meshes.empty() || get<0>(currentModel->meshes.back()) != *currentMaterial)
                {
                    currentModel->meshes.push_back(make_pair(*currentMaterial, faceMesh));
                }
                else
                {
                    get<1>(currentModel->meshes.back()).append(faceMesh);
                }
            }
            else if(line[0] == "o")
            {
                if(line.size() < 2)
                    throw ModelLoadException("too few arguments for o");
                if(line.size() > 2)
                    throw ModelLoadException("too many arguments for o");
                currentModelName = line[1];
                shared_ptr<Model> &m = models[currentModelName];
                if(m == nullptr)
                {
                    m = make_shared<Model>();
                    modelList.push_back(currentModelName);
                }
                currentModel = m;
            }
            else if(line[0] == "usemtl")
            {
                if(line.size() < 2)
                    throw ModelLoadException("too few arguments for usemtl");
                if(line.size() > 2)
                    throw ModelLoadException("too many arguments for usemtl");
                auto iter = materials.find(line[1]);
                if(iter == materials.end())
                    throw ModelLoadException("unknown material '" + line[1] + "'");
                currentMaterial = get<1>(*iter);
            }
            else if(line[0] == "mtllib")
            {
                if(line.size() < 2)
                    throw ModelLoadException("too few arguments for mtllib");
                if(line.size() > 2)
                    throw ModelLoadException("too many arguments for mtllib");
                parseMaterialLibrary(getNeighborFileName(fileName, line[1]), warningFunction);
            }
            else
            {
                throw ModelLoadException("unknown command : " + line[0]);
            }
        }

        is.close();
    }
    virtual pair<shared_ptr<Model>, string> load() override
    {
        if(currentReturnedModel >= modelList.size())
            return make_pair(nullptr, "");
        string name = modelList[currentReturnedModel++];
        return make_pair(models[name], name);
    }
};
}

shared_ptr<ModelLoader> ModelLoader::loadOBJ(string fileName, function<void(string)> warningFunction)
{
    return make_shared<OBJModelLoader>(fileName, warningFunction);
}

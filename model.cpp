#include "model.h"
#include <stdexcept>
#include <cassert>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <climits>
#include <cstdlib>
#include <sstream>
#include <cctype>

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

class AC3DModelLoader : public ModelLoader // documentation is at http://www.inivis.com/ac3d/man/ac3dfileformat.html
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
#if 0
            else if(is.peek() == '#') // comment
            {
                do
                {
                    is.get();
                }
                while(is.peek() != '\r' && is.peek() != '\n' && is.peek() != EOF);
            }
#endif
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
        bool isStartOfToken = true, isInString = false;
        for(char ch : line)
        {
            if(ch == '\"')
            {
                isInString = !isInString;
                if(isStartOfToken)
                    retval.push_back("");
                isStartOfToken = false;
                retval.back() += ch;
            }
            else if(isblank(ch) && !isInString)
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
    static float parseLimitedFloat(string str, float minV, float maxV, function<void(string)> warningFunction)
    {
        float retval = parseFloat(str);
        if(warningFunction && (retval < minV || retval > maxV))
        {
            ostringstream ss;
            ss << "number out of range (" << minV << " to " << maxV << ") : '" << str << "' : limiting";
            warningFunction(ss.str());
        }
        return limit<float>(retval, minV, maxV);
    }
    static int parseInt(string str)
    {
        istringstream is(str);
        int retval;
        if(!(is >> retval))
            throw ModelLoadException("invalid integer '" + str + "'");
        return retval;
    }
    static string parseString(string str)
    {
        size_t finalSize = 0;
        for(size_t i = 0; i < str.size(); i++)
        {
            if(str[i] != '\"')
                str[finalSize++] = str[i];
        }
        str.resize(finalSize);
        return str;
    }
    static size_t parseIndex(string str, size_t size)
    {
        istringstream is(str);
        ptrdiff_t retval;
        if(!(is >> retval) || retval < 0 || retval >= (ptrdiff_t)size)
            throw ModelLoadException("invalid index '" + str + "'");
        return retval;
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
    ColorF parseColorSpecifier(size_t startingIndex, function<void(string)> warningFunction, float scale = 1.0f)
    {
        if(line.size() < 3 + startingIndex)
            throw ModelLoadException("too few arguments for " + line[0]);
        return validateColor(RGBF(parseFloat(line[startingIndex]) * scale, parseFloat(line[startingIndex + 1]) * scale, parseFloat(line[startingIndex + 2]) * scale), warningFunction);
    }
    shared_ptr<Texture> loadImage(string fileName)
    {
        shared_ptr<Texture> &retval = images[fileName];
        if(retval == nullptr)
        {
            if(fileName == "")
            {
                retval = make_shared<ImageTexture>(make_shared<Image>(GrayscaleI(0xFF)));
            }
            else
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
        }
        return retval;
    }
    vector<pair<shared_ptr<Model>, string>> models;
    vector<Material> materials;
    size_t currentReturnedModel = 0;
    unordered_map<string, shared_ptr<Texture>> images;
    vector<string> line;
    template <typename Fn>
    static void warning(bool doWarning, function<void(string)> warningFunction, Fn msg)
    {
        if(doWarning && warningFunction)
            warningFunction(msg());
    }
    void checkLineLength(size_t expectedLength, function<void(string)>)
    {
        if(line.size() > expectedLength)
            throw ModelLoadException("too many arguments for " + line[0]);
        if(line.size() < expectedLength)
            throw ModelLoadException("too few arguments for " + line[0]);
    }
    pair<shared_ptr<Model>, string> parse(string fileName, function<void(string)> warningFunction)
    {
        shared_ptr<Model> model = make_shared<Model>();
        unordered_map<Material, size_t> materialsMap;
        if(line[0] == "OBJECT")
        {
            checkLineLength(2, warningFunction);
            string objectKind = parseString(line[1]);
            if(objectKind != "world" && objectKind != "group" && objectKind != "poly")
                throw ModelLoadException("unknown kind : " + line[0]);
            Matrix tform = Matrix::identity();
            string name = "", data = "", url = "";
            shared_ptr<Texture> texture = loadImage("");
            vector<VectorF> vertices;
            vector<tuple<VectorF, TextureCoord>> polyVertices;
            //float textureRepeatX = 1, textureRepeatY = 1;
            //float creaseAngle = 0; // i don't really know the default
            for(line = readTokenizedLine(is); !line.empty(); line = readTokenizedLine(is))
            {
                if(line[0] == "name")
                {
                    checkLineLength(2, warningFunction);
                    name = parseString(line[1]);
                }
                else if(line[0] == "data")
                {
                    checkLineLength(2, warningFunction);
                    int charCount = parseInt(line[1]);
                    if(charCount < 1)
                        throw ModelLoadException("invalid character count for data : " + line[1]);
                    if(is.peek() == '\r')
                    {
                        is.get();
                        if(is.peek() == '\n')
                            is.get();
                    }
                    else if(is.peek() == '\n')
                    {
                        is.get();
                    }
                    else
                        throw ModelLoadException("missing data line");
                    for(int i = 0; i < charCount; i++)
                    {
                        int ch = is.get();
                        if(ch == EOF)
                            throw ModelLoadException("missing rest of data line");
                        data += (char)ch;
                    }
                }
                else if(line[0] == "texture")
                {
                    checkLineLength(2, warningFunction);
                    string imageName = getNeighborFileName(fileName, parseString(line[1]));
                    texture = loadImage(imageName);
                }
                else if(line[0] == "texrep")
                {
                    checkLineLength(3, warningFunction);
                    //textureRepeatX =
                    parseLimitedFloat(line[1], 1, 1, warningFunction);
                    //textureRepeatY =
                    parseLimitedFloat(line[2], 1, 1, warningFunction);
                }
                else if(line[0] == "rot")
                {
                    checkLineLength(10, warningFunction);
                    tform.x00 = parseFloat(line[1]);
                    tform.x01 = parseFloat(line[2]);
                    tform.x02 = parseFloat(line[3]);
                    tform.x10 = parseFloat(line[4]);
                    tform.x11 = parseFloat(line[5]);
                    tform.x12 = parseFloat(line[6]);
                    tform.x20 = parseFloat(line[7]);
                    tform.x21 = parseFloat(line[8]);
                    tform.x22 = parseFloat(line[9]);
                    float maxV = max<float>(abs(tform.x00), abs(tform.x01));
                    maxV = max<float>(maxV, abs(tform.x02));
                    maxV = max<float>(maxV, abs(tform.x10));
                    maxV = max<float>(maxV, abs(tform.x11));
                    maxV = max<float>(maxV, abs(tform.x12));
                    maxV = max<float>(maxV, abs(tform.x20));
                    maxV = max<float>(maxV, abs(tform.x21));
                    maxV = max<float>(maxV, abs(tform.x22));
                    if(maxV < 1e-20 || abs(tform.determinant()) / maxV < 1e-5)
                    {
                        throw ModelLoadException("invalid rotation matrix : can not invert : " + line[0]);
                    }
                }
                else if(line[0] == "loc")
                {
                    checkLineLength(4, warningFunction);
                    VectorF translation;
                    translation.x = parseFloat(line[1]);
                    translation.y = parseFloat(line[2]);
                    translation.z = parseFloat(line[3]);
                    tform = tform.concat(Matrix::translate(translation));
                }
                else if(line[0] == "url")
                {
                    checkLineLength(2, warningFunction);
                    url = parseString(line[1]);
                }
                else if(line[0] == "crease") // from http://www.inivis.com/forum/showthread.php?t=5936
                {
                    checkLineLength(2, warningFunction);
                    //creaseAngle =
                    parseLimitedFloat(line[1], 0, 180, warningFunction);
                }
                else if(line[0] == "numvert")
                {
                    checkLineLength(2, warningFunction);
                    int vertexCount = parseInt(line[1]);
                    if(vertexCount < 0)
                    {
                        throw ModelLoadException("invalid vertex count : " + line[0]);
                    }
                    warning(vertexCount == 0, warningFunction, []()->string{return "zero vertices for numvert";});
                    for(int i = 0; i < vertexCount; i++)
                    {
                        line = readTokenizedLine(is);
                        if(line.empty())
                            throw ModelLoadException("too few lines in file : numvert");
                        if(line.size() < 3)
                            throw ModelLoadException("too few numbers on line : numvert");
                        if(line.size() > 3)
                            throw ModelLoadException("too many numbers on line : numvert");
                        VectorF v;
                        v.x = parseFloat(line[0]);
                        v.y = parseFloat(line[1]);
                        v.z = parseFloat(line[2]);
                        vertices.push_back(transform(tform, v));
                    }
                }
                else if(line[0] == "numsurf")
                {
                    checkLineLength(2, warningFunction);
                    int surfaceCount = parseInt(line[1]);
                    if(surfaceCount < 0)
                    {
                        throw ModelLoadException("invalid surface count : " + line[0]);
                    }
                    warning(surfaceCount == 0, warningFunction, []()->string{return "zero surfaces for numsurf";});
                    for(int i = 0; i < surfaceCount; i++)
                    {
                        line = readTokenizedLine(is);
                        if(line.empty())
                            throw ModelLoadException("too few lines in file : numsurf");
                        if(line[0] != "SURF")
                            throw ModelLoadException("missing SURF : numsurf");
                        checkLineLength(2, warningFunction);
                        int flags = parseInt(line[1]);
                        if(flags < 0)
                            throw ModelLoadException("invalid SURF flags : out of range");
                        const int shadedFlagMask = 0x10, twoSidedFlagMask = 0x20, typeMask = 0xF;
                        enum
                        {
                            typePolygon = 0,
                            typeLineLoop = 1,
                            typeLine = 2
                        };
                        if((flags & typeMask) > 2)
                            throw ModelLoadException("invalid SURF flags : invalid type");
                        bool isShaded = true;
                        if(flags & shadedFlagMask)
                            isShaded = false;
                        bool isTwoSided = false;
                        if(flags & twoSidedFlagMask)
                            isTwoSided = true;
                        bool isLine = (flags & typeMask) != typePolygon;
                        warning((flags & ~(typeMask | shadedFlagMask | twoSidedFlagMask)) != 0, warningFunction, []()->string
                        {
                            return "unknown flag set : SURF";
                        });
                        warning(isLine, warningFunction, []()->string
                        {
                            return "lines ignored : SURF";
                        });
                        line = readTokenizedLine(is);
                        Material material = Material(GrayscaleF(1), GrayscaleF(0), 1, texture);
                        if(line.empty())
                            throw ModelLoadException("too few lines in file : SURF");
                        if(line[0] == "mat")
                        {
                            checkLineLength(2, warningFunction);
                            size_t materialIndex = parseIndex(line[1], materials.size());
                            if(isShaded)
                            {
                                material = materials[materialIndex];
                                material.texture = texture;
                            }
                            warning(!isShaded, warningFunction, []()->string
                            {
                                return "material specified for non-shaded SURF";
                            });
                            line = readTokenizedLine(is);
                        }
                        if(line[0] == "refs")
                        {
                            checkLineLength(2, warningFunction);
                            int vertexCount = parseInt(line[1]);
                            if(vertexCount <= 0)
                                throw ModelLoadException("vertex count out of range : refs");
                            polyVertices.clear();
                            polyVertices.reserve(vertexCount);
                            for(int i = 0; i < vertexCount; i++)
                            {
                                line = readTokenizedLine(is);
                                if(line.empty())
                                    throw ModelLoadException("too few lines in file : refs");
                                if(line.size() < 3)
                                    throw ModelLoadException("too few numbers on line : refs");
                                if(line.size() > 3)
                                    throw ModelLoadException("too many numbers on line : refs");
                                size_t vertexIndex = parseIndex(line[0], vertices.size());
                                TextureCoord tc;
                                tc.u = parseFloat(line[1]);
                                tc.v = parseFloat(line[2]);
                                polyVertices.push_back(tuple<VectorF, TextureCoord>(vertices[vertexIndex], tc));
                            }
                            if(!isLine)
                            {
                                warning(vertexCount < 3, warningFunction, []()->string
                                {
                                    return "too few vertices to make a polygon : ignored";
                                });
                                if(vertexCount >= 3)
                                {
                                    Mesh theMesh = Generate::convexPolygon(texture, polyVertices);
                                    if(isTwoSided)
                                        theMesh.append(reverse(theMesh));
                                    if(materialsMap.count(material) == 0)
                                    {
                                        materialsMap[material] = model->meshes.size();
                                        model->meshes.push_back(pair<Material, Mesh>(material, theMesh));
                                    }
                                    else
                                    {
                                        std::get<1>(model->meshes[materialsMap[material]]).append(theMesh);
                                    }
                                }
                            }
                        }
                        else
                        {
                            throw ModelLoadException("unknown directive : " + line[0]);
                        }
                    }
                }
                else if(line[0] == "kids")
                {
                    checkLineLength(2, warningFunction);
                    int kidCount = parseInt(line[1]);
                    if(kidCount < 0)
                    {
                        throw ModelLoadException("invalid kid count : " + line[0]);
                    }
                    line = readTokenizedLine(is);
                    for(int i = 0; i < kidCount; i++)
                    {
                        shared_ptr<Model> kidModel = std::get<0>(parse(fileName, warningFunction));
                        for(pair<Material, Mesh> &mesh : kidModel->meshes)
                        {
                            Mesh theMesh = transform(tform, std::move(std::get<1>(mesh)));
                            Material material = std::get<0>(mesh);
                            if(materialsMap.count(material) == 0)
                            {
                                materialsMap[material] = model->meshes.size();
                                model->meshes.push_back(pair<Material, Mesh>(material, theMesh));
                            }
                            else
                            {
                                std::get<1>(model->meshes[materialsMap[material]]).append(theMesh);
                            }
                        }
                    }
                    break;
                }
                else
                    break;
            }
            return pair<shared_ptr<Model>, string>(model, name);
        }
        else
        {
            throw ModelLoadException("unknown directive : " + line[0]);
        }
    }
public:
    AC3DModelLoader(string fileName, function<void(string)> warningFunction)
        : is(fileName.c_str())
    {
        if(!is)
            throw ModelLoadException("can't open '" + fileName + "'");
        string magicLine = readLine(is);
        if(magicLine.substr(0, 4) != "AC3D")
            throw ModelLoadException("AC3D magic string doesn't match");
        if(magicLine.substr(4) != "b")
            throw ModelLoadException("invalid AC3D version number");
        line = readTokenizedLine(is);
        while(!line.empty() && line[0] == "MATERIAL") // material definition
        {
            // MATERIAL (name) rgb %f %f %f  amb %f %f %f  emis %f %f %f  spec %f %f %f  shi %d  trans %f
            // 0        1      2   3  4  5   6   7  8  9   10   11 12 13  14   15 16 17  18  19  20    21
            checkLineLength(22, warningFunction);
            string name = parseString(line[1]);
            if(line[2] != "rgb")
                throw ModelLoadException("missing rgb for " + line[0]);
            ColorF rgb = parseColorSpecifier(3, warningFunction);
            if(line[6] != "amb")
                throw ModelLoadException("missing amb for " + line[0]);
            ColorF amb = parseColorSpecifier(7, warningFunction, 0.2f);
            if(line[10] != "emis")
                throw ModelLoadException("missing emis for " + line[0]);
            ColorF emis = parseColorSpecifier(11, warningFunction);
            if(line[14] != "spec")
                throw ModelLoadException("missing spec for " + line[0]);
            //ColorF spec =
            parseColorSpecifier(15, warningFunction);
            if(line[18] != "shi")
                throw ModelLoadException("missing shi for " + line[0]);
            //int shi =
            parseInt(line[19]);
            if(line[20] != "trans")
                throw ModelLoadException("missing trans for " + line[0]);
            float opacity = 1 - parseLimitedFloat(line[21], 0, 1, warningFunction);

            materials.push_back(Material(add(amb, emis), rgb, opacity));
            line = readTokenizedLine(is);
        }
        while(!line.empty())
            models.push_back(parse(fileName, warningFunction));
        is.close();
    }
    virtual pair<shared_ptr<Model>, string> load() override
    {
        if(currentReturnedModel >= models.size())
            return make_pair(nullptr, "");
        return models[currentReturnedModel++];
    }
};
}

shared_ptr<ModelLoader> ModelLoader::loadOBJ(string fileName, function<void(string)> warningFunction)
{
    return make_shared<OBJModelLoader>(fileName, warningFunction);
}

shared_ptr<ModelLoader> ModelLoader::loadAC3D(string fileName, function<void(string)> warningFunction)
{
    return make_shared<AC3DModelLoader>(fileName, warningFunction);
}

namespace
{
struct Loader
{
    typedef shared_ptr<ModelLoader> (*LoadFn)(string fileName, function<void(string)> warningFunction);
    LoadFn loadFn;
    const char *const *extensions;
    constexpr Loader(LoadFn loadFn, const char *const *extensions)
        : loadFn(loadFn), extensions(extensions)
    {
    }
};
const char *loaderExtensionsOBJ[] =
{
    "obj",
    nullptr
};
const char *loaderExtensionsAC3D[] =
{
    "ac",
    nullptr
};
const Loader loaders[] =
{
    Loader(&ModelLoader::loadOBJ, loaderExtensionsOBJ),
    Loader(&ModelLoader::loadAC3D, loaderExtensionsAC3D),
};
}

shared_ptr<ModelLoader> ModelLoader::load(string fileName, function<void(string)> warningFunction)
{
    string extension = "";
    if(fileName.size() > 0)
    {
        size_t lastPeriod = fileName.find_last_of('.');
        size_t lastSlash = fileName.find_last_of('/');
        if(lastSlash != string::npos && lastPeriod != string::npos && lastSlash > lastPeriod)
            lastPeriod = string::npos;
        if(lastPeriod != string::npos)
        {
            extension = fileName.substr(lastPeriod + 1);
        }
    }
    for(char &ch : extension)
    {
        ch = tolower(ch);
    }
    for(const Loader &loader : loaders) // detect based on extension
    {
        for(const char *const *eptr = loader.extensions; *eptr != nullptr; eptr++)
        {
            if(*eptr == extension)
                return loader.loadFn(fileName, warningFunction);
        }
    }

    // try to detect by checking which one doesn't fail

    // cache warnings so we don't get warnings from a different loader than the successful one
    vector<string> warnings;
    function<void(string)> myWarningFunction = nullptr;
    if(warningFunction)
    {
        myWarningFunction = [&warnings](string msg)
        {
            warnings.push_back(msg);
        };
    }
    for(const Loader &loader : loaders)
    {
        try
        {
            shared_ptr<ModelLoader> retval = loader.loadFn(fileName, myWarningFunction);
            if(warningFunction) // succeeded, so send the cached warnings
            {
                for(string msg : warnings)
                {
                    warningFunction(msg);
                }
            }
            return retval;
        }
        catch(ModelLoadException &)
        {
            warnings.clear();
        }
    }
    throw ModelLoadException("can't determine file type");
}
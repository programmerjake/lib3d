#ifndef RAWRENDERER_H_INCLUDED
#define RAWRENDERER_H_INCLUDED

#include "renderer.h"
#include <functional>

using namespace std;

void setRawRendererOutputFile(string fileName);
string getRawRendererOutputFile();
void setRawRendererFrameRate(float fps);
float getRawRendererFrameRate();
shared_ptr<WindowRenderer> makeRawRenderer();

#endif // RAWRENDERER_H_INCLUDED

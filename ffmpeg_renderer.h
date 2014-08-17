#ifndef FFMPEG_RENDERER_H_INCLUDED
#define FFMPEG_RENDERER_H_INCLUDED

#include "renderer.h"
#include <functional>

using namespace std;

void setFFmpegOutputFile(string fileName);
string getFFmpegOutputFile();
void setFFmpegFrameRate(float fps);
float getFFmpegFrameRate();
void setFFmpegBitRate(size_t bitRate);
size_t getFFmpegBitRate();
void registerFFmpegRenderStatusCallback(function<void(void)> fn);
shared_ptr<WindowRenderer> makeFFmpegRenderer(function<shared_ptr<ImageRenderer>(size_t w, size_t h, float aspectRatio)> imageRendererMaker = makeImageRenderer);

#endif // FFMPEG_RENDERER_H_INCLUDED

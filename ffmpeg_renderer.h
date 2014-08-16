#ifndef FFMPEG_RENDERER_H_INCLUDED
#define FFMPEG_RENDERER_H_INCLUDED

#include "renderer.h"

void setFFmpegOutputFile(string fileName);
string getFFmpegOutputFile();
void setFFmpegFrameRate(float fps);
float getFFmpegFrameRate();
void setFFmpegBitRate(size_t bitRate);
size_t getFFmpegBitRate();
shared_ptr<WindowRenderer> makeFFmpegRenderer();

#endif // FFMPEG_RENDERER_H_INCLUDED

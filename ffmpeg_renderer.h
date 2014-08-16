#ifndef FFMPEG_RENDERER_H_INCLUDED
#define FFMPEG_RENDERER_H_INCLUDED

#include "renderer.h"

void setFFmpegOutputFile(string fileName);
string getFFmpegOutputFile();
void setFFmpegOutputFormat(string format);
string getFFmpegOutputFormat();
void setFFmpegOutputCodec(string codec);
string getFFmpegOutputCodec();
void setFFmpegFrameRate(float fps);
float getFFmpegFrameRate();
shared_ptr<WindowRenderer> makeFFmpegRenderer();

#endif // FFMPEG_RENDERER_H_INCLUDED

/******************************************************************************
 * Copyright (c) Artur Eganyan
 *
 * This software is provided "AS IS", WITHOUT ANY WARRANTY, express or implied.
 ******************************************************************************/

#ifndef FRAMECONTROL_H
#define FRAMECONTROL_H

#include <stdint.h>

void FrameControl_Init(uint8_t fps, uint64_t maxDeltaTime);
void FrameControl_Deinit();      // Must be called after startFrameControl(), before the program exits
void FrameControl_WaitForNextFrame();
uint64_t FrameControl_GetElapsedFrameTime(); // ms
uint64_t FrameControl_GetElapsedTime();      // ms
double FrameControl_GetCurrentFps();

#endif // FRAMECONTROL_H

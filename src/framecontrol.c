/******************************************************************************
 * Copyright (c) Artur Eganyan
 *
 * This software is provided "AS IS", WITHOUT ANY WARRANTY, express or implied.
 ******************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "framecontrol.h"
#include "helpers.h"

#include <time.h>

static struct {
    uint64_t startTime;
    uint64_t prevFrameTime;
    uint64_t elapsedFrameTime;
    uint64_t framePeriod;
    uint64_t frameCount;
    uint64_t maxDeltaTime;
} fc = {0};

// If fps <= 0, new frame will be ready right after the previous one is handled,
// i.e. there will be no fps limit.
// 
// The maxDeltaTime is the maximum time increment which can be correctly handled,
// in ms. If it's <= 0, there will be no increment limit.
//
// The maxDeltaTime is used because, if the frame takes too long, the game may
// change more than we can handle: e.g. some object may move too far, across the
// walls. To solve this, the long frame can be handled as multiple shorter frames,
// each <= maxDeltaTime. Or, if it's not required to sync the game time with the
// real time, the long frame can be simply truncated to maxDeltaTime. This is done
// in getElapsedFrameTime(), so that each long frame will be treated as a shorter
// one (the game will slow down at these moments). For more information, see
// https://gafferongames.com/post/fix_your_timestep/
void FrameControl_Init(uint8_t fps, uint64_t maxDeltaTime)
{
    fc.startTime = SDL_GetTicks64();
    fc.prevFrameTime = fc.startTime;
    fc.elapsedFrameTime = 0;
    fc.framePeriod = 1000 / fps;
    fc.frameCount = 0;
    fc.maxDeltaTime = maxDeltaTime;
}

void FrameControl_Deinit()
{
}

void FrameControl_WaitForNextFrame()
{
    const uint64_t nextFrameTime = fc.prevFrameTime + fc.framePeriod;
    uint64_t currentTime = SDL_GetTicks64();

    while (currentTime < nextFrameTime)
    {
        SDL_Delay(1);
        currentTime = SDL_GetTicks64();
    }

    fc.elapsedFrameTime = currentTime - fc.prevFrameTime;
    fc.prevFrameTime = currentTime;
    fc.frameCount += 1;
}

uint64_t FrameControl_GetElapsedFrameTime()
{
    return ((fc.maxDeltaTime > 0) && (fc.elapsedFrameTime > fc.maxDeltaTime))
        ? fc.maxDeltaTime
        : fc.elapsedFrameTime;
}

uint64_t FrameControl_GetElapsedTime()
{
    return SDL_GetTicks64() - fc.startTime;
}

double FrameControl_GetCurrentFps()
{
    return fc.frameCount / ((fc.prevFrameTime - fc.startTime) / 1000.0);
}

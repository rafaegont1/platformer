/******************************************************************************
 * Copyright (c) Artur Eganyan
 *
 * This software is provided "AS IS", WITHOUT ANY WARRANTY, express or implied.
 ******************************************************************************/

#ifndef RENDER_H
#define RENDER_H

#include "types.h"

extern SDL_Renderer* renderer;

void Render_Init(const char* spritesPath, const char* fontPath);
void Render_Deinit();
void Render_DrawSprite(SDL_Rect spriteRect, int x, int y, int frame, SDL_RendererFlip flip);
void Render_DrawObject(const Object* object);
void Render_DrawMessage(MessageId message);
void Render_DrawScreen();
void Render_SetAnimation(Object* object, int frameStart, int frameEnd, int fps);
void Render_SetAnimationWave(Object* object, int fps);
void Render_SetAnimationFlip(Object* object, int frame, int fps);

#endif // RENDER_H

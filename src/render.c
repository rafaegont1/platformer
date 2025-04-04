/******************************************************************************
 * Copyright (c) Artur Eganyan
 *
 * This software is provided "AS IS", WITHOUT ANY WARRANTY, express or implied.
 ******************************************************************************/

#include "render.h"
#include "game.h"
#include "framecontrol.h"
#include "helpers.h"
#include "types.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

SDL_Renderer* renderer;
static SDL_Texture* sprites;
static SDL_Window* window;
static SDL_Texture* messages[MESSAGE_COUNT];

static const SDL_Color TEXT_COLOR = {255, 255, 255, 255};
static const SDL_Color TEXT_BOX_CONTENT_COLOR = {0, 0, 0, 255};
static const SDL_Color TEXT_BOX_BORDER_COLOR = {255, 255, 255, 255};
static const int TEXT_BOX_BORDER = 1 * SIZE_FACTOR;
static const int TEXT_BOX_PADDING = 5 * SIZE_FACTOR;
static const int TEXT_FONT_SIZE = 8 * SIZE_FACTOR;

// The text must be one-line
static void Render_InitMessage(MessageId id, const char* text, TTF_Font* font)
{
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, TEXT_COLOR);
    messages[id] = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
}

static SDL_Texture* Render_LoadTexture(const char* filePath)
{
    static const Uint8 transparent[3] = {90, 82, 104};
    Uint32 opaqueColor;
    SDL_Texture* texture;
    SDL_Surface* surface;

    surface = SDL_LoadBMP(filePath);
    Util_EnsureSDL(surface != NULL, "Can't load sprite sheet");

    opaqueColor = SDL_MapRGB(
        surface->format, transparent[0], transparent[1], transparent[2]
    );
    SDL_SetColorKey(surface, SDL_TRUE, opaqueColor);

    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    return texture;
}

void Render_Init(const char* spritesPath, const char* fontPath)
{
    // Create a window
    window = SDL_CreateWindow(
        "platformer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        LEVEL_WIDTH * SIZE_FACTOR, LEVEL_HEIGHT * SIZE_FACTOR,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    Util_EnsureSDL(window != NULL, "Window could not be created!");

    // Create a renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    Util_EnsureSDL(renderer != NULL, "Renderer could not be created!");

    // Sprites
    sprites = Render_LoadTexture(spritesPath);

    // Open font
    TTF_Font* font = TTF_OpenFont(fontPath, TEXT_FONT_SIZE);
    Util_EnsureSDL(font != NULL, "Can't open font");

    // Init messages
    Render_InitMessage(MESSAGE_PLAYER_KILLED,  "You lost a life", font);
    Render_InitMessage(MESSAGE_GAME_OVER,      "Game over",       font);
    Render_InitMessage(MESSAGE_LEVEL_COMPLETE, "Level complete!", font);

    // Close font
    TTF_CloseFont(font);
}

void Render_Deinit()
{
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(sprites);

    for (int i = 0; i < MESSAGE_COUNT; i++)
    {
        SDL_DestroyTexture(messages[i]);
    }
}

void Render_DrawSprite(SDL_Rect spriteRect, int x, int y, int frame, SDL_RendererFlip flip)
{
    spriteRect.x += spriteRect.w * frame;
    const SDL_Rect dstRect = (SDL_Rect) {
        .x = x * SIZE_FACTOR,
        .y = y * SIZE_FACTOR,
        .w = spriteRect.w * SIZE_FACTOR,
        .h = spriteRect.h * SIZE_FACTOR
    };
    SDL_RenderCopyEx(renderer, sprites, &spriteRect, &dstRect, 0, NULL, flip);
}

static void Render_DrawObjectBody(Object* object)
{
    const SDL_Rect body = (SDL_Rect) {
        .x = (object->x + object->type->body.x) * SIZE_FACTOR,
        .y = (object->y + object->type->body.y) * SIZE_FACTOR,
        .w = object->type->body.w * SIZE_FACTOR,
        .h = object->type->body.h * SIZE_FACTOR
    };

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderDrawRect(renderer, &body);
}

void Render_DrawObject(const Object* object)
{
    const int frame = object->anim.frame;
    const int flip = object->anim.flip;
    const int x = object->x;
    const int y = object->y;

    SDL_SetTextureAlphaMod(sprites, object->anim.alpha);

    if (object->anim.type == ANIMATION_WAVE)
    {
        SDL_Rect spriteRect = object->type->sprite;
        spriteRect.w -= frame;
        Render_DrawSprite(spriteRect, x + frame, y, 0, flip);

        spriteRect.x += spriteRect.w;
        spriteRect.w = frame;
        Render_DrawSprite(spriteRect, x, y, 0, flip);
    }
    else
    {
        Render_DrawSprite(object->type->sprite, x, y, frame, flip);
    }

#ifdef DEBUG_MODE
    drawObjectBody(object);
#endif

    SDL_SetTextureAlphaMod(sprites, 255);
}

static void Render_DrawBox(SDL_Rect box, int border, SDL_Color borderColor, SDL_Color contentColor)
{
    const SDL_Rect borderRect = {
        .x = box.x - border,
        .y = box.y - border,
        .w = box.w + border * 2,
        .h = box.h + border * 2
    };
    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    SDL_RenderFillRect(renderer, &borderRect);

    SDL_SetRenderDrawColor(renderer, contentColor.r, contentColor.g, contentColor.b, contentColor.a);
    SDL_RenderFillRect(renderer, &box);
}

void Render_DrawMessage(MessageId id)
{
    SDL_Texture* texture = messages[id];

    SDL_Rect textRect = {.w = 0, .h = 0};
    SDL_QueryTexture(texture, NULL, NULL, &textRect.w, &textRect.h);
    textRect.x = (SIZE_FACTOR * LEVEL_WIDTH - textRect.w) / 2;
    textRect.y = (SIZE_FACTOR * LEVEL_HEIGHT - textRect.h) / 2;

    const int padding = TEXT_BOX_PADDING;
    const SDL_Rect boxRect = {
        .x = textRect.x - padding,
        .y = textRect.y - padding,
        .w = textRect.w + padding * 2,
        .h = textRect.h + padding * 2
    };
    Render_DrawBox(boxRect, TEXT_BOX_BORDER, TEXT_BOX_BORDER_COLOR, TEXT_BOX_CONTENT_COLOR);

    SDL_RenderCopy(renderer, texture, NULL, &textRect);
}

void Render_DrawScreen()
{
    // Level
    for (int r = 0; r < ROW_COUNT; r++)
    {
        for (int c = 0; c < COLUMN_COUNT; c++)
        {
            ObjectType* type = level->cells[r][c];
            Render_DrawSprite(type->sprite, CELL_SIZE * c, CELL_SIZE * r, 0, SDL_FLIP_NONE);
        }
    }

    // Objects
    const double dt = FrameControl_GetElapsedFrameTime() / 1000.0;

    // for (ObjectListNode* iter = level->objects.first; iter != NULL; iter = iter->next)
    for (ListNode* iter = level->objects.first; iter != NULL; iter = iter->next)
    {
        Object* object = iter->data;
        Animation* anim = &object->anim;

        if (object->removed)
        {
            continue;
        }

        anim->frameDelayCounter -= dt;

        if (anim->frameDelayCounter <= 0)
        {
            anim->frameDelayCounter = anim->frameDelay;
            anim->frame += 1;

            if (anim->frame > anim->frameEnd)
            {
                anim->frame = anim->frameStart;
            }

            if (anim->type == ANIMATION_FLIP)
            {
                anim->flip = (anim->flip == SDL_FLIP_NONE)
                    ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
            }
        }

        Render_DrawObject(object);
    }
}

static void Render_SetAnimationEx(Object* object, int start, int end, int fps, int type)
{
    Animation* anim = &object->anim;

    anim->type = type;
    anim->frameStart = start;
    anim->frameEnd = end;
    anim->frameDelay = 1.0 / fps;

    if ((anim->frame < anim->frameStart) || (anim->frame > anim->frameEnd))
    {
        anim->frame = anim->frameStart;
    }

    if ((anim->frameDelayCounter > anim->frameDelay) || (anim->frameDelayCounter < 0))
    {
        anim->frameDelayCounter = anim->frameDelay;
    }
}

void Render_SetAnimation(Object* object, int frameStart, int frameEnd, int fps)
{
    Render_SetAnimationEx(object, frameStart, frameEnd, fps, ANIMATION_FRAME);
}

void Render_SetAnimationWave(Object* object, int fps)
{
    Render_SetAnimationEx(object, 0, object->type->sprite.w - 1, fps, ANIMATION_WAVE);
}

void Render_SetAnimationFlip(Object* object, int frame, int fps)
{
    Render_SetAnimationEx(object, frame, frame, fps, ANIMATION_FLIP);
}

/******************************************************************************
 * Copyright (c) Artur Eganyan
 *
 * This software is provided "AS IS", WITHOUT ANY WARRANTY, express or implied.
 ******************************************************************************/

#include "game.h"
#include "framecontrol.h"
#include "helpers.h"
#include "render.h"
#include "levels.h"
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <math.h>

typedef enum {
    STATE_QUIT = 0,
    STATE_PLAYING,
    STATE_KILLED,
    STATE_GAMEOVER,
    STATE_LEVELCOMPLETE
} GAME_STATE;

static struct {
    GAME_STATE state;
    const Uint8* keystate;
    struct { double x, y; } respawnPos;
    uint64_t cleanTime;
    bool jumpDenied;
} game;

Level* level = NULL;
Player player;

static const double PLAYER_SPEED_RUN = 72;          // Pixels per second 
static const double PLAYER_SPEED_LADDER = 48;       //
static const double PLAYER_SPEED_JUMP = 216;        //
static const double PLAYER_SPEED_FALL_MAX = 120;    //

static const double PLAYER_GRAVITY = 24 * 48;       // Pixels per second per second

static const double PLAYER_ANIM_SPEED_RUN = 8;      // Frames per second
static const double PLAYER_ANIM_SPEED_LADDER = 6;   //

static const double CLEAN_PERIOD = 10000;           // Milliseconds


void Game_DamagePlayer(int damage)
{
    if (player.invincibility > 0)
    {
        return;
    }

    player.health -= damage;

    if (player.health <= 0)
    {
        player.health = 0;
        Game_KillPlayer();
    }
}

void Game_KillPlayer()
{
    if (player.invincibility > 0)
    {
        return;
    }

    Render_SetAnimation((Object*)&player, 5, 5, 0);

    if (--player.lives > 0)
    {
        game.state = STATE_KILLED;
    }
    else
    {
        game.state = STATE_GAMEOVER;
    }
}

void Game_RespawnPlayer()
{
    Render_SetAnimation((Object*)&player, 0, 0, 0);

    player.invincibility = 2000;
    player.onLadder = false;
    player.inAir = false;
    player.x = game.respawnPos.x;
    player.y = game.respawnPos.y;
}

void Game_SetLevel(int r, int c)
{
    level = &levels[r][c];

    if (level->init)
    {
        level->init();
    }
}

void Game_CompleteLevel()
{
    game.state = STATE_LEVELCOMPLETE;
}

static void Game_ProcessInput()
{
    // ... Left
    if (game.keystate[SDL_SCANCODE_LEFT])
    {
        if (!player.onLadder)
        {
            if (!player.inAir)
            {
                Render_SetAnimation((Object*)&player, 1, 2, PLAYER_ANIM_SPEED_RUN);
            }
            else
            {
                Render_SetAnimation((Object*)&player, 1, 1, PLAYER_ANIM_SPEED_RUN);
            }
        }
        player.anim.flip = SDL_FLIP_HORIZONTAL;
        player.vx = -PLAYER_SPEED_RUN;
    }
    // ... Right
    else if (game.keystate[SDL_SCANCODE_RIGHT])
    {
        if (!player.onLadder)
        {
            if (!player.inAir)
            {
                Render_SetAnimation((Object*)&player, 1, 2, PLAYER_ANIM_SPEED_RUN);
            }
            else
            {
                Render_SetAnimation((Object*)&player, 1, 1, PLAYER_ANIM_SPEED_RUN);
            }
        }
        player.anim.flip = SDL_FLIP_NONE;
        player.vx = PLAYER_SPEED_RUN;
    }
    // ... Not left or right
    else
    {
        if (!player.onLadder)
        {
            Render_SetAnimation((Object*)&player, 0, 0, 0);
        }
        player.vx = 0;
    }

    // ... Up
    if (game.keystate[SDL_SCANCODE_UP])
    {
        int r, c;
        Util_GetObjectCell((Object*)&player, &r, &c);
        if (!Util_IsLadder(r, c))
        {
            player.onLadder = false;
            // jumpDenied prevents jump when player reaches the top of the ladder
            // by holding UP key, until this key is released
            if (!player.inAir && !game.jumpDenied) {
                player.vy = -PLAYER_SPEED_JUMP;
            }
        }
        else
        {
            player.onLadder = true;
            player.vy = -PLAYER_SPEED_LADDER;
            player.x = c * CELL_SIZE;
            Render_SetAnimationFlip((Object*)&player, 3, PLAYER_ANIM_SPEED_LADDER);
            game.jumpDenied = true;
        }
    }
    // ... Down
    else if (game.keystate[SDL_SCANCODE_DOWN])
    {
        int r, c;
        Util_GetObjectCell((Object*)&player, &r, &c);
        if (Util_IsLadder(r + 1, c) || player.onLadder)
        {
            if (!player.onLadder)
            {
                player.onLadder = true;
                player.y = r * CELL_SIZE + CELL_HALF + 1;
            }
            player.vy = PLAYER_SPEED_LADDER;
            player.x = c * CELL_SIZE;
            Render_SetAnimationFlip((Object*)&player, 3, PLAYER_ANIM_SPEED_LADDER);
        }
    }
    // ... Not up or down
    else
    {
        if (player.onLadder)
        {
            Render_SetAnimation((Object*)&player, 3, 3, 0);
            player.vy = 0;
        }
        else
        {
            game.jumpDenied = false;
        }
    }

    // ... Space
    if (game.keystate[SDL_SCANCODE_SPACE])
    {
        int r, c;

        Util_GetObjectCell((Object*)&player, &r, &c);

        if (Util_FindNearDoor(&r, &c))
        {
            if (player.keys > 0)
            {
                player.keys -= 1;
                level->cells[r][c] = &objectTypes[TYPE_NONE];
            }
        }
    }

#ifdef DEBUG_MODE
    // ... F, simulate "frame by frame" mode
    if (game.keystate[SDL_SCANCODE_F])
    {
        SDL_Delay(1000);
    }
#endif // DEBUG_MODE
}

static void Game_ProcessPlayer()
{
    // Movement
    const double dt = FrameControl_GetElapsedFrameTime() / 1000.0;
    const double hitw = (CELL_SIZE - player.type->body.w) / 2.0;
    const double hith = hitw;

    int r, c; Borders cell, body;
    Util_GetObjectPos((Object*)&player, &r, &c, &cell, &body);

    player.vx = Util_LimitAbs(player.vx, MAX_SPEED);
    player.vy = Util_LimitAbs(player.vy, MAX_SPEED);

    // ... X
    player.x += player.vx * dt;
    Borders sprite = {player.x, player.x + CELL_SIZE, player.y, player.y + CELL_SIZE};

    // ... Left
    if (sprite.left < cell.left && player.vx <= 0)
    {
        if (Util_IsSolid(r, c - 1, SOLID_RIGHT)
        || (sprite.top + hith < cell.top && Util_IsSolid(r - 1, c - 1, SOLID_RIGHT))
        || (sprite.bottom - hith > cell.bottom && Util_IsSolid(r + 1, c - 1, SOLID_RIGHT)))
        {
            player.x = cell.left;
            player.vx = 0;
        }
    // ... Right
    }
    else if (sprite.right > cell.right && player.vx >= 0)
    {
        if (Util_IsSolid(r, c + 1, SOLID_LEFT)
        || (sprite.top + hith < cell.top && Util_IsSolid(r - 1, c + 1, SOLID_LEFT))
        || (sprite.bottom - hith > cell.bottom && Util_IsSolid(r + 1, c + 1, SOLID_LEFT)))
        {
            player.x = cell.left;
            player.vx = 0;
        }
    }

    // ... Y
    player.y += player.vy * dt;
    sprite = (Borders){player.x, player.x + CELL_SIZE, player.y, player.y + CELL_SIZE};

    // ... Bottom
    if (sprite.bottom > cell.bottom && player.vy >= 0)
    {
        if (Util_IsSolid(r + 1, c, SOLID_TOP)
        || (sprite.left + hitw < cell.left && Util_IsSolid(r + 1, c - 1, SOLID_TOP))
        || (sprite.right - hitw > cell.right && Util_IsSolid(r + 1, c + 1, SOLID_TOP))
        || (!player.onLadder && Util_isSolidLadder(r + 1, c)))
        {
            player.y = cell.top;
            player.vy = 0;
            player.inAir = false;

            if (player.onLadder)
            {
                player.onLadder = false;
                Render_SetAnimation((Object*)&player, 0, 0, 0);
            }
        }
        else
        {
            player.inAir = !player.onLadder;
        }
    // ... Top
    }
    else if (sprite.top < cell.top && player.vy <= 0)
    {
        if (Util_IsSolid(r - 1, c, SOLID_BOTTOM)
        || (sprite.left + hitw < cell.left && Util_IsSolid(r - 1, c - 1, SOLID_BOTTOM))
        || (sprite.right - hitw > cell.right && Util_IsSolid(r - 1, c + 1, SOLID_BOTTOM)))
        {
            player.y = cell.top;
            player.vy += 1;
        }
        player.inAir = !player.onLadder;
    }

    // Screen borders
    Util_GetObjectCell((Object*)&player, &r, &c);

    const int lc = level->c;
    const int lr = level->r;

    // ... Left
    if (player.x < 0)
    {
        if (lc > 0 && !levels[lr][lc - 1].cells[r][COLUMN_COUNT - 1]->solid)
        {
            if (player.x + CELL_HALF < 0)
            {
                Game_SetLevel(lr, lc - 1);
                player.x = LEVEL_WIDTH - CELL_HALF - 1;
            }
        }
        else
        {
            player.x = 0;
        }
    // ... Right
    }
    else if (player.x + CELL_SIZE > LEVEL_WIDTH)
    {
        if (lc < LEVEL_COUNTX - 1 && !levels[lr][lc + 1].cells[r][0]->solid)
        {
            if (player.x + CELL_HALF > LEVEL_WIDTH)
            {
                Game_SetLevel(lr, lc + 1);
                player.x = -CELL_HALF + 1;
            }
        }
        else
        {
            player.x = LEVEL_WIDTH - CELL_SIZE;
        }
    }
    // ... Bottom
    if (player.y + player.type->body.h > LEVEL_HEIGHT)
    {
        if (lr < LEVEL_COUNTY - 1)
        {
            if (!levels[lr + 1][lc].cells[0][c]->solid)
            {
                if (player.y + player.type->body.h / 2.0 > LEVEL_HEIGHT)
                {
                    Game_SetLevel(lr + 1, lc);
                    player.y = -CELL_HALF + 1;
                }
            }
            else
            {
                player.y = LEVEL_HEIGHT - player.type->body.h;
                player.inAir = false;
            }
        }
        else
        {
            Game_KillPlayer();
        }
    }
    // ... Top
    else if (player.y < 0)
    {
        if (lr > 0 && !levels[lr - 1][lc].cells[ROW_COUNT - 1][c]->solid)
        {
            if (player.y + CELL_HALF < 0)
            {
                Game_SetLevel(lr - 1, lc);
                player.y = LEVEL_HEIGHT - CELL_HALF - 1;
            }
        }
        else if (lr > 0)
        {
            player.y = 0;
        }
        else
        {
            // Player will simply fall down
        }
    }

    // Environment and others
    Util_GetObjectCell((Object*)&player, &r, &c);

    // ... Gravity
    if (!player.onLadder)
    {
        player.vy += PLAYER_GRAVITY * dt;
        if (player.vy > PLAYER_SPEED_FALL_MAX)
        {
            player.vy = PLAYER_SPEED_FALL_MAX;
        }
    }

    // ... Ladder
    if (player.onLadder && !Util_IsLadder(r, c))
    {
        player.onLadder = false;
        Render_SetAnimation((Object*)&player, 0, 0, 0);
        if (player.vy < 0)
        {
            player.vy = 0;
            player.y = CELL_SIZE * r;
        }
    }

    // ... Water
    if (Util_IsWater(r, c))
    {
        Game_KillPlayer();
    }

    // ... Invincibility
    if (player.invincibility > 0)
    {
        player.invincibility -= dt * 1000;
        if (player.invincibility < 0)
        {
            player.invincibility = 0;
        }
        player.anim.alpha = 255 * (1 - (player.invincibility / 200) % 2);  // Blink each 200 ms
    }

    // ... If player stands on the ground, remember this position
    if (!player.inAir && !player.onLadder)
    {
        game.respawnPos.x = player.x;
        game.respawnPos.y = player.y;
    }
}

static void Game_ProcessObjects()
{
    for (ObjectListNode* iter = level->objects.first; iter != NULL; iter = iter->next)
    {
        Object* object = iter->data;

        if (object == (Object*)&player || object->removed)
        {
            continue;
        }

        object->type->onFrame(object);

        if (Util_HitTest(object, (Object*)&player))
        {
            object->type->onHit(object);
        }
    }
}

static void Game_ProcessFrame()
{
    // Draw screen
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    Render_DrawScreen();

    switch (game.state)
    {
        case STATE_KILLED:
            Render_DrawMessage(MESSAGE_PLAYER_KILLED);
            break;

        case STATE_LEVELCOMPLETE:
            Render_DrawMessage(MESSAGE_LEVEL_COMPLETE);
            break;

        case STATE_GAMEOVER:
            Render_DrawMessage(MESSAGE_GAME_OVER);
            break;

        default:
            break;
    }

    SDL_RenderPresent(renderer);

    // Read all events
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            game.state = STATE_QUIT;
        }
    }

    // Process user input and game logic
    const uint64_t current_time = FrameControl_GetElapsedTime();

    switch (game.state)
    {
        case STATE_PLAYING:
            Game_ProcessInput();
            Game_ProcessPlayer();
            Game_ProcessObjects();
            break;

        case STATE_KILLED:
            if (game.keystate[SDL_SCANCODE_SPACE])
            {
                game.state = STATE_PLAYING;
                Game_RespawnPlayer();
            }
            break;

        case STATE_LEVELCOMPLETE:
            if (game.keystate[SDL_SCANCODE_SPACE])
            {
                game.state = STATE_QUIT;
            }
            break;

        case STATE_GAMEOVER:
            if (game.keystate[SDL_SCANCODE_SPACE])
            {
                game.state = STATE_QUIT;
            }
            break;

        default:
            break;
    }

    // Delete unused objects from memory
    if (current_time >= game.cleanTime)
    {
        game.cleanTime = current_time + CLEAN_PERIOD;
        ObjectList_clean(&level->objects);
    }

#ifdef DEBUG_MODE
    printf("fps=%f, objects=%d\n", getCurrentFps(), level->objects.count);
#endif
}

static void Game_OnExit()
{
    FrameControl_Deinit();
    Render_Deinit();

    TTF_Quit();
    SDL_Quit();
}

void Game_Init()
{
    atexit(Game_OnExit);

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    // Initialize SDL_ttf
    if (TTF_Init() < 0)
    {
        printf("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }

    Render_Init("image/sprites.bmp", "font/PressStart2P.ttf");
    Types_InitTypes();
    Types_InitPlayer(&player);
    Levels_Init();

    game.keystate = SDL_GetKeyboardState(NULL);
    game.state = STATE_PLAYING;
}

void Game_run()
{
    FrameControl_Init(FRAME_RATE, MAX_DELTA_TIME);

    while (game.state != STATE_QUIT)
    {
        Game_ProcessFrame();
        FrameControl_WaitForNextFrame();
    }
}

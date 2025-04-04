/******************************************************************************
 * Copyright (c) Artur Eganyan
 *
 * This software is provided "AS IS", WITHOUT ANY WARRANTY, express or implied.
 ******************************************************************************/

#ifndef GAME_H
#define GAME_H

#include "types.h"

extern Level* level;
extern Player player;

void Game_Init();
void Game_run();

void Game_SetLevel(int r, int c);
void Game_CompleteLevel();

void Game_DamagePlayer(int damage);
void Game_KillPlayer();

#endif // GAME_H

/******************************************************************************
 * Copyright (c) 2015 Artur Eganyan
 *
 * This software is provided "AS IS", WITHOUT ANY WARRANTY, express or implied.
 ******************************************************************************/

#ifndef CREATURES_H
#define CREATURES_H

#include "types.h"

void onInit_Object( Object* o );
void onFrame_Object( Object* o );
void onHit_Object( Object* o, Object* t );

void onInit_Enemy( Object* e );
void onFrame_Enemy( Object* e );
void onHit_Enemy( Object* e, Object* player );

void onInit_EnemyShooter( Object* e );
void onFrame_EnemyShooter( Object* e );

void onInit_Shot( Object* e );
void onFrame_Shot( Object* e );
void onHit_Shot( Object* e, Object* player );

void onInit_Bat( Object* e );
void onFrame_Bat( Object* e );
void onHit_Bat( Object* e, Object* player );

void onHit_Item( Object* item, Object* target );

void onInit_Drop( Object* e );
void onFrame_Drop( Object* e );
void onHit_Drop( Object* e, Object* player );

void onInit_Fireball( Object* e );
void onFrame_Fireball( Object* e );

#endif

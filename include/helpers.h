/******************************************************************************
 * Copyright (c) Artur Eganyan
 *
 * This software is provided "AS IS", WITHOUT ANY WARRANTY, express or implied.
 ******************************************************************************/

#ifndef HELPERS_H
#define HELPERS_H

#include <stdbool.h>
#include "types.h"

bool Util_IsCellValid(int r, int c);
bool Util_IsSolid(int r, int c, int flags);
bool Util_IsLadder(int r, int c);
bool Util_isSolidLadder(int r, int c);
bool Util_IsWater(int r, int c);
bool Util_CellContains(int r, int c, ObjectTypeId generalType);
bool Util_HitTest(const Object* object1, const Object* object2);

void Util_GetObjectCell(const Object* object, int* r, int* c);
void Util_GetObjectBody(const Object* object, Borders* body);
void Util_GetObjectPos(const Object* object, int* r, int* c, Borders* cell, Borders* body);

bool Util_FindNearDoor(int* r, int* c);
Object* Util_FindNearItem(int r, int c);
Object* Util_FindObject(Level* level, ObjectTypeId typeId);

double Util_LimitAbs(double value, double max);
void Util_EnsureSDL(int condition, const char* message);

#endif // HELPERS_H

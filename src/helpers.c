/******************************************************************************
 * Copyright (c) Artur Eganyan
 *
 * This software is provided "AS IS", WITHOUT ANY WARRANTY, express or implied.
 ******************************************************************************/

#include "helpers.h"
#include "game.h"
#include "levels.h"
#include <SDL_error.h>

bool Util_IsCellValid(int r, int c)
{
    return r >= 0 && r < ROW_COUNT && c >= 0 && c < COLUMN_COUNT;
}

bool Util_IsSolid(int r, int c, int flags)
{
    return Util_IsCellValid(r, c)
        ? (level->cells[r][c]->solid & flags) == flags
        : false;
}

bool Util_IsLadder(int r, int c)
{
    return Util_IsCellValid(r, c)
        ? level->cells[r][c]->generalTypeId == TYPE_LADDER
        : false;
}

// Returns 1 if there is a ladder at (r, c) and player can stay on it
bool Util_isSolidLadder(int r, int c)
{
    return Util_IsLadder(r, c) &&
        (Util_IsSolid(r, c - 1, SOLID_TOP) || Util_IsSolid(r, c + 1, SOLID_TOP) || !Util_IsLadder(r - 1, c));
}

bool Util_IsWater(int r, int c)
{
    return Util_IsCellValid(r, c) ? level->cells[r][c]->generalTypeId == TYPE_WATER : 0;
}

bool Util_CellContains(int r, int c, ObjectTypeId generalType)
{
    return Util_IsCellValid(r, c) ? level->cells[r][c]->generalTypeId == generalType : 0;
}

bool Util_HitTest(const Object* object1, const Object* object2)
{
    const SDL_Rect o1 = object1->type->body;
    const SDL_Rect o2 = object2->type->body;

    return (fabs((object1->x + o1.x + o1.w / 2.0) - (object2->x + o2.x + o2.w / 2.0)) < (o1.w + o2.w) / 2.0 &&
            fabs((object1->y + o1.y + o1.h / 2.0) - (object2->y + o2.y + o2.h / 2.0)) < (o1.h + o2.h) / 2.0);
}

void Util_GetObjectCell(const Object* object, int* r, int* c)
{
    const SDL_Rect body = object->type->body;
    *r = (object->y + body.y + body.h / 2.0) / CELL_SIZE;
    *c = (object->x + body.x + body.w / 2.0) / CELL_SIZE;
}

void Util_GetObjectBody(const Object* object, Borders* borders)
{
    const SDL_Rect body = object->type->body;

    borders->left = object->x + body.x;
    borders->right = borders->left + body.w;
    borders->top = object->y + body.y;
    borders->bottom = borders->top + body.h;
}

void Util_GetObjectPos(const Object* object, int* r, int* c, Borders* cell, Borders* body)
{
    Util_GetObjectCell(object, r, c);
    Util_GetObjectBody(object, body);

    cell->left = CELL_SIZE * (*c);
    cell->right = cell->left + CELL_SIZE;
    cell->top = CELL_SIZE * (*r);
    cell->bottom = cell->top + CELL_SIZE;
}

bool Util_FindNearDoor(int* r, int* c)
{
    const int r0 = *r;
    const int c0 = *c;

    if (Util_CellContains(r0, c0, TYPE_DOOR))
    {
        return true;
    }

    if (Util_CellContains(r0, c0 - 1, TYPE_DOOR))
    {
        *c = c0 - 1;
        return true;
    }

    if (Util_CellContains(r0, c0 + 1, TYPE_DOOR))
    {
        *c = c0 + 1;
        return true;
    }

    return false;
}

Object* Util_FindNearItem(int r, int c)
{
    // for (ObjectListNode* iter = level->objects.first; iter != NULL; iter = iter->next)
    for (ListNode* iter = level->objects.first; iter != NULL; iter = iter->next)
    {
        Object* object = iter->data;

        if (object->type->generalTypeId == TYPE_ITEM && !object->removed)
        {
            int or, oc;

            Util_GetObjectCell(object, &or, &oc);

            if (or == r && oc == c)
            {
                return object;
            }
        }
    }

    return NULL;
}

Object* Util_FindObject(Level* level, ObjectTypeId typeId)
{
    // for (ObjectListNode* iter = level->objects.first; iter != NULL; iter = iter->next)
    for (ListNode* iter = level->objects.first; iter != NULL; iter = iter->next)
    {
        Object* object = iter->data;

        if (object->type->typeId == typeId)
        {
            return object;
        }
    }
    return NULL;
}

double Util_LimitAbs(double value, double max)
{
    return (value >  max) ?  max :
           (value < -max) ? -max :
           value;
}

void Util_EnsureSDL(int condition, const char* message)
{
    if (!condition)
    {
        fprintf(stderr, "%s | SDL_Error: %s", message, SDL_GetError());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", message, NULL);
        exit(EXIT_FAILURE);
    }
}

/******************************************************************************
 * Copyright (c) 2015 Artur Eganyan
 *
 * This software is provided "AS IS", WITHOUT ANY WARRANTY, express or implied.
 ******************************************************************************/

#include "objects.h"
#include "render.h"
#include "helpers.h"
#include "levels.h"
#include "game.h"
#include <math.h>


// Helpers

typedef enum
{
    DIRECTION_NONE = 0x0000,
    DIRECTION_LEFT = 0x0001,
    DIRECTION_RIGHT = 0x0010,
    DIRECTION_UP = 0x0100,
    DIRECTION_DOWN = 0x1000,
    DIRECTION_X = DIRECTION_LEFT | DIRECTION_RIGHT,
    DIRECTION_Y = DIRECTION_UP | DIRECTION_DOWN,
    DIRECTION_XY = DIRECTION_X | DIRECTION_Y
} Direction;

// Moves the object by dx and dy, checking the floor if checkFloor != 0.
// Returns 0 on success, otherwise returns flags indicating if object
// could not move by dx (flag DIRECTION_X) or dy (flag DIRECTION_Y).
static int move( Object* object, int dx, int dy, int checkFloor )
{
    int r, c; Borders cell, body;
    int result = 0;

    object->x += dx;
    object->y += dy;
    getObjectPos(object, &r, &c, &cell, &body);

    if (isSolid(r, c, SOLID_ALL)) {
        object->x -= dx;
        object->y -= dy;
        return DIRECTION_XY;
    }
    if (dx > 0 && body.right > cell.right) {
        if (isSolid(r, c + 1, SOLID_LEFT) || body.right > LEVEL_WIDTH || (checkFloor && !isSolidOrLadder(r + 1, c + 1))) {
            object->x -= dx;
            result |= DIRECTION_X;
        }
    } else if (dx < 0 && body.left < cell.left) {
        if (isSolid(r, c - 1, SOLID_RIGHT) || body.left < 0 || (checkFloor && !isSolidOrLadder(r + 1, c - 1))) {
            object->x -= dx;
            result |= DIRECTION_X;
        }
    }
    if (dy > 0 && body.bottom > cell.bottom) {
        if (isSolid(r + 1, c, SOLID_TOP) || body.bottom > LEVEL_HEIGHT) {
            object->y -= dy;
            result |= DIRECTION_Y;
        }
    } else if (dy < 0 && body.top < cell.top) {
        if (isSolid(r - 1, c, SOLID_BOTTOM) || body.top < 0) {
            object->y -= dy;
            result |= DIRECTION_Y;
        }
    }
    return result;
}

// Returns 1 if the source sees the target
static int isVisible( Object* source, Object* target )
{
    if (target->y + CELL_SIZE > source->y + CELL_HALF &&
        target->y < source->y + CELL_HALF) {
        int x1, x2;
        if (target->x < source->x && source->anim.direction < 0) {
            x1 = target->x;
            x2 = source->x;
        } else if (target->x > source->x && source->anim.direction > 0) {
            x1 = source->x;
            x2 = target->x;
        } else {
            return 0;
        }
        const int r = (source->y + CELL_HALF) / CELL_SIZE;
        for (x1 = x1 + CELL_HALF; x1 < x2; x1 += CELL_SIZE) {
            const int c = x1 / CELL_SIZE;
            if (isSolid(r, c, SOLID_LEFT | SOLID_RIGHT)) {
                return 0;
            }
        }
        return 1;
    }
    return 0;
}


// Logic

/*
 * Any object has the integer field "state". It indicates the current object's
 * state and is usually increased on every frame. The states are going in
 * ascending order as follows:
 *
 * 0            STATE_1         STATE_2         STATE_N
 * |---------------|-----x---------|----- ... -----|
 *                       |
 *              object->state
 *
 * Values within [0; STATE_1] belongs to STATE_1, within [STATE_1 + 1; STATE2] -
 * to STATE_2, and so on. The state's range size determines how long the object
 * will be in that state.
 *
 * Note: The minimum state value is usually 0, but it can be negative as well.
 */


void Object_onInit( Object* object ) {}
void Object_onFrame( Object* object ) {}
void Object_onHit( Object* object ) {}


static const int ENEMY_STATE_MOVING = MS_TO_FRAMES(10000);
static const int ENEMY_STATE_WAITING = MS_TO_FRAMES(12000);

void Enemy_onInit( Object* e )
{
    const int dir = rand() % 2 ? 1 : -1;
    e->vx = e->type->speed * dir;
    e->anim.direction = dir;
}

void Enemy_onFrame( Object* e )
{
    if (e->state <= ENEMY_STATE_MOVING) {
        if (move(e, e->vx, e->vy, 1)) {
            e->vx = -e->vx;
            e->anim.direction = -e->anim.direction;
        }
        setAnimation(e, 1, 2, 24);

    } else if (e->state <= ENEMY_STATE_WAITING) {
        setAnimation(e, 2, 2, 24);

    } else {
        e->state = ENEMY_STATE_MOVING - rand() % MS_TO_FRAMES(20000);
        if (rand() % 2) {
            e->vx = -e->vx;
            e->anim.direction = -e->anim.direction;
        }
    }

    e->state += 1;
}

void Enemy_onHit( Object* e )
{
    if ((e->vx < 0 && player.x > e->x) || (e->vx > 0 && player.x < e->x)) {
        e->vx = -e->vx;
        e->anim.direction = -e->anim.direction;
    }
    e->state = ENEMY_STATE_MOVING + 1;
    setAnimation(e, 4, 4, 0);
    killPlayer();
}


static const int SHOOTERENEMY_STATE_MOVING = 0;
static const int SHOOTERENEMY_STATE_ATTACK1 = MS_TO_FRAMES(750);
static const int SHOOTERENEMY_STATE_ATTACK2 = MS_TO_FRAMES(1000);

void ShooterEnemy_onFrame( Object* e )
{
    if (e->state <= SHOOTERENEMY_STATE_MOVING) {
        if (isVisible(e, (Object*)&player)) {
            Object* shot = createObject(level, TYPE_ICESHOT, 0, 0);
            shot->x = e->anim.direction > 0 ? e->x + e->type->sprite.w * SIZE_FACTOR :
                                              e->x - shot->type->sprite.w * SIZE_FACTOR;
            shot->y = e->y;
            shot->vx *= e->anim.direction;
            shot->anim.direction = e->anim.direction;
            e->state = SHOOTERENEMY_STATE_MOVING + 1;
        } else if (move(e, e->vx, e->vy, 1)) {
            e->vx = -e->vx;
            e->anim.direction = -e->anim.direction;
        }
        setAnimation(e, 1, 2, 24);

    } else if (e->state <= SHOOTERENEMY_STATE_ATTACK1) {
        setAnimation(e, 4, 4, 24);

    } else if (e->state <= SHOOTERENEMY_STATE_ATTACK2) {
        setAnimation(e, 1, 1, 24);

    } else {
        e->state = SHOOTERENEMY_STATE_MOVING;
    }

    e->state += e->state > SHOOTERENEMY_STATE_MOVING;
}


static const int SHOT_STATE_MOVING = 0;
static const int SHOT_STATE_HIT = MS_TO_FRAMES(170);

void Shot_onInit( Object* e )
{
    e->vx = e->type->speed;
    setAnimation(e, 1, 2, 5);
}

void Shot_onFrame( Object* e )
{
    if (e->state <= SHOT_STATE_MOVING) {
        if (move(e, e->vx, e->vy, 0)) {
            setAnimation(e, 3, 3, 5);
            e->state = SHOT_STATE_MOVING + 1;
        }

    } else if (e->state <= SHOT_STATE_HIT) {
        e->state += 1;

    } else {
        e->removed = 1;
    }
}

void Shot_onHit( Object* e )
{
    setAnimation(e, 3, 3, 5);
    e->state += 1;
    killPlayer();
}


static const int BAT_STATE_MOVING = 0;
static const int BAT_STATE_NEWDIRECTION = CELL_SIZE;

void Bat_onInit( Object* e )
{
    Enemy_onInit(e);
    setAnimation(e, 0, 1, 12);
    e->vy = 1;  // vy must be > 0, so the bat firstly go down,
                // and then return to the previous height
}

void Bat_onFrame( Object* e )
{
    const int vy = e->state % 2 ? e->vy : 0;
    const int m = move(e, e->vx, vy, 0);
    if (m & DIRECTION_X) {
        e->vx = -e->vx;
        e->anim.direction = -e->anim.direction;
    }
    if (m & DIRECTION_Y) {
        e->vy = -e->vy;
    }
    if (++ e->state >= BAT_STATE_NEWDIRECTION) {
        e->state = BAT_STATE_MOVING;
        e->vy = -e->vy;
    }
}

void Bat_onHit( Object* e )
{
    killPlayer();
}


static const int ITEM_STATE_IDLE = 0;
static const int ITEM_STATE_TAKEN = 15;

void Item_onHit( Object* item )
{
    if (item->state <= ITEM_STATE_IDLE) {
        ObjectTypeId generalTypeId = item->type->generalTypeId;

        if (generalTypeId == TYPE_COIN) {
            player.coins += 1;
        } else if (generalTypeId == TYPE_KEY) {
            player.keys += 1;
        } else if (generalTypeId == TYPE_HEART) {
            player.lives += 1;
        } else if (generalTypeId == TYPE_STATUARY) {
            completeLevel();
        } else {
            // \todo Add the item to player.items, for example
        }

        item->state = ITEM_STATE_IDLE + 1;
        item->vy = -7;
        setAnimation(item, 0, 0, 0);
    }
}

void Item_onFrame( Object* item )
{
    if (item->state <= ITEM_STATE_IDLE) {
        // nothing

    } else if (item->state <= ITEM_STATE_TAKEN) {
        item->anim.alpha -= 25;
        if (item->anim.alpha < 0) {
            item->anim.alpha = 0;
        }
        if (item->vy < 0) {
            item->vy += item->state % 2;
        } else {
            item->vy = 0;
        }
        item->y += item->vy;
        item->state += 1;

    } else {
        item->removed = 1;
    }
}


static const int FIREBALL_STATE_MOVING = 0;
static const int FIREBALL_STATE_ATTACK1 = MS_TO_FRAMES(500);
static const int FIREBALL_STATE_ATTACK2 = MS_TO_FRAMES(1000);

void Fireball_onInit( Object* e )
{
    Enemy_onInit(e);
    e->vy = e->vx;
}

void Fireball_onFrame( Object* e )
{
    if (e->state <= FIREBALL_STATE_MOVING) {
        if (isVisible(e, (Object*)&player)) {
            Object* shot = createObject(level, TYPE_FIRESHOT, 0, 0);
            shot->x = e->x + e->anim.direction * 20;
            shot->y = e->y + 2;
            shot->vx *= e->anim.direction;
            shot->anim.direction = e->anim.direction;
            e->state = FIREBALL_STATE_MOVING + 1;
        }
        setAnimation(e, 0, 1, 24);

    } else if (e->state <= FIREBALL_STATE_ATTACK1) {
        setAnimation(e, 4, 4, 24);

    } else if (e->state <= FIREBALL_STATE_ATTACK2) {
        setAnimation(e, 0, 1, 24);

    } else {
        e->state = FIREBALL_STATE_MOVING;
    }

    const int m = move(e, e->vx, e->vy, 0);
    if (m) {
        if (m & DIRECTION_X) e->vx = -e->vx;
        if (m & DIRECTION_Y) e->vy = -e->vy;
        e->anim.direction = e->vx > 0 ? 1 : -1;
    }
    if (rand() % 100 == 99) {
        if (rand() % 2) {
            e->vx = -e->vx;
        }
        e->anim.direction = e->vx > 0 ? 1 : -1;
    }

    e->state += (e->state > FIREBALL_STATE_MOVING);
}


static const int DROP_STATE_CREATE = 0;
static const int DROP_STATE_FALLING = 1;
static const int DROP_STATE_FELL = MS_TO_FRAMES(1000);

void Drop_onInit( Object* e )
{
    e->state = -rand() % MS_TO_FRAMES(2000);
}

void Drop_onFrame( Object* e )
{
    if (e->state == DROP_STATE_CREATE) {
        Object* drop = createObject(level, TYPE_DROP, 0, 0);
        drop->x = e->x;
        drop->y = e->y;
        drop->state = DROP_STATE_FALLING;
        e->state -= rand() % MS_TO_FRAMES(10000);

    } else if (e->state == DROP_STATE_FALLING) {
        if (e->vy < 5) {
            e->vy += 1;
        }
        if (move(e, 0, e->vy, 0)) {
            e->y += e->vy;
            e->state += 1;
        } else {
            e->state -= 1;
        }

    } else if (e->state > DROP_STATE_FELL) {
        e->removed = 1;
    }

    e->state += 1;
}

void Drop_onHit( Object* e )
{
    killPlayer();
}


void Spider_onFrame( Object* e )
{
    Enemy_onFrame(e);
    if (rand() % 100 == 99) {
        e->vx = (abs(e->vx) == 2 ? 1 : 2) * e->anim.direction;
    }
}


static const int TELEPORTINGENEMY_STATE_MOVING = MS_TO_FRAMES(4000);
static const int TELEPORTINGENEMY_STATE_TELEPORTING = MS_TO_FRAMES(7000);
static const int TELEPORTINGENEMY_STATE_WAITING = MS_TO_FRAMES(7000);

void TeleportingEnemy_onFrame( Object* e )
{
    if (e->state <= TELEPORTINGENEMY_STATE_MOVING) {
        if (move(e, e->vx, e->vy, 1)) {
            e->vx = -e->vx;
            e->anim.direction = -e->anim.direction;
        }
        setAnimation(e, 1, 2, 24);

    } else if (e->state < TELEPORTINGENEMY_STATE_TELEPORTING) {
        setAnimation(e, 2, 2, 24);

    } else if (e->state == TELEPORTINGENEMY_STATE_TELEPORTING) {
        int r, c;
        int prevr = (e->y + CELL_HALF) / CELL_SIZE;
        int count = CELL_COUNT;
        while (count --) {
            r = rand() % (ROW_COUNT - 1);
            c = rand() % COLUMN_COUNT;
            if (isSolid(r + 1, c, SOLID_TOP) && !isSolid(r, c, SOLID_ALL) && r != prevr) {
                e->y = CELL_SIZE * r;
                e->x = CELL_SIZE * c;
            }
        }

    } else if (e->state <= TELEPORTINGENEMY_STATE_WAITING) {
        setAnimation(e, 2, 2, 24);

    } else {
        e->state = -rand() % MS_TO_FRAMES(2000);
    }

    e->state += 1;
}

void TeleportingEnemy_onHit( Object* e )
{
    if (e->state <= TELEPORTINGENEMY_STATE_MOVING) {
        Enemy_onHit(e);
    }
}


void Platform_onInit( Object* e )
{
    e->vx = e->type->speed;
    e->vy = 0;
}

void Platform_onFrame( Object* e )
{
    if (move(e, e->vx, e->vy, 0)) {
        e->vx = -e->vx;
        e->vy = -e->vy;
    }
}

void Platform_onHit( Object* e )
{
    const int dw = (CELL_SIZE - PLAYER_WIDTH) / 2;
    const int dh = (CELL_SIZE - PLAYER_HEIGHT) / 2;
    const int border = 5;
    int pr, pc; Borders pcell, pbody;
    int er, ec; Borders ecell, ebody;

    getObjectPos((Object*)&player, &pr, &pc, &pcell, &pbody);
    getObjectPos(e, &er, &ec, &ecell, &ebody);

    if ((pbody.bottom - ebody.top) > border && (ebody.bottom - pbody.top) > border) {
        if (pbody.right >= ebody.left && pbody.left <= ebody.left) {
            player.x = ebody.left - dw - PLAYER_WIDTH;
        } else if (pbody.left <= ebody.right && pbody.right >= ebody.right) {
            player.x = ebody.right - dw;
        }

    } else if ((pbody.right - ebody.left) > border && (ebody.right - pbody.left) > border) {
        if (pbody.bottom >= ebody.top && pbody.top <= ebody.top) {
            if (!player.vx) {
                player.x += e->vx;
            }
            player.y = ebody.top - dh - PLAYER_HEIGHT;
            player.inAir = 0;
        } else if (pbody.top <= ebody.bottom && pbody.bottom >= ebody.bottom) {
            player.y = ebody.bottom - dh;
        }
    }
}


void Spring_onInit( Object* e )
{
}

void Spring_onFrame( Object* e )
{
    if (e->state > 0) {
        e->state -= 1;
    } else {
        setAnimation(e, 0, 0, 24);
    }
}

void Spring_onHit( Object* e )
{
    const int h = 16;
    int pr, pc; Borders pcell, pbody;
    int er, ec; Borders ecell, ebody;

    getObjectPos((Object*)&player, &pr, &pc, &pcell, &pbody);
    getObjectPos(e, &er, &ec, &ecell, &ebody);

    if (e->state == 0 && pbody.bottom >= ebody.bottom - h && player.vy > 2) {
        player.vy = -15;
        e->state = 50;
        setAnimation(e, 1, 1, 24);
    }
}


void Fan_onInit( Object* e )
{
    setAnimation(e, 0, 3, 2);
}

void Fan_onFrame( Object* e )
{
    const double dx = player.x - e->x;
    const double dy = player.y - e->y;
    const double dr = sqrt(dx * dx + dy * dy);
    const double distance = CELL_SIZE * 2.5;
    if (fabs(dr) < distance) {
        // \todo Something better should be done here
        const double r = dr ? dr : 0.1;
        int vx = 2 * dx / r;
        int vy = 3 * dy / r;
        player.vx += vx;
        player.vy += vy;
    }
}


void Cloud_onHit( Object* e )
{
    if (player.y + CELL_HALF < e->y + CELL_SIZE) {
        if (player.vy > 0) {
            player.y -= player.vy - 1;
        } else if (player.vy < 0) {
            //player.y -= player.vy + 2;
        }
        player.inAir = 0;
    }
}


void Torch_onInit( Object* e )
{
    setAnimation(e, 0, 1, 12);
}

void Torch_onHit( Object* e )
{
}


void Water_onInit( Object* e )
{
    setAnimation(e, 0, 15, 2);
    e->anim.wave = 1;
}

void Water_onHit( Object* e )
{
    int er, ec;
    getObjectCell(e, &er, &ec);

    int pr, pc;
    getObjectCell((Object*)&player, &pr, &pc);

    if (er == pr) {
        killPlayer();
    }
}

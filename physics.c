#include <stdlib.h>
#include "lavalamp.h"

/* Simple pseudo-random in [0, FX_ONE) using fixed point */
static fx_t frand(void) {
    return (fx_t)(rand() & 0xFFFF) << (FX_SHIFT - 16);
}

void physics_init(blob_t *blobs, settings_t *settings) {
    int i;
    for (i = 0; i < MAX_BLOBS; i++) {
        blobs[i].alive = (i < settings->bubble_count);
        blobs[i].x = TO_FX(FIELD_W / 2) + (frand() - TO_FX(0.5f)) * 40;
        blobs[i].y = TO_FX(FIELD_H - 10) - (frand() * (FIELD_H - 20));
        blobs[i].vx = 0;
        blobs[i].vy = 0;
        blobs[i].radius = TO_FX(8) + (frand() * 10);
        blobs[i].temp = frand();
        blobs[i].wobble_phase = frand() * 6;
        blobs[i].wobble_speed = TO_FX(0.5f) + (frand() * TO_FX(1.0f)) / FX_ONE;
    }
}

/* Heats up near the bottom (the "base light"), cools near the top.
 * Rising blobs drift side to side via a slow sine wobble, and lose heat
 * as they climb until they're cold enough to sink back down -- the
 * classic lava lamp cycle. */
void physics_update(blob_t *blobs, settings_t *settings, fx_t dt) {
    int i, j;
    fx_t base_y = TO_FX(FIELD_H - 6);
    fx_t top_y  = TO_FX(6);

    for (i = 0; i < MAX_BLOBS; i++) {
        blob_t *b = &blobs[i];
        if (!b->alive) continue;

        /* heat exchange with surroundings */
        if (b->y > base_y - TO_FX(20)) {
            /* near the base lamp -> absorbs heat, scaled by settings->heat */
            b->temp += FX_MUL(TO_FX(0.02f), settings->heat) ;
        } else if (b->y < top_y + TO_FX(30)) {
            /* near the top / away from light -> cools off */
            b->temp -= TO_FX(0.015f);
        } else {
            b->temp -= TO_FX(0.002f); /* slow ambient cooling in transit */
        }
        if (b->temp > FX_ONE) b->temp = FX_ONE;
        if (b->temp < 0) b->temp = 0;

        /* buoyancy: hot blobs rise, cold blobs sink, damped for a lazy drift */
        fx_t buoyancy = FX_MUL(b->temp - TO_FX(0.4f), TO_FX(0.06f));
        b->vy = FX_MUL(b->vy + buoyancy, TO_FX(0.9f));

        /* gentle horizontal wobble so blobs don't move in a straight line */
        b->wobble_phase += FX_MUL(b->wobble_speed, dt);
        fx_t wobble = FX_MUL(TO_FX(0.35f),
                        TO_FX((float)__builtin_sinf(FX_TO_INT(b->wobble_phase * 1000) / 1000.0f)));
        b->vx = FX_MUL(b->vx + wobble, TO_FX(0.85f));

        b->x += FX_MUL(b->vx, dt);
        b->y -= FX_MUL(b->vy, dt); /* vy positive = rising = smaller y */

        /* soft walls */
        if (b->x < TO_FX(12)) { b->x = TO_FX(12); b->vx = -b->vx / 2; }
        if (b->x > TO_FX(FIELD_W - 12)) { b->x = TO_FX(FIELD_W - 12); b->vx = -b->vx / 2; }
        if (b->y < top_y) { b->y = top_y; b->vy = -b->vy / 2; }
        if (b->y > base_y) { b->y = base_y; b->vy = -b->vy / 2; }
    }

    /* occasional merge: two close, similarly-hot blobs combine into one
     * bigger blob, freeing a slot that respawns at the base -- keeps the
     * lamp looking alive instead of static */
    for (i = 0; i < MAX_BLOBS; i++) {
        blob_t *a = &blobs[i];
        if (!a->alive) continue;
        for (j = i + 1; j < MAX_BLOBS; j++) {
            blob_t *b = &blobs[j];
            if (!b->alive) continue;
            fx_t dx = a->x - b->x, dy = a->y - b->y;
            fx_t dist2 = FX_MUL(dx, dx) + FX_MUL(dy, dy);
            fx_t touch = FX_MUL(a->radius + b->radius, TO_FX(0.5f));
            if (dist2 < FX_MUL(touch, touch) && (rand() & 0x3FF) == 0) {
                a->radius = TO_FX((float)__builtin_sqrtf(
                    (float)FX_TO_INT(FX_MUL(a->radius, a->radius) + FX_MUL(b->radius, b->radius))));
                a->temp = (a->temp + b->temp) / 2;
                b->alive = 0;
                b->y = TO_FX(FIELD_H - 4);
                b->x = TO_FX(FIELD_W / 2) + (frand() - TO_FX(0.5f)) * 30;
                b->radius = TO_FX(6) + (frand() * 8);
                b->temp = TO_FX(0.9f);
                b->alive = (j < settings->bubble_count); /* respawn only if slot in use */
            }
        }
    }
}

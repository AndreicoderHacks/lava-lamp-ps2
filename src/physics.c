#include <stdlib.h>
#include "lavalamp.h"

/* Simple pseudo-random in [0, FX_ONE) using fixed point */
static fx_t frand(void) {
    return (fx_t)(rand() & 0xFFFF) << (FX_SHIFT - 16);
}

/* counts every bubble spawned from the bottom; every 10th one is a
 * bigger "combo" bubble, per your request */
static int s_spawn_count = 0;

static void spawn_blob(blob_t *b) {
    s_spawn_count++;
    int is_combo  = (s_spawn_count % 10 == 0);
    /* occasionally a bubble starts at the top and sinks down instead --
     * never for combo bubbles, those always rise */
    int is_sinker = !is_combo && ((rand() % 8) == 0);

    b->radius = is_combo
        ? COMBO_BLOB_RADIUS
        : MIN_BLOB_RADIUS + FX_MUL(frand(), MAX_BLOB_RADIUS - MIN_BLOB_RADIUS);

    b->wobble_phase = frand() * 6;
    b->wobble_speed = TO_FX(0.4f) + FX_MUL(frand(), TO_FX(1.2f));
    b->vx = 0;

    /* randomized per-blob speed so they don't all move in lockstep */
    fx_t base_speed = TO_FX(6) + FX_MUL(frand(), TO_FX(10));
    b->x = TO_FX(FIELD_W / 4) + FX_MUL(frand(), TO_FX(FIELD_W / 2));

    if (is_sinker) {
        b->rising = 0;
        b->y  = TO_FX(10) + FX_MUL(frand(), TO_FX(14));  /* starts near the top */
        b->vy = FX_MUL(base_speed, TO_FX(0.6f));           /* a bit lazier than rising ones */
    } else {
        b->rising = 1;
        b->y  = TO_FX(FIELD_H - 8) - FX_MUL(frand(), TO_FX(10)); /* starts at the very bottom */
        b->vy = base_speed;
    }
    b->alive = 1;
}

void physics_init(blob_t *blobs, settings_t *settings) {
    int i;
    s_spawn_count = 0;
    for (i = 0; i < MAX_BLOBS; i++) {
        if (i < settings->bubble_count) {
            spawn_blob(&blobs[i]);
            /* scatter initial positions through the tube so it doesn't
             * look like everything just started at the bottom at once */
            blobs[i].y = TO_FX(FIELD_H - 10) - FX_MUL(frand(), TO_FX(FIELD_H - 20));
        } else {
            blobs[i].alive = 0;
        }
    }
}

void physics_update(blob_t *blobs, settings_t *settings, fx_t dt) {
    int i;

    /* keep the live blob count in sync with the "Bule" menu setting */
    {
        int alive_count = 0;
        for (i = 0; i < MAX_BLOBS; i++) if (blobs[i].alive) alive_count++;
        for (i = 0; i < MAX_BLOBS && alive_count < settings->bubble_count; i++) {
            if (!blobs[i].alive) { spawn_blob(&blobs[i]); alive_count++; }
        }
        for (i = MAX_BLOBS - 1; i >= 0 && alive_count > settings->bubble_count; i--) {
            if (blobs[i].alive) { blobs[i].alive = 0; alive_count--; }
        }
    }

    for (i = 0; i < MAX_BLOBS; i++) {
        blob_t *b = &blobs[i];
        if (!b->alive) continue;

        /* gentle horizontal wobble, independent of vertical travel */
        b->wobble_phase += FX_MUL(b->wobble_speed, dt);
        fx_t wobble = FX_MUL(TO_FX(0.35f),
                        TO_FX((float)__builtin_sinf(FX_TO_INT(b->wobble_phase * 1000) / 1000.0f)));
        b->vx = FX_MUL(b->vx + wobble, TO_FX(0.85f));
        b->x += FX_MUL(b->vx, dt);

        /* straight vertical travel; "Caldura" in the menu scales how fast
         * bubbles move, both rising and sinking */
        fx_t vy = FX_MUL(b->vy, settings->heat);

        if (b->rising) {
            b->y -= FX_MUL(vy, dt);
            if (b->y <= TO_FX(8)) {
                /* reached the top -> this bubble is done, a fresh one
                 * enters from the bottom (keeps the combo counter going) */
                spawn_blob(b);
            }
        } else {
            b->y += FX_MUL(vy, dt);
            if (b->y >= TO_FX(FIELD_H - 8)) {
                /* reached the bottom -> "reheats" and turns to rise */
                b->rising = 1;
                b->y = TO_FX(FIELD_H - 8);
                b->vy = TO_FX(6) + FX_MUL(frand(), TO_FX(10));
            }
        }

        /* soft side walls for the wobble */
        if (b->x < TO_FX(12)) { b->x = TO_FX(12); b->vx = -b->vx / 2; }
        if (b->x > TO_FX(FIELD_W - 12)) { b->x = TO_FX(FIELD_W - 12); b->vx = -b->vx / 2; }
    }
}

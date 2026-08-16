#include <stdlib.h>
#include "lavalamp.h"

/* Simple pseudo-random in [0, FX_ONE) using fixed point */
static fx_t frand(void) {
    return (fx_t)(rand() & 0xFFFF) << (FX_SHIFT - 16);
}

static void respawn_at_base(blob_t *b) {
    b->x = TO_FX(FIELD_W / 4) + (frand() * (FIELD_W / 2));
    b->y = TO_FX(FIELD_H - 4) - (frand() * TO_FX(6));
    b->vx = 0;
    b->vy = 0;
    /* varied sizes -- small ones far more common than big ones, like a
     * real lamp, where most of what you see is little bubbles */
    b->radius = MIN_BLOB_RADIUS + FX_MUL(frand(), FX_MUL(frand(), TO_FX(11)));
    b->temp = TO_FX(0.85f) + FX_MUL(frand(), TO_FX(0.15f));
    b->wobble_phase = frand() * 6;
    b->wobble_speed = TO_FX(0.4f) + FX_MUL(frand(), TO_FX(1.2f));
    b->merge_cooldown = 30 + (rand() % 60); /* half..1.5s grace period, can't immediately re-merge */
    b->split_timer = 0;
    b->alive = 1;
}

void physics_init(blob_t *blobs, settings_t *settings) {
    int i;
    for (i = 0; i < MAX_BLOBS; i++) {
        if (i < settings->bubble_count) {
            respawn_at_base(&blobs[i]);
            /* scatter initial blobs through the tube instead of all at the base */
            blobs[i].y = TO_FX(FIELD_H - 10) - FX_MUL(frand(), TO_FX(FIELD_H - 20));
        } else {
            blobs[i].alive = 0;
        }
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

    /* keep the live blob count in sync with the "Bule" menu setting:
     * activate more from the base, or quietly retire extras */
    {
        int alive_count = 0;
        for (i = 0; i < MAX_BLOBS; i++) if (blobs[i].alive) alive_count++;
        for (i = 0; i < MAX_BLOBS && alive_count < settings->bubble_count; i++) {
            if (!blobs[i].alive) { respawn_at_base(&blobs[i]); alive_count++; }
        }
        for (i = MAX_BLOBS - 1; i >= 0 && alive_count > settings->bubble_count; i--) {
            if (blobs[i].alive) { blobs[i].alive = 0; alive_count--; }
        }
    }

    for (i = 0; i < MAX_BLOBS; i++) {
        blob_t *b = &blobs[i];
        if (!b->alive) continue;

        if (b->merge_cooldown > 0) b->merge_cooldown--;

        /* heat exchange with surroundings */
        if (b->y > base_y - TO_FX(20)) {
            /* near the base lamp -> absorbs heat, scaled by settings->heat */
            b->temp += FX_MUL(TO_FX(0.03f), settings->heat);
        } else if (b->y < top_y + TO_FX(30)) {
            /* near the top / away from light -> cools off */
            b->temp -= TO_FX(0.02f);
        } else {
            b->temp -= TO_FX(0.003f); /* slow ambient cooling in transit */
        }
        if (b->temp > FX_ONE) b->temp = FX_ONE;
        if (b->temp < 0) b->temp = 0;

        /* buoyancy: hot blobs rise, cold blobs sink, damped for a lazy drift.
         * Smaller blobs bob a bit faster than big ones, like real wax. */
        fx_t size_factor = FX_DIV(TO_FX(10), b->radius + TO_FX(4));
        fx_t buoyancy = FX_MUL(FX_MUL(b->temp - TO_FX(0.4f), TO_FX(0.07f)), size_factor);
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

        /* an oversized blob (result of merges) doesn't stay big forever --
         * after a short timer it splits back into two smaller blobs, which
         * is what keeps the lamp cycling instead of turning into one huge
         * puddle that eats the screen */
        if (b->radius >= MAX_BLOB_RADIUS) {
            if (b->split_timer == 0) b->split_timer = 90 + (rand() % 90); /* 1.5-3s */
            b->split_timer--;
            if (b->split_timer <= 0) {
                /* find a free slot to hold the second half */
                for (j = 0; j < MAX_BLOBS; j++) {
                    if (!blobs[j].alive) {
                        fx_t half = FX_MUL(b->radius, TO_FX(0.7f));
                        if (half < MIN_BLOB_RADIUS) half = MIN_BLOB_RADIUS;
                        b->radius = half;
                        b->merge_cooldown = 45;
                        b->split_timer = 0;

                        blobs[j].x = b->x + TO_FX(4) - frand() * TO_FX(8);
                        blobs[j].y = b->y;
                        blobs[j].vx = -b->vx;
                        blobs[j].vy = FX_MUL(b->vy, TO_FX(0.5f));
                        blobs[j].radius = half;
                        blobs[j].temp = b->temp;
                        blobs[j].wobble_phase = b->wobble_phase + TO_FX(2);
                        blobs[j].wobble_speed = b->wobble_speed;
                        blobs[j].merge_cooldown = 45;
                        blobs[j].split_timer = 0;
                        blobs[j].alive = 1;
                        break;
                    }
                }
                b->split_timer = 0;
            }
        } else {
            b->split_timer = 0;
        }
    }

    /* occasional merge: two close blobs that are both off cooldown combine
     * into one bigger blob (capped -- see split logic above) */
    for (i = 0; i < MAX_BLOBS; i++) {
        blob_t *a = &blobs[i];
        if (!a->alive || a->merge_cooldown > 0) continue;
        for (j = i + 1; j < MAX_BLOBS; j++) {
            blob_t *b = &blobs[j];
            if (!b->alive || b->merge_cooldown > 0) continue;
            if (a->radius >= MAX_BLOB_RADIUS || b->radius >= MAX_BLOB_RADIUS) continue;

            fx_t dx = a->x - b->x, dy = a->y - b->y;
            fx_t dist2 = FX_MUL(dx, dx) + FX_MUL(dy, dy);
            fx_t touch = FX_MUL(a->radius + b->radius, TO_FX(0.55f));
            if (dist2 < FX_MUL(touch, touch) && (rand() & 0x1F) == 0) {
                fx_t combined = TO_FX((float)__builtin_sqrtf(
                    (float)FX_TO_INT(FX_MUL(a->radius, a->radius) + FX_MUL(b->radius, b->radius))));
                if (combined > MAX_BLOB_RADIUS) combined = MAX_BLOB_RADIUS;
                a->radius = combined;
                a->temp = (a->temp + b->temp) / 2;
                a->vx = (a->vx + b->vx) / 2;
                a->vy = (a->vy + b->vy) / 2;
                a->merge_cooldown = 45;

                /* freed blob re-enters at the base, continuing the flow
                 * instead of just vanishing */
                respawn_at_base(b);
                break; /* a's geometry changed, re-check it against later blobs next frame */
            }
        }
    }
}

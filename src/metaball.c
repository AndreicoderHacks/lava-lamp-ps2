#include "lavalamp.h"

/* Metaball "iso-field": for each field pixel sum radius^2 / dist^2 from
 * every alive blob. Where the sum crosses a threshold, we're inside the
 * liquid; near the threshold we blend liquid colour into glow colour for
 * a soft edge instead of a hard cutoff (cheap fake anti-aliasing). */

#define ISO_THRESHOLD   TO_FX(1)
#define ISO_EDGE_SOFT   TO_FX(0.35f)

void field_render(const blob_t *blobs, const settings_t *settings, u8 *out) {
    const lamp_colour_t *col = &g_palette[settings->colour_index];
    int px, py, i;

    for (py = 0; py < FIELD_H; py++) {
        /* base glow is strongest near the bottom, fading toward the top */
        fx_t glow_here = FX_MUL(settings->light_intensity,
                            FX_DIV(TO_FX(FIELD_H - py), TO_FX(FIELD_H)));

        for (px = 0; px < FIELD_W; px++) {
            fx_t sum = 0;
            fx_t fx = TO_FX(px), fy = TO_FX(py);

            for (i = 0; i < MAX_BLOBS; i++) {
                const blob_t *b = &blobs[i];
                if (!b->alive) continue;
                fx_t dx = fx - b->x, dy = fy - b->y;
                fx_t dist2 = FX_MUL(dx, dx) + FX_MUL(dy, dy);
                if (dist2 < TO_FX(1)) dist2 = TO_FX(1); /* avoid div by 0 */
                fx_t r2 = FX_MUL(b->radius, b->radius);
                sum += FX_DIV(r2, dist2);
            }

            u8 *px_out = &out[(py * FIELD_W + px) * 4];

            if (sum > ISO_THRESHOLD + ISO_EDGE_SOFT) {
                /* fully inside a blob */
                px_out[0] = col->r;
                px_out[1] = col->g;
                px_out[2] = col->b;
                px_out[3] = 0x80; /* GS alpha is 0-0x80 */
            } else if (sum > ISO_THRESHOLD - ISO_EDGE_SOFT) {
                /* soft edge: blend liquid colour toward glow/background */
                fx_t t = FX_DIV(sum - (ISO_THRESHOLD - ISO_EDGE_SOFT), ISO_EDGE_SOFT * 2);
                int blend_r = col->gr + (((col->r - col->gr) * FX_TO_INT(t * 256)) >> 8);
                int blend_g = col->gg + (((col->g - col->gg) * FX_TO_INT(t * 256)) >> 8);
                int blend_b = col->gb + (((col->b - col->gb) * FX_TO_INT(t * 256)) >> 8);
                px_out[0] = blend_r;
                px_out[1] = blend_g;
                px_out[2] = blend_b;
                px_out[3] = (u8)FX_TO_INT(t * 0x80);
            } else if (settings->glow_enabled) {
                /* background glow from the base light, no liquid here */
                fx_t g = FX_MUL(glow_here, TO_FX(0.5f));
                px_out[0] = (u8)FX_TO_INT(FX_MUL(TO_FX(col->gr), g));
                px_out[1] = (u8)FX_TO_INT(FX_MUL(TO_FX(col->gg), g));
                px_out[2] = (u8)FX_TO_INT(FX_MUL(TO_FX(col->gb), g));
                px_out[3] = (u8)FX_TO_INT(g * 0x60);
            } else {
                px_out[0] = px_out[1] = px_out[2] = px_out[3] = 0;
            }
        }
    }
}

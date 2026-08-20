#include "lavalamp.h"

/* Metaball "iso-field": for each field pixel sum radius^2 / dist^2 from
 * every alive blob. Where the sum crosses a threshold, we're inside the
 * liquid; near the threshold we blend liquid colour into glow colour for
 * a soft edge instead of a hard cutoff (cheap fake anti-aliasing). */

#define ISO_THRESHOLD   TO_FX(1)
#define ISO_EDGE_SOFT   TO_FX(0.5f)   /* wider soft band = smoother-looking edges */

void field_render(const blob_t *blobs, const settings_t *settings, u8 *out) {
    u8 liquid_r, liquid_g, liquid_b, glow_r, glow_g, glow_b;
    colour_get_liquid(settings, &liquid_r, &liquid_g, &liquid_b);
    colour_get_glow(settings, &glow_r, &glow_g, &glow_b);
    int px, py, i;

    /* precompute radius^2 and a "reach" distance per blob once per frame
     * instead of once per pixel -- a blob's field contribution is
     * negligible past ~5x its radius, so rows/pixels outside that just
     * skip it entirely. This is what keeps the higher-res field cheap. */
    fx_t r2[MAX_BLOBS];
    fx_t reach[MAX_BLOBS];
    for (i = 0; i < MAX_BLOBS; i++) {
        if (!blobs[i].alive) continue;
        r2[i] = FX_MUL(blobs[i].radius, blobs[i].radius);
        reach[i] = FX_MUL(blobs[i].radius, TO_FX(5));
    }

    int row_active[MAX_BLOBS];
    fx_t row_dy2[MAX_BLOBS];

    for (py = 0; py < FIELD_H; py++) {
        fx_t fy = TO_FX(py);
        /* base glow is strongest near the bottom, fading toward the top */
        fx_t glow_here = FX_MUL(settings->light_intensity,
                            FX_DIV(TO_FX(FIELD_H - py), TO_FX(FIELD_H)));

        /* which blobs can possibly reach this row at all? */
        int row_count = 0;
        for (i = 0; i < MAX_BLOBS; i++) {
            if (!blobs[i].alive) continue;
            fx_t dy = fy - blobs[i].y;
            if (dy < 0) dy = -dy;
            if (dy > reach[i]) continue;
            row_active[row_count] = i;
            row_dy2[row_count] = FX_MUL(dy, dy);
            row_count++;
        }

        for (px = 0; px < FIELD_W; px++) {
            fx_t sum = 0;
            fx_t fx = TO_FX(px);
            int k;

            for (k = 0; k < row_count; k++) {
                i = row_active[k];
                fx_t dx = fx - blobs[i].x;
                fx_t dist2 = FX_MUL(dx, dx) + row_dy2[k];
                if (dist2 < TO_FX(1)) dist2 = TO_FX(1); /* avoid div by 0 */
                sum += FX_DIV(r2[i], dist2);
            }

            u8 *px_out = &out[(py * FIELD_W + px) * 4];

            if (sum > ISO_THRESHOLD + ISO_EDGE_SOFT) {
                /* fully inside a blob */
                px_out[0] = liquid_r;
                px_out[1] = liquid_g;
                px_out[2] = liquid_b;
                px_out[3] = 0x80; /* GS alpha is 0-0x80 */
            } else if (sum > ISO_THRESHOLD - ISO_EDGE_SOFT) {
                /* soft edge: blend liquid colour toward glow/background */
                fx_t t = FX_DIV(sum - (ISO_THRESHOLD - ISO_EDGE_SOFT), ISO_EDGE_SOFT * 2);
                int blend_r = glow_r + (((liquid_r - glow_r) * FX_TO_INT(t * 256)) >> 8);
                int blend_g = glow_g + (((liquid_g - glow_g) * FX_TO_INT(t * 256)) >> 8);
                int blend_b = glow_b + (((liquid_b - glow_b) * FX_TO_INT(t * 256)) >> 8);
                px_out[0] = blend_r;
                px_out[1] = blend_g;
                px_out[2] = blend_b;
                px_out[3] = (u8)FX_TO_INT(t * 0x80);
            } else if (settings->glow_enabled) {
                /* background glow from the base light, no liquid here */
                fx_t g = FX_MUL(glow_here, TO_FX(0.5f));
                px_out[0] = (u8)FX_TO_INT(FX_MUL(TO_FX(glow_r), g));
                px_out[1] = (u8)FX_TO_INT(FX_MUL(TO_FX(glow_g), g));
                px_out[2] = (u8)FX_TO_INT(FX_MUL(TO_FX(glow_b), g));
                px_out[3] = (u8)FX_TO_INT(g * 0x60);
            } else {
                px_out[0] = px_out[1] = px_out[2] = px_out[3] = 0;
            }
        }
    }
}

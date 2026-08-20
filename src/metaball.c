#include "lavalamp.h"

/* Metaball "iso-field": for each field pixel sum radius^2 / dist^2 from
 * every alive blob. Where the sum crosses a threshold, we're inside the
 * liquid; near the threshold we blend liquid colour into glow colour for
 * a soft edge instead of a hard cutoff.
 *
 * To keep the edge smooth on real hardware regardless of how the GS ends
 * up filtering the upscaled texture, the anti-aliasing is baked directly
 * into the texture: each output texel is the AVERAGE of 4 sub-samples
 * (a small 2x2 supersample grid) of the iso-field, not a single point
 * sample. That gives every texel its own soft, blended edge value up
 * front, instead of relying on hardware bilinear to smooth hard-edged
 * texels after the fact. */

#define ISO_THRESHOLD   TO_FX(1)
#define ISO_EDGE_SOFT   TO_FX(0.5f)   /* wider soft band = smoother-looking edges */
#define SS_OFFSET       TO_FX(0.25f)  /* 2x2 supersample offsets, in field pixels */

/* sums the iso-field at one sub-sample row (fy) across all blobs that can
 * reach it, for both sub-columns of every output pixel; adds the result
 * into accum[px] (caller clears/owns the accumulator) */
static void accumulate_subrow(const blob_t *blobs, fx_t fy, fx_t *accum) {
    fx_t r2[MAX_BLOBS];
    int row_active[MAX_BLOBS];
    fx_t row_dy2[MAX_BLOBS];
    int row_count = 0, i, px;

    for (i = 0; i < MAX_BLOBS; i++) {
        if (!blobs[i].alive) continue;
        fx_t reach = FX_MUL(blobs[i].radius, TO_FX(5));
        fx_t dy = fy - blobs[i].y;
        if (dy < 0) dy = -dy;
        if (dy > reach) continue;
        r2[row_count] = FX_MUL(blobs[i].radius, blobs[i].radius);
        row_active[row_count] = i;
        row_dy2[row_count] = FX_MUL(dy, dy);
        row_count++;
    }

    for (px = 0; px < FIELD_W; px++) {
        fx_t base_fx = TO_FX(px);
        int k;
        fx_t sum_a = 0, sum_b = 0; /* two sub-columns for this output pixel */

        for (k = 0; k < row_count; k++) {
            i = row_active[k];
            fx_t dxa = (base_fx - SS_OFFSET) - blobs[i].x;
            fx_t dxb = (base_fx + SS_OFFSET) - blobs[i].x;
            fx_t dist2a = FX_MUL(dxa, dxa) + row_dy2[k];
            fx_t dist2b = FX_MUL(dxb, dxb) + row_dy2[k];
            if (dist2a < TO_FX(1)) dist2a = TO_FX(1);
            if (dist2b < TO_FX(1)) dist2b = TO_FX(1);
            sum_a += FX_DIV(r2[k], dist2a);
            sum_b += FX_DIV(r2[k], dist2b);
        }
        accum[px] += sum_a + sum_b; /* two of the four sub-samples for this texel */
    }
}

void field_render(const blob_t *blobs, const settings_t *settings, u8 *out) {
    u8 liquid_r, liquid_g, liquid_b, glow_r, glow_g, glow_b;
    colour_get_liquid(settings, &liquid_r, &liquid_g, &liquid_b);
    colour_get_glow(settings, &glow_r, &glow_g, &glow_b);
    int px, py;

    static fx_t accum[FIELD_W];

    for (py = 0; py < FIELD_H; py++) {
        fx_t fy = TO_FX(py);

        for (px = 0; px < FIELD_W; px++) accum[px] = 0;
        accumulate_subrow(blobs, fy - SS_OFFSET, accum); /* top sub-row (2 samples) */
        accumulate_subrow(blobs, fy + SS_OFFSET, accum); /* bottom sub-row (2 samples) */

        /* base glow is strongest near the bottom, fading toward the top */
        fx_t glow_here = FX_MUL(settings->light_intensity,
                            FX_DIV(TO_FX(FIELD_H - py), TO_FX(FIELD_H)));

        for (px = 0; px < FIELD_W; px++) {
            fx_t sum = accum[px] >> 2; /* average of the 4 sub-samples */

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

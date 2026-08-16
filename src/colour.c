#include "lavalamp.h"

/* HSV -> RGB, hue in fixed point 0..FX_ONE (maps to 0..360deg), s/v fixed
 * at "nice glowing liquid" values. Used only for the colour fader, so it
 * doesn't need to be fast -- called once per frame, not per pixel. */
static void hue_to_rgb(fx_t hue, u8 sat255, u8 val255, u8 *r, u8 *g, u8 *b) {
    float hf = (FX_TO_INT(hue * 6000)) / 1000.0f; /* 0..6.0 */
    int   i  = (int)hf;
    float f  = hf - i;
    float s  = sat255 / 255.0f;
    float v  = val255 / 255.0f;
    float p  = v * (1.0f - s);
    float q  = v * (1.0f - s * f);
    float t  = v * (1.0f - s * (1.0f - f));
    float rf, gf, bf;

    switch (i % 6) {
        case 0:  rf = v; gf = t; bf = p; break;
        case 1:  rf = q; gf = v; bf = p; break;
        case 2:  rf = p; gf = v; bf = t; break;
        case 3:  rf = p; gf = q; bf = v; break;
        case 4:  rf = t; gf = p; bf = v; break;
        default: rf = v; gf = p; bf = q; break;
    }

    *r = (u8)(rf * 255.0f);
    *g = (u8)(gf * 255.0f);
    *b = (u8)(bf * 255.0f);
}

void colour_get_liquid(const settings_t *settings, u8 *r, u8 *g, u8 *b) {
    if (settings->fader_enabled) {
        hue_to_rgb(settings->fader_hue, 220, 235, r, g, b);
    } else {
        const lamp_colour_t *c = &g_palette[settings->colour_index];
        *r = c->r; *g = c->g; *b = c->b;
    }
}

void colour_get_glow(const settings_t *settings, u8 *r, u8 *g, u8 *b) {
    if (settings->fader_enabled) {
        /* glow = same hue, brighter & less saturated, like light through liquid */
        hue_to_rgb(settings->fader_hue, 140, 255, r, g, b);
    } else {
        const lamp_colour_t *c = &g_palette[settings->colour_index];
        *r = c->gr; *g = c->gg; *b = c->gb;
    }
}

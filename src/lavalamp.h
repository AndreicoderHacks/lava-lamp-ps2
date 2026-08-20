#ifndef LAVALAMP_H
#define LAVALAMP_H

#include <tamtypes.h>
#include <gsKit.h>

/* --- field / texture resolution ---------------------------------------
 * The metaball field is computed on the EE at low res, then uploaded as
 * a GS texture and stretched to fill the screen with bilinear filtering.
 * Bumped from 160x120 to 240x180 (2.25x the pixels) for a noticeably
 * smoother/less pixelated look -- still cheap: at ~14 alive blobs that's
 * well under 700k fixed-point ops/frame, comfortable for the EE at 60fps.
 * ---------------------------------------------------------------------*/
#define FIELD_W   240
#define FIELD_H   180

#define SCREEN_W  640
#define SCREEN_H  448

#define MAX_BLOBS 22

/* normal bubble size range, and the size of the rare "combo" bubble
 * (see s_spawn_count in physics.c -- every 10th spawn is a combo) */
#define MIN_BLOB_RADIUS   TO_FX(6)
#define MAX_BLOB_RADIUS   TO_FX(13)
#define COMBO_BLOB_RADIUS TO_FX(24)

/* fixed point helpers (16.16) used in the physics/field code so we avoid
 * float<->int conversions in the hot per-pixel loop */
typedef s32 fx_t;
#define FX_SHIFT 16
#define FX_ONE   (1 << FX_SHIFT)
#define TO_FX(x) ((fx_t)((x) * FX_ONE))
#define FX_TO_INT(x) ((x) >> FX_SHIFT)
#define FX_MUL(a,b) ((fx_t)(((s64)(a) * (s64)(b)) >> FX_SHIFT))
#define FX_DIV(a,b) ((fx_t)(((s64)(a) << FX_SHIFT) / (b)))

/* Each blob just travels in a straight line, bottom<->top, at its own
 * fixed radius and speed -- no radius ever changes at runtime. The
 * "merging" and "separating" look is purely visual, produced by the
 * metaball field in metaball.c when two blobs' circles overlap; there is
 * no persistent merge state to get stuck, which is what used to cause
 * blobs to grow forever and never shrink back. */
typedef struct {
    fx_t x, y;           /* position in field space */
    fx_t vx;              /* horizontal wobble velocity */
    fx_t vy;              /* vertical travel speed (magnitude, always >= 0) */
    fx_t radius;          /* fixed for this blob's lifetime */
    fx_t wobble_phase;
    fx_t wobble_speed;
    u8   rising;          /* 1 = travelling bottom->top, 0 = top->bottom */
    u8   alive;
} blob_t;

/* one entry of the lamp's colour palette: liquid colour + glow colour */
typedef struct {
    u8 r, g, b;      /* blob / liquid colour */
    u8 gr, gg, gb;   /* base-light glow colour */
    const char *name;
} lamp_colour_t;

typedef struct {
    int  colour_index;    /* index into g_palette, used when fader is OFF */
    int  bubble_count;    /* 3..MAX_BLOBS */
    fx_t heat;             /* speed multiplier for rising/sinking bubbles */
    fx_t light_intensity;  /* 0..FX_ONE, glow brightness at the base */
    u8   glow_enabled;
    fx_t speed;             /* global animation speed multiplier, e.g. 0.2..3.0 */
    u8   fader_enabled;     /* when ON, colour continuously cycles through hues */
    fx_t fader_hue;         /* 0..FX_ONE current position in the hue wheel */
    fx_t fader_speed;       /* hue units per second */
} settings_t;

extern const lamp_colour_t g_palette[];
extern const int g_palette_count;

void physics_init(blob_t *blobs, settings_t *settings);
void physics_update(blob_t *blobs, settings_t *settings, fx_t dt);

/* fills rgb (0-255 each) for the current colour -- either the selected
 * palette entry, or the live fader hue when fader_enabled is set */
void colour_get_liquid(const settings_t *settings, u8 *r, u8 *g, u8 *b);
void colour_get_glow(const settings_t *settings, u8 *r, u8 *g, u8 *b);

void field_render(const blob_t *blobs, const settings_t *settings,
                   u8 *rgba_out /* FIELD_W*FIELD_H*4 bytes */);

void menu_update_and_draw(GSGLOBAL *gsGlobal, GSFONTM *gsFontM, settings_t *settings, int visible);

#endif

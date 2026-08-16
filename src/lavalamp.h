#ifndef LAVALAMP_H
#define LAVALAMP_H

#include <tamtypes.h>
#include <gsKit.h>

/* --- field / texture resolution ---------------------------------------
 * The metaball field is computed on the EE at low res, then uploaded as
 * a GS texture and stretched to fill the screen with bilinear filtering.
 * 160x120 keeps the per-pixel blob-distance sum cheap (~19k px) while
 * still looking smooth once magnified with LINEAR filtering.
 * ---------------------------------------------------------------------*/
#define FIELD_W   160
#define FIELD_H   120

#define SCREEN_W  640
#define SCREEN_H  448

#define MAX_BLOBS 22

/* radius (in field-pixels) a merged blob is allowed to reach before it's
 * forced to split back into two -- this is what stops the "one giant
 * blob eats the whole lamp" bug */
#define MAX_BLOB_RADIUS   TO_FX(15)
#define MIN_BLOB_RADIUS   TO_FX(4)

/* fixed point helpers (16.16) used in the physics/field code so we avoid
 * float<->int conversions in the hot per-pixel loop */
typedef s32 fx_t;
#define FX_SHIFT 16
#define FX_ONE   (1 << FX_SHIFT)
#define TO_FX(x) ((fx_t)((x) * FX_ONE))
#define FX_TO_INT(x) ((x) >> FX_SHIFT)
#define FX_MUL(a,b) ((fx_t)(((s64)(a) * (s64)(b)) >> FX_SHIFT))
#define FX_DIV(a,b) ((fx_t)(((s64)(a) << FX_SHIFT) / (b)))

typedef struct {
    fx_t x, y;          /* position in field space */
    fx_t vx, vy;        /* velocity */
    fx_t radius;        /* base radius in field-pixels */
    fx_t temp;           /* 0 = cold (sinks), FX_ONE = hot (rises) */
    fx_t wobble_phase;
    fx_t wobble_speed;
    s16  merge_cooldown; /* frames left before this blob can merge again */
    s16  split_timer;    /* frames left before an oversized blob splits */
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
    fx_t heat;             /* overall lamp temperature, drives rise speed */
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

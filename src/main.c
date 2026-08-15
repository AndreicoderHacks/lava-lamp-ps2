#include <kernel.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <malloc.h>
#include <string.h>
#include "lavalamp.h"

extern int input_init(void);
extern void input_update(void);
extern u32 g_pad_start_pressed;

static blob_t     s_blobs[MAX_BLOBS];
static settings_t s_settings;

/* field buffer, RGBA32, aligned for DMA-friendly texture upload */
static u8 s_field_rgba[FIELD_W * FIELD_H * 4] __attribute__((aligned(64)));

int main(int argc, char *argv[]) {
    GSGLOBAL *gsGlobal = gsKit_init_global();

    gsGlobal->Mode   = GS_MODE_PAL;   /* swap to GS_MODE_NTSC if your unit/TV needs it */
    gsGlobal->Height = SCREEN_H;
    gsGlobal->Width  = SCREEN_W;
    gsGlobal->PSM    = GS_PSM_CT24;
    gsGlobal->PSMZ   = GS_PSMZ_16S;
    gsGlobal->DoubleBuffering = GS_SETTING_ON;
    gsGlobal->ZBuffering      = GS_SETTING_OFF;
    gsGlobal->PrimAlphaEnable = GS_SETTING_ON;

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsKit_vram_clear(gsGlobal);
    gsKit_init_screen(gsGlobal);
    gsKit_mode_switch(gsGlobal, GS_ONESHOT);

    GSFONTM *gsFontM = gsKit_init_fontm();
    gsKit_fontm_upload(gsGlobal, gsFontM);

    /* texture that the metaball field gets uploaded into each frame */
    GSTEXTURE lampTex;
    memset(&lampTex, 0, sizeof(lampTex));
    lampTex.Width    = FIELD_W;
    lampTex.Height   = FIELD_H;
    lampTex.PSM      = GS_PSM_CT32;
    lampTex.Filter   = GS_FILTER_LINEAR;  /* this is what makes the low-res field look smooth */
    lampTex.Mem      = s_field_rgba;
    lampTex.Vram     = gsKit_vram_alloc(gsGlobal,
                            gsKit_texture_size_ee(FIELD_W, FIELD_H, GS_PSM_CT32),
                            GSKIT_ALLOC_USERBUFFER);

    input_init();

    s_settings.colour_index    = 0;
    s_settings.bubble_count    = 8;
    s_settings.heat            = TO_FX(1.0f);
    s_settings.light_intensity = TO_FX(0.8f);
    s_settings.glow_enabled    = 1;

    physics_init(s_blobs, &s_settings);

    int menu_visible = 0;
    fx_t dt = TO_FX(1.0f / 60.0f); /* fixed step; PS2 vsync keeps this steady enough */

    while (1) {
        input_update();

        if (g_pad_start_pressed) menu_visible = !menu_visible;

        if (!menu_visible) {
            physics_update(s_blobs, &s_settings, dt);
        }

        field_render(s_blobs, &s_settings, s_field_rgba);
        gsKit_texture_upload(gsGlobal, &lampTex);

        gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0x10, 0x08, 0x02, 0x00, 0x00));

        /* fullscreen textured quad, GS upscales FIELD_W x FIELD_H -> SCREEN_W x SCREEN_H
         * with bilinear filtering, which is what gives the field its soft/liquid look */
        gsKit_prim_sprite_texture(gsGlobal, &lampTex,
            0, 0,                         /* screen x0,y0 */
            0, 0,                         /* tex u0,v0 */
            SCREEN_W, SCREEN_H,           /* screen x1,y1 */
            FIELD_W, FIELD_H,             /* tex u1,v1 */
            1,                            /* z */
            GS_SETREG_RGBAQ(0x80,0x80,0x80,0x80,0x00));

        /* simple glass-tube vignette: two side bars to suggest the lamp's
         * cylindrical glass rather than a flat rectangle of liquid */
        gsKit_prim_sprite(gsGlobal, 0, 0, 40, SCREEN_H, 1,
            GS_SETREG_RGBAQ(0,0,0,0x50,0x00));
        gsKit_prim_sprite(gsGlobal, SCREEN_W - 40, 0, SCREEN_W, SCREEN_H, 1,
            GS_SETREG_RGBAQ(0,0,0,0x50,0x00));

        menu_update_and_draw(gsGlobal, gsFontM, &s_settings, menu_visible);

        gsKit_queue_exec(gsGlobal);
        gsKit_finish();
        gsKit_sync_flip(gsGlobal);
        gsKit_queue_reset(gsGlobal->Per_Queue);
    }

    return 0;
}

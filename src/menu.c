#include <gsKit.h>
#include "lavalamp.h"

const lamp_colour_t g_palette[] = {
    /* liquid            glow (base light)   name */
    { 220,  40,  40,     255, 140,  40,  "Clasic rosu/portocaliu" },
    { 40,  200, 120,      20, 255, 140,  "Verde radioactiv"       },
    { 60,  90,  230,      140, 190, 255, "Albastru rece"          },
    { 230,  60, 200,      255, 120, 220, "Roz/magenta"            },
    { 240, 210,  30,      255, 240, 140, "Auriu"                  },
};
const int g_palette_count = sizeof(g_palette) / sizeof(g_palette[0]);

/* very small immediate-mode style menu: draws over the lamp when
 * `visible` is true. Navigation: D-pad up/down selects a row,
 * left/right changes the value, START closes the menu (handled by caller). */

typedef enum { ROW_COLOUR = 0, ROW_BUBBLES, ROW_HEAT, ROW_LIGHT, ROW_GLOW, ROW_COUNT } menu_row_t;

static int s_selected_row = 0;

void menu_update_and_draw(GSGLOBAL *gsGlobal, GSFONTM *gsFontM, settings_t *settings, int visible) {
    extern u32 g_pad_dpad_up, g_pad_dpad_down, g_pad_dpad_left, g_pad_dpad_right;

    if (!visible) return;

    if (g_pad_dpad_up)   s_selected_row = (s_selected_row + ROW_COUNT - 1) % ROW_COUNT;
    if (g_pad_dpad_down) s_selected_row = (s_selected_row + 1) % ROW_COUNT;

    switch (s_selected_row) {
        case ROW_COLOUR:
            if (g_pad_dpad_right) settings->colour_index = (settings->colour_index + 1) % g_palette_count;
            if (g_pad_dpad_left)  settings->colour_index = (settings->colour_index + g_palette_count - 1) % g_palette_count;
            break;
        case ROW_BUBBLES:
            if (g_pad_dpad_right && settings->bubble_count < MAX_BLOBS) settings->bubble_count++;
            if (g_pad_dpad_left  && settings->bubble_count > 3) settings->bubble_count--;
            break;
        case ROW_HEAT:
            if (g_pad_dpad_right && settings->heat < TO_FX(2.0f)) settings->heat += TO_FX(0.1f);
            if (g_pad_dpad_left  && settings->heat > TO_FX(0.2f)) settings->heat -= TO_FX(0.1f);
            break;
        case ROW_LIGHT:
            if (g_pad_dpad_right && settings->light_intensity < FX_ONE) settings->light_intensity += TO_FX(0.1f);
            if (g_pad_dpad_left  && settings->light_intensity > 0) settings->light_intensity -= TO_FX(0.1f);
            break;
        case ROW_GLOW:
            if (g_pad_dpad_right || g_pad_dpad_left) settings->glow_enabled = !settings->glow_enabled;
            break;
    }

    /* text drawing uses gsKit's built-in debug font -- fine for a settings
     * overlay; swap for a bitmap font later if you want a nicer look */
    int y = 60;
    gsKit_fontm_print_scaled(gsGlobal, gsFontM, gsGlobal->Width / 2 - 140, y, 2, 0.9f,
        GS_SETREG_RGBAQ(255,255,255,0x80,0x00), "SETARI LAVA LAMP (START = inchide)");
    y += 24;

    char line[64];
    const char *marker;

    marker = (s_selected_row == ROW_COLOUR) ? "> " : "  ";
    __builtin_snprintf(line, sizeof(line), "%sCuloare: %s", marker, g_palette[settings->colour_index].name);
    gsKit_fontm_print_scaled(gsGlobal, gsFontM, 100, y, 2, 0.8f, GS_SETREG_RGBAQ(255,255,0,0x80,0x00), line);
    y += 20;

    marker = (s_selected_row == ROW_BUBBLES) ? "> " : "  ";
    __builtin_snprintf(line, sizeof(line), "%sBule: %d", marker, settings->bubble_count);
    gsKit_fontm_print_scaled(gsGlobal, gsFontM, 100, y, 2, 0.8f, GS_SETREG_RGBAQ(255,255,0,0x80,0x00), line);
    y += 20;

    marker = (s_selected_row == ROW_HEAT) ? "> " : "  ";
    __builtin_snprintf(line, sizeof(line), "%sCaldura: %d%%", marker, (FX_TO_INT(settings->heat * 100)) / FX_ONE);
    gsKit_fontm_print_scaled(gsGlobal, gsFontM, 100, y, 2, 0.8f, GS_SETREG_RGBAQ(255,255,0,0x80,0x00), line);
    y += 20;

    marker = (s_selected_row == ROW_LIGHT) ? "> " : "  ";
    __builtin_snprintf(line, sizeof(line), "%sLumina: %d%%", marker, (FX_TO_INT(settings->light_intensity * 100)) / FX_ONE);
    gsKit_fontm_print_scaled(gsGlobal, gsFontM, 100, y, 2, 0.8f, GS_SETREG_RGBAQ(255,255,0,0x80,0x00), line);
    y += 20;

    marker = (s_selected_row == ROW_GLOW) ? "> " : "  ";
    __builtin_snprintf(line, sizeof(line), "%sGlow fundal: %s", marker, settings->glow_enabled ? "ON" : "OFF");
    gsKit_fontm_print_scaled(gsGlobal, gsFontM, 100, y, 2, 0.8f, GS_SETREG_RGBAQ(255,255,0,0x80,0x00), line);
}

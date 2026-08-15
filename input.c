#include <libpad.h>
#include <string.h>
#include "lavalamp.h"

static char padBuf[256] __attribute__((aligned(64)));

/* exposed edge-triggered flags read by menu.c / main.c each frame */
u32 g_pad_dpad_up, g_pad_dpad_down, g_pad_dpad_left, g_pad_dpad_right;
u32 g_pad_start_pressed;

static u16 s_prev_buttons = 0xFFFF;

int input_init(void) {
    padInit(0);
    padPortOpen(0, 0, padBuf);

    int ret, timeout = 100;
    do {
        ret = padGetState(0, 0);
        timeout--;
    } while (ret != PAD_STATE_STABLE && ret != PAD_STATE_FINDCTP1 && timeout > 0);

    return (timeout > 0);
}

void input_update(void) {
    struct padButtonStatus pad;
    u16 buttons, pressed;

    g_pad_dpad_up = g_pad_dpad_down = g_pad_dpad_left = g_pad_dpad_right = 0;
    g_pad_start_pressed = 0;

    if (padGetState(0, 0) != PAD_STATE_STABLE) return;
    if (padRead(0, 0, &pad) == 0) return;

    buttons = pad.btns;                        /* active-low: 0 = held down */
    pressed = (~buttons) & s_prev_buttons;      /* held now AND wasn't held last frame */

    g_pad_dpad_up       = (pressed & PAD_UP)    ? 1 : 0;
    g_pad_dpad_down     = (pressed & PAD_DOWN)  ? 1 : 0;
    g_pad_dpad_left     = (pressed & PAD_LEFT)  ? 1 : 0;
    g_pad_dpad_right    = (pressed & PAD_RIGHT) ? 1 : 0;
    g_pad_start_pressed = (pressed & PAD_START) ? 1 : 0;

    s_prev_buttons = buttons;
}

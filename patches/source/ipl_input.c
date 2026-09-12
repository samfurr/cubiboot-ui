#include <ogc/pad.h>
#include "ipl_input.h"
#include "ipl_pad_hook.h"
#include "os.h"
#include "reloc.h"

static void (*original_pad_clamp)(PADStatus *pads);
static menu_pad_sample_t sample;

static void capture_pad_clamp(PADStatus *pads) {
    original_pad_clamp(pads);

    // Capture before the IPL adds BOTH sticks to the D-pad bits and zeroes the
    // axes of its combined menu status. Do not poll PADRead a second time.
    menu_pad_sample_t ports[4];
    for (int i = 0; i < 4; i++) {
        ports[i] = (menu_pad_sample_t){
            .buttons = pads[i].button,
            .main_x = pads[i].stickX, .main_y = pads[i].stickY,
            .c_x = pads[i].substickX, .c_y = pads[i].substickY,
            .connected = pads[i].err == PAD_ERR_NONE,
        };
    }
    sample = menu_input_combine(ports, 4);
}

void ipl_input_init(void) {
    if (original_pad_clamp) return;

    // Locate the SDK input wrapper by its guarded instruction pattern instead
    // of guessing per-revision addresses. A mismatch leaves the IPL untouched.
    uint32_t *code = (uint32_t *)0x81300000;
    size_t offset = ipl_pad_clamp_call(code, 0x8000 / sizeof(*code));
    if (offset == SIZE_MAX) {
        OSReport("C-stick capture unavailable: DEMOPadRead pattern mismatch\n");
        return;
    }
    uint32_t *call = code + offset;
    int32_t displacement = (int32_t)((*call & 0x03fffffc) << 6) >> 6;
    uintptr_t target = (uintptr_t)call + displacement;
    intptr_t replacement = (intptr_t)capture_pad_clamp - (intptr_t)call;
    if (target < 0x81300000 || target >= 0x81400000 ||
        replacement < -0x02000000 || replacement > 0x01fffffc || (replacement & 3)) {
        OSReport("C-stick capture unavailable: invalid PADClamp branch\n");
        return;
    }
    original_pad_clamp = (void (*)(PADStatus *))target;
    *call = 0x48000001 | ((uint32_t)replacement & 0x03fffffc);
    DCFlushRange(call, sizeof(*call));
    ICInvalidateRange(call, sizeof(*call));
    OSReport("C-stick capture installed at %p (PADClamp %p)\n", call, original_pad_clamp);
}

bool ipl_input_available(void) { return original_pad_clamp != NULL; }
menu_pad_sample_t ipl_input_sample(void) { return sample; }

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ipl_pad_hook.h"

int main(int argc, char **argv) {
    uint32_t code[64] = {
        0x7c0802a6, 0x3c608146, 0x90010004, 0x9421ffe0,
        0xbf410008, 0x3be3f0e0, 0x387f0000, 0x4803da25,
        0x7fe3fb78, 0x4803c839, 0x38000000, 0x900d82d4,
        0x387f0060, 0x3b400000, 0x38800000, 0x38a0000c,
        0x4bffd54d,
    };
    assert(ipl_pad_clamp_call(code, 17) == 9);
    assert(ipl_pad_clamp_call(code, 16) == SIZE_MAX);
    assert(ipl_pad_clamp_call(code, 0) == SIZE_MAX);
    // Revision-dependent address operands may change, opcodes may not.
    code[1] = 0x3c60814b;
    code[5] = 0x3be3f5a0;
    code[7] = 0x48068b31;
    code[9] = 0x48067db5;
    code[11] = 0x900d8354;
    code[16] = 0x4bffd665;
    assert(ipl_pad_clamp_call(code, 64) == 9);
    code[9] &= ~1u; // Not a call: must not patch it.
    assert(ipl_pad_clamp_call(code, 64) == SIZE_MAX);
    code[9] |= 1;
    memcpy(code + 32, code, 17 * sizeof(*code));
    assert(ipl_pad_clamp_call(code, 64) == SIZE_MAX);

    // Optional local-only check against a user's decoded IPL. No ROM fixture
    // is stored in the repository; read only its initial BS2 code window.
    if (argc == 2) {
        FILE *file = fopen(argv[1], "rb");
        assert(file);
        assert(fseek(file, 0x820, SEEK_SET) == 0);
        uint32_t bs2[0x8000 / 4];
        for (size_t i = 0; i < sizeof(bs2) / sizeof(*bs2); i++) {
            unsigned char bytes[4];
            assert(fread(bytes, 1, 4, file) == 4);
            bs2[i] = (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 |
                     (uint32_t)bytes[2] << 8 | bytes[3];
        }
        fclose(file);
        size_t call = ipl_pad_clamp_call(bs2, sizeof(bs2) / sizeof(*bs2));
        assert(call != SIZE_MAX);
        printf("Local IPL PADClamp call: 0x%08zx\n", (size_t)0x81300000 + call * 4);
    }
    puts("IPL input hook tests passed");
    return 0;
}

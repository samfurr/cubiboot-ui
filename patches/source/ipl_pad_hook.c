#include "ipl_pad_hook.h"

size_t ipl_pad_clamp_call(const uint32_t *code, size_t words) {
    static const struct { uint32_t instruction, mask; } pattern[] = {
        {0x7c0802a6, 0xffffffff}, // mflr r0
        {0x3c600000, 0xffff0000}, // lis r3, PAD array high
        {0x90010004, 0xffffffff}, // stw r0, 4(r1)
        {0x9421ffe0, 0xffffffff}, // stwu r1, -32(r1)
        {0xbf410008, 0xffffffff}, // stmw r26, 8(r1)
        {0x3be30000, 0xffff0000}, // addi r31, r3, PAD array low
        {0x387f0000, 0xffffffff}, // addi r3, r31, 0
        {0x48000001, 0xfc000003}, // bl PADRead
        {0x7fe3fb78, 0xffffffff}, // mr r3, r31
        {0x48000001, 0xfc000003}, // bl PADClamp (the call we wrap)
        {0x38000000, 0xffffffff}, // li r0, 0
        {0x900d0000, 0xffff0000}, // clear connected-pad count
        {0x387f0060, 0xffffffff}, // combined PADStatus follows two arrays of four
        {0x3b400000, 0xffffffff}, // li r26, 0
        {0x38800000, 0xffffffff}, // li r4, 0
        {0x38a0000c, 0xffffffff}, // li r5, sizeof(PADStatus)
        {0x48000001, 0xfc000003}, // bl memset
    };
    const size_t count = sizeof(pattern) / sizeof(pattern[0]);
    size_t found = SIZE_MAX;
    if (words < count) return found;
    for (size_t i = 0; i <= words - count; i++) {
        size_t j = 0;
        while (j < count && (code[i + j] & pattern[j].mask) == pattern[j].instruction) j++;
        if (j != count) continue;
        if (found != SIZE_MAX) return SIZE_MAX; // Never patch an ambiguous match.
        found = i + 9;
    }
    return found;
}

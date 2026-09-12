#pragma once

#include <stddef.h>
#include <stdint.h>

// Find the unique PADClamp call inside DEMOPadRead, or SIZE_MAX on mismatch.
// Address operands vary between IPL revisions; the surrounding instructions do not.
size_t ipl_pad_clamp_call(const uint32_t *code, size_t words);

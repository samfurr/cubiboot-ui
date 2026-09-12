#pragma once

#include <stdbool.h>
#include <stddef.h>

#define TITLE_TEXT_CAPACITY 129
#define TITLE_LINE_COUNT 2
#define TITLE_FONT_SIZE 17
#define TITLE_MIN_FONT_SIZE 15

typedef int (*title_glyph_width_fn)(unsigned short code, int size);

typedef struct {
    char lines[TITLE_LINE_COUNT][TITLE_TEXT_CAPACITY];
    int line_count;
    int size;
} title_layout_t;

// Banner fields need not be NUL-terminated. Strip extensions before fitting.
void title_copy_text(char output[TITLE_TEXT_CAPACITY], const char *source,
                     size_t source_capacity, bool sjis);
// text is the bounded, normalized output of title_copy_text; widths are in
// native IPL layout units. max_width must accommodate at least "...".
void title_layout(const char *text, bool sjis, int max_width,
                  title_glyph_width_fn glyph_width, title_layout_t *layout);

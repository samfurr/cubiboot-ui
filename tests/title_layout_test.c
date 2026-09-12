#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "title_layout.h"

// Proportional test font, in the same 1/16px units as the IPL renderer.
static int glyph_width(unsigned short c, int size) {
    int width = c == 'i' || c == '.' ? 5 : c == ' ' ? 6 : c == 'W' || c > 255 ? 24 : 15;
    return width * size * 16 / 24 - 16;
}

static int line_width(const char *s, int size, bool sjis) {
    int total = 0;
    while (*s) {
        unsigned short code = (unsigned char)*s++;
        if (sjis && code >= 0x81 && code <= 0x9f) code = (code << 8) | (unsigned char)*s++;
        total += glyph_width(code, size);
    }
    return total;
}

static void check_bounds(title_layout_t *layout, int max_width, bool sjis) {
    assert(layout->line_count >= 1 && layout->line_count <= 2);
    assert(layout->size >= TITLE_MIN_FONT_SIZE && layout->size <= TITLE_FONT_SIZE);
    for (int i = 0; i < layout->line_count; i++) {
        assert(strlen(layout->lines[i]) < TITLE_TEXT_CAPACITY);
        assert(line_width(layout->lines[i], layout->size, sjis) <= max_width);
    }
}

int main(void) {
    char title[TITLE_TEXT_CAPACITY];
    title_layout_t layout;
    const int max_width = 2816;

    title_copy_text(title, "  Super Smash Bros.\nMelee.ISO  ", 31, false);
    assert(strcmp(title, "Super Smash Bros. Melee") == 0);
    title_layout(title, false, max_width, glyph_width, &layout);
    assert(layout.line_count == 2 && layout.size == 17);
    assert(strcmp(layout.lines[0], "Super Smash") == 0);
    assert(strcmp(layout.lines[1], "Bros. Melee") == 0);
    check_bounds(&layout, max_width, false);

    title_layout("Settings", false, max_width, glyph_width, &layout);
    assert(layout.line_count == 1 && layout.size == 17);
    assert(strcmp(layout.lines[0], "Settings") == 0);
    title_layout("iiiiiiiiiiiiiiiiiiiiiiii", false, max_width, glyph_width, &layout);
    assert(layout.line_count == 1);
    title_layout("WWWWWWWWWWWWWWWWWWWWWWWW", false, max_width, glyph_width, &layout);
    assert(layout.line_count == 2);
    check_bounds(&layout, max_width, false);

    // Use the entire fixed-length BNR field without reading beyond it.
    char unterminated[64];
    memset(unterminated, 'W', sizeof(unterminated));
    title_copy_text(title, unterminated, sizeof(unterminated), false);
    assert(strlen(title) == sizeof(unterminated));
    title_layout(title, false, max_width, glyph_width, &layout);
    assert(strstr(layout.lines[1], "...") != NULL);
    check_bounds(&layout, max_width, false);

    const char *long_title = "Paper Mario The Thousand-Year Door.iso";
    title_copy_text(title, long_title, strlen(long_title), false);
    assert(strcmp(title, "Paper Mario The Thousand-Year Door") == 0);
    title_layout(title, false, max_width, glyph_width, &layout);
    assert(strstr(layout.lines[1], "...") == NULL);
    check_bounds(&layout, max_width, false);

    // Shift-JIS trail bytes must stay paired during copy, wrap and ellipsis.
    char japanese[65];
    for (int i = 0; i < 64; i += 2) {
        japanese[i] = (char)0x82;
        japanese[i + 1] = (char)0xa0;
    }
    japanese[64] = 0;
    title_copy_text(title, japanese, 63, true);
    assert(strlen(title) == 62);
    title_layout(title, true, max_width, glyph_width, &layout);
    for (int line = 0; line < 2; line++) {
        int i = 0;
        while ((unsigned char)layout.lines[line][i] == 0x82) {
            assert((unsigned char)layout.lines[line][i + 1] == 0xa0);
            i += 2;
        }
        assert(layout.lines[line][i] == 0 || strcmp(layout.lines[line] + i, "...") == 0);
    }
    check_bounds(&layout, max_width, true);

    title_copy_text(title, "", 0, false);
    title_layout(title, false, max_width, glyph_width, &layout);
    assert(layout.line_count == 1 && layout.lines[0][0] == 0);
    puts("Title layout tests passed");
}

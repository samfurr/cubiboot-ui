#include <string.h>
#include "title_layout.h"

static int char_bytes(const char *text, bool sjis) {
    unsigned char c = (unsigned char)text[0];
    unsigned char next = (unsigned char)text[1];
    return sjis && ((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfc)) &&
           next >= 0x40 && next <= 0xfc && next != 0x7f ? 2 : 1;
}

void title_copy_text(char output[TITLE_TEXT_CAPACITY], const char *source,
                     size_t source_capacity, bool sjis) {
    size_t used = 0;
    for (size_t i = 0; i < source_capacity && source[i];) {
        unsigned char c = (unsigned char)source[i];
        int bytes = i + 1 < source_capacity ? char_bytes(source + i, sjis) : 1;
        // Do not leave a partial Shift-JIS character at a field boundary.
        if (sjis && ((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfc)) && bytes == 1)
            break;
        if (used + bytes >= TITLE_TEXT_CAPACITY) break;
        if (bytes == 1 && c <= ' ') {
            if (used && output[used - 1] != ' ') output[used++] = ' ';
        } else {
            memcpy(output + used, source + i, bytes);
            used += bytes;
        }
        i += bytes;
    }
    while (used && output[used - 1] == ' ') used--;
    output[used] = '\0';

    const char *extensions[] = {".iso", ".gcm", ".dol", ".dol+cli", ".fdi"};
    for (unsigned int e = 0; e < sizeof(extensions) / sizeof(extensions[0]); e++) {
        size_t length = strlen(extensions[e]);
        if (length > used) continue;
        size_t i;
        for (i = 0; i < length; i++) {
            char c = output[used - length + i];
            if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
            if (c != extensions[e][i]) break;
        }
        if (i == length) {
            used -= length;
            while (used && output[used - 1] == ' ') used--;
            output[used] = '\0';
            break;
        }
    }
}

static int width(const char *text, int length, bool sjis, int size,
                 title_glyph_width_fn glyph_width) {
    int result = 0;
    for (int i = 0; i < length;) {
        int bytes = char_bytes(text + i, sjis);
        unsigned short code = (unsigned char)text[i];
        if (bytes == 2) code = (code << 8) | (unsigned char)text[i + 1];
        result += glyph_width(code, size);
        i += bytes;
    }
    return result;
}

static void copy_line(char *line, const char *text, int length) {
    while (length && text[length - 1] == ' ') length--;
    memcpy(line, text, length);
    line[length] = '\0';
}

// Prefer a balanced pair of whole-word lines over a nearly empty last line.
static bool split_title(const char *text, bool sjis, int max_width,
                        title_glyph_width_fn glyph_width, bool words_only,
                        title_layout_t *layout) {
    int length = strlen(text), best = -1, best_next = 0, best_width = max_width + 1;
    for (int i = char_bytes(text, sjis); i < length; i += char_bytes(text + i, sjis)) {
        if (words_only && text[i] != ' ') continue;
        int next = i + (text[i] == ' ');
        if (next == length) continue;
        int left = width(text, i, sjis, layout->size, glyph_width);
        int right = width(text + next, length - next, sjis, layout->size, glyph_width);
        int widest = left > right ? left : right;
        if (widest < best_width) {
            best = i;
            best_next = next;
            best_width = widest;
        }
    }
    if (best < 0) return false;
    copy_line(layout->lines[0], text, best);
    copy_line(layout->lines[1], text + best_next, length - best_next);
    layout->line_count = 2;
    return true;
}

void title_layout(const char *text, bool sjis, int max_width,
                  title_glyph_width_fn glyph_width, title_layout_t *layout) {
    memset(layout, 0, sizeof(*layout));
    layout->size = TITLE_FONT_SIZE;
    layout->line_count = 1;
    int length = strlen(text);
    if (width(text, length, sjis, layout->size, glyph_width) <= max_width) {
        copy_line(layout->lines[0], text, length);
        return;
    }
    // Keep the normal size wherever possible, with a small readability floor.
    for (int size = TITLE_FONT_SIZE; size >= TITLE_MIN_FONT_SIZE; size--) {
        layout->size = size;
        if (split_title(text, sjis, max_width, glyph_width, !sjis, layout)) return;
    }
    // Long unbroken filenames can split at a glyph boundary, never mid-SJIS.
    if (split_title(text, sjis, max_width, glyph_width, false, layout)) return;

    int end = 0, last_space = 0;
    while (end < length) {
        int next = end + char_bytes(text + end, sjis);
        if (width(text, next, sjis, layout->size, glyph_width) > max_width) break;
        if (text[end] == ' ') last_space = end;
        end = next;
    }
    if (last_space) end = last_space;
    copy_line(layout->lines[0], text, end);
    if (text[end] == ' ') end++;
    const char *rest = text + end;
    int rest_length = length - end, fit = 0;
    int dots_width = width("...", 3, sjis, layout->size, glyph_width);
    while (fit < rest_length) {
        int next = fit + char_bytes(rest + fit, sjis);
        if (next + 3 >= TITLE_TEXT_CAPACITY) break;
        if (width(rest, next, sjis, layout->size, glyph_width) + dots_width > max_width) break;
        fit = next;
    }
    copy_line(layout->lines[1], rest, fit);
    strcat(layout->lines[1], "...");
    layout->line_count = 2;
}

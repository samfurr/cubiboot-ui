#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// These match the low four PAD button bits, without depending on libogc in tests.
#define MENU_NAV_LEFT  0x0001
#define MENU_NAV_RIGHT 0x0002
#define MENU_NAV_DOWN  0x0004
#define MENU_NAV_UP    0x0008

#define MENU_CUBE_MAX_PITCH 5461.0f // 30 degrees, in IPL's 16-bit turn units
#define MENU_CUBE_MAX_YAW   8192.0f // 45 degrees

typedef struct {
    uint16_t buttons;
    int8_t main_x, main_y;
    int8_t c_x, c_y;
    bool connected;
} menu_pad_sample_t;

// Preserve input from any connected port, as the stock IPL menu does.
menu_pad_sample_t menu_input_combine(const menu_pad_sample_t *ports, size_t count);

typedef struct {
    uint16_t held_navigation;
    uint16_t pressed_navigation; // Fresh edges only, never hold repeats.
    uint16_t blocked_navigation; // Boundaries already acknowledged while held.
    bool inspecting;
    bool user_activity;
    float repeat_remaining_ms;
    float pitch;
    float yaw;
} menu_input_t;

void menu_input_reset(menu_input_t *input);

// active is false outside the grid or when the controller is disconnected.
// Returns navigation presses/repeats from only the main stick and D-pad.
uint16_t menu_input_update(menu_input_t *input, uint16_t buttons,
                           int8_t main_x, int8_t main_y,
                           int8_t c_x, int8_t c_y,
                           bool active, float elapsed_ms);

// Call after resolving each navigation attempt. Returns newly encountered
// blocked directions, even when first reached by a hold repeat. A direction
// stays quiet until released or a later attempted move in that direction
// succeeds. blocked is filtered to attempted; attempted=0 is valid. Call update
// first so held_navigation is current. A native-input fallback may instead keep
// a separate reset-initialized state and supply its current held_navigation.
uint16_t menu_input_blocked_feedback(menu_input_t *input, uint16_t attempted,
                                     uint16_t blocked);

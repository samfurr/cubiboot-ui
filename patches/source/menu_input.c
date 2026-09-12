#include "menu_input.h"

#define NAV_THRESHOLD 32
#define NAV_INITIAL_REPEAT_MS 350.0f
#define NAV_REPEAT_MS 100.0f
#define C_STICK_DEAD_ZONE 8
#define C_STICK_FULL_SCALE 60
#define CUBE_RESPONSE_MS 60.0f

static int stick_magnitude(int8_t x, int8_t y) {
    return (int)x * x + (int)y * y;
}

menu_pad_sample_t menu_input_combine(const menu_pad_sample_t *ports, size_t count) {
    menu_pad_sample_t combined = {0};
    for (size_t i = 0; i < count; i++) {
        const menu_pad_sample_t *port = &ports[i];
        if (!port->connected) continue;
        combined.connected = true;
        combined.buttons |= port->buttons;
        // Keep each stick's X/Y pair together when multiple controllers act.
        if (stick_magnitude(port->main_x, port->main_y) >
            stick_magnitude(combined.main_x, combined.main_y)) {
            combined.main_x = port->main_x;
            combined.main_y = port->main_y;
        }
        if (stick_magnitude(port->c_x, port->c_y) >
            stick_magnitude(combined.c_x, combined.c_y)) {
            combined.c_x = port->c_x;
            combined.c_y = port->c_y;
        }
    }
    return combined;
}

void menu_input_reset(menu_input_t *input) {
    *input = (menu_input_t){0};
}

static float c_stick_axis(int8_t value) {
    int magnitude = value < 0 ? -(int)value : value;
    if (magnitude <= C_STICK_DEAD_ZONE) return 0.0f;
    if (magnitude > C_STICK_FULL_SCALE) magnitude = C_STICK_FULL_SCALE;
    float amount = (float)(magnitude - C_STICK_DEAD_ZONE) /
                   (C_STICK_FULL_SCALE - C_STICK_DEAD_ZONE);
    return value < 0 ? -amount : amount;
}

static float ease_angle(float current, float target, float elapsed_ms) {
    float next = current + (target - current) * elapsed_ms / (CUBE_RESPONSE_MS + elapsed_ms);
    float remaining = target - next;
    // Finish at exactly neutral instead of retaining a tiny floating-point tilt.
    return remaining > -1.0f && remaining < 1.0f ? target : next;
}

uint16_t menu_input_update(menu_input_t *input, uint16_t buttons,
                           int8_t main_x, int8_t main_y,
                           int8_t c_x, int8_t c_y,
                           bool active, float elapsed_ms) {
    if (!active) {
        menu_input_reset(input);
        return 0;
    }
    // Avoid a held direction jumping after a paused/stalled menu frame.
    if (!(elapsed_ms > 0.0f)) elapsed_ms = 0.0f;
    if (elapsed_ms > 50.0f) elapsed_ms = 50.0f;

    float inspect_x = c_stick_axis(c_x), inspect_y = c_stick_axis(c_y);
    input->inspecting = inspect_x != 0.0f || inspect_y != 0.0f;
    input->user_activity = buttons != 0 || input->inspecting ||
        main_x >= NAV_THRESHOLD || main_x <= -NAV_THRESHOLD ||
        main_y >= NAV_THRESHOLD || main_y <= -NAV_THRESHOLD;
    input->pitch = ease_angle(input->pitch, -inspect_y * MENU_CUBE_MAX_PITCH, elapsed_ms);
    input->yaw = ease_angle(input->yaw, inspect_x * MENU_CUBE_MAX_YAW, elapsed_ms);

    uint16_t held = buttons & (MENU_NAV_LEFT | MENU_NAV_RIGHT | MENU_NAV_DOWN | MENU_NAV_UP);
    if (main_x >= NAV_THRESHOLD) held |= MENU_NAV_RIGHT;
    if (main_x <= -NAV_THRESHOLD) held |= MENU_NAV_LEFT;
    if (main_y >= NAV_THRESHOLD) held |= MENU_NAV_UP;
    if (main_y <= -NAV_THRESHOLD) held |= MENU_NAV_DOWN;

    // Opposing D-pad/stick directions cancel rather than moving twice per frame.
    if ((held & (MENU_NAV_LEFT | MENU_NAV_RIGHT)) == (MENU_NAV_LEFT | MENU_NAV_RIGHT))
        held &= ~(MENU_NAV_LEFT | MENU_NAV_RIGHT);
    if ((held & (MENU_NAV_UP | MENU_NAV_DOWN)) == (MENU_NAV_UP | MENU_NAV_DOWN))
        held &= ~(MENU_NAV_UP | MENU_NAV_DOWN);

    uint16_t pressed = held & ~input->held_navigation;
    input->pressed_navigation = pressed;
    input->blocked_navigation &= held;
    if (held != input->held_navigation || !held) {
        input->held_navigation = held;
        input->repeat_remaining_ms = NAV_INITIAL_REPEAT_MS;
        return pressed;
    }

    input->repeat_remaining_ms -= elapsed_ms;
    if (input->repeat_remaining_ms <= 0.0f) {
        input->repeat_remaining_ms += NAV_REPEAT_MS;
        return held;
    }
    return 0;
}

uint16_t menu_input_blocked_feedback(menu_input_t *input, uint16_t attempted,
                                     uint16_t blocked) {
    attempted &= MENU_NAV_LEFT | MENU_NAV_RIGHT | MENU_NAV_DOWN | MENU_NAV_UP;
    blocked &= attempted;
    // Also clear releases here for native-input fallbacks that supply held
    // directions without invoking the custom stick/repeat update above.
    input->blocked_navigation &= input->held_navigation;
    input->blocked_navigation &= ~(attempted & ~blocked);
    uint16_t feedback = blocked & ~input->blocked_navigation;
    input->blocked_navigation |= blocked;
    return feedback;
}

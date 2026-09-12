#include <assert.h>
#include <stdio.h>
#include "menu_input.h"

static void close_to(float actual, float expected, float tolerance) {
    assert(actual >= expected - tolerance && actual <= expected + tolerance);
}

static void test_blocked_feedback(void) {
    menu_input_t input = {0};

    // Holding right moves through the grid first, then acknowledges its edge
    // once. The first blocked attempt is a repeat, not a fresh physical press.
    int selected = 0;
    int feedback_count = 0;
    int attempt_count = 0;
    for (int frame = 0; frame < 200; frame++) {
        uint16_t attempted = menu_input_update(&input, 0, 60, 0, 0, 0, true, 10);
        uint16_t blocked = 0;
        if (attempted & MENU_NAV_RIGHT) {
            attempt_count++;
            if (selected < 2) selected++;
            else blocked = MENU_NAV_RIGHT;
        }
        uint16_t feedback = menu_input_blocked_feedback(&input, attempted, blocked);
        if (feedback) {
            assert(feedback == MENU_NAV_RIGHT && selected == 2);
            assert(input.pressed_navigation == 0);
            feedback_count++;
        }
    }
    assert(selected == 2 && attempt_count > 3 && feedback_count == 1);
    assert(input.blocked_navigation == MENU_NAV_RIGHT);

    // Frames without a navigation attempt do not rearm a held boundary.
    assert(menu_input_blocked_feedback(&input, 0, 0) == 0);
    assert(input.blocked_navigation == MENU_NAV_RIGHT);
    // Release is enough to rearm, without requiring the feedback helper to run.
    assert(menu_input_update(&input, 0, 0, 0, 0, 0, true, 10) == 0);
    assert(input.blocked_navigation == 0);
    uint16_t attempted = menu_input_update(&input, MENU_NAV_RIGHT, 0, 0, 0, 0, true, 10);
    assert(menu_input_blocked_feedback(&input, attempted, MENU_NAV_RIGHT) == MENU_NAV_RIGHT);

    // Moving successfully in a previously blocked direction rearms it even
    // while held (for example, another axis moved to a longer grid row).
    assert(menu_input_blocked_feedback(&input, MENU_NAV_RIGHT, 0) == 0);
    assert(input.blocked_navigation == 0);
    assert(menu_input_blocked_feedback(&input, MENU_NAV_RIGHT, MENU_NAV_RIGHT) == MENU_NAV_RIGHT);
    assert(menu_input_blocked_feedback(&input, MENU_NAV_RIGHT, MENU_NAV_RIGHT) == 0);

    // Independent direction latches: an existing right edge does not suppress
    // the first down edge, nor can a successful down move rearm the right edge.
    attempted = menu_input_update(&input, MENU_NAV_RIGHT | MENU_NAV_DOWN,
                                   0, 0, 0, 0, true, 10);
    assert(attempted == MENU_NAV_DOWN);
    assert(menu_input_blocked_feedback(&input, attempted, MENU_NAV_RIGHT | MENU_NAV_DOWN) == MENU_NAV_DOWN);
    assert(input.blocked_navigation == (MENU_NAV_RIGHT | MENU_NAV_DOWN));
    assert(menu_input_blocked_feedback(&input, MENU_NAV_DOWN, 0) == 0);
    assert(input.blocked_navigation == MENU_NAV_RIGHT);
    assert(menu_input_blocked_feedback(&input, MENU_NAV_RIGHT | MENU_NAV_DOWN,
                                       MENU_NAV_RIGHT | MENU_NAV_DOWN) == MENU_NAV_DOWN);
    assert(menu_input_update(&input, MENU_NAV_DOWN, 0, 0, 0, 0, true, 10) == 0);
    assert(input.blocked_navigation == MENU_NAV_DOWN);
    assert(menu_input_blocked_feedback(&input, 0, MENU_NAV_RIGHT | 0x100) == 0);
    assert(input.blocked_navigation == MENU_NAV_DOWN);

    // Unattempted/non-navigation bits never produce feedback. Opposing input
    // cancellation and inactive controllers release all relevant latches.
    assert(menu_input_blocked_feedback(&input, 0x100, 0x100) == 0);
    assert(menu_input_update(&input, MENU_NAV_UP | MENU_NAV_DOWN,
                             0, 0, 0, 0, true, 10) == 0);
    assert(input.blocked_navigation == 0);
    attempted = menu_input_update(&input, MENU_NAV_LEFT, 0, 0, 0, 0, true, 10);
    assert(menu_input_blocked_feedback(&input, attempted, MENU_NAV_LEFT) == MENU_NAV_LEFT);
    menu_input_update(&input, MENU_NAV_LEFT, 0, 0, 0, 0, false, 10);
    assert(input.blocked_navigation == 0);

    // A native fallback can supply its latched held directions directly and
    // call the helper even on non-repeat frames to notice a release.
    menu_input_t fallback = {0};
    fallback.held_navigation = MENU_NAV_UP;
    assert(menu_input_blocked_feedback(&fallback, MENU_NAV_UP, MENU_NAV_UP) == MENU_NAV_UP);
    assert(menu_input_blocked_feedback(&fallback, 0, 0) == 0);
    assert(menu_input_blocked_feedback(&fallback, MENU_NAV_UP, MENU_NAV_UP) == 0);
    fallback.held_navigation = 0;
    assert(menu_input_blocked_feedback(&fallback, 0, 0) == 0);
    assert(fallback.blocked_navigation == 0);
    fallback.held_navigation = MENU_NAV_UP;
    assert(menu_input_blocked_feedback(&fallback, MENU_NAV_UP, MENU_NAV_UP) == MENU_NAV_UP);
    menu_input_reset(&fallback);
    assert(fallback.blocked_navigation == 0 && fallback.held_navigation == 0);
}

int main(void) {
    menu_input_t input = {0};
    test_blocked_feedback();

    // Regression: preserve physical buttons/axes BEFORE the native IPL adds
    // C-stick directions to D-pad bits and clears the combined analog axes.
    menu_pad_sample_t ports[4] = {{.connected = true, .c_x = 60, .c_y = 60}};
    menu_pad_sample_t captured = menu_input_combine(ports, 4);
    ports[0].buttons |= MENU_NAV_RIGHT | MENU_NAV_UP;
    ports[0].c_x = ports[0].c_y = 0; // What the IPL's combined status looks like.
    assert(menu_input_update(&input, captured.buttons, captured.main_x, captured.main_y,
                             captured.c_x, captured.c_y, captured.connected, 20) == 0);
    assert(input.pitch < 0 && input.yaw > 0);
    // Real D-pad input remains distinguishable while the C-stick is held.
    ports[0] = (menu_pad_sample_t){.connected = true, .buttons = MENU_NAV_LEFT, .c_x = 60};
    ports[1] = (menu_pad_sample_t){.connected = true, .main_y = 60};
    ports[2] = (menu_pad_sample_t){.connected = false, .buttons = MENU_NAV_DOWN, .c_y = -128};
    captured = menu_input_combine(ports, 4);
    assert(captured.buttons == MENU_NAV_LEFT && captured.main_y == 60 && captured.c_y == 0);
    assert(menu_input_update(&input, captured.buttons, captured.main_x, captured.main_y,
                             captured.c_x, captured.c_y, captured.connected, 20) ==
           (MENU_NAV_LEFT | MENU_NAV_UP));
    for (int i = 0; i < 4; i++) ports[i].connected = false;
    captured = menu_input_combine(ports, 4);
    assert(!captured.connected && captured.buttons == 0 && captured.c_x == 0);
    menu_input_reset(&input);

    // C-stick input never navigates, even when held/repeated or diagonal.
    for (int frame = 0; frame < 120; frame++)
        assert(menu_input_update(&input, 0, 0, 0, 60, 60, true, 16.667f) == 0);
    close_to(input.pitch, -MENU_CUBE_MAX_PITCH, 1.0f);
    close_to(input.yaw, MENU_CUBE_MAX_YAW, 1.0f);
    assert(input.inspecting && input.user_activity && input.pressed_navigation == 0);

    // Release, including small neutral drift, settles to exactly zero.
    for (int frame = 0; frame < 90; frame++)
        assert(menu_input_update(&input, 0, 0, 0, 8, -8, true, 16.667f) == 0);
    assert(input.pitch == 0.0f && input.yaw == 0.0f);
    assert(!input.inspecting && !input.user_activity);

    // Partial travel is proportional; extreme signed inputs clamp safely.
    for (int frame = 0; frame < 120; frame++)
        menu_input_update(&input, 0, 0, 0, 34, 0, true, 16.667f);
    close_to(input.yaw, MENU_CUBE_MAX_YAW / 2.0f, 1.0f);
    assert(input.pitch == 0.0f);
    for (int frame = 0; frame < 120; frame++)
        menu_input_update(&input, 0, 0, 0, -128, -128, true, 16.667f);
    close_to(input.pitch, MENU_CUBE_MAX_PITCH, 1.0f);
    close_to(input.yaw, -MENU_CUBE_MAX_YAW, 1.0f);

    // Main-stick navigation has its own immediate press and hold-repeat timer.
    menu_input_reset(&input);
    assert(menu_input_update(&input, 0, 60, 0, 0, 0, true, 10) == MENU_NAV_RIGHT);
    assert(input.pressed_navigation == MENU_NAV_RIGHT && input.user_activity);
    for (int i = 0; i < 34; i++)
        assert(menu_input_update(&input, 0, 60, 0, -60, 60, true, 10) == 0);
    assert(menu_input_update(&input, 0, 60, 0, 60, -60, true, 10) == MENU_NAV_RIGHT);
    assert(input.pressed_navigation == 0); // A hold repeat is not a fresh press.
    for (int i = 0; i < 9; i++)
        assert(menu_input_update(&input, 0, 60, 0, 0, 0, true, 10) == 0);
    assert(menu_input_update(&input, 0, 60, 0, 0, 0, true, 10) == MENU_NAV_RIGHT);
    assert(menu_input_update(&input, 0, 0, 0, 60, 0, true, 10) == 0);
    assert(menu_input_update(&input, 0, -60, 0, 60, 0, true, 10) == MENU_NAV_LEFT);

    // D-pad, simultaneous inspection/navigation, and opposing directions.
    menu_input_reset(&input);
    assert(menu_input_update(&input, MENU_NAV_UP, 0, 0, 60, 0, true, 20) == MENU_NAV_UP);
    assert(input.yaw > 0.0f);
    assert(menu_input_update(&input, MENU_NAV_DOWN, 0, 0, 0, 60, true, 20) == MENU_NAV_DOWN);
    assert(menu_input_update(&input, MENU_NAV_LEFT, 60, 0, 0, 0, true, 20) == 0);
    assert(menu_input_update(&input, 0, 60, 60, 0, 0, true, 20) == (MENU_NAV_UP | MENU_NAV_RIGHT));

    // Inactive menus/disconnects reset both control systems; idle main stick drifts don't navigate.
    assert(menu_input_update(&input, MENU_NAV_UP, 60, 60, 60, 60, false, 20) == 0);
    assert(input.held_navigation == 0 && input.pitch == 0 && input.yaw == 0);
    assert(!input.inspecting && !input.user_activity && input.pressed_navigation == 0);
    assert(menu_input_update(&input, 0, 31, -31, 0, 0, true, 20) == 0);
    assert(!input.user_activity);
    assert(menu_input_update(&input, 0x100, 0, 0, 0, 0, true, 20) == 0);
    assert(input.user_activity); // A/B/etc also interrupt idle gestures.

    // Both 50 Hz and 60 Hz settle, without overshooting either axis.
    for (int hz = 50; hz <= 60; hz += 10) {
        menu_input_reset(&input);
        for (int i = 0; i < hz; i++) {
            menu_input_update(&input, 0, 0, 0, 60, 60, true, 1000.0f / hz);
            assert(input.yaw >= 0 && input.yaw <= MENU_CUBE_MAX_YAW);
            assert(input.pitch <= 0 && input.pitch >= -MENU_CUBE_MAX_PITCH);
        }
        for (int i = 0; i < hz; i++) menu_input_update(&input, 0, 0, 0, 0, 0, true, 1000.0f / hz);
        assert(input.pitch == 0 && input.yaw == 0);
    }
    puts("Menu input tests passed");
    return 0;
}

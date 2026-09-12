#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "menu_motion.h"

static void close_to(float actual, float expected, float tolerance) {
    assert(actual >= expected - tolerance && actual <= expected + tolerance);
}

static void check_bounds(const menu_motion_t *motion) {
    assert(isfinite(motion->pull) && motion->pull >= 0.0f && motion->pull <= 1.0f);
    assert(isfinite(motion->selected_scale) && motion->selected_scale >= 1.0f &&
           motion->selected_scale <= MENU_MOTION_MAX_SCALE);
    assert(isfinite(motion->hero_pitch) && motion->hero_pitch >= 0.0f &&
           motion->hero_pitch <= MENU_MOTION_IDLE_PITCH);
    assert(isfinite(motion->hero_yaw) && motion->hero_yaw >= 0.0f &&
           motion->hero_yaw <= MENU_MOTION_ARRIVAL_YAW);
    assert(isfinite(motion->bob_x) && motion->bob_x >= -MENU_MOTION_BOB_X &&
           motion->bob_x <= MENU_MOTION_BOB_X);
    assert(isfinite(motion->bob_y) && motion->bob_y >= -MENU_MOTION_BOB_Y &&
           motion->bob_y <= MENU_MOTION_BOB_Y);
    assert(isfinite(motion->bump_x) && isfinite(motion->bump_y));
    assert(motion->bump_x * motion->bump_x + motion->bump_y * motion->bump_y <=
           MENU_MOTION_BUMP_DISTANCE * MENU_MOTION_BUMP_DISTANCE + 0.0001f);
    assert(isfinite(motion->launch_blend) && motion->launch_blend >= 0.0f &&
           motion->launch_blend <= 1.0f);
}

static void frames(menu_motion_t *motion, int count, float elapsed_ms) {
    for (int frame = 0; frame < count; frame++) {
        menu_motion_update(motion, elapsed_ms, motion->selected_slot, true, false, false);
        check_bounds(motion);
    }
}

static void advance_ms(menu_motion_t *motion, double duration_ms, float frame_ms) {
    while (duration_ms > 0.0) {
        float step = duration_ms < frame_ms ? (float)duration_ms : frame_ms;
        frames(motion, 1, step);
        duration_ms -= step;
    }
}

static void idle_peak(menu_motion_t *motion, float frame_ms) {
    menu_motion_reset(motion, 0);
    menu_motion_update(motion, 0.0f, 0, true, true, false);
    advance_ms(motion, MENU_MOTION_IDLE_DELAY_MS + MENU_MOTION_IDLE_DIP_MS, frame_ms);
    assert(motion->idle_nodded);
    close_to(motion->hero_pitch, MENU_MOTION_IDLE_PITCH, 0.1f);
}

static void test_idle_rate(int hz) {
    const float frame_ms = 1000.0f / hz;
    menu_motion_t motion;
    menu_motion_reset(&motion, 0);
    menu_motion_update(&motion, 0.0f, 0, true, true, false);
    advance_ms(&motion, MENU_MOTION_IDLE_DELAY_MS - frame_ms, frame_ms);
    assert(!motion.idle_nodded && motion.hero_pitch == 0.0f);
    advance_ms(&motion, frame_ms, frame_ms);

    float previous_pitch = motion.hero_pitch;
    for (double remaining = MENU_MOTION_IDLE_DIP_MS; remaining > 0.0;) {
        float step = remaining < frame_ms ? (float)remaining : frame_ms;
        frames(&motion, 1, step);
        assert(motion.hero_pitch >= previous_pitch);
        previous_pitch = motion.hero_pitch;
        remaining -= step;
    }
    assert(motion.idle_nodded);
    close_to(motion.hero_pitch, MENU_MOTION_IDLE_PITCH, 0.1f);
    for (double remaining = MENU_MOTION_IDLE_NOD_MS - MENU_MOTION_IDLE_DIP_MS;
         remaining > 0.0;) {
        float step = remaining < frame_ms ? (float)remaining : frame_ms;
        frames(&motion, 1, step);
        assert(motion.hero_pitch <= previous_pitch);
        previous_pitch = motion.hero_pitch;
        remaining -= step;
    }
    close_to(motion.hero_pitch, 0.0f, 0.1f);
    frames(&motion, 1, frame_ms); // Exact neutral no later than the next frame.
    assert(motion.hero_pitch == 0.0f);
    advance_ms(&motion, MENU_MOTION_IDLE_DELAY_MS * 2.0f, frame_ms);
    assert(motion.idle_nodded && motion.hero_pitch == 0.0f);

    // Activity rearms the one-shot; continued input cancels the new nod once,
    // without stretching its short cancellation decay indefinitely.
    menu_motion_update(&motion, 0.0f, 0, true, true, false);
    assert(!motion.idle_nodded);
    advance_ms(&motion, MENU_MOTION_IDLE_DELAY_MS + MENU_MOTION_IDLE_DIP_MS, frame_ms);
    close_to(motion.hero_pitch, MENU_MOTION_IDLE_PITCH, 0.1f);
    previous_pitch = motion.hero_pitch;
    for (int frame = 0; frame < hz; frame++) {
        menu_motion_update(&motion, frame_ms, 0, true, true, false);
        check_bounds(&motion);
        assert(motion.hero_pitch <= previous_pitch);
        previous_pitch = motion.hero_pitch;
        if ((frame + 1) * frame_ms > MENU_MOTION_IDLE_RETURN_MS + 1.0f)
            assert(motion.hero_pitch == 0.0f);
    }
    assert(!motion.idle_nodded && motion.hero_pitch == 0.0f);

    idle_peak(&motion, frame_ms);
    menu_motion_update(&motion, 0.0f, 0, true, true, true);
    assert(motion.hero_pitch == 0.0f && motion.hero_yaw == 0.0f);
    advance_ms(&motion, MENU_MOTION_IDLE_DELAY_MS / 2.0f, frame_ms);
    assert(!motion.idle_nodded && motion.hero_pitch == 0.0f);
}

int main(void) {
    menu_motion_t motion;
    menu_motion_reset(&motion, 4);
    assert(motion.selected_slot == 4 && motion.pull == 0.0f && !motion.active);
    assert(motion.selected_scale == 1.0f);

    // Arrival is immediate, bounded, and complete in 200 ms at both rates.
    // The settling pulse finishes earlier so the selected slot stops moving.
    for (int hz = 50; hz <= 60; hz += 10) {
        menu_motion_reset(&motion, 4);
        float previous_pull = 0.0f;
        for (int frame = 0; frame < hz; frame++) {
            menu_motion_update(&motion, 1000.0f / hz, 4, true, false, false);
            check_bounds(&motion);
            assert(motion.pull >= previous_pull);
            previous_pull = motion.pull;
            if ((frame + 1) * (1000.0f / hz) > MENU_MOTION_SETTLE_MS + 1.0f)
                assert(motion.selected_scale == 1.0f);
            if ((frame + 1) * (1000.0f / hz) > MENU_MOTION_ARRIVAL_MS + 1.0f)
                assert(motion.pull == 1.0f && motion.hero_yaw == 0.0f);
        }
        assert(motion.pull == 1.0f && motion.selected_scale == 1.0f);
        assert(motion.hero_pitch == 0.0f && motion.hero_yaw == 0.0f);
    }
    menu_motion_reset(&motion, 4);
    menu_motion_update(&motion, 30.0f, 4, true, false, false);
    menu_motion_update(&motion, 30.0f, 4, true, false, false);
    close_to(motion.selected_scale, MENU_MOTION_MAX_SCALE, 0.00001f);
    menu_motion_update(&motion, 40.0f, 4, true, false, false);
    close_to(motion.hero_yaw, MENU_MOTION_ARRIVAL_YAW, 0.0001f);

    // Rapid selections restart rather than queue. C-stick input wins immediately
    // and releasing it never brings the canceled arrival gesture back.
    menu_motion_update(&motion, 0.0f, 5, true, true, false);
    assert(motion.pull == 0.0f && motion.hero_yaw == 0.0f);
    menu_motion_update(&motion, 30.0f, 5, true, false, false);
    assert(motion.hero_yaw > 0.0f);
    menu_motion_update(&motion, 0.0f, 5, true, true, true);
    assert(motion.hero_pitch == 0.0f && motion.hero_yaw == 0.0f);
    frames(&motion, 2, 30.0f);
    assert(motion.hero_yaw == 0.0f);
    menu_motion_update(&motion, 30.0f, 6, true, true, true);
    assert(motion.hero_yaw == 0.0f);
    menu_motion_update(&motion, 30.0f, 7, true, true, false);
    assert(motion.hero_yaw > 0.0f);

    // Idle curiosity is one nod per quiet interval, not a loop.
    menu_motion_reset(&motion, 0);
    menu_motion_update(&motion, 0.0f, 0, true, true, false);
    advance_ms(&motion, MENU_MOTION_IDLE_DELAY_MS - 50.0f, 50.0f);
    assert(!motion.idle_nodded && motion.hero_pitch == 0.0f);
    advance_ms(&motion, 50.0f, 50.0f);
    assert(motion.idle_nodded && motion.hero_pitch == 0.0f);
    advance_ms(&motion, MENU_MOTION_IDLE_DIP_MS / 2.0f, 50.0f);
    close_to(motion.hero_pitch, MENU_MOTION_IDLE_PITCH / 2.0f, 0.01f);
    advance_ms(&motion, MENU_MOTION_IDLE_DIP_MS / 2.0f, 50.0f);
    assert(motion.idle_nodded);
    close_to(motion.hero_pitch, MENU_MOTION_IDLE_PITCH, 0.01f);
    advance_ms(&motion, (MENU_MOTION_IDLE_NOD_MS - MENU_MOTION_IDLE_DIP_MS) / 2.0f, 50.0f);
    close_to(motion.hero_pitch, MENU_MOTION_IDLE_PITCH / 2.0f, 0.01f);
    advance_ms(&motion, (MENU_MOTION_IDLE_NOD_MS - MENU_MOTION_IDLE_DIP_MS) / 2.0f, 50.0f);
    assert(motion.hero_pitch == 0.0f);
    advance_ms(&motion, MENU_MOTION_IDLE_DELAY_MS * 2.0f, 50.0f);
    assert(motion.idle_nodded && motion.hero_pitch == 0.0f);
    menu_motion_update(&motion, 0.0f, 0, true, true, false);
    assert(!motion.idle_nodded);
    advance_ms(&motion, MENU_MOTION_IDLE_DELAY_MS + MENU_MOTION_IDLE_DIP_MS, 50.0f);
    close_to(motion.hero_pitch, MENU_MOTION_IDLE_PITCH, 0.01f);

    // Buttons smoothly cancel a nod within 100 ms, even if held continuously.
    idle_peak(&motion, 50.0f);
    float previous_pitch = motion.hero_pitch;
    menu_motion_update(&motion, 0.0f, 0, true, true, false);
    assert(motion.hero_pitch == previous_pitch);
    for (int frame = 0; frame < 5; frame++) {
        menu_motion_update(&motion, 20.0f, 0, true, true, false);
        assert(motion.hero_pitch <= previous_pitch);
        previous_pitch = motion.hero_pitch;
    }
    assert(motion.hero_pitch == 0.0f && !motion.idle_nodded);
    idle_peak(&motion, 50.0f);
    menu_motion_update(&motion, 0.0f, 0, true, true, true);
    assert(motion.hero_pitch == 0.0f && motion.hero_yaw == 0.0f);
    advance_ms(&motion, MENU_MOTION_IDLE_NOD_MS, 50.0f);
    assert(motion.hero_pitch == 0.0f);
    test_idle_rate(50);
    test_idle_rate(60);

    // Directional boundary bumps stop, never queue, and reset on selection.
    menu_motion_bump(&motion, -1, 0);
    frames(&motion, 7, 10.0f);
    close_to(motion.bump_x, -MENU_MOTION_BUMP_DISTANCE, 0.0001f);
    assert(motion.bump_y == 0.0f);
    menu_motion_bump(&motion, 1, 0);
    frames(&motion, 7, 10.0f);
    assert(motion.bump_x == 0.0f && motion.bump_y == 0.0f);
    frames(&motion, 10, 20.0f);
    assert(motion.bump_x == 0.0f);
    menu_motion_bump(&motion, 9, -4);
    frames(&motion, 7, 10.0f);
    assert(motion.bump_x > 0.0f && motion.bump_y < 0.0f);
    menu_motion_update(&motion, 10.0f, 1, true, true, false);
    assert(motion.bump_x == 0.0f && motion.bump_y == 0.0f);

    // Launch alignment overlaps the caller's transition; repeated events do not
    // delay it. A new selection or inactive menu cancels every launch transient.
    menu_motion_launch(&motion);
    float previous_blend = 0.0f;
    for (int frame = 0; frame < 6; frame++) {
        menu_motion_launch(&motion);
        frames(&motion, 1, 20.0f);
        assert(motion.launch_blend >= previous_blend);
        previous_blend = motion.launch_blend;
    }
    assert(motion.launch_blend == 1.0f);
    menu_motion_bump(&motion, 1, 0);
    frames(&motion, 2, 20.0f);
    assert(motion.bump_x == 0.0f);
    menu_motion_update(&motion, 20.0f, 2, true, true, false);
    assert(!motion.launching && motion.launch_blend == 0.0f);
    menu_motion_update(&motion, 20.0f, 2, false, false, false);
    check_bounds(&motion);
    assert(!motion.active && !motion.idle_nodded && !motion.launching);
    assert(motion.pull == 0.0f && motion.hero_pitch == 0.0f && motion.hero_yaw == 0.0f);
    assert(motion.bob_x == 0.0f && motion.bob_y == 0.0f);
    menu_motion_bump(&motion, 1, 1);
    menu_motion_launch(&motion);
    assert(!motion.launching && motion.bump_ms == MENU_MOTION_BUMP_MS);

    // Invalid/stalled clocks cannot poison transforms or skip the arrival.
    menu_motion_update(&motion, -50.0f, 2, true, false, false);
    assert(motion.arrival_ms == 0.0f);
    menu_motion_update(&motion, NAN, 2, true, false, false);
    assert(motion.arrival_ms == 0.0f);
    menu_motion_update(&motion, INFINITY, 2, true, false, false);
    assert(motion.arrival_ms == 50.0f);
    menu_motion_update(&motion, 10000.0f, 2, true, false, false);
    assert(motion.arrival_ms == 100.0f);
    check_bounds(&motion);

    // Equal wall-clock positions remain consistent at 50/60 Hz. Over a long run
    // all motion remains finite/bounded and the bob phase cannot grow forever.
    menu_motion_t pal, ntsc;
    menu_motion_reset(&pal, 0);
    menu_motion_reset(&ntsc, 0);
    frames(&pal, 50, 20.0f);
    frames(&ntsc, 60, 1000.0f / 60.0f);
    close_to(pal.bob_x, ntsc.bob_x, 0.001f);
    close_to(pal.bob_y, ntsc.bob_y, 0.001f);
    menu_motion_reset(&pal, 0);
    menu_motion_reset(&ntsc, 0);
    menu_motion_update(&pal, 0.0f, 0, true, true, false);
    menu_motion_update(&ntsc, 0.0f, 0, true, true, false);
    advance_ms(&pal, MENU_MOTION_IDLE_DELAY_MS + MENU_MOTION_IDLE_DIP_MS, 20.0f);
    advance_ms(&ntsc, MENU_MOTION_IDLE_DELAY_MS + MENU_MOTION_IDLE_DIP_MS, 1000.0f / 60.0f);
    assert(pal.idle_nodded && ntsc.idle_nodded);
    close_to(pal.hero_pitch, ntsc.hero_pitch, 0.1f);
    advance_ms(&pal, MENU_MOTION_IDLE_NOD_MS, 20.0f);
    advance_ms(&ntsc, MENU_MOTION_IDLE_NOD_MS, 1000.0f / 60.0f);
    assert(pal.hero_pitch == 0.0f && ntsc.hero_pitch == 0.0f);
    for (int frame = 0; frame < 200000; frame++) {
        menu_motion_update(&motion, frame % 2 ? 20.0f : 1000.0f / 60.0f,
                            frame / 137, true, frame % 123 == 0, frame % 83 == 0);
        if (frame % 97 == 0) menu_motion_bump(&motion, frame % 3 - 1, frame % 5 - 2);
        if (frame % 431 == 0) menu_motion_launch(&motion);
        check_bounds(&motion);
        assert(motion.bob_phase_ms >= 0.0f && motion.bob_phase_ms < MENU_MOTION_BOB_PERIOD_MS);
    }

    puts("Menu motion tests passed");
    return 0;
}

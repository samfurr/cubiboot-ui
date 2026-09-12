#include "menu_motion.h"

static float bounded_time(float elapsed_ms) {
    if (!(elapsed_ms > 0.0f)) return 0.0f;
    return elapsed_ms > 50.0f ? 50.0f : elapsed_ms;
}

static float advance(float current, float elapsed_ms, float duration) {
    current += elapsed_ms;
    return current < duration ? current : duration;
}

static float smoothstep(float phase) {
    return phase * phase * (3.0f - 2.0f * phase);
}

// A soft pulse with zero velocity at both ends and exactly one at its midpoint.
static float pulse(float phase) {
    float arch = 4.0f * phase * (1.0f - phase);
    return arch * arch;
}

// A deliberate dip and slower recovery read as a nod instead of a brief wobble.
// Both halves meet at zero velocity, including the neutral start and finish.
static float idle_nod(float elapsed_ms) {
    if (elapsed_ms < MENU_MOTION_IDLE_DIP_MS)
        return smoothstep(elapsed_ms / MENU_MOTION_IDLE_DIP_MS);
    float recovery = (elapsed_ms - MENU_MOTION_IDLE_DIP_MS) /
                     (MENU_MOTION_IDLE_NOD_MS - MENU_MOTION_IDLE_DIP_MS);
    return 1.0f - smoothstep(recovery);
}

// A periodic cubic approximation keeps the original tiny bob without sinf or a
// frame-count dependence. Both value and velocity meet across the four quarters.
static float wave(float phase) {
    if (phase < 0.0f) phase += 1.0f;
    if (phase >= 1.0f) phase -= 1.0f;
    float sign = phase < 0.5f ? 1.0f : -1.0f;
    if (phase >= 0.5f) phase -= 0.5f;
    float quarter = phase * 4.0f;
    if (quarter > 1.0f) quarter = 2.0f - quarter;
    const float slope = 1.5707963268f;
    return sign * quarter * (slope + quarter *
                            ((3.0f - 2.0f * slope) + quarter * (slope - 2.0f)));
}

static void reset_idle(menu_motion_t *motion) {
    motion->idle_ms = 0.0f;
    motion->idle_nodded = false;
    motion->idle_pitch = 0.0f;
    motion->idle_return_ms = MENU_MOTION_IDLE_RETURN_MS;
    motion->idle_return_from = 0.0f;
}

void menu_motion_reset(menu_motion_t *motion, int selected_slot) {
    *motion = (menu_motion_t){
        .selected_scale = 1.0f,
        .selected_slot = selected_slot,
        .bump_ms = MENU_MOTION_BUMP_MS,
        .idle_return_ms = MENU_MOTION_IDLE_RETURN_MS,
    };
}

static void update_idle(menu_motion_t *motion, float elapsed_ms,
                        bool user_activity, bool inspecting) {
    if (inspecting) {
        reset_idle(motion);
        return;
    }

    if (user_activity || motion->launching) {
        motion->idle_ms = 0.0f;
        motion->idle_nodded = false;
        if (motion->idle_pitch != 0.0f &&
            motion->idle_return_ms >= MENU_MOTION_IDLE_RETURN_MS) {
            motion->idle_return_from = motion->idle_pitch;
            motion->idle_return_ms = 0.0f;
        }
    } else {
        motion->idle_ms = advance(motion->idle_ms, elapsed_ms,
                                  MENU_MOTION_IDLE_DELAY_MS + MENU_MOTION_IDLE_NOD_MS);
    }

    if (motion->idle_return_ms < MENU_MOTION_IDLE_RETURN_MS) {
        motion->idle_return_ms = advance(motion->idle_return_ms, elapsed_ms,
                                         MENU_MOTION_IDLE_RETURN_MS);
        float phase = motion->idle_return_ms / MENU_MOTION_IDLE_RETURN_MS;
        motion->idle_pitch = motion->idle_return_from * (1.0f - smoothstep(phase));
    } else if (motion->idle_ms >= MENU_MOTION_IDLE_DELAY_MS) {
        motion->idle_nodded = true;
        motion->idle_pitch = MENU_MOTION_IDLE_PITCH *
            idle_nod(motion->idle_ms - MENU_MOTION_IDLE_DELAY_MS);
    } else {
        motion->idle_pitch = 0.0f;
    }
}

void menu_motion_update(menu_motion_t *motion, float elapsed_ms, int selected_slot,
                        bool active, bool user_activity, bool inspecting) {
    if (!active) {
        menu_motion_reset(motion, selected_slot);
        return;
    }

    elapsed_ms = bounded_time(elapsed_ms);
    bool selection_changed = selected_slot != motion->selected_slot;
    bool new_selection = !motion->active || selection_changed;
    // Entrance choreography yields to interaction. Thereafter the preview stays
    // revealed while navigation only gives the selected small tile a pulse.
    if (motion->active && (selection_changed || user_activity || inspecting) &&
        motion->entry_ms < MENU_MOTION_ARRIVAL_MS) {
        motion->entry_ms = MENU_MOTION_ARRIVAL_MS;
        motion->arrival_suppressed = true;
    }
    motion->active = true;
    if (new_selection) {
        motion->selected_slot = selected_slot;
        motion->arrival_ms = 0.0f;
        motion->bump_ms = MENU_MOTION_BUMP_MS;
        motion->launching = false;
        motion->launch_ms = 0.0f;
        motion->launch_blend = 0.0f;
        reset_idle(motion);
        user_activity = true;
    }

    motion->entry_ms = advance(motion->entry_ms, elapsed_ms, MENU_MOTION_ARRIVAL_MS);
    float remaining = 1.0f - motion->entry_ms / MENU_MOTION_ARRIVAL_MS;
    motion->pull = 1.0f - remaining * remaining * remaining;
    motion->arrival_ms = advance(motion->arrival_ms, elapsed_ms, MENU_MOTION_ARRIVAL_MS);
    float settle_phase = motion->arrival_ms / MENU_MOTION_SETTLE_MS;
    motion->selected_scale = settle_phase < 1.0f ?
        1.0f + (MENU_MOTION_MAX_SCALE - 1.0f) * pulse(settle_phase) : 1.0f;

    if (inspecting) motion->arrival_suppressed = true;
    motion->hero_yaw = motion->arrival_suppressed ? 0.0f :
        MENU_MOTION_ARRIVAL_YAW * pulse(motion->entry_ms / MENU_MOTION_ARRIVAL_MS);

    update_idle(motion, elapsed_ms, user_activity, inspecting);
    motion->hero_pitch = motion->idle_pitch;

    motion->bob_phase_ms += elapsed_ms;
    if (motion->bob_phase_ms >= MENU_MOTION_BOB_PERIOD_MS)
        motion->bob_phase_ms -= MENU_MOTION_BOB_PERIOD_MS;
    float bob_phase = motion->bob_phase_ms / MENU_MOTION_BOB_PERIOD_MS;
    motion->bob_x = MENU_MOTION_BOB_X * wave(bob_phase - 0.25f);
    motion->bob_y = MENU_MOTION_BOB_Y * wave(2.0f * bob_phase);

    motion->bump_ms = advance(motion->bump_ms, elapsed_ms, MENU_MOTION_BUMP_MS);
    float bump = MENU_MOTION_BUMP_DISTANCE * pulse(motion->bump_ms / MENU_MOTION_BUMP_MS);
    motion->bump_x = motion->bump_direction_x * bump;
    motion->bump_y = motion->bump_direction_y * bump;

    if (motion->launching) {
        motion->launch_ms = advance(motion->launch_ms, elapsed_ms, MENU_MOTION_LAUNCH_MS);
        motion->launch_blend = smoothstep(motion->launch_ms / MENU_MOTION_LAUNCH_MS);
    }
}

void menu_motion_bump(menu_motion_t *motion, int x_direction, int y_direction) {
    if (!motion->active || motion->launching || motion->bump_ms < MENU_MOTION_BUMP_MS)
        return;
    float x = x_direction < 0 ? -1.0f : (x_direction > 0 ? 1.0f : 0.0f);
    float y = y_direction < 0 ? -1.0f : (y_direction > 0 ? 1.0f : 0.0f);
    if (x == 0.0f && y == 0.0f) return;
    if (x != 0.0f && y != 0.0f) {
        x *= 0.7071067811f;
        y *= 0.7071067811f;
    }
    motion->bump_direction_x = x;
    motion->bump_direction_y = y;
    motion->bump_ms = 0.0f;
}

void menu_motion_launch(menu_motion_t *motion) {
    if (!motion->active || motion->launching) return;
    motion->launching = true;
    motion->launch_ms = 0.0f;
    motion->launch_blend = 0.0f;
}

#pragma once

#include <stdbool.h>

#define MENU_MOTION_ARRIVAL_MS 200.0f
#define MENU_MOTION_SETTLE_MS 120.0f
#define MENU_MOTION_BUMP_MS 140.0f
#define MENU_MOTION_LAUNCH_MS 120.0f
#define MENU_MOTION_IDLE_DELAY_MS 11000.0f
#define MENU_MOTION_IDLE_NOD_MS 1200.0f
#define MENU_MOTION_IDLE_DIP_MS 400.0f
#define MENU_MOTION_IDLE_RETURN_MS 100.0f // Cancellation by user activity.
#define MENU_MOTION_BOB_PERIOD_MS 4500.0f

#define MENU_MOTION_MAX_SCALE 1.03f
#define MENU_MOTION_ARRIVAL_YAW (65536.0f * 3.0f / 360.0f)
#define MENU_MOTION_IDLE_PITCH (65536.0f * 5.0f / 360.0f)
#define MENU_MOTION_BOB_X 2.30f
#define MENU_MOTION_BOB_Y 1.38f
#define MENU_MOTION_BUMP_DISTANCE 3.0f

typedef struct {
    // Rendering outputs. Angles use IPL's 16-bit turn units; offsets use the
    // same world units as the cube transform. Keep text outside this transform.
    float pull;
    float selected_scale;
    float hero_pitch, hero_yaw;
    float bob_x, bob_y;
    float bump_x, bump_y;
    float launch_blend;

    // Internal state. Callers should only change it through the functions below.
    int selected_slot;
    bool active;
    bool arrival_suppressed;
    bool idle_nodded;
    bool launching;
    float arrival_ms;
    float bob_phase_ms;
    float bump_ms;
    float bump_direction_x, bump_direction_y;
    float idle_ms;
    float idle_pitch;
    float idle_return_ms;
    float idle_return_from;
    float launch_ms;
} menu_motion_t;

// A newly opened menu starts its selected cube's arrival on the next update.
void menu_motion_reset(menu_motion_t *motion, int selected_slot);

// Call exactly once per frame, after applying logical navigation. active means
// that the cube is still visible, including its launch transition. Inspection
// immediately cancels decorative rotation and cannot resume an old animation.
// Any physical input counts as user_activity; neutral controller drift need not.
// Negative/NaN frame times become zero; stalls are capped at 50 ms.
void menu_motion_update(menu_motion_t *motion, float elapsed_ms, int selected_slot,
                        bool active, bool user_activity, bool inspecting);

// A blocked navigation attempt gives one directional bump. Repeats during the
// bump are ignored, never queued; diagonal distance is bounded to three units.
void menu_motion_bump(menu_motion_t *motion, int x_direction, int y_direction);

// Begin a one-shot alignment blend without delaying the existing boot flow.
// Keep updating while the launch is visible; blend rotations toward the desired
// launch pose with launch_blend. Repeated launch calls do not restart the blend.
void menu_motion_launch(menu_motion_t *motion);

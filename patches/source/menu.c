#include <math.h>
#include "picolibc.h"
#include "structs.h"

#include "util.h"
#include "reloc.h"
#include "attr.h"

#include <ogc/machine/processor.h>

#include "usbgecko.h"
#include "menu.h"
#include "menu_input.h"
#include "menu_motion.h"
#include "ipl_input.h"
#include "grid.h"
#include "games.h"
#include "gameid.h"

#include "dolphin_dvd.h"

#include "dir_tex_bin.h"
#include "dol_tex_bin.h"
#include "settings_tex_bin.h"
#include "font.h"
#include "title_layout.h"
#include "boot.h"
#include "ipl.h"
#include "os.h"

#include "time.h"

// TODO: this is all zeros except for one BNRDesc, so replace it with a sparse version
#include "default_opening_bin.h"
#include "gcm.h"
#include "bnr.h"

#include "emu/tweaks.h"

// for setup
__attribute_reloc__ void (*menu_alpha_setup)();

// for custom menus
__attribute_reloc__ void (*prep_text_mode)();
__attribute_reloc__ void (*gx_draw_text)(u16 index, text_group* text, text_draw_group* text_draw, GXColor* color);
__attribute_reloc__ void (*setup_gameselect_menu)(u8 alpha_0, u8 alpha_1, u8 alpha_2);
__attribute_reloc__ GXColorS10 *(*get_save_color)(u32 color_index, s32 save_type);
__attribute_reloc__ void (*setup_gameselect_anim)();
__attribute_reloc__ void (*setup_cube_anim)();
__attribute_reloc__ model_data *save_icon;
__attribute_reloc__ model_data *save_empty;

// for audio
__attribute_reloc__ void (*Jac_PlaySe)(u32);
__attribute_reloc__ void (*Jac_StopSoundAll)();

// for model gx
__attribute_reloc__ void (*model_init)(model* m, int process);
__attribute_reloc__ void (*draw_model)(model* m);
__attribute_reloc__ void (*draw_partial)(model* m, model_part* part);
__attribute_reloc__ void (*change_model)(model* m);

// for menu elements
__attribute_reloc__ void (*draw_grid)(Mtx position, u8 alpha);
__attribute_reloc__ void (*draw_box)(u32 index, box_draw_group* header, GXColor* texa, int inside_x, int inside_y, int inside_width, int inside_height);
// __attribute_reloc__ void (*draw_start_info)(u8 alpha);
__attribute_reloc__ void (*draw_start_anim)(u8 alpha);
__attribute_reloc__ void (*draw_blob_fixed)(void *blob_ptr, void *blob_a, void *blob_b, GXColor *color);
__attribute_reloc__ void (*draw_blob_text)(u32 type, void *blob, GXColor *color, char *str, s32 len);
__attribute_reloc__ void (*draw_blob_text_long)(u32 type, void *blob, GXColor *color, char *str, s32 len);
__attribute_reloc__ void (*draw_blob_border)(u32 type, void *blob, GXColor *color);
__attribute_reloc__ void (*draw_blob_tex)(u32 type, void *blob, GXColor *color, tex_data *dat);
__attribute_reloc__ void (*setup_tex_draw)(s32 unk0, s32 unk1, s32 unk2);
__attribute_reloc__ void (*draw_named_tex)(u32 type, void *blob, GXColor *color, s16 x, s16 y);

// unknown blob (from memcard menu)
__attribute_reloc__ void **ptr_menu_blob;
__attribute_data__ void *menu_blob = NULL;

// unknown blobs (from gameselect menu)
__attribute_reloc__ void *game_blob_text;
__attribute_reloc__ void **ptr_game_blob_a;
__attribute_data__ void *game_blob_a = NULL;
__attribute_reloc__ void **ptr_game_blob_b;
__attribute_data__ void *game_blob_b = NULL;

// for camera gx
__attribute_reloc__ void (*set_obj_pos)(model* m, MtxP matrix, guVector vector);
__attribute_reloc__ void (*set_obj_cam)(model* m, MtxP matrix);
__attribute_reloc__ MtxP (*get_camera_mtx)();

// helpers
__attribute_reloc__ f32 (*fast_sin)(s16 deg);
__attribute_reloc__ f32 (*fast_cos)(s16 deg);
__attribute_reloc__ void (*apply_save_rot)(s32 x, s32 y, s32 z, Mtx matrix);
__attribute_reloc__ u32 *bs2start_ready;
__attribute_reloc__ u32 *banner_pointer;
__attribute_reloc__ u32 *banner_ready;

typedef struct {
    f32 scale;
    f32 opacity;
    Mtx m;
} position_t;

static position_t icons_positions[GRID_COLUMN_COUNT];

#define GRID_ICON_SCALE 1.55f
#define GRID_SELECTED_SCALE 1.72f
#define HERO_ICON_SCALE 4.9f
#define GRID_ICON_FACE_SCALE 0.84f
#define HERO_ICON_FACE_SCALE 1.18f
#define GRID_BANNER_FACE_SCALE 1.12f
#define HERO_BANNER_FACE_SCALE 0.92f
#define GRID_BASE_X -240.0f
#define HERO_POSITION_X 152.0f
#define HERO_POSITION_Y 62.0f
#define HERO_POSITION_Z 4.0f
#define GRID_ICON_OPACITY 0.72f
#define GRID_SELECTED_GHOST_OPACITY 0.92f
#define DETAIL_PANEL_WIDTH 0x0D80
#define DETAIL_PANEL_HEIGHT 0x0440
#define DETAIL_PANEL_CENTER_X 0x1B90
#define DETAIL_PANEL_CENTER_Y 0x1300
#define DETAIL_TITLE_X 294
#define DETAIL_TITLE_Y 404
#define DETAIL_COMPANY_Y 426
#define DETAIL_TEXT_WIDTH (DETAIL_PANEL_WIDTH - 0x280)
#define DETAIL_WRAPPED_TITLE_Y 394
#define DETAIL_TITLE_LINE_STEP 21
#define DETAIL_WRAPPED_COMPANY_Y 438

static menu_input_t menu_input;
static menu_input_t fallback_feedback;
static menu_motion_t menu_motion;
static u64 last_input_time;
static f32 launch_pitch, launch_yaw;
static u16 fallback_held_navigation;

_Static_assert(PAD_BUTTON_LEFT == MENU_NAV_LEFT && PAD_BUTTON_RIGHT == MENU_NAV_RIGHT &&
               PAD_BUTTON_DOWN == MENU_NAV_DOWN && PAD_BUTTON_UP == MENU_NAV_UP,
               "menu navigation bits must match PAD button bits");

// Define constants for max dimensions
void setup_icon_positions();

__attribute__((aligned(4))) static tex_data icon_texture;
__attribute__((aligned(4))) static tex_data banner_texture;

static void draw_text_aligned(char *s, s16 size, u16 x, u16 y, u8 x_align, GXColor *color) {
    static struct {
        text_group group;
        text_metadata metadata;
        char contents[255];
    } text = {
        .group = {
            .type = make_type('S','T','H','0'),
            .arr_size = 1, // arr size
        },
        .metadata = {
            .draw_metadata_index = 0,
            .text_data_offset = sizeof(text_metadata),
        },
    };

    static struct {
        text_draw_group group;
        text_draw_metadata metadata;
    } draw = {
        .group = {
            .type = make_type('G','L','H','0'),
            .metadata_offset = sizeof(text_draw_group),
        },
        .metadata = {
            .type = make_type('m','e','s','g'),
            .x = 0, // x position
            .y = 0, // y position
            .y_align = TEXT_ALIGN_CENTER,
            .x_align = TEXT_ALIGN_TOP,
            .letter_spacing = -1,
            .line_spacing = 0,
            .size = 0,
            .border_obj = 0xffff,
        }
    };

    strcpy(text.contents, s);

    draw.metadata.size = draw.metadata.line_spacing = size;
    draw.metadata.x = (x + 64) * 20;
    draw.metadata.y = (y + 64) * 10;
    draw.metadata.x_align = x_align;

    gx_draw_text(0, &text.group, &draw.group, color);
}

void draw_text(char *s, s16 size, u16 x, u16 y, GXColor *color) {
    draw_text_aligned(s, size, x, y, TEXT_ALIGN_TOP, color);
}

static void draw_text_centered(char *s, s16 size, u16 x, u16 y, GXColor *color) {
    draw_text_aligned(s, size, x, y, TEXT_ALIGN_CENTER, color);
}

static void get_display_title(gm_file_entry_t *entry, char title[TITLE_TEXT_CAPACITY]) {
    bool sjis = entry->extra.game_id[3] == 'J';
    if (entry->desc.fullGameName[0]) {
        title_copy_text(title, entry->desc.fullGameName, sizeof(entry->desc.fullGameName), sjis);
    } else if (entry->desc.gameName[0]) {
        title_copy_text(title, entry->desc.gameName, sizeof(entry->desc.gameName), sjis);
    } else {
        const char *base = strrchr(entry->path, '/');
        const char *source = base ? base + 1 : entry->path;
        title_copy_text(title, source, sizeof(entry->path) - (source - entry->path), sjis);
    }
}

__attribute_data__ GXColorS10 *menu_color_icon;
__attribute_data__ GXColorS10 *menu_color_icon_sel;

__attribute_data__ GXColorS10 *menu_color_empty;
__attribute_data__ GXColorS10 *menu_color_empty_sel;

// IPL model data is shared with the stock memory-card screen. Keep our palette
// local and bind it only for the duration of each custom model draw.
static GXColorS10 cabinet_colors[4];
static GXColorS10 selection_color = {220, 180, 45, 255};

static GXColorS10 cabinet_color(const GXColorS10 *native) {
    GXColorS10 color = *native;
    color.r = color.r * 3 / 4;
    color.g = (color.g * 3 + color.b) / 4;
    return color;
}

__attribute_data__ model global_textured_icon = {};
__attribute_data__ model global_empty_icon = {};
// __attribute_data__ BNR global_start_banner = {};

// pointers
model *textured_icon = &global_textured_icon;
model *empty_icon = &global_empty_icon;

void set_empty_icon_selected() {
    empty_icon->data->mat[0].tev_color[0] = menu_color_empty_sel;
    empty_icon->data->mat[1].tev_color[0] = menu_color_empty_sel;
}

void set_empty_icon_unselected() {
    empty_icon->data->mat[0].tev_color[0] = menu_color_empty;
    empty_icon->data->mat[1].tev_color[0] = menu_color_empty;
}

void set_textured_icon_selected() {
    textured_icon->data->mat[0].tev_color[0] = menu_color_icon_sel;
    textured_icon->data->mat[2].tev_color[0] = menu_color_icon_sel;
}

void set_textured_icon_unselected() {
    textured_icon->data->mat[0].tev_color[0] = menu_color_icon;
    textured_icon->data->mat[2].tev_color[0] = menu_color_icon;
}

__attribute_used__ void custom_gameselect_init() {
    menu_input_reset(&menu_input);
    menu_input_reset(&fallback_feedback);
    menu_motion_reset(&menu_motion, selected_slot);
    last_input_time = 0;
    fallback_held_navigation = 0;
    // default banner
    *banner_pointer = (u32)&default_opening_bin[0];
    *banner_ready = 1;

    // menu setup
    menu_blob = *ptr_menu_blob;
    game_blob_a = *ptr_game_blob_a;
    game_blob_b = *ptr_game_blob_b;


    // if (*banner_pointer) {
    //     char *names[] = {"blue", "green", "yellow", "orange", "red", "purple"};
    //     u32 colors[] = {SAVE_COLOR_BLUE, SAVE_COLOR_GREEN, SAVE_COLOR_YELLOW, SAVE_COLOR_ORANGE, SAVE_COLOR_RED, SAVE_COLOR_PURPLE};
    //     for (int i = 0; i < countof(colors); i++) {
    //         u32 color_num = colors[i];
    //         u32 color_index = 1 << (10 + 3 + color_num);
    //         GXColorS10 *color_bright = get_save_color(color_index, SAVE_ICON);
    //         GXColorS10 *color_bright_seleted = get_save_color(color_index, SAVE_ICON_SEL);
    //         GXColorS10 *color_dim = get_save_color(color_index, SAVE_EMPTY);
    //         GXColorS10 *color_dim_selected = get_save_color(color_index, SAVE_EMPTY_SEL);
    //         OSReport("color = %s\n", names[i]);

    //         DUMP_COLOR(color_bright);
    //         DUMP_COLOR(color_bright_seleted);
    //         DUMP_COLOR(color_dim);
    //         DUMP_COLOR(color_dim_selected);
    //     }

    //     while(1);
    // }

    // colors
    u32 color_num = SAVE_COLOR_PURPLE; // TODO: make a setting for this
    u32 color_index = 1 << (10 + 3 + color_num);
    for (int i = 0; i < 4; i++)
        cabinet_colors[i] = cabinet_color(get_save_color(color_index, i));
    menu_color_icon = &cabinet_colors[SAVE_ICON];
    menu_color_icon_sel = &cabinet_colors[SAVE_ICON_SEL];
    menu_color_empty = &cabinet_colors[SAVE_EMPTY];
    menu_color_empty_sel = &cabinet_colors[SAVE_EMPTY_SEL];

    // DUMP_COLOR(menu_color_icon);
    // DUMP_COLOR(menu_color_icon_sel);
    // DUMP_COLOR(menu_color_empty);
    // DUMP_COLOR(menu_color_empty_sel);

    // empty icon
    empty_icon->data = save_empty;
    model_init(empty_icon, 0);

    // textured icon
    textured_icon->data = save_icon;
    model_init(textured_icon, 0);

    // change the texture format (disc scans)
    tex_data *textured_icon_tex = &textured_icon->data->tex->dat[1];
    textured_icon_tex->format = GX_TF_RGB5A3;
    textured_icon_tex->width = 64;
    textured_icon_tex->height = 64;

    // icon texture
    icon_texture.format = GX_TF_RGB5A3;
    icon_texture.width = 32;
    icon_texture.height = 32;

    icon_texture.lodbias = 0; // used by GX_InitTexObjLOD
    icon_texture.index = 0x00;

    icon_texture.unk1 = 0x00;
    icon_texture.unk2 = 0x00;
    icon_texture.unk3 = 0x00;
    icon_texture.unk4 = 0x00;
    icon_texture.unk5 = 0x00;
    icon_texture.unk6 = 0x00;
    icon_texture.unk7 = 0x01; // used by GX_InitTexObjLOD
    icon_texture.unk8 = 0x01; // used by GX_InitTexObjLOD
    icon_texture.unk9 = 0x00;
    icon_texture.unk10 = 0x00;

    // banner image
    banner_texture.format = GX_TF_RGB5A3;
    banner_texture.width = 96;
    banner_texture.height = 32;

    banner_texture.lodbias = 0; // used by GX_InitTexObjLOD
    banner_texture.index = 0x00;

    banner_texture.unk1 = 0x00;
    banner_texture.unk2 = 0x00;
    banner_texture.unk3 = 0x00;
    banner_texture.unk4 = 0x00;
    banner_texture.unk5 = 0x00;
    banner_texture.unk6 = 0x00;
    banner_texture.unk7 = 0x01; // used by GX_InitTexObjLOD
    banner_texture.unk8 = 0x01; // used by GX_InitTexObjLOD
    banner_texture.unk9 = 0x00;
    banner_texture.unk10 = 0x00;

    // // init anim list
    // ????

    // icon positions
    setup_icon_positions();
}

int selected_slot = 0;
int top_line_num = 0;

__attribute_used__ void draw_save_icon(position_t *pos, u32 slot_num, u8 alpha, bool selected) {
    f32 sc = pos->scale;
    guVector cube_scale = {sc, sc, sc};
    bool has_texture = false;

    gm_file_entry_t *entry = gm_get_game_entry(slot_num);
    if (entry != NULL) {
        if (entry->type == GM_FILE_TYPE_PROGRAM || entry->type == GM_FILE_TYPE_DIRECTORY) {
            has_texture = true;
        } else if (entry->asset.use_banner && entry->asset.banner.state == GM_LOAD_STATE_LOADED) {
            has_texture = true;
        } else if (entry->asset.icon.state == GM_LOAD_STATE_LOADED) {
            has_texture = true;
        }
    }

    model *m = has_texture ? textured_icon : empty_icon;
    int second_material = has_texture ? 2 : 1;
    GXColorS10 *saved_color_0 = m->data->mat[0].tev_color[0];
    GXColorS10 *saved_color_1 = m->data->mat[second_material].tev_color[0];
    s16 saved_alpha = m->alpha;
    if (has_texture) {
        if (selected) {
            set_textured_icon_selected();
        } else {
            set_textured_icon_unselected();
        }
    } else {
        if (selected) {
            set_empty_icon_selected();
        } else {
            set_empty_icon_unselected();
        }
    }

    // setup camera
    set_obj_pos(m, pos->m, cube_scale);
    set_obj_cam(m, get_camera_mtx());
    change_model(m);

    // draw icon
    m->alpha = (u8)((f32)alpha * pos->opacity);
    if (has_texture) {
        // cube
        draw_partial(m, &m->data->parts[2]);
        draw_partial(m, &m->data->parts[10]);

        // icon
        tex_data *icon_tex = &m->data->tex->dat[1];
        bool uses_square_icon = entry->type == GM_FILE_TYPE_PROGRAM || entry->type == GM_FILE_TYPE_DIRECTORY || entry->asset.icon.state != GM_LOAD_STATE_NONE;
        if (uses_square_icon) {
            u32 target_texture_data = 0;
            bool settings_icon = gm_is_settings_entry(entry);
            if (entry->asset.icon.state == GM_LOAD_STATE_NONE) {
                const uint8_t *default_icon = entry->type == GM_FILE_TYPE_DIRECTORY ? &dir_tex_bin[0] : &dol_tex_bin[0];
                if (settings_icon) default_icon = &settings_tex_bin[0];
                target_texture_data = (u32)default_icon;
            } else {
                target_texture_data = (u32)entry->asset.icon.buf->data;
            }

            s32 desired_offset = (s32)((u32)target_texture_data - (u32)icon_tex);
            icon_tex->offset = desired_offset;
            icon_tex->format = GX_TF_RGB5A3;
            icon_tex->width = settings_icon && entry->asset.icon.state == GM_LOAD_STATE_NONE ? 64 : 32;
            icon_tex->height = icon_tex->width;
        } else {
            u16 *source_texture_data = (u16*)entry->asset.banner.buf->data;
            u32 target_texture_data = (u32)source_texture_data;

            s32 desired_offset = (s32)((u32)target_texture_data - (u32)icon_tex);
            icon_tex->offset = desired_offset;
            icon_tex->format = GX_TF_RGB5A3;
            icon_tex->width = 96;
            icon_tex->height = 32;
        }

        // Keep artwork inset within the cube face. GameCube banners are 96x32,
        // so flatten only their face geometry to preserve the native 3:1 ratio.
        // Give small banners more face area for legibility, then ease toward
        // roomier padding on the hero cube. Square utility icons stay unchanged.
        f32 face_inset = uses_square_icon ? GRID_ICON_FACE_SCALE : GRID_BANNER_FACE_SCALE;
        f32 hero_face_inset = uses_square_icon ? HERO_ICON_FACE_SCALE : HERO_BANNER_FACE_SCALE;
        if (sc > GRID_SELECTED_SCALE) {
            f32 hero_mix = (sc - GRID_SELECTED_SCALE) / (HERO_ICON_SCALE - GRID_SELECTED_SCALE);
            if (hero_mix > 1.0f) hero_mix = 1.0f;
            face_inset += (hero_face_inset - face_inset) * hero_mix;
        }
        f32 face_width = sc * face_inset;
        f32 face_height = uses_square_icon ? face_width : face_width / 3.0f;
        guVector face_scale = {face_width, face_height, sc};
        set_obj_pos(m, pos->m, face_scale);
        set_obj_cam(m, get_camera_mtx());
        change_model(m);

        // TODO: instead set m->data->mat[1].texmap_index[0] = 0xFFFF
        draw_partial(m, &m->data->parts[6]);
    } else {
        draw_model(m);
    }

    m->data->mat[0].tev_color[0] = saved_color_0;
    m->data->mat[second_material].tev_color[0] = saved_color_1;
    m->alpha = saved_alpha;
}

static void draw_selection_corners(position_t *pos, u8 alpha, f32 visibility) {
    model *m = empty_icon;
    GXColorS10 *saved_color_0 = m->data->mat[0].tev_color[0];
    GXColorS10 *saved_color_1 = m->data->mat[1].tev_color[0];
    s16 saved_alpha = m->alpha;
    m->data->mat[0].tev_color[0] = &selection_color;
    m->data->mat[1].tev_color[0] = &selection_color;
    m->alpha = (u8)((f32)alpha * visibility);

    // Eight little native blocks form four solid corners. Use the IPL's model
    // renderer so we don't desynchronize its GX state with libogc's state cache.
    for (int y = -1; y <= 1; y += 2) {
        for (int x = -1; x <= 1; x += 2) {
            for (int vertical = 0; vertical < 2; vertical++) {
                Mtx marker;
                C_MTXCopy(pos->m, marker);
                marker[0][3] += x * (vertical ? 35.0f : 31.5f);
                marker[1][3] += y * (vertical ? 31.5f : 35.0f);
                guVector scale = vertical ? (guVector){0.10f, 0.25f, 0.07f} :
                                             (guVector){0.25f, 0.10f, 0.07f};
                set_obj_pos(m, marker, scale);
                set_obj_cam(m, get_camera_mtx());
                change_model(m);
                draw_model(m);
            }
        }
    }
    m->data->mat[0].tev_color[0] = saved_color_0;
    m->data->mat[1].tev_color[0] = saved_color_1;
    m->alpha = saved_alpha;
}

inline u16 get_border_index() {
    u16 border_index = 0;
    switch (get_ipl_revision()) {
    case IPL_NTSC_10_001:
    case IPL_NTSC_10_002:
    case IPL_NTSC_11_001:
    case IPL_NTSC_12_001:
    case IPL_NTSC_12_101:
        border_index = 0x28;
        break;
    case IPL_PAL_10_001:
    case IPL_PAL_10_002:
    case IPL_PAL_12_101:
        border_index = 0x4b;
        break;
    case IPL_MPAL_11:
        border_index = 0x12;
        break;
    default:
        break;
    }

    return border_index;
}

__attribute_used__ void draw_info_box(u16 width, u16 height, u16 center_x, u16 center_y, u8 alpha, GXColor *top_color, GXColor *bottom_color) {
    struct {
        box_draw_group group;
        box_draw_metadata metadata;
    } blob = {
        .group = {
            .type = make_type('G','L','H','0'),
            .metadata_offset = sizeof(box_draw_group),
        },
        .metadata = {0}, // zero out
    };

    box_draw_metadata *box = &blob.metadata;
    u16 border_index = get_border_index();
    box->border_index[0] = box->border_index[1] = box->border_index[2] = box->border_index[3] = border_index;
    box->border_unk[0] = 0x27; // unk const

    s16 border_offset = 0x80;
    box->inside_center_x = border_offset;
    box->inside_center_y = border_offset;
    box->center_x = center_x;
    box->center_y = center_y;

    box->width = width;
    box->height = height;
    box->inside_width = width - (border_offset << 1);
    box->inside_height = height - (border_offset << 1);

    copy_gx_color(top_color, &box->top_color[0]);
    copy_gx_color(top_color, &box->top_color[1]);
    copy_gx_color(bottom_color, &box->bottom_color[0]);
    copy_gx_color(bottom_color, &box->bottom_color[1]);

	int inside_x = box->center_x - (box->inside_width / 2);
	int inside_y = box->center_y - (box->inside_height / 2);

    GXColor box_color = {0xA0, 0x8C, 0xD0, alpha};
	draw_box(0, &blob.group, &box_color, inside_x, inside_y, box->inside_width, box->inside_height);

    return;
}

#if 0
void patch_anim_draw() {
    prep_text_mode();

    GXColor top_color = {0x6e, 0x00, 0xb3, 0xc8};
    GXColor bottom_color = {0x80, 0x00, 0x57, 0xb4};
    draw_info_box(0x1200, 0x560, 0x1230, 0xb20, 0xff, &top_color, &bottom_color);
}
#endif

// #define WITH_SPACE 1

void setup_icon_positions() {
    for (int col = 0; col < GRID_COLUMN_COUNT; col++) {
        position_t *pos = &icons_positions[col];
        pos->scale = GRID_ICON_SCALE;
        pos->opacity = 1.0;

        f32 pos_x = GRID_BASE_X + (col * DRAW_OFFSET_X);

        C_MTXIdentity(pos->m);
        pos->m[0][3] = pos_x;
        pos->m[1][3] = 0.0;
        pos->m[2][3] = 1.0;
    }
}

__attribute_data__ Mtx global_gameselect_matrix;
__attribute_data__ Mtx global_gameselect_inverse;
void set_gameselect_view(Mtx matrix, Mtx inverse) {
    C_MTXCopy(matrix, global_gameselect_matrix);
    C_MTXCopy(inverse, global_gameselect_inverse);
}

void fix_gameselect_view() {
    GX_LoadPosMtxImm(global_gameselect_matrix,0);
    GX_LoadNrmMtxImm(global_gameselect_inverse,0);
    GXSetCurrentMtx(0);
}

__attribute_data__ u32 current_gameselect_state = SUBMENU_GAMESELECT_LOADER;

__attribute_used__ void custom_gameselect_menu(u8 broken_alpha_0, u8 alpha_1, u8 broken_alpha_2) {
    // color
    u8 ui_alpha = alpha_1;
    // u8 ui_alpha = alpha_2; // correct with animation

    // Grid motion belongs to the grid. Keep each draw's transform local so the
    // selected preview never inherits a row's movement or visibility fade.
    for (int line_num = 0; line_num < number_of_lines; line_num++) {
        line_backing_t *line_backing = &browser_lines[line_num];
        f32 line_visibility = line_backing->transparency;
        if (line_visibility <= 0 || line_backing->raw_position_y < 0 ||
            line_backing->raw_position_y >= SCREEN_BOUND_TOTAL_Y) continue;

        f32 real_position_y = SCREEN_BOUND_TOP - line_backing->raw_position_y;
        for (int col = 0; col < GRID_COLUMN_COUNT; col++) {
            int slot_num = (line_num * GRID_COLUMN_COUNT) + col;
            bool selected = slot_num == selected_slot && slot_num < game_backing_count;
            position_t pos = icons_positions[col];
            pos.scale = selected ? GRID_SELECTED_SCALE * menu_motion.selected_scale : GRID_ICON_SCALE;
            pos.opacity = line_visibility * (selected ? GRID_SELECTED_GHOST_OPACITY : GRID_ICON_OPACITY);
            pos.m[1][3] = real_position_y;
            if (selected) {
                pos.m[0][3] += menu_motion.bump_x;
                pos.m[1][3] += menu_motion.bump_y;
            }
            draw_save_icon(&pos, slot_num, alpha_1, selected);
            if (selected) draw_selection_corners(&pos, alpha_1, line_visibility);
        }
    }

    gm_file_entry_t *entry = gm_get_game_entry(selected_slot);
    if (entry != NULL && selected_slot >= 0 && selected_slot < game_backing_count) {
        // One persistent full-size preview: navigation changes its artwork in
        // place, rather than relaunching a cube across the screen every time.
        position_t hero = {.scale = HERO_ICON_SCALE, .opacity = menu_motion.pull};
        f32 pitch = menu_motion.launching ? launch_pitch : menu_input.pitch + menu_motion.hero_pitch;
        f32 yaw = menu_motion.launching ? launch_yaw : menu_input.yaw + menu_motion.hero_yaw;
        f32 pose = 1.0f - menu_motion.launch_blend;
        C_MTXIdentity(hero.m);
        apply_save_rot((s32)((520.0f + pitch) * pose),
                       (s32)((-1850.0f + yaw) * pose), 0, hero.m);
        hero.m[0][3] = HERO_POSITION_X + menu_motion.bob_x * pose;
        hero.m[1][3] = HERO_POSITION_Y - menu_motion.bob_y * pose;
        hero.m[2][3] = HERO_POSITION_Z;
        draw_save_icon(&hero, selected_slot, alpha_1, true);
    }

    // Restore the normal view before drawing the selected cube's face UI.
    fix_gameselect_view();

    if (entry != NULL && selected_slot < game_backing_count) {
        if (entry->extra.game_id[3] == 'J') switch_lang_jpn();
        else switch_lang_eng();

        char title[TITLE_TEXT_CAPACITY];
        get_display_title(entry, title);
        // Refit only when selection/metadata changes, not on every bob frame.
        static char cached_title[TITLE_TEXT_CAPACITY];
        static bool cached_sjis;
        static title_layout_t title_lines;
        bool sjis = entry->extra.game_id[3] == 'J';
        if (!title_lines.size || cached_sjis != sjis || strcmp(cached_title, title) != 0) {
            strcpy(cached_title, title);
            cached_sjis = sjis;
            title_layout(title, sjis, DETAIL_TEXT_WIDTH, font_title_glyph_width, &title_lines);
        }
        u8 title_alpha = ui_alpha; // Selection text updates immediately, without blinking.

        // Keep metadata in a stable native IPL panel below the hero cube. The
        // cube face stays dedicated to the game's banner artwork.
        GXColor screen_top = {0x18, 0x02, 0x2C, 0xE0};
        GXColor screen_bottom = {0x08, 0x00, 0x16, 0xE0};
        draw_info_box(
            DETAIL_PANEL_WIDTH,
            DETAIL_PANEL_HEIGHT,
            DETAIL_PANEL_CENTER_X,
            DETAIL_PANEL_CENTER_Y,
            title_alpha,
            &screen_top,
            &screen_bottom
        );

        prep_text_mode();
        GXColor title_color = {0xF0, 0xEC, 0xFF, title_alpha};
        int title_y = title_lines.line_count == 1 ? DETAIL_TITLE_Y : DETAIL_WRAPPED_TITLE_Y;
        for (int i = 0; i < title_lines.line_count; i++) {
            draw_text_centered(title_lines.lines[i], title_lines.size, DETAIL_TITLE_X,
                               title_y + i * DETAIL_TITLE_LINE_STEP, &title_color);
        }

        if (entry->type == GM_FILE_TYPE_GAME && entry->desc.fullCompany[0]) {
            GXColor company_color = {0xB8, 0xA9, 0xD0, title_alpha};
            int company_y = title_lines.line_count == 1 ? DETAIL_COMPANY_Y : DETAIL_WRAPPED_COMPANY_Y;
            draw_text_centered(entry->desc.fullCompany, 13, DETAIL_TITLE_X, company_y, &company_color);
        }
        switch_lang_orig();
    }

    return;
}

__attribute_used__ void original_gameselect_menu(u8 broken_alpha_0, u8 alpha_1, u8 broken_alpha_2) {
    // menu alpha
    u8 ui_alpha = alpha_1;
    GXColor white = {0xFF, 0xFF, 0xFF, ui_alpha};

    gm_file_entry_t *entry = gm_get_game_entry(selected_slot);
    if (entry == NULL) return; // protect against transition during enum
    if (entry->extra.game_id[3] == 'J') switch_lang_jpn();
    else switch_lang_eng();

    bool can_boot = emu_can_boot(entry->type);
    if (!can_boot)
        emu_draw_boot_error(entry->type, alpha_1);

    if (entry->type == GM_FILE_TYPE_GAME && entry->asset.banner.state == GM_LOAD_STATE_LOADED) {
        // game banner
        setup_tex_draw(1, 0, 1);
        banner_texture.offset = (s32)((u32)(entry->asset.banner.buf->data) - (u32)&banner_texture);
        draw_blob_tex(make_type('b','a','n','a'), game_blob_b, &white, &banner_texture);
    }

    // game info
    prep_text_mode();
    draw_blob_text(make_type('t','i','t','l'), game_blob_b, &white, entry->desc.fullGameName, 0x40);
    if (entry->type == GM_FILE_TYPE_GAME) {
        draw_blob_text(make_type('m','a','k','r'), game_blob_b, &white, entry->desc.fullCompany, 0x40);
        draw_blob_text_long(make_type('i','n','f','o'), game_blob_b, &white, entry->desc.description, 0x80);
    } else {
        draw_blob_text(make_type('m','a','k','r'), game_blob_b, &white, entry->desc.description, 0x40);
    }

    // press start anim
    if (can_boot)
        draw_start_anim(ui_alpha); // TODO: fix alpha timing

    // fix camera again
    setup_gameselect_menu(0, 0, 0);

    // start string
    switch_lang_orig();
    if (can_boot)
        draw_blob_fixed(game_blob_text, game_blob_a, game_blob_b, &white);

    return;
}

static bool first_transition = true;
static bool in_submenu_transition = false;
static u8 custom_menu_transition_alpha = 0xFF;
static u8 original_menu_transition_alpha = 0;
__attribute_used__ void pre_menu_alpha_setup() {
    menu_alpha_setup(); // run original function

    if (*cur_menu_id == MENU_GAMESELECT_ID && *prev_menu_id == MENU_GAMESELECT_TRANSITION_ID) {
        OSReport("Resetting back to SUBMENU_GAMESELECT_LOADER\n");
        current_gameselect_state = SUBMENU_GAMESELECT_LOADER;
        menu_input_reset(&menu_input);
        last_input_time = 0;

        if (first_transition) {
            Jac_PlaySe(SOUND_MENU_ENTER);
            first_transition = false;
        }
    }
}

__attribute_used__ void mod_gameselect_draw(u8 alpha_0, u8 alpha_1, u8 alpha_2) {
    // this is for the camera
    setup_gameselect_menu(0, 0, 0);

    // Use the stock IPL background treatment and animation.
    draw_grid(global_gameselect_matrix, alpha_1);

    // TODO: use GXColor instead of alpha byte
    u8 custom_alpha_1 = custom_menu_transition_alpha;
    u8 original_alpha_1 = original_menu_transition_alpha;

    if (alpha_1 != 0xFF) {
        custom_alpha_1 = alpha_1;
    }

    if (custom_alpha_1 != 0) custom_gameselect_menu(alpha_0, custom_alpha_1, alpha_2);
    if (original_alpha_1 != 0) original_gameselect_menu(0, original_alpha_1, 0); // fix alpha_0 + alpha_2?

    return;
}

__attribute_used__ s32 handle_gameselect_inputs() {
    u64 now = gettime();
    f32 elapsed_ms = last_input_time ? (f32)diff_usec(last_input_time, now) / 1000.0f : 16.667f;
    last_input_time = now;
    menu_pad_sample_t pad = ipl_input_sample();
    bool begin_launch = false;
    // Use the sample captured before the IPL synthesizes D-pad buttons from
    // both sticks. pad_status->pad is NOT a raw controller sample.
    uint16_t navigation_down = menu_input_update(
        &menu_input, pad.buttons, pad.main_x, pad.main_y, pad.c_x, pad.c_y,
        pad.connected &&
            current_gameselect_state == SUBMENU_GAMESELECT_LOADER && !in_submenu_transition,
        elapsed_ms);
    // An unfamiliar IPL must remain navigable even if capture cannot install.
    if (!ipl_input_available()) {
        u16 native = pad_status->analog_down;
        navigation_down = ((native & ANALOG_LEFT) ? MENU_NAV_LEFT : 0) |
                          ((native & ANALOG_RIGHT) ? MENU_NAV_RIGHT : 0) |
                          ((native & ANALOG_DOWN) ? MENU_NAV_DOWN : 0) |
                          ((native & ANALOG_UP) ? MENU_NAV_UP : 0);
        u16 released = pad_status->analog_up;
        fallback_held_navigation &= ~(((released & ANALOG_LEFT) ? MENU_NAV_LEFT : 0) |
                                     ((released & ANALOG_RIGHT) ? MENU_NAV_RIGHT : 0) |
                                     ((released & ANALOG_DOWN) ? MENU_NAV_DOWN : 0) |
                                     ((released & ANALOG_UP) ? MENU_NAV_UP : 0));
        fallback_held_navigation |= navigation_down;
        if (current_gameselect_state != SUBMENU_GAMESELECT_LOADER || in_submenu_transition) {
            navigation_down = 0;
            menu_input_reset(&fallback_feedback);
        }
        fallback_feedback.held_navigation = fallback_held_navigation;
    }

    grid_update_icon_positions();

    // TODO: this code is so annoying haha... I should add a direction var
    // TODO: only works with numbers that do not divide into 255 (switch to floats?)
    u8 transition_step = 14;
    if (rmode->viTVMode >> 2 != VI_NTSC) transition_step = 16;
    if (in_submenu_transition) {
        if (custom_menu_transition_alpha != 0 && original_menu_transition_alpha != 0) {
            if ((255 - custom_menu_transition_alpha) < transition_step || (255 - original_menu_transition_alpha) < transition_step) {
                in_submenu_transition = false;

                custom_menu_transition_alpha = custom_menu_transition_alpha < 127 ? 0 : 255;
                original_menu_transition_alpha = original_menu_transition_alpha < 127 ? 0 : 255;
            }
        }
    }

    if (in_submenu_transition) {
        switch(current_gameselect_state) {
            case SUBMENU_GAMESELECT_LOADER:
                custom_menu_transition_alpha += transition_step;
                original_menu_transition_alpha -= transition_step;
                break;
            case SUBMENU_GAMESELECT_START:
                custom_menu_transition_alpha -= transition_step;
                original_menu_transition_alpha += transition_step;
                break;
            default:
        }
    }

    if (pad_status->buttons_down & PAD_TRIGGER_Z) {
        if (emu_has_dvd()) {
            Jac_StopSoundAll();
            Jac_PlaySe(SOUND_MENU_FINAL);

            extern u32 start_passthrough_game;
            start_passthrough_game = 1;
            *bs2start_ready = 1;
        }
        
        // add test code here
        /*load_stub(); // exit to loader again
        u32 *sig = (u32*)0x80001804;
        if ((*sig++ == 0x53545542 || *sig++ == 0x53545542) && *sig == 0x48415858) {
            static void (*reload)(void) = (void(*)(void))0x80001800;
            run(reload);
        }*/
    }

    if (pad_status->buttons_down & PAD_BUTTON_B) {
        if (current_gameselect_state == SUBMENU_GAMESELECT_START && !in_submenu_transition) {
            in_submenu_transition = true;
            current_gameselect_state = SUBMENU_GAMESELECT_LOADER;
            menu_motion_reset(&menu_motion, selected_slot);
            Jac_PlaySe(SOUND_SUBMENU_EXIT);
        } else if (!in_submenu_transition) {
            // TODO: check current path depth
            if (strcmp(game_enum_path, "/") != 0) {
                gm_deinit_thread();
                menu_motion_reset(&menu_motion, -1);
                Jac_PlaySe(SOUND_MENU_EXIT);
                gm_start_thread("..");
            } else {
                menu_motion_reset(&menu_motion, selected_slot);
                *banner_pointer = (u32)&default_opening_bin[0]; // banner reset
                Jac_PlaySe(SOUND_MENU_EXIT);
                return MENU_GAMESELECT_ID;
            }
        }
    }

    if (pad_status->buttons_down & PAD_BUTTON_A && current_gameselect_state == SUBMENU_GAMESELECT_LOADER) {
        if (selected_slot < game_backing_count && !in_submenu_transition) {
            gm_file_entry_t *entry = gm_get_game_entry(selected_slot);
            if (entry->type == GM_FILE_TYPE_DIRECTORY) {
                OSReport("Selected DIR slot: %d (%p)\n", selected_slot, entry);

                gm_deinit_thread();
                menu_motion_reset(&menu_motion, -1);
                Jac_PlaySe(SOUND_SUBMENU_ENTER);

                char path[128];
                sprintf(path, "%s/", entry->path);
                gm_start_thread(path);
            } else {
                in_submenu_transition = true;
                current_gameselect_state = SUBMENU_GAMESELECT_START;
                launch_pitch = menu_input.pitch + menu_motion.hero_pitch;
                launch_yaw = menu_input.yaw + menu_motion.hero_yaw;
                begin_launch = true;

                Jac_PlaySe(SOUND_SUBMENU_ENTER);
                setup_gameselect_anim();
                setup_cube_anim();

                if (entry->type == GM_FILE_TYPE_GAME) {
                    mcp_set_gameid(entry);
                }

                // OSReport("Selected slot: %d (%p)\n", selected_slot, asset);
            }
        }
    }

    if (pad_status->buttons_down & PAD_BUTTON_START && current_gameselect_state == SUBMENU_GAMESELECT_START) {
        Jac_StopSoundAll();
        Jac_PlaySe(SOUND_MENU_FINAL);
        gm_file_entry_t *entry = gm_get_game_entry(selected_slot);

        if (!emu_can_boot(entry->type))
            return MENU_GAMESELECT_TRANSITION_ID;

        memcpy(&boot_entry, entry, sizeof(gm_file_entry_t));
        if (boot_entry.second != NULL) {
            memcpy(&second_boot_entry, boot_entry.second, sizeof(gm_file_entry_t));
            boot_entry.second = &second_boot_entry;
        }
        *bs2start_ready = 1;
    }

    u16 blocked_navigation = 0;
    if (current_gameselect_state == SUBMENU_GAMESELECT_LOADER) {
        if (navigation_down & MENU_NAV_RIGHT) {
            if ((selected_slot % GRID_COLUMN_COUNT) == (GRID_COLUMN_COUNT - 1) || selected_slot + 1 >= game_backing_count) {
                blocked_navigation |= MENU_NAV_RIGHT;
            }
            else {
                Jac_PlaySe(SOUND_CARD_MOVE);
                selected_slot++;
            }
        }

        if (navigation_down & MENU_NAV_LEFT) {
            if ((selected_slot % GRID_COLUMN_COUNT) == 0) {
                blocked_navigation |= MENU_NAV_LEFT;
            }
            else {
                Jac_PlaySe(SOUND_CARD_MOVE);
                selected_slot--;
            }
        }

        if (navigation_down & MENU_NAV_DOWN) {
            if (selected_slot + GRID_COLUMN_COUNT >= game_backing_count) {
                // OSReport("SKIP MOVE DOWN: top_line_num = %d\n", top_line_num);
                blocked_navigation |= MENU_NAV_DOWN;
            } else {
                Jac_PlaySe(SOUND_CARD_MOVE);
                line_backing_t *line_backing = &browser_lines[selected_slot / GRID_COLUMN_COUNT];
                if (get_position_after(line_backing) >= DRAW_BOUND_BOTTOM - DRAW_OFFSET_Y - 10) {
                    if (gm_can_move() && grid_dispatch_navigate_down() == GRID_MOVE_SUCCESS) {
                        gm_line_changed(1);
                        selected_slot += GRID_COLUMN_COUNT;
                        top_line_num++;
                    }
                } else {
                    selected_slot += GRID_COLUMN_COUNT;
                }
            }
        }

        if (navigation_down & MENU_NAV_UP) {
            if (top_line_num == 0 && (selected_slot - GRID_COLUMN_COUNT) < 0) {
                // OSReport("SKIP MOVE UP: top_line_num = %d\n", top_line_num);
                blocked_navigation |= MENU_NAV_UP;
            } else {
                Jac_PlaySe(SOUND_CARD_MOVE);
                line_backing_t *line_backing = &browser_lines[selected_slot / GRID_COLUMN_COUNT];
                if (top_line_num != 0 && get_position_after(line_backing) <= DRAW_BOUND_TOP + DRAW_OFFSET_Y - 10) {
                    if (gm_can_move() && grid_dispatch_navigate_up() == GRID_MOVE_SUCCESS) {
                        gm_line_changed(-1);
                        selected_slot -= GRID_COLUMN_COUNT;
                        top_line_num--;
                    }
                } else {
                    selected_slot -= GRID_COLUMN_COUNT;
                }
            }
            
            // OSReport("top_line_num = %d\n", top_line_num);
            // OSReport("selected_slot = %d\n", selected_slot);
        }
    }

    u16 feedback = menu_input_blocked_feedback(
        ipl_input_available() ? &menu_input : &fallback_feedback,
        navigation_down, blocked_navigation);
    bool visible = current_gameselect_state == SUBMENU_GAMESELECT_LOADER ||
                   (in_submenu_transition && custom_menu_transition_alpha != 0);
    bool activity = menu_input.user_activity || pad_status->buttons_down ||
                    (!ipl_input_available() && fallback_held_navigation);
    menu_motion_update(&menu_motion, elapsed_ms, selected_slot,
                        visible && selected_slot < game_backing_count,
                        activity, menu_input.inspecting);
    // Resolve transient events after the selected object has been initialized,
    // including A or a boundary press on the very first active menu frame.
    if (begin_launch) menu_motion_launch(&menu_motion);
    if (feedback) {
        int x = ((feedback & MENU_NAV_RIGHT) != 0) - ((feedback & MENU_NAV_LEFT) != 0);
        int y = ((feedback & MENU_NAV_UP) != 0) - ((feedback & MENU_NAV_DOWN) != 0);
        menu_motion_bump(&menu_motion, x, y);
        Jac_PlaySe(SOUND_CARD_ERROR);
    }
    return MENU_GAMESELECT_TRANSITION_ID;
}

__attribute_data__ u8 show_watermark = 1;
void alpha_watermark(void) {
    if (!show_watermark) return;
    prep_text_mode();

    GXColor yellow_alpha = {0xFF, 0xFF, 0x00, 0x80};
    draw_text("BETA TEST", 24, 330, 0, &yellow_alpha);
    draw_text("cubeboot rc" CONFIG_BETA_RC, 22, 330, 28, &yellow_alpha);
}

/*  This file is part of emerald

    Copyright (C) 2006  Novell, Inc.
    Copyright (C) 2013  Povilas Kanapickas <povilas@radix.lt>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef EMERALD_LIBENGINE_EMERALD_H
#define EMERALD_LIBENGINE_EMERALD_H
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// include gtkmm before decoration.h
#include <gtkmm.h>

#include <decoration.h>

/*
#ifndef GDK_DISABLE_DEPRECATED
#define GDK_DISABLE_DEPRECATED
#endif

#ifndef GTK_DISABLE_DEPRECATED
#define GTK_DISABLE_DEPRECATED
#endif
*/

#include <gtk/gtk.h>
#include <gtk/gtkwindow.h>
#include <gdk/gdkx.h>

#define IS_VALID(o) (o && o->parent_instance.ref_count)

#ifdef USE_DBUS
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#endif

//#include <gconf/gconf-client.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <wnckmm.h>
#include <libwnck/libwnck.h>
#include <libwnck/window-action-menu.h>

#include <cairo.h>
#include <cairo-xlib.h>

#include <pango/pango-context.h>
#include <pango/pangocairo.h>

#include <libengine/keyfile.h>

#include <dlfcn.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <libintl.h>
#include <locale.h>
#define _(String) gettext (String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

#define FAKE_WINDOW_ACTION_HELP (1 << 20)

#define WM_MOVERESIZE_SIZE_TOPLEFT      0
#define WM_MOVERESIZE_SIZE_TOP          1
#define WM_MOVERESIZE_SIZE_TOPRIGHT     2
#define WM_MOVERESIZE_SIZE_RIGHT        3
#define WM_MOVERESIZE_SIZE_BOTTOMRIGHT  4
#define WM_MOVERESIZE_SIZE_BOTTOM       5
#define WM_MOVERESIZE_SIZE_BOTTOMLEFT   6
#define WM_MOVERESIZE_SIZE_LEFT         7
#define WM_MOVERESIZE_MOVE              8
#define WM_MOVERESIZE_SIZE_KEYBOARD     9
#define WM_MOVERESIZE_MOVE_KEYBOARD    10

#define SHADOW_RADIUS      8.0
#define SHADOW_OPACITY     0.5
#define SHADOW_COLOR_RED   0x0000
#define SHADOW_COLOR_GREEN 0x0000
#define SHADOW_COLOR_BLUE  0x0000
#define SHADOW_OFFSET_X    1
#define SHADOW_OFFSET_Y    1

#define MWM_HINTS_DECORATIONS (1L << 1)

#define MWM_DECOR_ALL      (1L << 0)
#define MWM_DECOR_BORDER   (1L << 1)
#define MWM_DECOR_HANDLE   (1L << 2)
#define MWM_DECOR_TITLE    (1L << 3)
#define MWM_DECOR_MENU     (1L << 4)
#define MWM_DECOR_MINIMIZE (1L << 5)
#define MWM_DECOR_MAXIMIZE (1L << 6)

#define BLUR_TYPE_NONE     0
#define BLUR_TYPE_TITLEBAR 1
#define BLUR_TYPE_ALL      2

#define PROP_MOTIF_WM_HINT_ELEMENTS 3

struct MwmHints {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
};

//static double decoration_alpha = 0.5; //Decoration Alpha

#define SWITCHER_SPACE     40
#define SWITCHER_TOP_EXTRA 4

struct decor_color_t {
    double r;
    double g;
    double b;
};


#include <libengine/titlebar.h>

typedef void (*event_callback)(Wnck::Window* win, XEvent* event);

#define ACOLOR(pfs,idn,zr,zg,zb,za) \
    (pfs)->idn.color.r = (zr);\
    (pfs)->idn.color.g = (zg);\
    (pfs)->idn.color.b = (zb);\
    (pfs)->idn.alpha   = (za);

#define CCOLOR(pfs,idn,zc) \
    (pfs)->idn.color.r = ((pfs)->color_contrast * pfs->zc.color.r);\
    (pfs)->idn.color.g = ((pfs)->color_contrast * pfs->zc.color.g);\
    (pfs)->idn.color.b = ((pfs)->color_contrast * pfs->zc.color.b);\
    (pfs)->idn.alpha   = ((pfs)->alpha_contrast * pfs->zc.alpha);

struct alpha_color {
    decor_color_t color;
    double alpha;
};

struct pos_t {
    int x, y, w, h;
    int xw, yh, ww, hh;
};

struct frame_settings;

struct window_settings {
    window_settings();

    void* engine_ws;
    int button_offset;
    int button_hoffset;
    std::string tobj_layout;

    int double_click_action;
    int button_hover_cursor;

    bool round_top_left;
    bool round_top_right;
    bool round_bottom_left;
    bool round_bottom_right;

    frame_settings* fs_act;
    frame_settings* fs_inact;
    int min_titlebar_height;
    bool use_pixmap_buttons;// = false;
    double  corner_radius;//    =   5.0;
    PangoAlignment title_text_align;// = PANGO_ALIGN_CENTER;
    GdkPixbuf* ButtonPix[S_COUNT* B_COUNT];
    GdkPixbuf* ButtonArray[B_COUNT];

    bool    use_button_glow;
    bool    use_button_inactive_glow;
    bool    use_decoration_cropping;
    bool    use_button_fade;
    GdkPixbuf* ButtonGlowPix[B_COUNT];
    GdkPixbuf* ButtonGlowArray;
    GdkPixbuf* ButtonInactiveGlowArray;
    GdkPixbuf* ButtonInactiveGlowPix[B_COUNT];
    int         button_fade_num_steps;        // number of steps
    int         button_fade_step_duration;    // step duration in milliseconds
    int         button_fade_pulse_len_steps;  // length of pulse (number of steps)
    int         button_fade_pulse_wait_steps; // how much pulse waits before fade out
    /* = {
    { 0, 6, 16, 16,   1, 0, 0, 0 },
    { 0, 6, 16, 16,   1, 0, 0, 0 },
    { 0, 6, 16, 16,   1, 0, 0, 0 },
    { 0, 6, 16, 16,   1, 0, 0, 0 },
    };*/
    double shadow_radius;
    double shadow_opacity;
    int    shadow_color[3];
    int    shadow_offset_x;
    int    shadow_offset_y;
    decor_extents_t shadow_extents;//   = { 0, 0, 0, 0 };
    decor_extents_t win_extents;//      = { 6, 6, 4, 6 };
    pos_t pos[3][3];
    int left_space;//   = 6;
    int right_space;//  = 6;
    int top_space;//    = 4;
    int bottom_space;// = 6;

    int left_corner_space;//   = 0;
    int right_corner_space;//  = 0;
    int top_corner_space;//    = 0;
    int bottom_corner_space;// = 0;

    int titlebar_height;// = 17; //Titlebar Height

    int normal_top_corner_space;//      = 0;

    int shadow_left_space;//   = 0;
    int shadow_right_space;//  = 0;
    int shadow_top_space;//    = 0;
    int shadow_bottom_space;// = 0;

    int shadow_left_corner_space;//   = 0;
    int shadow_right_corner_space;//  = 0;
    int shadow_top_corner_space;//    = 0;
    int shadow_bottom_corner_space;// = 0;


    GdkPixmap* shadow_pixmap;// = NULL;
    GdkPixmap* large_shadow_pixmap;// = NULL;
    GdkPixmap* decor_normal_pixmap;// = NULL;
    GdkPixmap* decor_active_pixmap;// = NULL;

    cairo_pattern_t* shadow_pattern;// = NULL;

    int            text_height;

    PangoFontDescription* font_desc;
    PangoContext* pango_context;

    decor_extents_t switcher_extents;// = { 0, 0, 0, 0 };
    GdkPixmap* switcher_pixmap;// = NULL;
    GdkPixmap* switcher_buffer_pixmap;// = NULL;
    int      switcher_width;
    int      switcher_height;

    int switcher_top_corner_space;//    = 0;
    int switcher_bottom_corner_space;// = 0;

    struct _icon_size {
        int w, h;
    } c_icon_size[B_T_COUNT],
    c_glow_size; // one glow size for all buttons
    // (buttons will be centered in their glows)
    // active and inactive glow pixmaps are assumed to be of same size
    bool stretch_sides;
    int blur_type;// = BLUR_TYPE_NONE;

};

struct frame_settings {
    void* engine_fs;
    window_settings* ws;
    alpha_color button;
    alpha_color button_halo;
    alpha_color text;
    alpha_color text_halo;
};

struct rectangle_t {
    int        x1, y1, x2, y2;
};

struct button_fade_info_t {
    void** d;  // needed by the timer function
    cairo_t* cr;
    double    y1;
    int  counters[B_T_COUNT]; // 0: not fading, > 0: fading in, < 0: fading out
    // max value:  ws->button_fade_num_steps+1 (1 is reserved to indicate
    //                                          fade-in initiation)
    // min value: -ws->button_fade_num_steps
    bool pulsating[B_T_COUNT];
    int    timer;
    bool first_draw;
};

struct button_region_t {
    int        base_x1, base_y1, base_x2, base_y2; // button coords with no glow
    int        glow_x1, glow_y1, glow_x2, glow_y2; // glow coordinates

    // holds whether this button's glow overlap with the other button's non-glow (base) area
    bool    overlap_buttons[B_T_COUNT];
    GdkPixmap* bg_pixmap;
};

struct decor_t {

    decor_t();

    Window        event_windows[3][3];
    Window        button_windows[B_T_COUNT];
    unsigned         button_states[B_T_COUNT];
    int tobj_pos[3];
    int tobj_size[3];
    int tobj_item_pos[11];
    int tobj_item_state[11];
    int tobj_item_width[11];
    GdkPixmap*         pixmap;
    GdkPixmap*         buffer_pixmap;
    GdkGC*         gc;
    int          width;
    int          height;
    int              client_width;
    int              client_height;
    bool          decorated;
    bool          active;
    PangoLayout*       layout;
    std::string        name;
    cairo_pattern_t*   icon;
    GdkPixmap*         icon_pixmap;
    GdkPixbuf*         icon_pixbuf;
    Wnck::WindowState   state;
    Wnck::WindowActions actions;
    XID           prop_xid;
    GtkWidget*         force_quit_dialog;
    frame_settings* fs;
    void (*draw)(decor_t* d);
    button_region_t   button_region[B_T_COUNT];
    rectangle_t       min_drawn_buttons_region; // minimal rectangle enclosing all drawn regions
    bool          draw_only_buttons_region;
    int              button_last_drawn_state[B_T_COUNT]; // last drawn state or fade counter
    button_fade_info_t button_fade_info;
    GdkPixmap* p_active, * p_active_buffer;
    GdkPixmap* p_inactive, * p_inactive_buffer;
    button_region_t   button_region_inact[B_T_COUNT];
    bool only_change_active;
};

#define LFACSS(f,ws,zc,sec) \
    load_color_setting(f,&ws->fs_act->zc.color,"active_" #zc , #sec);\
    load_color_setting(f,&ws->fs_inact->zc.color,"inactive_" #zc , #sec);\
    load_float_setting(f,&ws->fs_act->zc.alpha,"active_" #zc "_alpha", #sec);\
    load_float_setting(f,&ws->fs_inact->zc.alpha,"inactive_" #zc "_alpha", #sec);

#define SHADE_LEFT   (1 << 0)
#define SHADE_RIGHT  (1 << 1)
#define SHADE_TOP    (1 << 2)
#define SHADE_BOTTOM (1 << 3)

#define CORNER_TOPLEFT     (1 << 0)
#define CORNER_TOPRIGHT    (1 << 1)
#define CORNER_BOTTOMRIGHT (1 << 2)
#define CORNER_BOTTOMLEFT  (1 << 3)

#endif

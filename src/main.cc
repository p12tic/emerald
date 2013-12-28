/*
 * Copyright © 2006 Novell, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <emerald.h>
#include <engine.h>
#include "cairo_utils.h"
#include "window.h"
#include "utils.h"
#include "decor.h"
#include "quads.h"

//#define BASE_PROP_SIZE 12
//#define QUAD_PROP_SIZE 9


#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xregion.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrender.h>

#ifndef DECOR_INTERFACE_VERSION
#define DECOR_INTERFACE_VERSION 0
#endif

#include <algorithm>
#include <vector>

#if defined (HAVE_LIBWNCK_2_19_4)
#define wnck_window_get_geometry wnck_window_get_client_window_geometry
#endif

void reload_all_settings(int sig);

GdkPixmap* pdeb;
bool do_reload = false;

Atom frame_window_atom;
Atom win_decor_atom;
Atom win_blur_decor_atom;
Atom wm_move_resize_atom;
Atom restack_window_atom;
Atom select_window_atom;
Atom net_wm_context_help_atom;
Atom wm_protocols_atom;
Atom mwm_hints_atom;
Atom switcher_fg_atom;

Atom toolkit_action_atom;
Atom toolkit_action_window_menu_atom;
Atom toolkit_action_force_quit_dialog_atom;

Atom emerald_sigusr1_atom;

Atom utf8_string_atom;

Time dm_sn_timestamp;

std::string program_name;

GtkWidget* style_window;

GHashTable* frame_table;
GtkWidget* action_menu = NULL;
bool action_menu_mapped = false;
int double_click_timeout = 250;

Gtk::Window* tip_window;
Gtk::Label* tip_label;
GTimeVal tooltip_last_popdown = { -1, -1 };
int tooltip_timer_tag = 0;

std::vector<decor_t*> g_draw_list;

bool enable_tooltips = true;
std::string engine;

window_settings* global_ws;

_cursor cursor[3][3] = {
    {
        C(top_left_corner), C(top_side), C(top_right_corner)
    },
    {
        C(left_side), C(left_ptr), C(right_side)
    },
    {
        C(bottom_left_corner), C(bottom_side), C(bottom_right_corner)
    }
};
_cursor button_cursor = C(hand2);

static void
update_default_decorations(GdkScreen* screen, frame_settings* fs_act,
                           frame_settings* fs_inact)
{
    long* data = NULL;
    Window xroot;
    GdkDisplay* gdkdisplay = gdk_display_get_default();
    Display* xdisplay = gdk_x11_display_get_xdisplay(gdkdisplay);
    Atom bareAtom, activeAtom;
    decor_t d;
    unsigned int nQuad;
    decor_quad_t quads[N_QUADS_MAX];
    window_settings* ws = fs_act->ws;        // hackish, I know, FIXME
    decor_extents_t extents = ws->win_extents;

    bzero(&d, sizeof(decor_t));

    xroot = RootWindowOfScreen(gdk_x11_screen_get_xscreen(screen));

    bareAtom = XInternAtom(xdisplay, DECOR_BARE_ATOM_NAME, false);
    activeAtom = XInternAtom(xdisplay, DECOR_ACTIVE_ATOM_NAME, false);

    if (ws->shadow_pixmap) {
        int width, height;

        gdk_drawable_get_size(ws->shadow_pixmap, &width, &height);

        nQuad = set_shadow_quads(quads, width, height, ws);

        data = decor_alloc_property(1, WINDOW_DECORATION_TYPE_PIXMAP);

        decor_quads_to_property(data, 0, GDK_PIXMAP_XID(ws->shadow_pixmap),
                                &ws->shadow_extents, &ws->shadow_extents, &ws->shadow_extents, &ws->shadow_extents,
                                0, 0, quads, nQuad, 0xffffff, 0, 0);

        XChangeProperty(xdisplay, xroot,
                        bareAtom,
                        XA_INTEGER,
                        32, PropModeReplace, (guchar*) data,
                        PROP_HEADER_SIZE + BASE_PROP_SIZE + QUAD_PROP_SIZE * N_QUADS_MAX);

    } else {
        XDeleteProperty(xdisplay, xroot, bareAtom);
    }

    d.width =
        ws->left_space + ws->left_corner_space + 200 +
        ws->right_corner_space + ws->right_space;
    d.height =
        ws->top_space + ws->titlebar_height +
        ws->normal_top_corner_space + 2 + ws->bottom_corner_space +
        ws->bottom_space;

    extents.top += ws->titlebar_height;

    d.buffer_pixmap = NULL;
    d.layout = NULL;
    d.icon = NULL;
    d.state = 0;
    d.actions = 0;
    d.prop_xid = 0;
    d.draw = draw_window_decoration;
    d.only_change_active = false;

    reset_buttons_bg_and_fade(&d);

    if (ws->decor_normal_pixmap) {
        g_object_unref(G_OBJECT(ws->decor_normal_pixmap));
    }
    if (ws->decor_active_pixmap) {
        g_object_unref(G_OBJECT(ws->decor_active_pixmap));
    }

    nQuad = my_set_window_quads(quads, d.width, d.height, ws, false, false);

    ws->decor_active_pixmap = ws->decor_normal_pixmap =
                                  create_pixmap(MAX(d.width, d.height),
                                          ws->top_space + ws->left_space + ws->right_space +
                                          ws->bottom_space + ws->titlebar_height);

    g_object_ref(G_OBJECT(ws->decor_active_pixmap));

    if (ws->decor_normal_pixmap && ws->decor_active_pixmap) {
        d.p_inactive = ws->decor_normal_pixmap;
        d.p_active = ws->decor_active_pixmap;
        d.p_active_buffer = NULL;
        d.p_inactive_buffer = NULL;
        d.active = false;
        d.fs = fs_inact;

        (*d.draw)(&d);

        if (!data) {
            data = decor_alloc_property(1, WINDOW_DECORATION_TYPE_PIXMAP);
        }

        decor_quads_to_property(data, 0, GDK_PIXMAP_XID(d.p_inactive),
                                &extents, &extents, &extents, &extents,
                                0, 0, quads, nQuad, 0xffffff, 0, 0);

        decor_quads_to_property(data, 0, GDK_PIXMAP_XID(d.p_active),
                                &extents, &extents, &extents, &extents,
                                0, 0, quads, nQuad, 0xffffff, 0, 0);

        XChangeProperty(xdisplay, xroot,
                        activeAtom,
                        XA_INTEGER,
                        32, PropModeReplace, (guchar*) data,
                        PROP_HEADER_SIZE + BASE_PROP_SIZE + QUAD_PROP_SIZE * N_QUADS_MAX);
    }

    if (data) {
        free(data);
    }
}

/* stolen from gtktooltip.c */

#define DEFAULT_DELAY 500                /* Default delay in ms */
#define STICKY_DELAY 0                        /* Delay before popping up next tip
                                         * if we're sticky
                                         */
#define STICKY_REVERT_DELAY 1000        /* Delay before sticky tooltips revert
                                         * to normal
                                         */

static void show_tooltip(const char* text)
{
    if (!enable_tooltips) {
        return;
    }
    int x, y, w, h;

    auto gdkdisplay = Gdk::Display::get_default();

    tip_label->set_text(text);

    Gtk::Requisition req;
    tip_window->set_size_request(req.width, req.height);

    w = req.width;
    h = req.height;

    /*
    Gdk::ModifierType dummy;
    Glib::RefPtr<Gdk::Screen> screen;
    gdkdisplay->get_pointer(screen, x, y, dummy);
        get_pointer is buggy. It takes a reference when it should not.
    */
    GdkScreen* screen;
    gdk_display_get_pointer(gdkdisplay->gobj(), &screen, x, y, nullptr);

    x -= (w / 2 + 4);

    //int monitor_num = screen->get_monitor_at_point(x, y);
    int monitor_num = gdk_screen_get_monitor_at_point(screen, x, y);
    Gdk::Rectangle monitor;
    //screen->get_monitor_geometry(monitor_num, monitor);
    gdk_screen_get_monitor_geometry(screen, monitor_num, monitor.gobj());

    if ((x + w) > monitor.get_x() + monitor.get_width()) {
        x -= (x + w) - (monitor.get_x() + monitor.get_width());
    } else if (x < monitor.get_x()) {
        x = monitor.get_x();
    }

    if ((y + h + 16) > monitor.get_y() + monitor.get_height()) {
        y = y - h - 16;
    } else {
        y = y + 16;
    }

    tip_window->move(x, y);
    tip_window->show();
}

static void hide_tooltip(void)
{
    if (tip_window->get_visible()) {
        g_get_current_time(&tooltip_last_popdown);
    }

    tip_window->hide();

    if (tooltip_timer_tag) {
        g_source_remove(tooltip_timer_tag);
        tooltip_timer_tag = 0;
    }
}

static bool tooltip_recently_shown(void)
{
    GTimeVal now;
    glong msec;

    g_get_current_time(&now);

    msec = (now.tv_sec - tooltip_last_popdown.tv_sec) * 1000 +
           (now.tv_usec - tooltip_last_popdown.tv_usec) / 1000;

    return (msec < STICKY_REVERT_DELAY);
}

static int tooltip_timeout(void* data)
{
    tooltip_timer_tag = 0;

    show_tooltip((const char*)data);

    return false;
}

static void tooltip_start_delay(const char* text)
{
    unsigned delay = DEFAULT_DELAY;

    if (tooltip_timer_tag) {
        return;
    }

    if (tooltip_recently_shown()) {
        delay = STICKY_DELAY;
    }

    tooltip_timer_tag = g_timeout_add(delay,
                                      tooltip_timeout, (void*) text);
}

static int tooltip_paint_window()
{
    Gtk::Requisition req = tip_window->size_request();
    Gdk::Rectangle dummy;
    tip_window->get_style()->paint_box(tip_window->get_window(),
                                       Gtk::STATE_NORMAL,
                                       Gtk::SHADOW_OUT,
                                       dummy,
                                       *tip_window, "tooltip", 0, 0, req.width,
                                       req.height);

    return false;
}

bool tooltip_paint_window_lambda(GdkEventExpose*)
{
    return tooltip_paint_window();
}

static bool create_tooltip_window()
{
    tip_window = new Gtk::Window(Gtk::WINDOW_POPUP); // FIXME: this is a leak

    tip_window->set_app_paintable(true);
    tip_window->set_resizable(false);
    tip_window->set_name("gtk-tooltips");
    tip_window->set_border_width(4);

#if GTK_CHECK_VERSION (2, 10, 0)
    if (!gtk_check_version(2, 10, 0)) {
        tip_window->set_type_hint(Gdk::WINDOW_TYPE_HINT_TOOLTIP);
    }
#endif

    tip_window->signal_expose_event().connect(&tooltip_paint_window_lambda);

    tip_label = Gtk::manage(new Gtk::Label());
    tip_label->set_line_wrap();
    tip_label->set_alignment(0.5, 0.5);
    tip_label->show();

    tip_window->add(*tip_label);
    tip_window->ensure_style();

    return true;
}

static void
handle_tooltip_event(WnckWindow* win,
                     XEvent* xevent, unsigned state, const char* tip)
{
    switch (xevent->type) {
    case ButtonPress:
        hide_tooltip();
        break;
    case ButtonRelease:
        break;
    case EnterNotify:
        if (!(state & PRESSED_EVENT_WINDOW)) {
            if (wnck_window_is_active(win)) {
                tooltip_start_delay(tip);
            }
        }
        break;
    case LeaveNotify:
        hide_tooltip();
        break;
    }
}
static void action_menu_unmap(GObject* object)
{
    action_menu_mapped = false;
}

static void action_menu_map(WnckWindow* win, long button, Time time)
{
    GdkDisplay* gdkdisplay;
    GdkScreen* screen;

    gdkdisplay = gdk_display_get_default();
    screen = gdk_display_get_default_screen(gdkdisplay);

    if (action_menu) {
        if (action_menu_mapped) {
            gtk_widget_destroy(action_menu);
            action_menu_mapped = false;
            action_menu = NULL;
            return;
        } else {
            gtk_widget_destroy(action_menu);
        }
    }

    switch (wnck_window_get_window_type(win)) {
    case WNCK_WINDOW_DESKTOP:
    case WNCK_WINDOW_DOCK:
        /* don't allow window action */
        return;
    case WNCK_WINDOW_NORMAL:
    case WNCK_WINDOW_DIALOG:
#ifndef HAVE_LIBWNCK_2_19_3
    case WNCK_WINDOW_MODAL_DIALOG:
#endif
    case WNCK_WINDOW_TOOLBAR:
    case WNCK_WINDOW_MENU:
    case WNCK_WINDOW_UTILITY:
    case WNCK_WINDOW_SPLASHSCREEN:
        /* allow window action menu */
        break;
    }

    action_menu = wnck_create_window_action_menu(win);

    gtk_menu_set_screen(GTK_MENU(action_menu), screen);

    g_signal_connect_object(G_OBJECT(action_menu), "unmap",
                            G_CALLBACK(action_menu_unmap), 0, 0);

    gtk_widget_show(action_menu);
    gtk_menu_popup(GTK_MENU(action_menu),
                   NULL, NULL, NULL, NULL, button, time);

    action_menu_mapped = true;
}

/* generic_button_event returns:
 * 0: nothing, hover, ButtonPress
 * XEvent Button code: ButtonRelease (mouse click)
 */
static int generic_button_event(WnckWindow* win, XEvent* xevent,
                                 int button, int bpict)
{
    // Display *xdisplay = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
    const char* tooltips[B_COUNT] = {
        _("Close Window"),
        _("Maximize Window"),
        _("Restore Window"),
        _("Minimize Window"),
        _("Context Help"),
        _("Window Menu"),
        _("Shade Window"),
        _("UnShade Window"),
        _("Set Above"),
        _("UnSet Above"),
        _("Stick Window"),
        _("UnStick Window"),
    };

    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");
    unsigned state = d->button_states[button];
    int ret = 0;

    // window_settings * ws = d->fs->ws;

    handle_tooltip_event(win, xevent, state, tooltips[bpict]);

    /*
     * "!(xevent->xbutton.button & -4)" tests whether event button is one of
     * three major mouse buttons
     * "!(xevent->xbutton.button & -4)" evaluates to:
     * 0: xevent->xbutton.button < 0, xevent->xbutton.button > 3
     * 1: -1 < xevent->xbutton.button < 4
     */
    switch (xevent->type) {
    case ButtonPress:
        if (((button == B_T_MAXIMIZE) && !(xevent->xbutton.button & -4)) ||
                (xevent->xbutton.button == Button1)) {
            d->button_states[button] |= PRESSED_EVENT_WINDOW;
        }
        break;
    case ButtonRelease:
        if (((button == B_T_MAXIMIZE) && !(xevent->xbutton.button & -4)) ||
                (xevent->xbutton.button == Button1)) {
            if (d->button_states[button] ==
                    (PRESSED_EVENT_WINDOW | IN_EVENT_WINDOW)) {
                ret = xevent->xbutton.button;
            }

            d->button_states[button] &= ~PRESSED_EVENT_WINDOW;
        }
        break;
    case EnterNotify:
        d->button_states[button] |= IN_EVENT_WINDOW;
        break;
    case LeaveNotify:
        d->button_states[button] &= ~IN_EVENT_WINDOW;
        break;
    }

    if (state != d->button_states[button]) {
        queue_decor_draw_for_buttons(d, true);
    }

    return ret;
}

static void close_button_event(WnckWindow* win, XEvent* xevent)
{
    if (generic_button_event(win, xevent, B_T_CLOSE, B_CLOSE)) {
        wnck_window_close(win, xevent->xbutton.time);
    }
}

static void max_button_event(WnckWindow* win, XEvent* xevent)
{
    bool maximized = wnck_window_is_maximized(win);

    switch (generic_button_event(win,
                                 xevent,
                                 B_T_MAXIMIZE,
                                 (maximized ? B_RESTORE : B_MAXIMIZE))) {
    case Button1:
        maximized ? wnck_window_unmaximize(win) : wnck_window_maximize(win);
        break;
    case Button2:
        if (wnck_window_is_maximized_vertically(win)) {
            wnck_window_unmaximize_vertically(win);
        } else {
            wnck_window_maximize_vertically(win);
        }
        break;
    case Button3:
        if (wnck_window_is_maximized_horizontally(win)) {
            wnck_window_unmaximize_horizontally(win);
        } else {
            wnck_window_maximize_horizontally(win);
        }
        break;
    }
}

static void min_button_event(WnckWindow* win, XEvent* xevent)
{
    if (generic_button_event(win, xevent, B_T_MINIMIZE, B_MINIMIZE)) {
        wnck_window_minimize(win);
    }
}

static void above_button_event(WnckWindow* win, XEvent* xevent)
{
    if (wnck_window_is_above(win)) {
        if (generic_button_event(win, xevent, B_T_ABOVE, B_UNABOVE)) {
            wnck_window_unmake_above(win);
        }
    } else {
        if (generic_button_event(win, xevent, B_T_ABOVE, B_ABOVE)) {
            wnck_window_make_above(win);
        }
    }
}

static void sticky_button_event(WnckWindow* win, XEvent* xevent)
{
    if (wnck_window_is_sticky(win)) {
        if (generic_button_event(win, xevent, B_T_STICKY, B_UNSTICK)) {
            wnck_window_unstick(win);
        }
    } else {
        if (generic_button_event(win, xevent, B_T_STICKY, B_STICK)) {
            wnck_window_stick(win);
        }
    }
}

static void send_help_message(WnckWindow* win)
{
    Display* xdisplay;
    GdkDisplay* gdkdisplay;

    //GdkScreen  *screen;
    Window id;
    XEvent ev;

    gdkdisplay = gdk_display_get_default();
    xdisplay = GDK_DISPLAY_XDISPLAY(gdkdisplay);
    //screen     = gdk_display_get_default_screen (gdkdisplay);
    //xroot      = RootWindowOfScreen (gdk_x11_screen_get_xscreen (screen));
    id = wnck_window_get_xid(win);

    ev.xclient.type = ClientMessage;
    //ev.xclient.display = xdisplay;

    //ev.xclient.serial     = 0;
    //ev.xclient.send_event = true;

    ev.xclient.window = id;
    ev.xclient.message_type = wm_protocols_atom;
    ev.xclient.data.l[0] = net_wm_context_help_atom;
    ev.xclient.data.l[1] = 0L;
    ev.xclient.format = 32;

    XSendEvent(xdisplay, id, false, 0L, &ev);

    XSync(xdisplay, false);
}

static void help_button_event(WnckWindow* win, XEvent* xevent)
{
    if (generic_button_event(win, xevent, B_T_HELP, B_HELP)) {
        send_help_message(win);
    }
}
static void menu_button_event(WnckWindow* win, XEvent* xevent)
{
    if (generic_button_event(win, xevent, B_T_MENU, B_MENU)) {
        action_menu_map(win, xevent->xbutton.button, xevent->xbutton.time);
    }
}
static void shade_button_event(WnckWindow* win, XEvent* xevent)
{
    if (generic_button_event(win, xevent, B_T_SHADE, B_SHADE)) {
        if (wnck_window_is_shaded(win)) {
            wnck_window_unshade(win);
        } else {
            wnck_window_shade(win);
        }
    }
}


static void top_left_event(WnckWindow* win, XEvent* xevent)
{
    if (xevent->xbutton.button == 1) {
        move_resize_window(win, WM_MOVERESIZE_SIZE_TOPLEFT, xevent);
    }
}

static void top_event(WnckWindow* win, XEvent* xevent)
{
    if (xevent->xbutton.button == 1) {
        move_resize_window(win, WM_MOVERESIZE_SIZE_TOP, xevent);
    }
}

static void top_right_event(WnckWindow* win, XEvent* xevent)
{
    if (xevent->xbutton.button == 1) {
        move_resize_window(win, WM_MOVERESIZE_SIZE_TOPRIGHT, xevent);
    }
}

static void left_event(WnckWindow* win, XEvent* xevent)
{
    if (xevent->xbutton.button == 1) {
        move_resize_window(win, WM_MOVERESIZE_SIZE_LEFT, xevent);
    }
}

static void title_event(WnckWindow* win, XEvent* xevent)
{
    static unsigned int last_button_num = 0;
    static Window last_button_xwindow = None;
    static Time last_button_time = 0;
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");
    window_settings* ws = d->fs->ws;

    if (xevent->type != ButtonPress) {
        return;
    }

    if (xevent->xbutton.button == 1) {
        if (xevent->xbutton.button == last_button_num &&
                xevent->xbutton.window == last_button_xwindow &&
                xevent->xbutton.time < last_button_time + double_click_timeout) {
            switch (ws->double_click_action) {
            case DOUBLE_CLICK_SHADE:
                if (wnck_window_is_shaded(win)) {
                    wnck_window_unshade(win);
                } else {
                    wnck_window_shade(win);
                }
                break;
            case DOUBLE_CLICK_MAXIMIZE:
                if (wnck_window_is_maximized(win)) {
                    wnck_window_unmaximize(win);
                } else {
                    wnck_window_maximize(win);
                }
                break;
            case DOUBLE_CLICK_MINIMIZE:
                wnck_window_minimize(win);
                break;
            default:
                break;
            }

            last_button_num = 0;
            last_button_xwindow = None;
            last_button_time = 0;
        } else {
            last_button_num = xevent->xbutton.button;
            last_button_xwindow = xevent->xbutton.window;
            last_button_time = xevent->xbutton.time;

            restack_window(win, Above);

            move_resize_window(win, WM_MOVERESIZE_MOVE, xevent);
        }
    } else if (xevent->xbutton.button == 2) {
        restack_window(win, Below);
    } else if (xevent->xbutton.button == 3) {
        action_menu_map(win, xevent->xbutton.button, xevent->xbutton.time);
    } else if (xevent->xbutton.button == 4) {
        if (!wnck_window_is_shaded(win)) {
            wnck_window_shade(win);
        }
    } else if (xevent->xbutton.button == 5) {
        if (wnck_window_is_shaded(win)) {
            wnck_window_unshade(win);
        }
    }
}

static void right_event(WnckWindow* win, XEvent* xevent)
{
    if (xevent->xbutton.button == 1) {
        move_resize_window(win, WM_MOVERESIZE_SIZE_RIGHT, xevent);
    }
}

static void bottom_left_event(WnckWindow* win, XEvent* xevent)
{
    if (xevent->xbutton.button == 1) {
        move_resize_window(win, WM_MOVERESIZE_SIZE_BOTTOMLEFT, xevent);
    }
}

static void bottom_event(WnckWindow* win, XEvent* xevent)
{
    if (xevent->xbutton.button == 1) {
        move_resize_window(win, WM_MOVERESIZE_SIZE_BOTTOM, xevent);
    }
}

static void bottom_right_event(WnckWindow* win, XEvent* xevent)
{
    if (xevent->xbutton.button == 1) {
        move_resize_window(win, WM_MOVERESIZE_SIZE_BOTTOMRIGHT, xevent);
    }
}

static void force_quit_dialog_realize(GtkWidget* dialog, void* data)
{
    WnckWindow* win = data;

    gdk_error_trap_push();
    XSetTransientForHint(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()),
                         GDK_WINDOW_XID(dialog->window),
                         wnck_window_get_xid(win));
    XSync(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), false);
    gdk_error_trap_pop();
}

static char* get_client_machine(Window xwindow)
{
    Atom atom, type;
    gulong nitems, bytes_after;
    char* str = NULL;
    unsigned char* sstr = NULL;
    int format, result;
    char* retval;

    atom = XInternAtom(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), "WM_CLIENT_MACHINE", false);

    gdk_error_trap_push();

    result = XGetWindowProperty(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()),
                                xwindow, atom,
                                0, G_MAXLONG,
                                false, XA_STRING, &type, &format, &nitems,
                                &bytes_after, &sstr);

    str = (char*) sstr;
    gdk_error_trap_pop();

    if (result != Success) {
        return NULL;
    }

    if (type != XA_STRING) {
        XFree(str);
        return NULL;
    }

    retval = g_strdup(str);

    XFree(str);

    return retval;
}

static void kill_window(WnckWindow* win)
{
    WnckApplication* app;

    app = wnck_window_get_application(win);
    if (app) {
        char buf[257], *client_machine;
        int pid;

        pid = wnck_application_get_pid(app);
        client_machine = get_client_machine(wnck_application_get_xid(app));

        if (client_machine && pid > 0) {
            if (gethostname(buf, sizeof(buf) - 1) == 0) {
                if (strcmp(buf, client_machine) == 0) {
                    kill(pid, 9);
                }
            }
        }

        if (client_machine) {
            g_free(client_machine);
        }
    }

    gdk_error_trap_push();
    XKillClient(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), wnck_window_get_xid(win));
    XSync(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), false);
    gdk_error_trap_pop();
}

static void
force_quit_dialog_response(GtkWidget* dialog, int response, void* data)
{
    WnckWindow* win = data;
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");

    if (response == GTK_RESPONSE_ACCEPT) {
        kill_window(win);
    }

    if (d->force_quit_dialog) {
        d->force_quit_dialog = NULL;
        gtk_widget_destroy(dialog);
    }
}

static void show_force_quit_dialog(WnckWindow* win, Time timestamp)
{
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");
    GtkWidget* dialog;
    char* str, *tmp;
    const char* name;

    if (d->force_quit_dialog) {
        return;
    }

    name = wnck_window_get_name(win);
    if (!name) {
        name = "";
    }

    tmp = g_markup_escape_text(name, -1);
    str = g_strdup_printf(_("The window \"%s\" is not responding."), tmp);

    g_free(tmp);

    dialog = gtk_message_dialog_new(NULL, 0,
                                    GTK_MESSAGE_WARNING,
                                    GTK_BUTTONS_NONE,
                                    "<b>%s</b>\n\n%s",
                                    str,
                                    _("Forcing this application to "
                                      "quit will cause you to lose any "
                                      "unsaved changes."));
    g_free(str);

    gtk_window_set_icon_name(GTK_WINDOW(dialog), "force-quit");

    gtk_label_set_use_markup(GTK_LABEL(GTK_MESSAGE_DIALOG(dialog)->label),
                             true);
    gtk_label_set_line_wrap(GTK_LABEL(GTK_MESSAGE_DIALOG(dialog)->label),
                            true);

    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                           GTK_STOCK_CANCEL,
                           GTK_RESPONSE_REJECT,
                           _("_Force Quit"), GTK_RESPONSE_ACCEPT, NULL);

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_REJECT);

    g_signal_connect(G_OBJECT(dialog), "realize",
                     G_CALLBACK(force_quit_dialog_realize), win);

    g_signal_connect(G_OBJECT(dialog), "response",
                     G_CALLBACK(force_quit_dialog_response), win);

    gtk_window_set_modal(GTK_WINDOW(dialog), true);

    gtk_widget_realize(dialog);

    gdk_x11_window_set_user_time(dialog->window, timestamp);

    gtk_widget_show(dialog);

    d->force_quit_dialog = dialog;
}

static void hide_force_quit_dialog(WnckWindow* win)
{
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");

    if (d->force_quit_dialog) {
        gtk_widget_destroy(d->force_quit_dialog);
        d->force_quit_dialog = NULL;
    }
}

static GdkFilterReturn
event_filter_func(GdkXEvent* gdkxevent, GdkEvent* event, void* data)
{
    Display* xdisplay;
    GdkDisplay* gdkdisplay;
    XEvent* xevent = gdkxevent;
    gulong xid = 0;

    gdkdisplay = gdk_display_get_default();
    xdisplay = GDK_DISPLAY_XDISPLAY(gdkdisplay);

    switch (xevent->type) {
    case ButtonPress:
    case ButtonRelease:
        xid = (gulong)
              g_hash_table_lookup(frame_table,
                                  GINT_TO_POINTER(xevent->xbutton.window));
        break;
    case EnterNotify:
    case LeaveNotify:
        xid = (gulong)
              g_hash_table_lookup(frame_table,
                                  GINT_TO_POINTER(xevent->xcrossing.
                                          window));
        break;
    case MotionNotify:
        xid = (gulong)
              g_hash_table_lookup(frame_table,
                                  GINT_TO_POINTER(xevent->xmotion.window));
        break;
    case PropertyNotify:
        if (xevent->xproperty.atom == frame_window_atom) {
            WnckWindow* win;

            xid = xevent->xproperty.window;

            win = wnck_window_get(xid);
            if (win) {
                Window frame;

                if (get_window_prop(xid, frame_window_atom, &frame)) {
                    add_frame_window(win, frame);
                } else {
                    remove_frame_window(win);
                }
            }
        } else if (xevent->xproperty.atom == mwm_hints_atom) {
            WnckWindow* win;

            xid = xevent->xproperty.window;

            win = wnck_window_get(xid);
            if (win) {
                decor_t* d = g_object_get_data(G_OBJECT(win), "decor");
                bool decorated = false;

                if (get_mwm_prop(xid) & (MWM_DECOR_ALL | MWM_DECOR_TITLE)) {
                    decorated = true;
                }

                if (decorated != d->decorated) {
                    d->decorated = decorated;
                    if (decorated) {
                        d->width = d->height = 0;

                        update_window_decoration_size(win);
                        update_event_windows(win);
                    } else {
                        gdk_error_trap_push();
                        XDeleteProperty(xdisplay, xid, win_decor_atom);
                        XSync(xdisplay, false);
                        gdk_error_trap_pop();
                    }
                }
            }
        } else if (xevent->xproperty.atom == select_window_atom) {
            WnckWindow* win;

            xid = xevent->xproperty.window;

            win = wnck_window_get(xid);
            if (win) {
                Window select;

                if (get_window_prop(xid, select_window_atom, &select)) {
                    update_switcher_window(win, select);
                }
            }
        }
        break;
    case DestroyNotify:
        g_hash_table_remove(frame_table,
                            GINT_TO_POINTER(xevent->xproperty.window));
        break;
    case ClientMessage:
        if (xevent->xclient.message_type == toolkit_action_atom) {
            unsigned long action;

            action = xevent->xclient.data.l[0];
            if (action == toolkit_action_window_menu_atom) {
                WnckWindow* win;

                win = wnck_window_get(xevent->xclient.window);
                if (win) {
                    action_menu_map(win,
                                    xevent->xclient.data.l[2],
                                    xevent->xclient.data.l[1]);
                }
            } else if (action == toolkit_action_force_quit_dialog_atom) {
                WnckWindow* win;

                win = wnck_window_get(xevent->xclient.window);
                if (win) {
                    if (xevent->xclient.data.l[2])
                        show_force_quit_dialog(win,
                                               xevent->xclient.data.l[1]);
                    else {
                        hide_force_quit_dialog(win);
                    }
                }
            }
        } else if (xevent->xclient.message_type == emerald_sigusr1_atom) {
            reload_all_settings(SIGUSR1);
        }

    default:
        break;
    }

    if (xid) {
        WnckWindow* win;

        win = wnck_window_get(xid);
        if (win) {
            static event_callback callback[3][3] = {
                {top_left_event, top_event, top_right_event},
                {left_event, title_event, right_event},
                {bottom_left_event, bottom_event, bottom_right_event}
            };
            static event_callback button_callback[B_T_COUNT] = {
                close_button_event,
                max_button_event,
                min_button_event,
                help_button_event,
                menu_button_event,
                shade_button_event,
                above_button_event,
                sticky_button_event,
            };
            decor_t* d = g_object_get_data(G_OBJECT(win), "decor");

            if (d->decorated) {
                int i, j;

                for (i = 0; i < 3; i++)
                    for (j = 0; j < 3; j++)
                        if (d->event_windows[i][j] == xevent->xany.window) {
                            (*callback[i][j])(win, xevent);
                        }

                for (i = 0; i < B_T_COUNT; i++)
                    if (d->button_windows[i] == xevent->xany.window) {
                        (*button_callback[i])(win, xevent);
                    }
            }
        }
    }

    return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
selection_event_filter_func(GdkXEvent* gdkxevent,
                            GdkEvent* event, void* data)
{
    Display* xdisplay;
    GdkDisplay* gdkdisplay;
    XEvent* xevent = gdkxevent;
    int status;

    gdkdisplay = gdk_display_get_default();
    xdisplay = GDK_DISPLAY_XDISPLAY(gdkdisplay);

    switch (xevent->type) {
    case SelectionRequest:
        decor_handle_selection_request(xdisplay, xevent, dm_sn_timestamp);
        break;
    case SelectionClear:
        status = decor_handle_selection_clear(xdisplay, xevent, 0);
        if (status == DECOR_SELECTION_GIVE_UP) {
            exit(0);
        }
        break;
    default:
        break;
    }

    return GDK_FILTER_CONTINUE;
}

#if G_MAXINT != G_MAXLONG
/* XRenderSetPictureFilter used to be broken on LP64. This
 * works with either the broken or fixed version.
 */
static void
XRenderSetPictureFilter_wrapper(Display* dpy,
                                Picture picture,
                                char* filter, XFixed* params, int nparams)
{
    gdk_error_trap_push();
    XRenderSetPictureFilter(dpy, picture, filter, params, nparams);
    XSync(dpy, False);
    if (gdk_error_trap_pop()) {
        long* long_params = g_new(long, nparams);
        int i;

        for (i = 0; i < nparams; i++) {
            long_params[i] = params[i];
        }

        XRenderSetPictureFilter(dpy, picture, filter,
                                (XFixed*) long_params, nparams);
        g_free(long_params);
    }
}

#define XRenderSetPictureFilter XRenderSetPictureFilter_wrapper
#endif

static void
set_picture_transform(Display* xdisplay, Picture p, int dx, int dy)
{
    XTransform transform = {
        {
            {1 << 16, 0, -dx << 16},
            {0, 1 << 16, -dy << 16},
            {0, 0, 1 << 16},
        }
    };

    XRenderSetPictureTransform(xdisplay, p, &transform);
}

static XFixed* create_gaussian_kernel(double radius,
                                      double sigma,
                                      double alpha,
                                      double opacity, int* r_size)
{
    XFixed* params;
    double* amp, scale, x_scale, fx, sum;
    int size, x, i, n;

    scale = 1.0 / (2.0 * M_PI * sigma * sigma);

    if (alpha == -0.5) {
        alpha = 0.5;
    }

    size = alpha * 2 + 2;

    x_scale = 2.0 * radius / size;

    if (size < 3) {
        return NULL;
    }

    n = size;

    amp = g_malloc(sizeof(double) * n);
    if (!amp) {
        return NULL;
    }

    n += 2;

    params = g_malloc(sizeof(XFixed) * n);
    if (!params) {
        return NULL;
    }

    i = 0;
    sum = 0.0f;

    for (x = 0; x < size; x++) {
        fx = x_scale * (x - alpha - 0.5);

        amp[i] = scale * exp((-1.0 * (fx * fx)) / (2.0 * sigma * sigma));

        sum += amp[i];

        i++;
    }

    /* normalize */
    if (sum) {
        sum = 1.0 / sum;
    }

    params[0] = params[1] = 0;

    for (i = 2; i < n; i++) {
        params[i] = XDoubleToFixed(amp[i - 2] * sum * opacity * 1.2);
    }

    g_free(amp);

    *r_size = size;

    return params;
}

/* to save some memory, value is specific to current decorations */
#define CORNER_REDUCTION 3

static int update_shadow(frame_settings* fs)
{
    Display* xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    XRenderPictFormat* format;
    GdkPixmap* pixmap;
    Picture src, dst, tmp;
    XFixed* params;
    XFilters* filters;
    char* filter = NULL;
    int size, n_params = 0;
    cairo_t* cr;
    decor_t d;

    bzero(&d, sizeof(decor_t));
    window_settings* ws = fs->ws;

    //    double        save_decoration_alpha;
    static XRenderColor color;
    static XRenderColor clear = { 0x0000, 0x0000, 0x0000, 0x0000 };
    static XRenderColor white = { 0xffff, 0xffff, 0xffff, 0xffff };

    color.red = ws->shadow_color[0];
    color.green = ws->shadow_color[1];
    color.blue = ws->shadow_color[2];
    color.alpha = 0xffff;

    /* compute a gaussian convolution kernel */
    params = create_gaussian_kernel(ws->shadow_radius, ws->shadow_radius / 2.0,        // SIGMA
                                    ws->shadow_radius,        // ALPHA
                                    ws->shadow_opacity, &size);
    if (!params) {
        ws->shadow_offset_x = ws->shadow_offset_y = size = 0;
    }

    if (ws->shadow_radius <= 0.0 && ws->shadow_offset_x == 0 &&
            ws->shadow_offset_y == 0) {
        size = 0;
    }

    n_params = size + 2;
    size = size / 2;

    ws->left_space = ws->win_extents.left + size - ws->shadow_offset_x;
    ws->right_space = ws->win_extents.right + size + ws->shadow_offset_x;
    ws->top_space = ws->win_extents.top + size - ws->shadow_offset_y;
    ws->bottom_space = ws->win_extents.bottom + size + ws->shadow_offset_y;


    ws->left_space = MAX(ws->win_extents.left, ws->left_space);
    ws->right_space = MAX(ws->win_extents.right, ws->right_space);
    ws->top_space = MAX(ws->win_extents.top, ws->top_space);
    ws->bottom_space = MAX(ws->win_extents.bottom, ws->bottom_space);

    ws->shadow_left_space = MAX(0, size - ws->shadow_offset_x);
    ws->shadow_right_space = MAX(0, size + ws->shadow_offset_x);
    ws->shadow_top_space = MAX(0, size - ws->shadow_offset_y);
    ws->shadow_bottom_space = MAX(0, size + ws->shadow_offset_y);

    ws->shadow_left_corner_space = MAX(0, size + ws->shadow_offset_x);
    ws->shadow_right_corner_space = MAX(0, size - ws->shadow_offset_x);
    ws->shadow_top_corner_space = MAX(0, size + ws->shadow_offset_y);
    ws->shadow_bottom_corner_space = MAX(0, size - ws->shadow_offset_y);

    ws->left_corner_space =
        MAX(0, ws->shadow_left_corner_space - CORNER_REDUCTION);
    ws->right_corner_space =
        MAX(0, ws->shadow_right_corner_space - CORNER_REDUCTION);
    ws->top_corner_space =
        MAX(0, ws->shadow_top_corner_space - CORNER_REDUCTION);
    ws->bottom_corner_space =
        MAX(0, ws->shadow_bottom_corner_space - CORNER_REDUCTION);

    ws->normal_top_corner_space =
        MAX(0, ws->top_corner_space - ws->titlebar_height);
    ws->switcher_top_corner_space =
        MAX(0, ws->top_corner_space - SWITCHER_TOP_EXTRA);
    ws->switcher_bottom_corner_space =
        MAX(0, ws->bottom_corner_space - SWITCHER_SPACE);

    d.buffer_pixmap = NULL;
    d.layout = NULL;
    d.icon = NULL;
    d.state = 0;
    d.actions = 0;
    d.prop_xid = 0;
    d.draw = draw_shadow_window;
    d.active = true;
    d.fs = fs;

    reset_buttons_bg_and_fade(&d);

    d.width =
        ws->left_space + ws->left_corner_space + 1 +
        ws->right_corner_space + ws->right_space;
    d.height =
        ws->top_space + ws->titlebar_height +
        ws->normal_top_corner_space + 2 + ws->bottom_corner_space +
        ws->bottom_space;

    /* all pixmaps are ARGB32 */
    format = XRenderFindStandardFormat(xdisplay, PictStandardARGB32);

    /* shadow color */
    src = XRenderCreateSolidFill(xdisplay, &color);

    if (ws->large_shadow_pixmap) {
        g_object_unref(G_OBJECT(ws->large_shadow_pixmap));
        ws->large_shadow_pixmap = NULL;
    }

    if (ws->shadow_pattern) {
        cairo_pattern_destroy(ws->shadow_pattern);
        ws->shadow_pattern = NULL;
    }

    if (ws->shadow_pixmap) {
        g_object_unref(G_OBJECT(ws->shadow_pixmap));
        ws->shadow_pixmap = NULL;
    }

    /* no shadow */
    if (size <= 0) {
        if (params) {
            g_free(params);
        }

        return 1;
    }

    pixmap = create_pixmap(d.width, d.height);
    if (!pixmap) {
        g_free(params);
        return 0;
    }

    /* query server for convolution filter */
    filters = XRenderQueryFilters(xdisplay, GDK_PIXMAP_XID(pixmap));
    if (filters) {
        int i;

        for (i = 0; i < filters->nfilter; i++) {
            if (strcmp(filters->filter[i], FilterConvolution) == 0) {
                filter = FilterConvolution;
                break;
            }
        }

        XFree(filters);
    }

    if (!filter) {
        fprintf(stderr, "can't generate shadows, X server doesn't support "
                "convolution filters\n");

        g_free(params);
        g_object_unref(G_OBJECT(pixmap));
        return 1;
    }

    /* WINDOWS WITH DECORATION */

    d.pixmap = create_pixmap(d.width, d.height);
    if (!d.pixmap) {
        g_free(params);
        g_object_unref(G_OBJECT(pixmap));
        return 0;
    }

    /* draw decorations */
    (*d.draw)(&d);

    dst = XRenderCreatePicture(xdisplay, GDK_PIXMAP_XID(d.pixmap),
                               format, 0, NULL);
    tmp = XRenderCreatePicture(xdisplay, GDK_PIXMAP_XID(pixmap),
                               format, 0, NULL);

    /* first pass */
    params[0] = (n_params - 2) << 16;
    params[1] = 1 << 16;

    set_picture_transform(xdisplay, dst, ws->shadow_offset_x, 0);
    XRenderSetPictureFilter(xdisplay, dst, filter, params, n_params);
    XRenderComposite(xdisplay,
                     PictOpSrc,
                     src, dst, tmp, 0, 0, 0, 0, 0, 0, d.width, d.height);

    /* second pass */
    params[0] = 1 << 16;
    params[1] = (n_params - 2) << 16;

    set_picture_transform(xdisplay, tmp, 0, ws->shadow_offset_y);
    XRenderSetPictureFilter(xdisplay, tmp, filter, params, n_params);
    XRenderComposite(xdisplay,
                     PictOpSrc,
                     src, tmp, dst, 0, 0, 0, 0, 0, 0, d.width, d.height);

    XRenderFreePicture(xdisplay, tmp);
    XRenderFreePicture(xdisplay, dst);

    g_object_unref(G_OBJECT(pixmap));

    ws->large_shadow_pixmap = d.pixmap;

    cr = gdk_cairo_create(GDK_DRAWABLE(ws->large_shadow_pixmap));
    ws->shadow_pattern =
        cairo_pattern_create_for_surface(cairo_get_target(cr));
    cairo_pattern_set_filter(ws->shadow_pattern, CAIRO_FILTER_NEAREST);
    cairo_destroy(cr);


    /* WINDOWS WITHOUT DECORATIONS */

    d.width = ws->shadow_left_space + ws->shadow_left_corner_space + 1 +
              ws->shadow_right_space + ws->shadow_right_corner_space;
    d.height = ws->shadow_top_space + ws->shadow_top_corner_space + 1 +
               ws->shadow_bottom_space + ws->shadow_bottom_corner_space;

    pixmap = create_pixmap(d.width, d.height);
    if (!pixmap) {
        g_free(params);
        return 0;
    }

    d.pixmap = create_pixmap(d.width, d.height);
    if (!d.pixmap) {
        g_object_unref(G_OBJECT(pixmap));
        g_free(params);
        return 0;
    }

    dst = XRenderCreatePicture(xdisplay, GDK_PIXMAP_XID(d.pixmap),
                               format, 0, NULL);

    /* draw rectangle */
    XRenderFillRectangle(xdisplay, PictOpSrc, dst, &clear,
                         0, 0, d.width, d.height);
    XRenderFillRectangle(xdisplay, PictOpSrc, dst, &white,
                         ws->shadow_left_space,
                         ws->shadow_top_space,
                         d.width - ws->shadow_left_space -
                         ws->shadow_right_space,
                         d.height - ws->shadow_top_space -
                         ws->shadow_bottom_space);

    tmp = XRenderCreatePicture(xdisplay, GDK_PIXMAP_XID(pixmap),
                               format, 0, NULL);

    /* first pass */
    params[0] = (n_params - 2) << 16;
    params[1] = 1 << 16;

    set_picture_transform(xdisplay, dst, ws->shadow_offset_x, 0);
    XRenderSetPictureFilter(xdisplay, dst, filter, params, n_params);
    XRenderComposite(xdisplay,
                     PictOpSrc,
                     src, dst, tmp, 0, 0, 0, 0, 0, 0, d.width, d.height);

    /* second pass */
    params[0] = 1 << 16;
    params[1] = (n_params - 2) << 16;

    set_picture_transform(xdisplay, tmp, 0, ws->shadow_offset_y);
    XRenderSetPictureFilter(xdisplay, tmp, filter, params, n_params);
    XRenderComposite(xdisplay,
                     PictOpSrc,
                     src, tmp, dst, 0, 0, 0, 0, 0, 0, d.width, d.height);

    XRenderFreePicture(xdisplay, tmp);
    XRenderFreePicture(xdisplay, dst);
    XRenderFreePicture(xdisplay, src);

    g_object_unref(G_OBJECT(pixmap));

    g_free(params);

    ws->shadow_pixmap = d.pixmap;

    return 1;
}
static void titlebar_font_changed(window_settings* ws)
{
    PangoFontMetrics* metrics;
    PangoLanguage* lang;

    pango_context_set_font_description(ws->pango_context, ws->font_desc);
    lang = pango_context_get_language(ws->pango_context);
    metrics =
        pango_context_get_metrics(ws->pango_context, ws->font_desc, lang);

    ws->text_height = PANGO_PIXELS(pango_font_metrics_get_ascent(metrics) +
                                   pango_font_metrics_get_descent(metrics));

    ws->titlebar_height = ws->text_height;
    if (ws->titlebar_height < ws->min_titlebar_height) {
        ws->titlebar_height = ws->min_titlebar_height;
    }

    pango_font_metrics_unref(metrics);

}
static void load_buttons_image(window_settings* ws, int y)
{
    int rel_button = get_b_offset(y);

    if (ws->ButtonArray[y]) {
        g_object_unref(ws->ButtonArray[y]);
    }
    std::string file = make_filename("buttons", b_types[y], "png");
    if (!(ws->ButtonArray[y] = gdk_pixbuf_new_from_file(file.c_str(), NULL))) {
        ws->ButtonArray[y] = gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, 16 * S_COUNT, 16);    // create a blank pixbuf
    }

    int pix_width = gdk_pixbuf_get_width(ws->ButtonArray[y]) / S_COUNT;
    int pix_height = gdk_pixbuf_get_height(ws->ButtonArray[y]);
    ws->c_icon_size[rel_button].w = pix_width;
    ws->c_icon_size[rel_button].h = pix_height;
    for (unsigned x = 0; x < S_COUNT; x++) {
        if (ws->ButtonPix[x + y * S_COUNT]) {
            g_object_unref(ws->ButtonPix[x + y * S_COUNT]);
        }

        ws->ButtonPix[x + y * S_COUNT] =
            gdk_pixbuf_new_subpixbuf(ws->ButtonArray[y], x * pix_width, 0,
                                     pix_width, pix_height);
    }
}
static void load_buttons_glow_images(window_settings* ws)
{
    char* file1 = NULL;
    char* file2 = NULL;
    int x, pix_width, pix_height;
    int pix_width2, pix_height2;
    bool success1 = false;
    bool success2 = false;

    if (ws->use_button_glow) {
        if (ws->ButtonGlowArray) {
            g_object_unref(ws->ButtonGlowArray);
        }
        std::string file1 = make_filename("buttons", "glow", "png");
        if (ws->ButtonGlowArray = gdk_pixbuf_new_from_file(file1.c_str(), NULL)) {
            success1 = true;
        }
    }
    if (ws->use_button_inactive_glow) {
        if (ws->ButtonInactiveGlowArray) {
            g_object_unref(ws->ButtonInactiveGlowArray);
        }
        std::string file2 = make_filename("buttons", "inactive_glow", "png");
        if (ws->ButtonInactiveGlowArray =
                     gdk_pixbuf_new_from_file(file2.c_str(), NULL)) {
            success2 = true;
        }
    }
    if (success1 && success2) {
        pix_width = gdk_pixbuf_get_width(ws->ButtonGlowArray) / B_COUNT;
        pix_height = gdk_pixbuf_get_height(ws->ButtonGlowArray);
        pix_width2 =
            gdk_pixbuf_get_width(ws->ButtonInactiveGlowArray) / B_COUNT;
        pix_height2 = gdk_pixbuf_get_height(ws->ButtonInactiveGlowArray);

        if (pix_width != pix_width2 || pix_height != pix_height2) {
            g_warning
            ("Choose same size glow images for active and inactive windows."
             "\nInactive glow image is scaled for now.");
            // Scale the inactive one
            GdkPixbuf* tmp_pixbuf =
                gdk_pixbuf_new(gdk_pixbuf_get_colorspace
                               (ws->ButtonGlowArray),
                               true,
                               gdk_pixbuf_get_bits_per_sample(ws->
                                       ButtonGlowArray),
                               pix_width * B_COUNT,
                               pix_height);

            gdk_pixbuf_scale(ws->ButtonInactiveGlowArray, tmp_pixbuf,
                             0, 0,
                             pix_width * B_COUNT, pix_height,
                             0, 0,
                             pix_width / (double)pix_width2,
                             pix_height / (double)pix_height2,
                             GDK_INTERP_BILINEAR);
            g_object_unref(ws->ButtonInactiveGlowArray);
            ws->ButtonInactiveGlowArray = tmp_pixbuf;
        }
    } else {
        pix_width = 16;
        pix_height = 16;
        if (success1) {
            pix_width = gdk_pixbuf_get_width(ws->ButtonGlowArray) / B_COUNT;
            pix_height = gdk_pixbuf_get_height(ws->ButtonGlowArray);
        } else if (success2) {
            pix_width =
                gdk_pixbuf_get_width(ws->ButtonInactiveGlowArray) /
                B_COUNT;
            pix_height = gdk_pixbuf_get_height(ws->ButtonInactiveGlowArray);
        }
        if (!success1 && ws->use_button_glow) {
            ws->ButtonGlowArray = gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, pix_width * B_COUNT, pix_height);    // create a blank pixbuf
        }
        if (!success2 && ws->use_button_inactive_glow) {
            ws->ButtonInactiveGlowArray = gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, pix_width * B_COUNT, pix_height);    // create a blank pixbuf
        }
    }
    ws->c_glow_size.w = pix_width;
    ws->c_glow_size.h = pix_height;

    if (ws->use_button_glow) {
        for (unsigned x = 0; x < B_COUNT; x++) {
            if (ws->ButtonGlowPix[x]) {
                g_object_unref(ws->ButtonGlowPix[x]);
            }
            ws->ButtonGlowPix[x] =
                gdk_pixbuf_new_subpixbuf(ws->ButtonGlowArray,
                                         x * pix_width, 0, pix_width,
                                         pix_height);
        }
    }
    if (ws->use_button_inactive_glow) {
        for (unsigned x = 0; x < B_COUNT; x++) {
            if (ws->ButtonInactiveGlowPix[x]) {
                g_object_unref(ws->ButtonInactiveGlowPix[x]);
            }
            ws->ButtonInactiveGlowPix[x] =
                gdk_pixbuf_new_subpixbuf(ws->ButtonInactiveGlowArray,
                                         x * pix_width, 0, pix_width,
                                         pix_height);
        }
    }
}
void load_button_image_setting(window_settings* ws)
{
    for (unsigned i = 0; i < B_COUNT; i++) {
        load_buttons_image(ws, i);
    }

    // load active and inactive glow image
    if (ws->use_button_glow || ws->use_button_inactive_glow) {
        load_buttons_glow_images(ws);
    }
}
static void load_settings(window_settings* ws)
{
    char* path =
        g_strjoin("/", g_get_home_dir(), ".emerald/settings.ini", NULL);
    KeyFile f;

    copy_from_defaults_if_needed();

    //settings
    f.load_from_file(path);
    g_free(path);
    load_int_setting(f, &ws->double_click_action, "double_click_action",
                     "titlebars");
    load_int_setting(f, &ws->button_hover_cursor, "hover_cursor", "buttons");
    load_bool_setting(f, &ws->use_decoration_cropping,
                      "use_decoration_cropping", "decorations");
    load_bool_setting(f, &ws->use_button_fade, "use_button_fade", "buttons");
    int button_fade_step_duration = ws->button_fade_step_duration;

    load_int_setting(f, &button_fade_step_duration,
                     "button_fade_step_duration", "buttons");
    if (button_fade_step_duration > 0) {
        ws->button_fade_step_duration = button_fade_step_duration;
    }
    int button_fade_total_duration = 250;

    load_int_setting(f, &button_fade_total_duration,
                     "button_fade_total_duration", "buttons");
    if (button_fade_total_duration > 0)
        ws->button_fade_num_steps =
            button_fade_total_duration / ws->button_fade_step_duration;
    if (ws->button_fade_num_steps == 0) {
        ws->button_fade_num_steps = 1;
    }
    bool use_button_fade_pulse = false;

    load_bool_setting(f, &use_button_fade_pulse, "use_button_fade_pulse",
                      "buttons");

    ws->button_fade_pulse_wait_steps = 0;
    if (use_button_fade_pulse) {
        int button_fade_pulse_min_opacity = 0;

        load_int_setting(f, &button_fade_pulse_min_opacity,
                         "button_fade_pulse_min_opacity", "buttons");
        ws->button_fade_pulse_len_steps =
            ws->button_fade_num_steps * (100 -
                                         button_fade_pulse_min_opacity) /
            100;
        int button_fade_pulse_wait_duration = 0;

        load_int_setting(f, &button_fade_pulse_wait_duration,
                         "button_fade_pulse_wait_duration", "buttons");
        if (button_fade_pulse_wait_duration > 0)
            ws->button_fade_pulse_wait_steps =
                button_fade_pulse_wait_duration /
                ws->button_fade_step_duration;
    } else {
        ws->button_fade_pulse_len_steps = 0;
    }

    load_bool_setting(f, &enable_tooltips, "enable_tooltips", "buttons");
    load_int_setting(f, &ws->blur_type,
                     "blur_type", "decorations");

    //theme
    path = g_strjoin("/", g_get_home_dir(), ".emerald/theme/theme.ini", NULL);
    f.load_from_file(path);
    g_free(path);
    load_string_setting(f, engine, "engine", "engine");
    if (!load_engine(engine, ws)) {
        engine = "legacy";
        load_engine(engine, ws);
    }
    LFACSS(f, ws, text, titlebar);
    LFACSS(f, ws, text_halo, titlebar);
    LFACSS(f, ws, button, buttons);
    LFACSS(f, ws, button_halo, buttons);
    load_engine_settings(f, ws);
    load_font_setting(f, &ws->font_desc, "titlebar_font", "titlebar");
    load_bool_setting(f, &ws->use_pixmap_buttons, "use_pixmap_buttons",
                      "buttons");
    load_bool_setting(f, &ws->use_button_glow, "use_button_glow", "buttons");
    load_bool_setting(f, &ws->use_button_inactive_glow,
                      "use_button_inactive_glow", "buttons");

    if (ws->use_pixmap_buttons) {
        load_button_image_setting(ws);
    }
    load_shadow_color_setting(f, ws->shadow_color, "shadow_color", "shadow");
    load_int_setting(f, &ws->shadow_offset_x, "shadow_offset_x", "shadow");
    load_int_setting(f, &ws->shadow_offset_y, "shadow_offset_y", "shadow");
    load_float_setting(f, &ws->shadow_radius, "shadow_radius", "shadow");
    load_float_setting(f, &ws->shadow_opacity, "shadow_opacity", "shadow");
    std::string tmp;
    load_string_setting(f, tmp, "title_object_layout",
                        "titlebar");
    ws->tobj_layout = g_strdup(tmp.c_str()); // FIXME memory leak
    load_int_setting(f, &ws->button_offset, "vertical_offset", "buttons");
    load_int_setting(f, &ws->button_hoffset, "horizontal_offset", "buttons");
    load_int_setting(f, &ws->win_extents.top, "top", "borders");
    load_int_setting(f, &ws->win_extents.left, "left", "borders");
    load_int_setting(f, &ws->win_extents.right, "right", "borders");
    load_int_setting(f, &ws->win_extents.bottom, "bottom", "borders");
    load_int_setting(f, &ws->min_titlebar_height, "min_titlebar_height",
                     "titlebar");
}

static void update_settings(window_settings* ws)
{
    //assumes ws is fully allocated

    GdkDisplay* gdkdisplay;

    // Display    *xdisplay;
    GdkScreen* gdkscreen;
    WnckScreen* screen = wnck_screen_get_default();
    GList* windows;

    load_settings(ws);

    gdkdisplay = gdk_display_get_default();
    // xdisplay   = gdk_x11_display_get_xdisplay (gdkdisplay);
    gdkscreen = gdk_display_get_default_screen(gdkdisplay);

    titlebar_font_changed(ws);
    update_window_extents(ws);
    update_shadow(ws->fs_act);
    update_default_decorations(gdkscreen, ws->fs_act, ws->fs_inact);

    windows = wnck_screen_get_windows(screen);
    while (windows) {
        decor_t* d = g_object_get_data(G_OBJECT(windows->data), "decor");

        if (d->decorated) {
            d->width = d->height = 0;
            update_window_decoration_size(WNCK_WINDOW(windows->data));
            update_event_windows(WNCK_WINDOW(windows->data));
        }
        windows = windows->next;
    }
}

#ifdef USE_DBUS
static DBusHandlerResult
dbus_signal_filter(DBusConnection* connection, DBusMessage* message,
                   void* user_data)
{
    if (dbus_message_is_signal
            (message, "org.metascape.emerald.dbus.Signal", "Reload")) {
        puts("Reloading...");
        update_settings(global_ws);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void dbc(DBusError* err)
{
    if (dbus_error_is_set(err)) {
        fprintf(stderr, "emerald: Connection Error (%s)\n", err->message);
        dbus_error_free(err);
    }
}
#else
void reload_all_settings(int sig)
{
    if (sig == SIGUSR1) {
        do_reload = true;
    }
}
#endif
bool reload_if_needed(void* p)
{
    if (do_reload) {
        do_reload = false;
        puts("Reloading...");
        update_settings(global_ws);
    }
    return true;
}

int main(int argc, char* argv[])
{
    GdkDisplay* gdkdisplay;
    Display* xdisplay;
    WnckScreen* screen;
    int status;

    int i, j;
    bool replace = false;
    PangoFontMetrics* metrics;
    PangoLanguage* lang;
    frame_settings* pfs;
    window_settings* ws;

    ws = malloc(sizeof(window_settings));
    bzero(ws, sizeof(window_settings));
    global_ws = ws;
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    ws->win_extents.left = 6;
    ws->win_extents.top = 4;
    ws->win_extents.right = 6;
    ws->win_extents.bottom = 6;
    ws->corner_radius = 5;
    ws->shadow_radius = 15;
    ws->shadow_opacity = .8;
    ws->min_titlebar_height = 17;
    ws->double_click_action = DOUBLE_CLICK_SHADE;
    ws->button_hover_cursor = 1;
    ws->button_offset = 1;
    ws->button_hoffset = 1;
    ws->button_fade_step_duration = 50;
    ws->button_fade_num_steps = 5;
    ws->blur_type = BLUR_TYPE_NONE;

    ws->tobj_layout = g_strdup("IT::HNXC");        // DEFAULT TITLE OBJECT LAYOUT, does not use any odd buttons
    //ws->tobj_layout=g_strdup("CNX:IT:HM");

    pfs = malloc(sizeof(frame_settings));
    bzero(pfs, sizeof(frame_settings));
    pfs->ws = ws;
    ACOLOR(pfs, text, 1.0, 1.0, 1.0, 1.0);
    ACOLOR(pfs, text_halo, 0.0, 0.0, 0.0, 0.2);
    ACOLOR(pfs, button, 1.0, 1.0, 1.0, 0.8);
    ACOLOR(pfs, button_halo, 0.0, 0.0, 0.0, 0.2);
    ws->fs_act = pfs;

    pfs = malloc(sizeof(frame_settings));
    bzero(pfs, sizeof(frame_settings));
    pfs->ws = ws;
    ACOLOR(pfs, text, 0.8, 0.8, 0.8, 0.8);
    ACOLOR(pfs, text_halo, 0.0, 0.0, 0.0, 0.2);
    ACOLOR(pfs, button, 0.8, 0.8, 0.8, 0.8);
    ACOLOR(pfs, button_halo, 0.0, 0.0, 0.0, 0.2);
    ws->fs_inact = pfs;

    ws->round_top_left = true;
    ws->round_top_right = true;
    ws->round_bottom_left = true;
    ws->round_bottom_right = true;

    engine = g_strdup("legacy");
    load_engine(engine, ws);        // assumed to always return true

    program_name = argv[0];

    //ws->ButtonBase = NULL;
    for (i = 0; i < (S_COUNT * B_COUNT); i++) {
        ws->ButtonPix[i] = NULL;
    }
    Gtk::Main kit(argc, argv);
    gdk_error_trap_push();
#ifdef USE_DBUS
    if (!g_thread_supported()) {
        g_thread_init(NULL);
    }
    dbus_g_thread_init();
#endif

    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--replace") == 0) {
            replace = true;
        } else if (strcmp(argv[i], "--version") == 0) {
            printf("%s: %s version %s\n", program_name.c_str(), PACKAGE, VERSION);
            return 0;
        } else if (strcmp(argv[i], "--help") == 0) {
            fprintf(stderr, "%s [--replace] [--help] [--version]\n",
                    program_name.c_str());
            return 0;
        }
    }

#ifdef USE_DBUS
    {
        DBusConnection* dbcon;
        DBusError err;

        dbus_error_init(&err);
        dbcon = dbus_bus_get(DBUS_BUS_SESSION, &err);
        dbc(&err);
        dbus_connection_setup_with_g_main(dbcon, NULL);
        dbc(&err);
        dbus_bus_request_name(dbcon, "org.metascape.emerald.dbus",
                              DBUS_NAME_FLAG_REPLACE_EXISTING |
                              DBUS_NAME_FLAG_ALLOW_REPLACEMENT, &err);
        dbc(&err);
        dbus_bus_add_match(dbcon,
                           "type='signal',interface='org.metascape.emerald.dbus.Signal'",
                           &err);
        dbc(&err);
        dbus_connection_add_filter(dbcon, dbus_signal_filter, NULL, NULL);
    }
#endif
    signal(SIGUSR1, reload_all_settings);


    gdkdisplay = gdk_display_get_default();
    xdisplay = gdk_x11_display_get_xdisplay(gdkdisplay);

    frame_window_atom = XInternAtom(xdisplay, DECOR_INPUT_FRAME_ATOM_NAME, false);
    win_decor_atom = XInternAtom(xdisplay, DECOR_WINDOW_ATOM_NAME, false);
    win_blur_decor_atom = XInternAtom(xdisplay, DECOR_BLUR_ATOM_NAME, false);
    wm_move_resize_atom = XInternAtom(xdisplay, "_NET_WM_MOVERESIZE", false);
    restack_window_atom = XInternAtom(xdisplay, "_NET_RESTACK_WINDOW", false);
    select_window_atom = XInternAtom(xdisplay, DECOR_SWITCH_WINDOW_ATOM_NAME,
                                     false);
    mwm_hints_atom = XInternAtom(xdisplay, "_MOTIF_WM_HINTS", false);
    switcher_fg_atom = XInternAtom(xdisplay,
                                   DECOR_SWITCH_FOREGROUND_COLOR_ATOM_NAME,
                                   false);
    wm_protocols_atom = XInternAtom(xdisplay, "WM_PROTOCOLS", false);
    net_wm_context_help_atom =
        XInternAtom(xdisplay, "_NET_WM_CONTEXT_HELP", false);

    toolkit_action_atom =
        XInternAtom(xdisplay, "_COMPIZ_TOOLKIT_ACTION", false);
    toolkit_action_window_menu_atom =
        XInternAtom(xdisplay, "_COMPIZ_TOOLKIT_ACTION_WINDOW_MENU",
                    false);
    toolkit_action_force_quit_dialog_atom =
        XInternAtom(xdisplay, "_COMPIZ_TOOLKIT_ACTION_FORCE_QUIT_DIALOG",
                    false);

    emerald_sigusr1_atom = XInternAtom(xdisplay, "emerald-sigusr1", false);

    utf8_string_atom = XInternAtom(xdisplay, "UTF8_STRING", false);

    status = decor_acquire_dm_session(xdisplay, DefaultScreen(xdisplay),
                                      "emerald", replace, &dm_sn_timestamp);

    if (status != DECOR_ACQUIRE_STATUS_SUCCESS) {
        if (status == DECOR_ACQUIRE_STATUS_FAILED) {
            fprintf(stderr,
                    "%s: Could not acquire decoration manager "
                    "selection on screen %d display \"%s\"\n",
                    program_name.c_str(), DefaultScreen(xdisplay),
                    DisplayString(xdisplay));
        } else if (status == DECOR_ACQUIRE_STATUS_OTHER_DM_RUNNING) {
            fprintf(stderr,
                    "%s: Screen %d on display \"%s\" already "
                    "has a decoration manager; try using the "
                    "--replace option to replace the current "
                    "decoration manager.\n",
                    program_name.c_str(), DefaultScreen(xdisplay),
                    DisplayString(xdisplay));
        }

        return 1;
    }

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            if (cursor[i][j].shape != XC_left_ptr)
                cursor[i][j].cursor =
                    XCreateFontCursor(xdisplay, cursor[i][j].shape);
        }
    }
    if (button_cursor.shape != XC_left_ptr)
        button_cursor.cursor =
            XCreateFontCursor(xdisplay, button_cursor.shape);

    frame_table = g_hash_table_new(NULL, NULL);

    if (!create_tooltip_window()) {
        fprintf(stderr, "%s, Couldn't create tooltip window\n", argv[0]);
        return 1;
    }

    screen = wnck_screen_get_default();

    gdk_window_add_filter(NULL, selection_event_filter_func, NULL);

    gdk_window_add_filter(NULL, event_filter_func, NULL);

    connect_screen(screen);

    style_window = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_widget_realize(style_window);
    ws->pango_context = gtk_widget_create_pango_context(style_window);
    ws->font_desc = pango_font_description_from_string("Sans Bold 12");
    pango_context_set_font_description(ws->pango_context, ws->font_desc);
    lang = pango_context_get_language(ws->pango_context);
    metrics =
        pango_context_get_metrics(ws->pango_context, ws->font_desc, lang);

    ws->text_height = PANGO_PIXELS(pango_font_metrics_get_ascent(metrics) +
                                   pango_font_metrics_get_descent(metrics));

    ws->titlebar_height = ws->text_height;
    if (ws->titlebar_height < ws->min_titlebar_height) {
        ws->titlebar_height = ws->min_titlebar_height;
    }

    pango_font_metrics_unref(metrics);

    update_window_extents(ws);
    update_shadow(pfs);

    decor_set_dm_check_hint(xdisplay, DefaultScreen(xdisplay),
                            WINDOW_DECORATION_TYPE_PIXMAP);

    update_settings(ws);

    g_timeout_add(500, reload_if_needed, NULL);

    kit.run();
    gdk_error_trap_pop();

    return 0;
}

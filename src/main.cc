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
#include <libengine/emerald.h>
#include <libengine/engine.h>
#include <libengine/format.h>
#include <libengine/filesystem.h>
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
#include <functional>

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

Gtk::Widget* style_window;

GHashTable* frame_table;
Wnck::ActionMenu* action_menu = NULL;
bool action_menu_mapped = false;
int double_click_timeout = 250;

Gtk::Window* tip_window;
Gtk::Label* tip_label;
Glib::TimeVal tooltip_last_popdown = { -1, -1 };

sigc::connection tooltip_timeout_conn;

std::vector<decor_t*> g_draw_list;

bool enable_tooltips = true;
std::string engine;

window_settings* global_ws;

_cursor cursor[3][3] = {
    {
        { 0, XC_top_left_corner },
        { 0, XC_top_side },
        { 0, XC_top_right_corner }
    },
    {
        { 0, XC_left_side },
        { 0, XC_left_ptr },
        { 0, XC_right_side }
    },
    {
        { 0, XC_bottom_left_corner },
        { 0, XC_bottom_side },
        { 0, XC_bottom_right_corner }
    }
};
_cursor button_cursor = { 0, XC_hand2 };

void
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
    d.state = static_cast<Wnck::WindowState>(0);
    d.actions = static_cast<Wnck::WindowActions>(0);
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

void show_tooltip(const std::string& text)
{
    if (!enable_tooltips) {
        return;
    }
    int x, y, w, h;

    auto gdkdisplay = Gdk::Display::get_default();

    tip_label->set_text(text);

    Gtk::Requisition req{};
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
    gdk_display_get_pointer(gdkdisplay->gobj(), &screen, &x, &y, nullptr);

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

void hide_tooltip()
{
    if (tip_window->get_visible()) {
        g_get_current_time(&tooltip_last_popdown);
    }

    tip_window->hide();

    if (tooltip_timeout_conn) {
        tooltip_timeout_conn.disconnect();
    }
}

bool tooltip_recently_shown()
{
    Glib::TimeVal now;
    now.assign_current_time();
    now -= tooltip_last_popdown;
    return now.as_double() * 1000 < STICKY_REVERT_DELAY;
}

bool tooltip_timeout(const std::string& msg)
{
    show_tooltip(msg);
    return false;
}

void tooltip_start_delay(const std::string& msg)
{
    if (tooltip_timeout_conn) {
        return;
    }

    unsigned delay = DEFAULT_DELAY;
    if (tooltip_recently_shown()) {
        delay = STICKY_DELAY;
    }

    tooltip_timeout_conn = Glib::signal_timeout().connect(
                sigc::bind(&tooltip_timeout, msg), delay);
}

int tooltip_paint_window()
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

bool create_tooltip_window()
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

void
handle_tooltip_event(Wnck::Window* win,
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
            if (win->is_active()) {
                tooltip_start_delay(tip);
            }
        }
        break;
    case LeaveNotify:
        hide_tooltip();
        break;
    }
}
void action_menu_unmap()
{
    action_menu_mapped = false;
}

void action_menu_map(Wnck::Window* win, long button, Time time)
{
    // FIXME:
    // do not convert to gdkmm for now since get_default_screen has a bug that
    // makes it unusable
    GdkDisplay* gdkdisplay;
    GdkScreen* screen;

    gdkdisplay = gdk_display_get_default();
    screen = gdk_display_get_default_screen(gdkdisplay);

    if (action_menu) {
        if (action_menu_mapped) {
            delete action_menu;
            action_menu_mapped = false;
            action_menu = NULL;
            return;
        } else {
            delete action_menu;
        }
    }

    switch (win->get_window_type()) {
    case Wnck::WINDOW_DESKTOP:
    case Wnck::WINDOW_DOCK:
        /* don't allow window action */
        return;
    case Wnck::WINDOW_NORMAL:
    case Wnck::WINDOW_DIALOG:
    case Wnck::WINDOW_TOOLBAR:
    case Wnck::WINDOW_MENU:
    case Wnck::WINDOW_UTILITY:
    case Wnck::WINDOW_SPLASHSCREEN:
        /* allow window action menu */
        break;
    }

    action_menu = new Wnck::ActionMenu(win);
    // don't convert to gtkmm, see above
    gtk_menu_set_screen(GTK_MENU(action_menu->gobj()), screen);

    action_menu->signal_unmap().connect(&action_menu_unmap);

    action_menu->show();
    action_menu->popup(button, time);

    action_menu_mapped = true;
}

/* generic_button_event returns:
 * 0: nothing, hover, ButtonPress
 * XEvent Button code: ButtonRelease (mouse click)
 */
int generic_button_event(Wnck::Window* win, XEvent* xevent, int button, int bpict)
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

    decor_t* d = get_decor(win);
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

void close_button_event(Wnck::Window* win, XEvent* xevent)
{
    if (generic_button_event(win, xevent, B_T_CLOSE, B_CLOSE)) {
        win->close(xevent->xbutton.time);
    }
}

void max_button_event(Wnck::Window* win, XEvent* xevent)
{
    bool maximized = win->is_maximized();

    switch (generic_button_event(win,
                                 xevent,
                                 B_T_MAXIMIZE,
                                 (maximized ? B_RESTORE : B_MAXIMIZE))) {
    case Button1:
        maximized ? win->unmaximize() : win->maximize();
        break;
    case Button2:
        if (win->is_maximized_vertically()) {
            win->unmaximize_vertically();
        } else {
            win->maximize_vertically();
        }
        break;
    case Button3:
        if (win->is_maximized_horizontally()) {
            win->unmaximize_horizontally();
        } else {
            win->maximize_horizontally();
        }
        break;
    }
}

void min_button_event(Wnck::Window* win, XEvent* xevent)
{
    if (generic_button_event(win, xevent, B_T_MINIMIZE, B_MINIMIZE)) {
        win->minimize();
    }
}

void above_button_event(Wnck::Window* win, XEvent* xevent)
{
    if (win->is_above()) {
        if (generic_button_event(win, xevent, B_T_ABOVE, B_UNABOVE)) {
            win->unmake_above();
        }
    } else {
        if (generic_button_event(win, xevent, B_T_ABOVE, B_ABOVE)) {
            win->make_above();
        }
    }
}

void sticky_button_event(Wnck::Window* win, XEvent* xevent)
{
    if (win->is_sticky()) {
        if (generic_button_event(win, xevent, B_T_STICKY, B_UNSTICK)) {
            win->unstick();
        }
    } else {
        if (generic_button_event(win, xevent, B_T_STICKY, B_STICK)) {
            win->stick();
        }
    }
}

void send_help_message(Wnck::Window* win)
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
    id = win->get_xid();

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

void help_button_event(Wnck::Window* win, XEvent* xevent)
{
    if (generic_button_event(win, xevent, B_T_HELP, B_HELP)) {
        send_help_message(win);
    }
}
void menu_button_event(Wnck::Window* win, XEvent* xevent)
{
    if (generic_button_event(win, xevent, B_T_MENU, B_MENU)) {
        action_menu_map(win, xevent->xbutton.button, xevent->xbutton.time);
    }
}
void shade_button_event(Wnck::Window* win, XEvent* xevent)
{
    if (generic_button_event(win, xevent, B_T_SHADE, B_SHADE)) {
        if (win->is_shaded()) {
            win->unshade();
        } else {
            win->shade();
        }
    }
}


void top_left_event(Wnck::Window* win, XEvent* xevent)
{
    if (xevent->xbutton.button == 1) {
        move_resize_window(win, WM_MOVERESIZE_SIZE_TOPLEFT, xevent);
    }
}

void top_event(Wnck::Window* win, XEvent* xevent)
{
    if (xevent->xbutton.button == 1) {
        move_resize_window(win, WM_MOVERESIZE_SIZE_TOP, xevent);
    }
}

void top_right_event(Wnck::Window* win, XEvent* xevent)
{
    if (xevent->xbutton.button == 1) {
        move_resize_window(win, WM_MOVERESIZE_SIZE_TOPRIGHT, xevent);
    }
}

void left_event(Wnck::Window* win, XEvent* xevent)
{
    if (xevent->xbutton.button == 1) {
        move_resize_window(win, WM_MOVERESIZE_SIZE_LEFT, xevent);
    }
}

void title_event(Wnck::Window* win, XEvent* xevent)
{
    static unsigned int last_button_num = 0; // FIXME: not thread-safe, fix
    static Window last_button_xwindow = None;
    static Time last_button_time = 0;
    decor_t* d = get_decor(win);
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
                if (win->is_shaded()) {
                    win->unshade();
                } else {
                    win->shade();
                }
                break;
            case DOUBLE_CLICK_MAXIMIZE:
                if (win->is_maximized()) {
                    win->unmaximize();
                } else {
                    win->maximize();
                }
                break;
            case DOUBLE_CLICK_MINIMIZE:
                win->minimize();
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
        if (!win->is_shaded()) {
            win->shade();
        }
    } else if (xevent->xbutton.button == 5) {
        if (win->is_shaded()) {
            win->unshade();
        }
    }
}

void right_event(Wnck::Window* win, XEvent* xevent)
{
    if (xevent->xbutton.button == 1) {
        move_resize_window(win, WM_MOVERESIZE_SIZE_RIGHT, xevent);
    }
}

void bottom_left_event(Wnck::Window* win, XEvent* xevent)
{
    if (xevent->xbutton.button == 1) {
        move_resize_window(win, WM_MOVERESIZE_SIZE_BOTTOMLEFT, xevent);
    }
}

void bottom_event(Wnck::Window* win, XEvent* xevent)
{
    if (xevent->xbutton.button == 1) {
        move_resize_window(win, WM_MOVERESIZE_SIZE_BOTTOM, xevent);
    }
}

void bottom_right_event(Wnck::Window* win, XEvent* xevent)
{
    if (xevent->xbutton.button == 1) {
        move_resize_window(win, WM_MOVERESIZE_SIZE_BOTTOMRIGHT, xevent);
    }
}

void force_quit_dialog_realize(Gtk::MessageDialog* dialog, Wnck::Window* win)
{
    gdk_error_trap_push();
    auto drawable = Glib::RefPtr<Gdk::Drawable>(dialog->get_window());
    auto xid = gdk_x11_drawable_get_xid(drawable->gobj());

    XSetTransientForHint(gdk_x11_display_get_xdisplay(gdk_display_get_default()),
                         xid, win->get_xid());
    XSync(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), false);
    gdk_error_trap_pop();
}

std::string get_client_machine(Window xwindow)
{
    Atom atom, type;
    unsigned long nitems, bytes_after;
    char* str = NULL;
    unsigned char* sstr = NULL;
    int format, result;

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

    std::string ret = str ? str : "";

    XFree(str);

    return ret;
}

void kill_window(Wnck::Window* win)
{
    Wnck::Application* app = win->get_application();
    if (app) {
        char buf[257];
        int pid;

        pid = app->get_pid();
        std::string client_machine = get_client_machine(app->get_xid());

        if (!client_machine.empty() && pid > 0) {
            if (gethostname(buf, sizeof(buf) - 1) == 0) {
                if (client_machine == buf) {
                    kill(pid, 9);
                }
            }
        }
    }

    gdk_error_trap_push();
    XKillClient(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), win->get_xid());
    XSync(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), false);
    gdk_error_trap_pop();
}

void force_quit_dialog_response(int response, Wnck::Window* win)
{
    decor_t* d = get_decor(win);

    if (response == GTK_RESPONSE_ACCEPT) {
        kill_window(win);
    }

    if (d->force_quit_dialog) {
        auto dlg = d->force_quit_dialog;
        d->force_quit_dialog = nullptr;
        delete dlg;
    }
}

void show_force_quit_dialog(Wnck::Window* win, Time timestamp)
{
    decor_t* d = get_decor(win);

    if (d->force_quit_dialog) {
        return;
    }

    std::string top = format(_("The window \"%s\" is not responding."),
                             markup_escape(win->get_name()));
    std::string bot = _("Forcing this application to quit will cause you "
                        " to lose any unsaved changes.");

    std::string markup = format("<b>%s</b>\n\n%s", markup_escape(top),
                                markup_escape(bot));

    auto *dlg = new Gtk::MessageDialog(markup, true, Gtk::MESSAGE_WARNING,
                                       Gtk::BUTTONS_NONE, true);

    dlg->set_icon_name("force-quit");
    dlg->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_REJECT);
    dlg->add_button(_("_Force Quit"), Gtk::RESPONSE_ACCEPT);
    dlg->set_default_response(Gtk::RESPONSE_REJECT);
    dlg->signal_realize().connect(std::bind(force_quit_dialog_realize, dlg, win));
    dlg->signal_response().connect(std::bind(force_quit_dialog_response,
                                             std::placeholders::_1, win));
    dlg->set_modal();
    gtk_widget_realize(static_cast<Gtk::Widget*>(dlg)->gobj()); // protected method

    gdk_x11_window_set_user_time(dlg->get_window()->gobj(), timestamp);

    dlg->show();

    d->force_quit_dialog = dlg;
}

void hide_force_quit_dialog(Wnck::Window* win)
{
    decor_t* d = get_decor(win);

    if (d->force_quit_dialog) {
        auto dlg = d->force_quit_dialog;
        d->force_quit_dialog = nullptr;
        delete dlg;
    }
}

GdkFilterReturn
event_filter_func(GdkXEvent* gdkxevent, GdkEvent* event, void* data)
{
    (void) event;
    (void) data;
    Display* xdisplay;
    GdkDisplay* gdkdisplay;
    XEvent* xevent = reinterpret_cast<XEvent*>(gdkxevent);
    unsigned long xid = 0;

    gdkdisplay = gdk_display_get_default();
    xdisplay = GDK_DISPLAY_XDISPLAY(gdkdisplay);

    switch (xevent->type) {
    case ButtonPress:
    case ButtonRelease:
        xid = (unsigned long)
              g_hash_table_lookup(frame_table,
                                  GINT_TO_POINTER(xevent->xbutton.window));
        break;
    case EnterNotify:
    case LeaveNotify:
        xid = (unsigned long)
              g_hash_table_lookup(frame_table,
                                  GINT_TO_POINTER(xevent->xcrossing.
                                          window));
        break;
    case MotionNotify:
        xid = (unsigned long)
              g_hash_table_lookup(frame_table,
                                  GINT_TO_POINTER(xevent->xmotion.window));
        break;
    case PropertyNotify:
        if (xevent->xproperty.atom == frame_window_atom) {
            Wnck::Window* win;

            xid = xevent->xproperty.window;

            win = Wnck::Window::get_for_xid(xid);
            if (win) {
                Window frame;

                if (get_window_prop(xid, frame_window_atom, &frame)) {
                    add_frame_window(win, frame);
                } else {
                    remove_frame_window(win);
                }
            }
        } else if (xevent->xproperty.atom == mwm_hints_atom) {
            Wnck::Window* win;

            xid = xevent->xproperty.window;

            win = Wnck::Window::get_for_xid(xid);
            if (win) {
                decor_t* d = get_decor(win);
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
            Wnck::Window* win;

            xid = xevent->xproperty.window;

            win = Wnck::Window::get_for_xid(xid);
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
                Wnck::Window* win = Wnck::Window::get_for_xid(xevent->xclient.window);
                if (win) {
                    action_menu_map(win,
                                    xevent->xclient.data.l[2],
                                    xevent->xclient.data.l[1]);
                }
            } else if (action == toolkit_action_force_quit_dialog_atom) {
                Wnck::Window* win;

                win = Wnck::Window::get_for_xid(xevent->xclient.window);
                if (win) {
                    if (xevent->xclient.data.l[2])
                        show_force_quit_dialog(win, xevent->xclient.data.l[1]);
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
        Wnck::Window* win;

        win = Wnck::Window::get_for_xid(xid);
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
            decor_t* d = get_decor(win);

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

GdkFilterReturn selection_event_filter_func(GdkXEvent* gdkxevent,
                                            GdkEvent* event, void* data)
{
    (void) event;
    (void) data;
    Display* xdisplay;
    GdkDisplay* gdkdisplay;
    XEvent* xevent = reinterpret_cast<XEvent*>(gdkxevent);
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

void set_picture_transform(Display* xdisplay, Picture p, int dx, int dy)
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

std::vector<XFixed> create_gaussian_kernel(double radius, double sigma,
                                           double alpha, double opacity,
                                           int* r_size)
{
    double scale, x_scale, fx, sum;
    int size, x, i, n;

    scale = 1.0 / (2.0 * M_PI * sigma * sigma);

    if (alpha == -0.5) {
        alpha = 0.5;
    }

    size = alpha * 2 + 2;

    x_scale = 2.0 * radius / size;

    if (size < 3) {
        return {};
    }

    n = size;

    std::vector<double> amp;
    amp.resize(n);

    n += 2;

    std::vector<XFixed> params;
    params.resize(n);

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

    *r_size = size;

    return params;
}

/* to save some memory, value is specific to current decorations */
#define CORNER_REDUCTION 3

int update_shadow(frame_settings* fs)
{
    Display* xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    XRenderPictFormat* format;
    GdkPixmap* pixmap;
    Picture src, dst, tmp;
    XFilters* filters;
    const char* filter = NULL;
    int size, n_params = 0;
    decor_t d;

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
    auto params = create_gaussian_kernel(ws->shadow_radius, ws->shadow_radius / 2.0,        // SIGMA
                                         ws->shadow_radius,        // ALPHA
                                         ws->shadow_opacity, &size);
    if (params.empty()) {
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
    d.state = static_cast<Wnck::WindowState>(0);
    d.actions = static_cast<Wnck::WindowActions>(0);
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

    ws->shadow_pattern.clear();

    if (ws->shadow_pixmap) {
        g_object_unref(G_OBJECT(ws->shadow_pixmap));
        ws->shadow_pixmap = NULL;
    }

    /* no shadow */
    if (size <= 0) {
        return 1;
    }

    pixmap = create_pixmap(d.width, d.height);
    if (!pixmap) {
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
        std::cerr << "can't generate shadows, X server doesn't support "
                     "convolution filters\n";
        g_object_unref(G_OBJECT(pixmap));
        return 1;
    }

    /* WINDOWS WITH DECORATION */

    d.pixmap = create_pixmap(d.width, d.height);
    if (!d.pixmap) {
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
    XRenderSetPictureFilter(xdisplay, dst, filter, params.data(), n_params);
    XRenderComposite(xdisplay,
                     PictOpSrc,
                     src, dst, tmp, 0, 0, 0, 0, 0, 0, d.width, d.height);

    /* second pass */
    params[0] = 1 << 16;
    params[1] = (n_params - 2) << 16;

    set_picture_transform(xdisplay, tmp, 0, ws->shadow_offset_y);
    XRenderSetPictureFilter(xdisplay, tmp, filter, params.data(), n_params);
    XRenderComposite(xdisplay,
                     PictOpSrc,
                     src, tmp, dst, 0, 0, 0, 0, 0, 0, d.width, d.height);

    XRenderFreePicture(xdisplay, tmp);
    XRenderFreePicture(xdisplay, dst);

    g_object_unref(G_OBJECT(pixmap));

    ws->large_shadow_pixmap = d.pixmap;

    auto g_cr = gdk_cairo_create(GDK_DRAWABLE(ws->large_shadow_pixmap));
    auto cr = Cairo::RefPtr<Cairo::Context>{new Cairo::Context{g_cr, true}};

    ws->shadow_pattern = Cairo::SurfacePattern::create(cr->get_target());
    ws->shadow_pattern->set_filter(Cairo::FILTER_NEAREST);

    /* WINDOWS WITHOUT DECORATIONS */

    d.width = ws->shadow_left_space + ws->shadow_left_corner_space + 1 +
              ws->shadow_right_space + ws->shadow_right_corner_space;
    d.height = ws->shadow_top_space + ws->shadow_top_corner_space + 1 +
               ws->shadow_bottom_space + ws->shadow_bottom_corner_space;

    pixmap = create_pixmap(d.width, d.height);
    if (!pixmap) {
        return 0;
    }

    d.pixmap = create_pixmap(d.width, d.height);
    if (!d.pixmap) {
        g_object_unref(G_OBJECT(pixmap));
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
    XRenderSetPictureFilter(xdisplay, dst, filter, params.data(), n_params);
    XRenderComposite(xdisplay,
                     PictOpSrc,
                     src, dst, tmp, 0, 0, 0, 0, 0, 0, d.width, d.height);

    /* second pass */
    params[0] = 1 << 16;
    params[1] = (n_params - 2) << 16;

    set_picture_transform(xdisplay, tmp, 0, ws->shadow_offset_y);
    XRenderSetPictureFilter(xdisplay, tmp, filter, params.data(), n_params);
    XRenderComposite(xdisplay,
                     PictOpSrc,
                     src, tmp, dst, 0, 0, 0, 0, 0, 0, d.width, d.height);

    XRenderFreePicture(xdisplay, tmp);
    XRenderFreePicture(xdisplay, dst);
    XRenderFreePicture(xdisplay, src);

    g_object_unref(G_OBJECT(pixmap));

    ws->shadow_pixmap = d.pixmap;

    return 1;
}

void titlebar_font_changed(window_settings* ws)
{
    ws->pango_context->set_font_description(ws->font_desc);
    auto lang = ws->pango_context->get_language();
    auto metrics = ws->pango_context->get_metrics(ws->font_desc, lang);

    ws->text_height = PANGO_PIXELS(metrics.get_ascent() + metrics.get_descent());

    ws->titlebar_height = ws->text_height;
    if (ws->titlebar_height < ws->min_titlebar_height) {
        ws->titlebar_height = ws->min_titlebar_height;
    }
}

void load_buttons_image(window_settings* ws, int y)
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

void load_buttons_glow_images(window_settings* ws)
{
    int pix_width, pix_height;
    int pix_width2, pix_height2;
    bool success1 = false;
    bool success2 = false;

    if (ws->use_button_glow) {
        if (ws->ButtonGlowArray) {
            g_object_unref(ws->ButtonGlowArray);
        }
        std::string file1 = make_filename("buttons", "glow", "png");
        if ((ws->ButtonGlowArray = gdk_pixbuf_new_from_file(file1.c_str(), NULL))) {
            success1 = true;
        }
    }
    if (ws->use_button_inactive_glow) {
        if (ws->ButtonInactiveGlowArray) {
            g_object_unref(ws->ButtonInactiveGlowArray);
        }
        std::string file2 = make_filename("buttons", "inactive_glow", "png");
        if ((ws->ButtonInactiveGlowArray =
                     gdk_pixbuf_new_from_file(file2.c_str(), NULL))) {
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

void load_settings(window_settings* ws)
{
    fs::path path = fs::path(g_get_home_dir()) / ".emerald/settings.ini";
    KeyFile f;

    copy_from_defaults_if_needed();

    //settings
    f.load_from_file(path.native());
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
    path = fs::path(g_get_home_dir()) / ".emerald/theme/theme.ini";
    f.load_from_file(path.native());

    load_string_setting(f, engine, "engine", "engine");
    if (!load_engine(engine, ws)) {
        engine = "legacy";
        load_engine(engine, ws);
    }

    load_color_setting(f,&ws->fs_act->text.color,"active_text", "titlebar");
    load_color_setting(f,&ws->fs_inact->text.color,"inactive_text", "titlebar");
    load_float_setting(f,&ws->fs_act->text.alpha,"active_text_alpha", "titlebar");
    load_float_setting(f,&ws->fs_inact->text.alpha,"inactive_text_alpha", "titlebar");

    load_color_setting(f,&ws->fs_act->text_halo.color,"active_text_halo", "titlebar");
    load_color_setting(f,&ws->fs_inact->text_halo.color,"inactive_text_halo", "titlebar");
    load_float_setting(f,&ws->fs_act->text_halo.alpha,"active_text_halo_alpha", "titlebar");
    load_float_setting(f,&ws->fs_inact->text_halo.alpha,"inactive_text_halo_alpha", "titlebar");

    load_color_setting(f,&ws->fs_act->button.color,"active_button", "buttons");
    load_color_setting(f,&ws->fs_inact->button.color,"inactive_button", "buttons");
    load_float_setting(f,&ws->fs_act->button.alpha,"active_button_alpha", "buttons");
    load_float_setting(f,&ws->fs_inact->button.alpha,"inactive_button_alpha", "buttons");

    load_color_setting(f,&ws->fs_act->button_halo.color,"active_button_halo", "buttons");
    load_color_setting(f,&ws->fs_inact->button_halo.color,"inactive_button_halo", "buttons");
    load_float_setting(f,&ws->fs_act->button_halo.alpha,"active_button_halo_alpha", "buttons");
    load_float_setting(f,&ws->fs_inact->button_halo.alpha,"inactive_button_halo_alpha", "buttons");

    load_engine_settings(f, ws);
    load_font_setting(f, ws->font_desc, "titlebar_font", "titlebar");
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
    ws->tobj_layout = tmp;
    load_int_setting(f, &ws->button_offset, "vertical_offset", "buttons");
    load_int_setting(f, &ws->button_hoffset, "horizontal_offset", "buttons");
    load_int_setting(f, &ws->win_extents.top, "top", "borders");
    load_int_setting(f, &ws->win_extents.left, "left", "borders");
    load_int_setting(f, &ws->win_extents.right, "right", "borders");
    load_int_setting(f, &ws->win_extents.bottom, "bottom", "borders");
    load_int_setting(f, &ws->min_titlebar_height, "min_titlebar_height",
                     "titlebar");
}

void update_settings(window_settings* ws)
{
    //assumes ws is fully allocated

    GdkDisplay* gdkdisplay;

    // Display    *xdisplay;
    GdkScreen* gdkscreen;
    Wnck::Screen* screen = Wnck::Screen::get_default();

    load_settings(ws);

    gdkdisplay = gdk_display_get_default();
    // xdisplay   = gdk_x11_display_get_xdisplay (gdkdisplay);
    gdkscreen = gdk_display_get_default_screen(gdkdisplay);

    titlebar_font_changed(ws);
    update_window_extents(ws);
    update_shadow(ws->fs_act);
    update_default_decorations(gdkscreen, ws->fs_act, ws->fs_inact);

    for (auto* win : screen->get_windows()) {
        decor_t* d = get_decor(win);

        if (d->decorated) {
            d->width = d->height = 0;
            update_window_decoration_size(win);
            update_event_windows(win);
        }
    }
}

#ifdef USE_DBUS
DBusHandlerResult
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
        std::cout << format("emerald: Connection Error (%s)\n", err->message);
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
bool reload_if_needed()
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
    int status;

    int i, j;
    bool replace = false;
    frame_settings* pfs;

    window_settings* ws = new window_settings;
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

    ws->tobj_layout = "IT::HNXC";        // DEFAULT TITLE OBJECT LAYOUT, does not use any odd buttons
    //ws->tobj_layout=g_strdup("CNX:IT:HM");

    pfs = new frame_settings{};
    pfs->ws = ws;
    pfs->text = alpha_color(1.0, 1.0, 1.0, 1.0);
    pfs->text_halo = alpha_color(0.0, 0.0, 0.0, 0.2);
    pfs->button = alpha_color(1.0, 1.0, 1.0, 0.8);
    pfs->button_halo = alpha_color(0.0, 0.0, 0.0, 0.2);
    ws->fs_act = pfs;

    pfs = new frame_settings{};
    pfs->ws = ws;
    pfs->text = alpha_color(0.8, 0.8, 0.8, 0.8);
    pfs->text_halo = alpha_color(0.0, 0.0, 0.0, 0.2);
    pfs->button = alpha_color(0.8, 0.8, 0.8, 0.8);
    pfs->button_halo = alpha_color(0.0, 0.0, 0.0, 0.2);
    ws->fs_inact = pfs;

    ws->round_top_left = true;
    ws->round_top_right = true;
    ws->round_bottom_left = true;
    ws->round_bottom_right = true;

    load_engine("legacy", ws);        // assumed to always return true

    program_name = argv[0];

    //ws->ButtonBase = NULL;
    for (i = 0; i < (S_COUNT * B_COUNT); i++) {
        ws->ButtonPix[i] = NULL;
    }
    Gtk::Main kit(argc, argv);
    Wnck::init();
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
            std::cerr << format("%s: %s version %s\n", program_name, PACKAGE,
                                VERSION);
            return 0;
        } else if (strcmp(argv[i], "--help") == 0) {
            std::cerr << format("%s [--replace] [--help] [--version]\n",
                                program_name);
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
            std::cerr << format("%s: Could not acquire decoration manager "
                                "selection on screen %d display \"%s\"\n",
                                program_name, DefaultScreen(xdisplay),
                                DisplayString(xdisplay));
        } else if (status == DECOR_ACQUIRE_STATUS_OTHER_DM_RUNNING) {
            std::cerr << format("%s: Screen %d on display \"%s\" already "
                                "has a decoration manager; try using the "
                                "--replace option to replace the current "
                                "decoration manager.\n",
                                program_name, DefaultScreen(xdisplay),
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
        std::cerr << format("%s, Couldn't create tooltip window\n", argv[0]);
        return 1;
    }

    Wnck::Screen* screen = Wnck::Screen::get_default();

    gdk_window_add_filter(NULL, selection_event_filter_func, NULL);

    gdk_window_add_filter(NULL, event_filter_func, NULL);

    connect_screen(screen);

    style_window = Gtk::manage(new Gtk::Window(Gtk::WINDOW_POPUP));
    gtk_widget_realize(static_cast<Gtk::Widget*>(style_window)->gobj());
    ws->pango_context = style_window->create_pango_context();
    ws->font_desc = Pango::FontDescription("Sans Bold 12");
    ws->pango_context->set_font_description(ws->font_desc);
    auto lang = ws->pango_context->get_language();
    auto metrics = ws->pango_context->get_metrics(ws->font_desc, lang);

    ws->text_height = PANGO_PIXELS(metrics.get_ascent() + metrics.get_descent());

    ws->titlebar_height = ws->text_height;
    if (ws->titlebar_height < ws->min_titlebar_height) {
        ws->titlebar_height = ws->min_titlebar_height;
    }

    update_window_extents(ws);
    update_shadow(pfs);

    decor_set_dm_check_hint(xdisplay, DefaultScreen(xdisplay),
                            WINDOW_DECORATION_TYPE_PIXMAP);

    update_settings(ws);

    Glib::signal_timeout().connect(&reload_if_needed, 500);

    kit.run();
    gdk_error_trap_pop();

    return 0;
}

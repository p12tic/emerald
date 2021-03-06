/*  This file is part of emerald

    Copyright (C) 2006  Novell Inc.
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

#include "window.h"
#include "utils.h"
#include "decor.h"
#include <libengine/titlebar.h>
#include <libengine/format.h>
#include <X11/Xatom.h>

Glib::RefPtr<Gdk::Pixmap> create_pixmap(int w, int h)
{
    try {
        Glib::RefPtr<Gdk::Drawable> empty;
        auto visual = Gdk::Visual::get_best(32);
        auto pixmap = Gdk::Pixmap::create(empty, w, h, 32);
        auto colormap = Gdk::Colormap::create(visual, false);
        pixmap->set_colormap(colormap);
        return pixmap;
    } catch (...) {
        return {};
    }
}

Glib::RefPtr<Gdk::Pixmap> pixmap_new_from_pixbuf(const Glib::RefPtr<Gdk::Pixbuf>& pixbuf)
{

    unsigned width = pixbuf->get_width();
    unsigned height = pixbuf->get_height();

    Glib::RefPtr<Gdk::Pixmap> pixmap = create_pixmap(width, height);
    if (!pixmap) {
        return pixmap;
    }

    Cairo::RefPtr<Cairo::Context> cr = pixmap->create_cairo_context();
    Gdk::Cairo::set_source_pixbuf(cr, pixbuf);
    cr->set_operator(Cairo::OPERATOR_SOURCE);
    cr->paint();

    return pixmap;
}

void position_title_object(char obj, Wnck::Window* win, window_settings* ws,
                           int x, int s)
{
    decor_t* d = get_decor(win);
    int i = get_title_object_type(obj);

    if (i < 0) {
        return;
    }
    if (i < B_T_COUNT) {
        Display* xdisplay;
        int w = ws->use_pixmap_buttons ? ws->c_icon_size[i].w : 16;
        int h = ws->use_pixmap_buttons ? ws->c_icon_size[i].h : 16;
        int y = ws->button_offset;

        xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
        gdk_error_trap_push();
        if (d->actions & button_actions[i]) {
            XMoveResizeWindow(xdisplay, d->button_windows[i], x -
                              ((ws->use_decoration_cropping &&
                                (d->state & Wnck::WINDOW_STATE_MAXIMIZED_HORIZONTALLY)) ?
                               ws->win_extents.left : 0), y, w, h);
            if (button_cursor.cursor && ws->button_hover_cursor == 1)
                XDefineCursor(xdisplay,
                              d->button_windows[i], button_cursor.cursor);
            else {
                XUndefineCursor(xdisplay, d->button_windows[i]);
            }
        }
        XSync(xdisplay, false);
        gdk_error_trap_pop();
    }
    d->tobj_item_pos[i] = x - d->tobj_pos[s];
    d->tobj_item_state[i] = s;
}

void layout_title_objects(Wnck::Window* win)
{
    decor_t* d = get_decor(win);
    window_settings* ws = d->fs->ws;
    unsigned i;
    int state = 0;
    int owidth;
    int x;
    bool removed = false;

    d->tobj_size[0] = 0;
    d->tobj_size[1] = 0;
    d->tobj_size[2] = 0;

    Gdk::Rectangle rect = win->get_geometry();
    for (i = 0; i < ws->tobj_layout.length(); i++) {
        if (ws->tobj_layout[i] == '(') {
            i++;
            d->tobj_size[state] +=
                g_ascii_strtoull(&ws->tobj_layout[i], NULL, 0);
            while (ws->tobj_layout[i] && g_ascii_isdigit(ws->tobj_layout[i])) {
                i++;
            }
            continue;
        }
        if ((owidth =
                    get_title_object_width(ws->tobj_layout[i], ws, d)) == -1) {
            state++;
            if (state > 2) {
                break;
            }
        } else {
            d->tobj_size[state] += owidth;
        }

        d->tobj_item_width[state] = owidth;
    }
    state = 0;
    d->tobj_pos[0] = ws->win_extents.left;        // always true
    d->tobj_pos[2] = rect.get_width() - d->tobj_size[2] + d->tobj_pos[0];
    d->tobj_pos[1] =
        MAX((d->tobj_pos[2] + d->tobj_size[0] - d->tobj_size[1]) / 2,
            0) + d->tobj_pos[0];
    x = d->tobj_pos[0] + ws->button_hoffset;
    for (i = 0; i < TBT_COUNT; i++) {
        d->tobj_item_state[i] = 3;
    }

    for (i = 0; i < ws->tobj_layout.length(); i++) {
        if (ws->tobj_layout[i] == '(') {
            i++;
            x += g_ascii_strtoull(&ws->tobj_layout[i], NULL, 0);
            while (ws->tobj_layout[i] && std::isdigit(ws->tobj_layout[i])) {
                i++;
            }
            continue;
        }
        if (get_title_object_type(ws->tobj_layout[i]) == -1) {
            state++;
            if (state > 2) {
                break;
            } else {
                x = d->tobj_pos[state];
            }
        } else {
            if (state == 2 && !removed) {
                x -= ws->button_hoffset;
                removed = true;
            }
            position_title_object(ws->tobj_layout[i], win, ws, x, state);
            x += get_title_object_width(ws->tobj_layout[i], ws, d);
        }
    }
}

void update_event_windows(Wnck::Window* win)
{
    Display* xdisplay;
    decor_t* d = get_decor(win);
    int x, y, w, h;
    int i, j, k, l;
    window_settings* ws = d->fs->ws;

    xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());

    Gdk::Rectangle rect = win->get_geometry();

    if (d->state & Wnck::WINDOW_STATE_SHADED) {
        rect.set_height(0);
        k = l = 1;
    } else {
        k = 0;
        l = 2;
    }

    gdk_error_trap_push();

    for (i = 0; i < 3; i++) {
        static unsigned event_window_actions[3][3] = {
            {
                Wnck::WINDOW_ACTION_RESIZE,
                Wnck::WINDOW_ACTION_RESIZE,
                Wnck::WINDOW_ACTION_RESIZE
            }, {
                Wnck::WINDOW_ACTION_RESIZE,
                Wnck::WINDOW_ACTION_MOVE,
                Wnck::WINDOW_ACTION_RESIZE
            }, {
                Wnck::WINDOW_ACTION_RESIZE,
                Wnck::WINDOW_ACTION_RESIZE,
                Wnck::WINDOW_ACTION_RESIZE
            }
        };

        for (j = 0; j < 3; j++) {
            if (d->actions & event_window_actions[i][j] && i >= k && i <= l) {
                x = ws->pos[i][j].x + ws->pos[i][j].xw * rect.get_width();
                y = ws->pos[i][j].y + ws->pos[i][j].yh * rect.get_height();
                w = ws->pos[i][j].w + ws->pos[i][j].ww * rect.get_width();
                h = ws->pos[i][j].h + ws->pos[i][j].hh * rect.get_height();

                XMapWindow(xdisplay, d->event_windows[i][j]);
                XMoveResizeWindow(xdisplay, d->event_windows[i][j], x -
                                  ((ws->use_decoration_cropping &&
                                    (d->state & Wnck::WINDOW_STATE_MAXIMIZED_HORIZONTALLY)) ?
                                   ws->win_extents.left : 0), y, w, h);
            } else {
                XUnmapWindow(xdisplay, d->event_windows[i][j]);
            }
        }
    }

    for (i = 0; i < B_T_COUNT; i++) {
        if (d->actions & button_actions[i]) {
            XMapWindow(xdisplay, d->button_windows[i]);
        } else {
            XUnmapWindow(xdisplay, d->button_windows[i]);
        }
    }
    layout_title_objects(win);
    XSync(xdisplay, false);
    gdk_error_trap_pop();
}

int max_window_name_width(Wnck::Window* win)
{
    decor_t* d = get_decor(win);
    window_settings* ws = d->fs->ws;

    std::string name = win->get_name();
    if (name.empty()) {
        return 0;
    }

    if (!d->layout) {
        d->layout = Pango::Layout::create(ws->pango_context);
        if (!d->layout) {
            return 0;
        }

        d->layout->set_wrap(Pango::WRAP_CHAR);
    }

    d->layout->set_auto_dir(false);
    d->layout->set_width(-1);
    d->layout->set_text(name);
    int w, h;
    d->layout->get_pixel_size(w, h);

    if (!d->name.empty()) {
        d->layout->set_text(d->name);
    }

    return w + 6;
}

void update_window_decoration_name(Wnck::Window* win)
{
    decor_t* d = get_decor(win);
    window_settings* ws = d->fs->ws;

    d->name = "";

    std::string name = win->get_name();
    if (!name.empty()) {
        d->layout->set_auto_dir(false);
        d->layout->set_text("");
        d->layout->set_width(0);
        layout_title_objects(win);

        int w = d->width - ws->left_space - ws->right_space - d->tobj_size[0]
                - d->tobj_size[1] - d->tobj_size[2];
        if (w < 1) {
            w = 1;
        }

        d->layout->set_width(w * PANGO_SCALE);
        d->layout->set_text(name);

        auto line = d->layout->get_line(0);
        long name_length = line->get_length();

        if (d->layout->get_line_count() > 1) {
            if (name_length < 4) {
                d->layout.clear();
                return;
            }
            d->name = name.substr(name_length - 3) + "...";
        } else {
            d->name = name;
        }

        d->layout->set_text(d->name);
        layout_title_objects(win);
    } else {
        d->layout.clear();
    }
}

void update_window_decoration_icon(Wnck::Window* win)
{
    decor_t* d = get_decor(win);

    d->icon.clear();
    d->icon_pixmap.clear();
    d->icon_pixbuf.clear();

    d->icon_pixbuf = win->get_mini_icon();
    if (d->icon_pixbuf) {

        d->icon_pixmap = pixmap_new_from_pixbuf(d->icon_pixbuf);
        Cairo::RefPtr<Cairo::Context> cr = d->icon_pixmap->create_cairo_context();
        d->icon = Cairo::SurfacePattern::create(cr->get_target());
    }
}

void update_window_decoration_state(Wnck::Window* win)
{
    decor_t* d = get_decor(win);

    d->state = win->get_state();
}

void update_window_decoration_actions(Wnck::Window* win)
{
    decor_t* d = get_decor(win);

    /* code to check for context help protocol */
    Atom actual;
    int result;
    unsigned long n, left;
    unsigned long offset;
    unsigned char* data;
    Window id = win->get_xid();
    Display* xdisplay;
    GdkDisplay* gdkdisplay;

    //GdkScreen   *screen;
    //Window      xroot;
    //XEvent      ev;

    gdkdisplay = gdk_display_get_default();
    xdisplay = GDK_DISPLAY_XDISPLAY(gdkdisplay);
    //screen     = gdk_display_get_default_screen (gdkdisplay);
    //xroot      = RootWindowOfScreen (gdk_x11_screen_get_xscreen (screen));

    d->actions = win->get_actions();
    data = NULL;
    left = 1;
    offset = 0;
    while (left) {
        int fmt;
        result = XGetWindowProperty(xdisplay, id, wm_protocols_atom,
                                    offset, 1L, false, XA_ATOM, &actual,
                                    &fmt, &n, &left, &data);
        offset++;
        if (result == Success && n && data) {
            Atom a;

            memcpy(&a, data, sizeof(Atom));
            XFree((void*)data);
            if (a == net_wm_context_help_atom) {
                d->actions |= static_cast<Wnck::WindowActions>(FAKE_WINDOW_ACTION_HELP);
            }
            data = NULL;
        } else if (result != Success) {
            /* Closes #161 */
            std::cerr << format("XGetWindowProperty() returned non-success value"
                                "(%d) for window '%s'.\n", result, win->get_name());
            break;
        }
    }
}

bool update_window_decoration_size(Wnck::Window* win)
{
    decor_t* d = get_decor(win);
    Glib::RefPtr<Gdk::Pixmap> pixmap, buffer_pixmap;
    Glib::RefPtr<Gdk::Pixmap> ipixmap, ibuffer_pixmap;
    int width, height;

    window_settings* ws = d->fs->ws;

    max_window_name_width(win);
    layout_title_objects(win);

    Gdk::Rectangle rect = win->get_geometry();
    int w = rect.get_width();
    int h = rect.get_height();

    width = ws->left_space + MAX(w, 1) + ws->right_space;
    height = ws->top_space + ws->bottom_space + ws->titlebar_height;

    if (ws->stretch_sides) {
        height += ws->normal_top_corner_space + ws->bottom_corner_space + 2;
    } else {
        height += MAX(h, 1);
    }

    if (width == d->width && height == d->height) {
        update_window_decoration_name(win);
        return false;
    }
    reset_buttons_bg_and_fade(d);

    pixmap = create_pixmap(MAX(width, height),
                           ws->top_space + ws->titlebar_height +
                           ws->left_space + ws->right_space +
                           ws->bottom_space);
    if (!pixmap) {
        return false;
    }

    buffer_pixmap =
        create_pixmap(MAX(width, height),
                      ws->top_space + ws->titlebar_height +
                      ws->left_space + ws->right_space +
                      ws->bottom_space);
    if (!buffer_pixmap) {
        return false;
    }

    ipixmap = create_pixmap(MAX(width, height),
                            ws->top_space + ws->titlebar_height +
                            ws->left_space + ws->right_space +
                            ws->bottom_space);

    ibuffer_pixmap = create_pixmap(MAX(width, height),
                                   ws->top_space + ws->titlebar_height +
                                   ws->left_space + ws->right_space +
                                   ws->bottom_space);

    d->p_active.clear();
    d->p_active_buffer.clear();
    d->p_inactive.clear();
    d->p_inactive_buffer.clear();
    d->gc.clear();

    d->only_change_active = false;

    d->pixmap = d->active ? pixmap : ipixmap;
    d->buffer_pixmap = d->active ? buffer_pixmap : ibuffer_pixmap;
    d->p_active = pixmap;
    d->p_active_buffer = buffer_pixmap;
    d->p_inactive = ipixmap;
    d->p_inactive_buffer = ibuffer_pixmap;
    d->gc = Gdk::GC::create(pixmap);

    d->width = width;
    d->height = height;

    d->prop_xid = win->get_xid();

    update_window_decoration_name(win);

    update_button_regions(d);
    stop_button_fade(d);

    queue_decor_draw(d);

    return true;
}

void add_frame_window(Wnck::Window* win, Window frame)
{
    Display* xdisplay;
    XSetWindowAttributes attr{};
    unsigned long xid = win->get_xid();
    decor_t* d = get_decor(win);
    int i, j;

    xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());

    attr.event_mask = ButtonPressMask | EnterWindowMask | LeaveWindowMask;
    attr.override_redirect = true;

    gdk_error_trap_push();

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            d->event_windows[i][j] =
                XCreateWindow(xdisplay,
                              frame,
                              0, 0, 1, 1, 0,
                              CopyFromParent, CopyFromParent,
                              CopyFromParent,
                              CWOverrideRedirect | CWEventMask, &attr);

            if (cursor[i][j].cursor)
                XDefineCursor(xdisplay, d->event_windows[i][j],
                              cursor[i][j].cursor);
        }
    }

    attr.event_mask |= ButtonReleaseMask;

    for (i = 0; i < B_T_COUNT; i++) {
        d->button_windows[i] =
            XCreateWindow(xdisplay,
                          frame,
                          0, 0, 1, 1, 0,
                          CopyFromParent, CopyFromParent, CopyFromParent,
                          CWOverrideRedirect | CWEventMask, &attr);

        d->button_states[i] = 0;
    }

    XSync(xdisplay, false);
    if (!gdk_error_trap_pop()) {
        if (get_mwm_prop(xid) & (MWM_DECOR_ALL | MWM_DECOR_TITLE)) {
            d->decorated = true;
        }

        for (i = 0; i < 3; i++)
            for (j = 0; j < 3; j++)
                g_frame_table.emplace(d->event_windows[i][j], xid);

        for (i = 0; i < B_T_COUNT; i++)
            g_frame_table.emplace(d->button_windows[i], xid);

        update_window_decoration_state(win);
        update_window_decoration_actions(win);
        update_window_decoration_icon(win);
        update_window_decoration_size(win);

        update_event_windows(win);
    } else {
        memset(d->event_windows, 0, sizeof(d->event_windows));
    }
}

bool update_switcher_window(Wnck::Window* win, Window selected)
{
    decor_t* d = get_decor(win);
    int height, width = 0;
    Wnck::Window* selected_win;
    window_settings* ws = d->fs->ws;

    width = win->get_geometry().get_width();

    width += ws->left_space + ws->right_space;
    height = ws->top_space + SWITCHER_TOP_EXTRA +
             ws->switcher_top_corner_space + SWITCHER_SPACE +
             ws->switcher_bottom_corner_space + ws->bottom_space;

    d->decorated = false;
    d->draw = draw_switcher_decoration;

    if (!d->pixmap) {
        d->pixmap = ws->switcher_pixmap;
    }

    if (!d->buffer_pixmap) {
        d->buffer_pixmap = ws->switcher_buffer_pixmap;
    }

    if (!d->width) {
        d->width = ws->switcher_width;
    }

    if (!d->height) {
        d->height = ws->switcher_height;
    }

    selected_win = Wnck::Window::get_for_xid(selected);
    if (selected_win) {

        d->name = "";

        std::string name = selected_win->get_name();
        if (!name.empty()) {
            if (!d->layout) {
                d->layout = Pango::Layout::create(ws->pango_context);
                if (d->layout) {
                    d->layout->set_wrap(Pango::WRAP_CHAR);
                }
            }

            if (d->layout) {
                d->layout->set_auto_dir(false);
                d->layout->set_width(-1);
                d->layout->set_text(name);

                auto line = d->layout->get_line(0);

                long name_length = line->get_length();
                if (d->layout->get_line_count() > 1) {
                    if (name_length < 4) {
                        d->layout.clear();
                    } else {
                        d->name = d->name.substr(name_length - 3) + "...";
                    }
                } else {
                    d->name = name;
                }

                if (d->layout) {
                    d->layout->set_text(d->name);
                }
            }
        } else {
            d->layout.clear();
        }
    }

    if (width == d->width && height == d->height) {
        if (!d->gc && d->pixmap) {
            d->gc = Gdk::GC::create(d->pixmap);
        }

        if (d->pixmap) {
            queue_decor_draw(d);
            return false;
        }
    }

    Glib::RefPtr<Gdk::Pixmap> pixmap = create_pixmap(width, height);
    if (!pixmap) {
        return false;
    }

    Glib::RefPtr<Gdk::Pixmap> buffer_pixmap = create_pixmap(width, height);
    if (!buffer_pixmap) {
        return false;
    }

    ws->switcher_pixmap = pixmap;
    ws->switcher_buffer_pixmap = buffer_pixmap;

    ws->switcher_width = width;
    ws->switcher_height = height;

    d->pixmap = pixmap;
    d->buffer_pixmap = buffer_pixmap;
    d->gc = Gdk::GC::create(pixmap);

    d->width = width;
    d->height = height;

    d->prop_xid = win->get_xid();

    reset_buttons_bg_and_fade(d);

    queue_decor_draw(d);

    return true;
}

void remove_frame_window(Wnck::Window* win)
{
    decor_t* d = get_decor(win);

    d->p_active.clear();
    d->p_active_buffer.clear();
    d->p_inactive.clear();
    d->p_inactive_buffer.clear();
    d->gc.clear();

    for (int b_t = 0; b_t < B_T_COUNT; b_t++) {
        d->button_region[b_t].bg_pixmap.clear();
        d->button_region_inact[b_t].bg_pixmap.clear();
    }
    d->button_fade_info.cr.clear();

    if (d->button_fade_info.timer_conn) {
        d->button_fade_info.timer_conn.disconnect();
    }

    d->name = "";
    d->layout.clear();
    d->icon.clear();

    d->icon_pixmap.clear();
    d->icon_pixbuf.clear();

    if (d->force_quit_dialog) {
        auto dlg = d->force_quit_dialog;
        d->force_quit_dialog = nullptr;
        delete dlg;
    }

    d->width = 0;
    d->height = 0;

    d->decorated = false;

    d->state = static_cast<Wnck::WindowState>(0);
    d->actions = static_cast<Wnck::WindowActions>(0);

    auto it = std::find(g_draw_list.begin(), g_draw_list.end(), d);
    if (it != g_draw_list.end()) {
        g_draw_list.erase(it);
    }
}

void window_name_changed(Wnck::Window* win)
{
    decor_t* d = get_decor(win);

    if (d->decorated && !update_window_decoration_size(win)) {
        update_button_regions(d);
        queue_decor_draw(d);
    }
}

void window_geometry_changed(Wnck::Window* win)
{
    decor_t* d = get_decor(win);

    if (d->decorated) {
        auto r = win->get_geometry();
        if ((r.get_width() != d->client_width) || (r.get_height() != d->client_height)) {
            d->client_width = r.get_width();
            d->client_height = r.get_height();

            update_window_decoration_size(win);
            update_event_windows(win);
        }
    }
}

void window_icon_changed(Wnck::Window* win)
{
    decor_t* d = get_decor(win);

    if (d->decorated) {
        update_window_decoration_icon(win);
        update_button_regions(d);
        queue_decor_draw(d);
    }
}

void window_state_changed(Wnck::WindowState, Wnck::WindowState, Wnck::Window* win)
{
    decor_t* d = get_decor(win);

    if (d->decorated) {
        update_window_decoration_state(win);
        update_button_regions(d);
        stop_button_fade(d);
        update_window_decoration_size(win);
        update_event_windows(win);

        d->prop_xid = win->get_xid();
        queue_decor_draw(d);
    }
}

void window_actions_changed(Wnck::WindowActions,
                            Wnck::WindowActions, Wnck::Window* win)
{
    decor_t* d = get_decor(win);

    if (d->decorated) {
        update_window_decoration_actions(win);
        update_window_decoration_size(win);
        update_event_windows(win);

        d->prop_xid = win->get_xid();
        queue_decor_draw(d);
    }
}

void connect_window(Wnck::Window* win)
{
    win->signal_name_changed().connect([=](){ window_name_changed(win); });
    win->signal_geometry_changed().connect([=](){ window_geometry_changed(win); });
    win->signal_icon_changed().connect([=](){ window_icon_changed(win); });
    win->signal_state_changed().connect(sigc::bind(&window_state_changed, win));
    win->signal_actions_changed().connect(sigc::bind(&window_actions_changed, win));
}

void active_window_changed(Wnck::Window* previously_active_win,
                           Wnck::Screen* screen)
{
    Wnck::Window* win = previously_active_win;
    decor_t* d;

    if (win) {
        d = get_decor(win);
        if (d && d->pixmap && d->decorated) {
            d->active = win->is_active();
            d->fs = (d->active ? d->fs->ws->fs_act : d->fs->ws->fs_inact);
            if (std::find(g_draw_list.begin(), g_draw_list.end(), d) ==
                    g_draw_list.end()) {
                d->only_change_active = true;
            }
            d->prop_xid = win->get_xid();
            stop_button_fade(d);
            queue_decor_draw_for_buttons(d, true);
        }
    }

    win = screen->get_active_window();
    if (win) {
        d = get_decor(win);
        if (d && d->pixmap && d->decorated) {
            d->active = win->is_active();
            d->fs = (d->active ? d->fs->ws->fs_act : d->fs->ws->fs_inact);
            if (std::find(g_draw_list.begin(), g_draw_list.end(), d) ==
                    g_draw_list.end()) {
                d->only_change_active = true;
            }
            d->prop_xid = win->get_xid();
            stop_button_fade(d);
            queue_decor_draw_for_buttons(d, true);
        }
    }
}

void window_opened(Wnck::Window* win)
{
    Window window;
    unsigned long xid;

    decor_t* d = new decor_t;
    if (!d) {
        return;
    }

    auto rect = win->get_geometry();
    d->client_width = rect.get_width();
    d->client_height = rect.get_height();

    d->active = win->is_active();

    d->draw = draw_window_decoration;
    d->fs = d->active ? global_ws->fs_act : global_ws->fs_inact;

    reset_buttons_bg_and_fade(d);

    set_decor(win, d);

    connect_window(win);

    xid = win->get_xid();

    if (get_window_prop(xid, select_window_atom, &window)) {
        d->prop_xid = win->get_xid();
        update_switcher_window(win, window);
    } else if (get_window_prop(xid, frame_window_atom, &window)) {
        add_frame_window(win, window);
    }
}

void window_closed(Wnck::Window* win)
{
    Display* xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    decor_t* d = get_decor(win);
    Window window;

    gdk_error_trap_push();
    XDeleteProperty(xdisplay, win->get_xid(), win_decor_atom);
    XSync(xdisplay, false);
    gdk_error_trap_pop();

    if (!get_window_prop(win->get_xid(), select_window_atom, &window)) {
        remove_frame_window(win);
    }

    set_decor(win, nullptr);

    delete d;
}

void connect_screen(Wnck::Screen* screen)
{
    screen->signal_active_window_changed().connect(
                [=](Wnck::Window* w){ active_window_changed(w, screen); });
    screen->signal_window_opened().connect(&window_opened);
    screen->signal_window_closed().connect(&window_closed);

    for (auto* win : screen->get_windows()) {
        window_opened(win);
    }
}

void move_resize_window(Wnck::Window* win, int direction, XEvent* xevent)
{
    Display* xdisplay;
    GdkDisplay* gdkdisplay;
    GdkScreen* screen;
    Window xroot;
    XEvent ev;

    gdkdisplay = gdk_display_get_default();
    xdisplay = GDK_DISPLAY_XDISPLAY(gdkdisplay);
    screen = gdk_display_get_default_screen(gdkdisplay);
    xroot = RootWindowOfScreen(gdk_x11_screen_get_xscreen(screen));

    if (action_menu_mapped) {
        delete action_menu;
        action_menu_mapped = false;
        action_menu = nullptr;
        return;
    }

    ev.xclient.type = ClientMessage;
    ev.xclient.display = xdisplay;

    ev.xclient.serial = 0;
    ev.xclient.send_event = true;

    ev.xclient.window = win->get_xid();
    ev.xclient.message_type = wm_move_resize_atom;
    ev.xclient.format = 32;

    ev.xclient.data.l[0] = xevent->xbutton.x_root;
    ev.xclient.data.l[1] = xevent->xbutton.y_root;
    ev.xclient.data.l[2] = direction;
    ev.xclient.data.l[3] = xevent->xbutton.button;
    ev.xclient.data.l[4] = 1;

    XUngrabPointer(xdisplay, xevent->xbutton.time);
    XUngrabKeyboard(xdisplay, xevent->xbutton.time);

    XSendEvent(xdisplay, xroot, false,
               SubstructureRedirectMask | SubstructureNotifyMask, &ev);

    XSync(xdisplay, false);
}

void restack_window(Wnck::Window* win, int stack_mode)
{
    Display* xdisplay;
    GdkDisplay* gdkdisplay;
    GdkScreen* screen;
    Window xroot;
    XEvent ev;

    gdkdisplay = gdk_display_get_default();
    xdisplay = GDK_DISPLAY_XDISPLAY(gdkdisplay);
    screen = gdk_display_get_default_screen(gdkdisplay);
    xroot = RootWindowOfScreen(gdk_x11_screen_get_xscreen(screen));

    if (action_menu_mapped) {
        delete action_menu;
        action_menu_mapped = false;
        action_menu = nullptr;
        return;
    }

    ev.xclient.type = ClientMessage;
    ev.xclient.display = xdisplay;

    ev.xclient.serial = 0;
    ev.xclient.send_event = true;

    ev.xclient.window = win->get_xid();
    ev.xclient.message_type = restack_window_atom;
    ev.xclient.format = 32;

    ev.xclient.data.l[0] = 2;
    ev.xclient.data.l[1] = None;
    ev.xclient.data.l[2] = stack_mode;
    ev.xclient.data.l[3] = 0;
    ev.xclient.data.l[4] = 0;

    XSendEvent(xdisplay, xroot, false,
               SubstructureRedirectMask | SubstructureNotifyMask, &ev);

    XSync(xdisplay, false);
}

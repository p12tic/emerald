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
#include <titlebar.h>
#include <X11/Xatom.h>

GdkPixmap* create_pixmap(int w, int h)
{
    GdkPixmap* pixmap;
    GdkVisual* visual;
    GdkColormap* colormap;

    visual = gdk_visual_get_best_with_depth(32);
    if (!visual) {
        return NULL;
    }

    pixmap = gdk_pixmap_new(NULL, w, h, 32);
    if (!pixmap) {
        return NULL;
    }

    colormap = gdk_colormap_new(visual, false);
    if (!colormap) {
        g_object_unref(G_OBJECT(pixmap));
        return NULL;
    }

    gdk_drawable_set_colormap(GDK_DRAWABLE(pixmap), colormap);
    g_object_unref(G_OBJECT(colormap));

    return pixmap;
}

GdkPixmap* pixmap_new_from_pixbuf(GdkPixbuf* pixbuf)
{
    GdkPixmap* pixmap;
    unsigned width, height;
    cairo_t* cr;

    width = gdk_pixbuf_get_width(pixbuf);
    height = gdk_pixbuf_get_height(pixbuf);

    pixmap = create_pixmap(width, height);
    if (!pixmap) {
        return NULL;
    }

    cr = (cairo_t*) gdk_cairo_create(GDK_DRAWABLE(pixmap));
    gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_destroy(cr);

    return pixmap;
}

void position_title_object(char obj, WnckWindow* win, window_settings* ws,
                           int x, int s)
{
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");
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
                                (d->state & WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY)) ?
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

void layout_title_objects(WnckWindow* win)
{
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");
    window_settings* ws = d->fs->ws;
    int x0, y0;
    int width, height;
    unsigned i;
    int state = 0;
    int owidth;
    int x;
    bool removed = false;

    d->tobj_size[0] = 0;
    d->tobj_size[1] = 0;
    d->tobj_size[2] = 0;
    wnck_window_get_geometry(win, &x0, &y0, &width, &height);
    for (i = 0; i < strlen(ws->tobj_layout); i++) {
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
    d->tobj_pos[2] = width - d->tobj_size[2] + d->tobj_pos[0];
    d->tobj_pos[1] =
        MAX((d->tobj_pos[2] + d->tobj_size[0] - d->tobj_size[1]) / 2,
            0) + d->tobj_pos[0];
    x = d->tobj_pos[0] + ws->button_hoffset;
    for (i = 0; i < TBT_COUNT; i++) {
        d->tobj_item_state[i] = 3;
    }

    for (i = 0; i < strlen(ws->tobj_layout); i++) {
        if (ws->tobj_layout[i] == '(') {
            i++;
            x += g_ascii_strtoull(&ws->tobj_layout[i], NULL, 0);
            while (ws->tobj_layout[i] && g_ascii_isdigit(ws->tobj_layout[i])) {
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

void update_event_windows(WnckWindow* win)
{
    Display* xdisplay;
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");
    int x0, y0, width, height, x, y, w, h;
    int i, j, k, l;
    window_settings* ws = d->fs->ws;

    xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());

    wnck_window_get_geometry(win, &x0, &y0, &width, &height);

    if (d->state & WNCK_WINDOW_STATE_SHADED) {
        height = 0;
        k = l = 1;
    } else {
        k = 0;
        l = 2;
    }

    gdk_error_trap_push();

    for (i = 0; i < 3; i++) {
        static unsigned event_window_actions[3][3] = {
            {
                WNCK_WINDOW_ACTION_RESIZE,
                WNCK_WINDOW_ACTION_RESIZE,
                WNCK_WINDOW_ACTION_RESIZE
            }, {
                WNCK_WINDOW_ACTION_RESIZE,
                WNCK_WINDOW_ACTION_MOVE,
                WNCK_WINDOW_ACTION_RESIZE
            }, {
                WNCK_WINDOW_ACTION_RESIZE,
                WNCK_WINDOW_ACTION_RESIZE,
                WNCK_WINDOW_ACTION_RESIZE
            }
        };

        for (j = 0; j < 3; j++) {
            if (d->actions & event_window_actions[i][j] && i >= k && i <= l) {
                x = ws->pos[i][j].x + ws->pos[i][j].xw * width;
                y = ws->pos[i][j].y + ws->pos[i][j].yh * height;
                w = ws->pos[i][j].w + ws->pos[i][j].ww * width;
                h = ws->pos[i][j].h + ws->pos[i][j].hh * height;

                XMapWindow(xdisplay, d->event_windows[i][j]);
                XMoveResizeWindow(xdisplay, d->event_windows[i][j], x -
                                  ((ws->use_decoration_cropping &&
                                    (d->state & WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY)) ?
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

#if HAVE_WNCK_WINDOW_HAS_NAME
static const char* wnck_window_get_real_name(WnckWindow* win)
{
    return wnck_window_has_name(win) ? wnck_window_get_name(win) : NULL;
}
#define wnck_window_get_name(w) wnck_window_get_real_name(w)
#endif

int max_window_name_width(WnckWindow* win)
{
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");
    const char* name;
    int w;
    window_settings* ws = d->fs->ws;

    name = wnck_window_get_name(win);
    if (!name) {
        return 0;
    }

    if (!d->layout) {
        d->layout = pango_layout_new(ws->pango_context);
        if (!d->layout) {
            return 0;
        }

        pango_layout_set_wrap(d->layout, PANGO_WRAP_CHAR);
    }

    pango_layout_set_auto_dir(d->layout, false);
    pango_layout_set_width(d->layout, -1);
    pango_layout_set_text(d->layout, name, strlen(name));
    pango_layout_get_pixel_size(d->layout, &w, NULL);

    if (d->name) {
        pango_layout_set_text(d->layout, d->name, strlen(d->name));
    }

    return w + 6;
}

void update_window_decoration_name(WnckWindow* win)
{
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");
    const char* name;
    glong name_length;
    PangoLayoutLine* line;
    window_settings* ws = d->fs->ws;

    if (d->name) {
        g_free(d->name);
        d->name = NULL;
    }

    name = wnck_window_get_name(win);
    if (name && (name_length = strlen(name))) {
        int w;

        pango_layout_set_auto_dir(d->layout, false);
        pango_layout_set_text(d->layout, "", 0);
        pango_layout_set_width(d->layout, 0);
        layout_title_objects(win);
        w = d->width - ws->left_space - ws->right_space - d->tobj_size[0]
            - d->tobj_size[1] - d->tobj_size[2];
        if (w < 1) {
            w = 1;
        }

        pango_layout_set_width(d->layout, w * PANGO_SCALE);
        pango_layout_set_text(d->layout, name, name_length);

        line = pango_layout_get_line(d->layout, 0);

        name_length = line->length;
        if (pango_layout_get_line_count(d->layout) > 1) {
            if (name_length < 4) {
                g_object_unref(G_OBJECT(d->layout));
                d->layout = NULL;
                return;
            }

            d->name = g_strndup(name, name_length);
            strcpy(d->name + name_length - 3, "...");
        } else {
            d->name = g_strndup(name, name_length);
        }

        pango_layout_set_text(d->layout, d->name, name_length);
        layout_title_objects(win);
    } else if (d->layout) {
        g_object_unref(G_OBJECT(d->layout));
        d->layout = NULL;
    }
}

void update_window_decoration_icon(WnckWindow* win)
{
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");

    if (d->icon) {
        cairo_pattern_destroy(d->icon);
        d->icon = NULL;
    }

    if (d->icon_pixmap) {
        g_object_unref(G_OBJECT(d->icon_pixmap));
        d->icon_pixmap = NULL;
    }
    if (d->icon_pixbuf) {
        g_object_unref(G_OBJECT(d->icon_pixbuf));
    }

    d->icon_pixbuf = wnck_window_get_mini_icon(win);
    if (d->icon_pixbuf) {
        cairo_t* cr;

        g_object_ref(G_OBJECT(d->icon_pixbuf));

        d->icon_pixmap = pixmap_new_from_pixbuf(d->icon_pixbuf);
        cr = gdk_cairo_create(GDK_DRAWABLE(d->icon_pixmap));
        d->icon = cairo_pattern_create_for_surface(cairo_get_target(cr));
        cairo_destroy(cr);
    }
}

void update_window_decoration_state(WnckWindow* win)
{
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");

    d->state = wnck_window_get_state(win);
}

void update_window_decoration_actions(WnckWindow* win)
{
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");

    /* code to check for context help protocol */
    Atom actual;
    int result, format;
    unsigned long n, left;
    unsigned long offset;
    unsigned char* data;
    Window id = wnck_window_get_xid(win);
    Display* xdisplay;
    GdkDisplay* gdkdisplay;

    //GdkScreen   *screen;
    //Window      xroot;
    //XEvent      ev;

    gdkdisplay = gdk_display_get_default();
    xdisplay = GDK_DISPLAY_XDISPLAY(gdkdisplay);
    //screen     = gdk_display_get_default_screen (gdkdisplay);
    //xroot      = RootWindowOfScreen (gdk_x11_screen_get_xscreen (screen));

    d->actions = wnck_window_get_actions(win);
    data = NULL;
    left = 1;
    offset = 0;
    while (left) {
        result = XGetWindowProperty(xdisplay, id, wm_protocols_atom,
                                    offset, 1L, false, XA_ATOM, &actual,
                                    &format, &n, &left, &data);
        offset++;
        if (result == Success && n && data) {
            Atom a;

            memcpy(&a, data, sizeof(Atom));
            XFree((void*)data);
            if (a == net_wm_context_help_atom) {
                d->actions |= FAKE_WINDOW_ACTION_HELP;
            }
            data = NULL;
        } else if (result != Success) {
            /* Closes #161 */
            fprintf(stderr,
                    "XGetWindowProperty() returned non-success value (%d) for window '%s'.\n",
                    result, wnck_window_get_name(win));
            break;
        }
    }
}

bool update_window_decoration_size(WnckWindow* win)
{
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");
    GdkPixmap* pixmap, *buffer_pixmap = NULL;
    GdkPixmap* ipixmap, *ibuffer_pixmap = NULL;
    int width, height;
    int w;
    int h;

    window_settings* ws = d->fs->ws;

    max_window_name_width(win);
    layout_title_objects(win);

    wnck_window_get_geometry(win, NULL, NULL, &w, &h);

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
        g_object_unref(G_OBJECT(pixmap));
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

    if (d->p_active) {
        g_object_unref(G_OBJECT(d->p_active));
    }

    if (d->p_active_buffer) {
        g_object_unref(G_OBJECT(d->p_active_buffer));
    }

    if (d->p_inactive) {
        g_object_unref(G_OBJECT(d->p_inactive));
    }

    if (d->p_inactive_buffer) {
        g_object_unref(G_OBJECT(d->p_inactive_buffer));
    }

    if (d->gc) {
        g_object_unref(G_OBJECT(d->gc));
    }

    d->only_change_active = false;

    d->pixmap = d->active ? pixmap : ipixmap;
    d->buffer_pixmap = d->active ? buffer_pixmap : ibuffer_pixmap;
    d->p_active = pixmap;
    d->p_active_buffer = buffer_pixmap;
    d->p_inactive = ipixmap;
    d->p_inactive_buffer = ibuffer_pixmap;
    d->gc = gdk_gc_new(pixmap);

    d->width = width;
    d->height = height;

    d->prop_xid = wnck_window_get_xid(win);

    update_window_decoration_name(win);

    update_button_regions(d);
    stop_button_fade(d);

    queue_decor_draw(d);

    return true;
}

void add_frame_window(WnckWindow* win, Window frame)
{
    Display* xdisplay;
    XSetWindowAttributes attr;
    gulong xid = wnck_window_get_xid(win);
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");
    int i, j;

    xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());

    bzero(&attr, sizeof(XSetWindowAttributes));
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
                g_hash_table_insert(frame_table,
                                    GINT_TO_POINTER(d->event_windows[i][j]),
                                    GINT_TO_POINTER(xid));

        for (i = 0; i < B_T_COUNT; i++)
            g_hash_table_insert(frame_table,
                                GINT_TO_POINTER(d->button_windows[i]),
                                GINT_TO_POINTER(xid));


        update_window_decoration_state(win);
        update_window_decoration_actions(win);
        update_window_decoration_icon(win);
        update_window_decoration_size(win);

        update_event_windows(win);
    } else {
        memset(d->event_windows, 0, sizeof(d->event_windows));
    }
}

bool update_switcher_window(WnckWindow* win, Window selected)
{
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");
    GdkPixmap* pixmap, *buffer_pixmap = NULL;
    int height, width = 0;
    WnckWindow* selected_win;
    window_settings* ws = d->fs->ws;

    wnck_window_get_geometry(win, NULL, NULL, &width, NULL);

    width += ws->left_space + ws->right_space;
    height = ws->top_space + SWITCHER_TOP_EXTRA +
             ws->switcher_top_corner_space + SWITCHER_SPACE +
             ws->switcher_bottom_corner_space + ws->bottom_space;

    d->decorated = false;
    d->draw = draw_switcher_decoration;

    if (!IS_VALID(d->pixmap) && IS_VALID(ws->switcher_pixmap)) {
        g_object_ref(G_OBJECT(ws->switcher_pixmap));
        d->pixmap = ws->switcher_pixmap;
    }

    if (!IS_VALID(d->buffer_pixmap) && IS_VALID(ws->switcher_buffer_pixmap)) {
        g_object_ref(G_OBJECT(ws->switcher_buffer_pixmap));
        d->buffer_pixmap = ws->switcher_buffer_pixmap;
    }

    if (!d->width) {
        d->width = ws->switcher_width;
    }

    if (!d->height) {
        d->height = ws->switcher_height;
    }

    selected_win = wnck_window_get(selected);
    if (selected_win) {
        glong name_length;
        PangoLayoutLine* line;
        const char* name;

        if (d->name) {
            g_free(d->name);
            d->name = NULL;
        }

        name = wnck_window_get_name(selected_win);
        if (name && (name_length = strlen(name))) {
            if (!d->layout) {
                d->layout = pango_layout_new(ws->pango_context);
                if (d->layout) {
                    pango_layout_set_wrap(d->layout, PANGO_WRAP_CHAR);
                }
            }

            if (d->layout) {
                pango_layout_set_auto_dir(d->layout, false);
                pango_layout_set_width(d->layout, -1);
                pango_layout_set_text(d->layout, name, name_length);

                line = pango_layout_get_line(d->layout, 0);

                name_length = line->length;
                if (pango_layout_get_line_count(d->layout) > 1) {
                    if (name_length < 4) {
                        g_object_unref(G_OBJECT(d->layout));
                        d->layout = NULL;
                    } else {
                        d->name = g_strndup(name, name_length);
                        strcpy(d->name + name_length - 3, "...");
                    }
                } else {
                    d->name = g_strndup(name, name_length);
                }

                if (d->layout) {
                    pango_layout_set_text(d->layout, d->name, name_length);
                }
            }
        } else if (d->layout) {
            g_object_unref(G_OBJECT(d->layout));
            d->layout = NULL;
        }
    }

    if (width == d->width && height == d->height) {
        if (!d->gc) {
            if (d->pixmap->parent_instance.ref_count) {
                d->gc = gdk_gc_new(d->pixmap);
            } else {
                d->pixmap = NULL;
            }
        }

        if (d->pixmap) {
            queue_decor_draw(d);
            return false;
        }
    }

    pixmap = create_pixmap(width, height);
    if (!pixmap) {
        return false;
    }

    buffer_pixmap = create_pixmap(width, height);
    if (!buffer_pixmap) {
        g_object_unref(G_OBJECT(pixmap));
        return false;
    }

    if (ws->switcher_pixmap) {
        g_object_unref(G_OBJECT(ws->switcher_pixmap));
    }

    if (ws->switcher_buffer_pixmap) {
        g_object_unref(G_OBJECT(ws->switcher_buffer_pixmap));
    }

    if (d->pixmap) {
        g_object_unref(G_OBJECT(d->pixmap));
    }

    if (d->buffer_pixmap) {
        g_object_unref(G_OBJECT(d->buffer_pixmap));
    }

    if (d->gc) {
        g_object_unref(G_OBJECT(d->gc));
    }

    ws->switcher_pixmap = pixmap;
    ws->switcher_buffer_pixmap = buffer_pixmap;

    ws->switcher_width = width;
    ws->switcher_height = height;

    g_object_ref(G_OBJECT(pixmap));
    g_object_ref(G_OBJECT(buffer_pixmap));

    d->pixmap = pixmap;
    d->buffer_pixmap = buffer_pixmap;
    d->gc = gdk_gc_new(pixmap);

    d->width = width;
    d->height = height;

    d->prop_xid = wnck_window_get_xid(win);

    reset_buttons_bg_and_fade(d);

    queue_decor_draw(d);

    return true;
}

void remove_frame_window(WnckWindow* win)
{
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");

    if (d->p_active) {
        g_object_unref(G_OBJECT(d->p_active));
        d->p_active = NULL;
    }

    if (d->p_active_buffer) {
        g_object_unref(G_OBJECT(d->p_active_buffer));
        d->p_active_buffer = NULL;
    }

    if (d->p_inactive) {
        g_object_unref(G_OBJECT(d->p_inactive));
        d->p_inactive = NULL;
    }

    if (d->p_inactive_buffer) {
        g_object_unref(G_OBJECT(d->p_inactive_buffer));
        d->p_inactive_buffer = NULL;
    }

    if (d->gc) {
        g_object_unref(G_OBJECT(d->gc));
        d->gc = NULL;
    }

    int b_t;

    for (b_t = 0; b_t < B_T_COUNT; b_t++) {
        if (d->button_region[b_t].bg_pixmap) {
            g_object_unref(G_OBJECT(d->button_region[b_t].bg_pixmap));
            d->button_region[b_t].bg_pixmap = NULL;
        }
        if (d->button_region_inact[b_t].bg_pixmap) {
            g_object_unref(G_OBJECT(d->button_region_inact[b_t].bg_pixmap));
            d->button_region_inact[b_t].bg_pixmap = NULL;
        }
    }
    if (d->button_fade_info.cr) {
        cairo_destroy(d->button_fade_info.cr);
        d->button_fade_info.cr = NULL;
    }

    if (d->button_fade_info.timer >= 0) {
        g_source_remove(d->button_fade_info.timer);
        d->button_fade_info.timer = -1;
    }

    if (d->name) {
        g_free(d->name);
        d->name = NULL;
    }

    if (d->layout) {
        g_object_unref(G_OBJECT(d->layout));
        d->layout = NULL;
    }

    if (d->icon) {
        cairo_pattern_destroy(d->icon);
        d->icon = NULL;
    }

    if (d->icon_pixmap) {
        g_object_unref(G_OBJECT(d->icon_pixmap));
        d->icon_pixmap = NULL;
    }

    if (d->icon_pixbuf) {
        g_object_unref(G_OBJECT(d->icon_pixbuf));
        d->icon_pixbuf = NULL;
    }


    if (d->force_quit_dialog) {
        GtkWidget* dialog = d->force_quit_dialog;

        d->force_quit_dialog = NULL;
        gtk_widget_destroy(dialog);
    }

    d->width = 0;
    d->height = 0;

    d->decorated = false;

    d->state = 0;
    d->actions = 0;

    auto it = std::find(g_draw_list.begin(), g_draw_list.end(), d);
    if (it != g_draw_list.end()) {
        g_draw_list.erase(it);
    }
}

void window_name_changed(WnckWindow* win)
{
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");

    if (d->decorated && !update_window_decoration_size(win)) {
        update_button_regions(d);
        queue_decor_draw(d);
    }
}

void window_geometry_changed(WnckWindow* win)
{
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");

    if (d->decorated) {
        int width, height;

        wnck_window_get_geometry(win, NULL, NULL, &width, &height);
        if ((width != d->client_width) || (height != d->client_height)) {
            d->client_width = width;
            d->client_height = height;

            update_window_decoration_size(win);
            update_event_windows(win);
        }
    }
}

void window_icon_changed(WnckWindow* win)
{
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");

    if (d->decorated) {
        update_window_decoration_icon(win);
        update_button_regions(d);
        queue_decor_draw(d);
    }
}

void window_state_changed(WnckWindow* win)
{
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");

    if (d->decorated) {
        update_window_decoration_state(win);
        update_button_regions(d);
        stop_button_fade(d);
        update_window_decoration_size(win);
        update_event_windows(win);

        d->prop_xid = wnck_window_get_xid(win);
        queue_decor_draw(d);
    }
}

void window_actions_changed(WnckWindow* win)
{
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");

    if (d->decorated) {
        update_window_decoration_actions(win);
        update_window_decoration_size(win);
        update_event_windows(win);

        d->prop_xid = wnck_window_get_xid(win);
        queue_decor_draw(d);
    }
}

void connect_window(WnckWindow* win)
{
    g_signal_connect_object(win, "name_changed",
                            G_CALLBACK(window_name_changed), 0, 0);
    g_signal_connect_object(win, "geometry_changed",
                            G_CALLBACK(window_geometry_changed), 0, 0);
    g_signal_connect_object(win, "icon_changed",
                            G_CALLBACK(window_icon_changed), 0, 0);
    g_signal_connect_object(win, "state_changed",
                            G_CALLBACK(window_state_changed), 0, 0);
    g_signal_connect_object(win, "actions_changed",
                            G_CALLBACK(window_actions_changed), 0, 0);
}

void active_window_changed(WnckScreen* screen)
{
    WnckWindow* win;
    decor_t* d;

    win = wnck_screen_get_previously_active_window(screen);
    if (win) {
        d = g_object_get_data(G_OBJECT(win), "decor");
        if (d && d->pixmap && d->decorated) {
            d->active = wnck_window_is_active(win);
            d->fs = (d->active ? d->fs->ws->fs_act : d->fs->ws->fs_inact);
            if (std::find(g_draw_list.begin(), g_draw_list.end(), d) ==
                    g_draw_list.end()) {
                d->only_change_active = true;
            }
            d->prop_xid = wnck_window_get_xid(win);
            stop_button_fade(d);
            queue_decor_draw_for_buttons(d, true);
        }
    }

    win = wnck_screen_get_active_window(screen);
    if (win) {
        d = g_object_get_data(G_OBJECT(win), "decor");
        if (d && d->pixmap && d->decorated) {
            d->active = wnck_window_is_active(win);
            d->fs = (d->active ? d->fs->ws->fs_act : d->fs->ws->fs_inact);
            if (std::find(g_draw_list.begin(), g_draw_list.end(), d) ==
                    g_draw_list.end()) {
                d->only_change_active = true;
            }
            d->prop_xid = wnck_window_get_xid(win);
            stop_button_fade(d);
            queue_decor_draw_for_buttons(d, true);
        }
    }
}

void window_opened(WnckScreen* screen, WnckWindow* win)
{
    decor_t* d;
    Window window;
    gulong xid;

    d = g_malloc(sizeof(decor_t));
    if (!d) {
        return;
    }
    bzero(d, sizeof(decor_t));

    wnck_window_get_geometry(win, NULL, NULL, &d->client_width, &d->client_height);
    d->active = wnck_window_is_active(win);

    d->draw = draw_window_decoration;
    d->fs = d->active ? global_ws->fs_act : global_ws->fs_inact;

    reset_buttons_bg_and_fade(d);

    g_object_set_data(G_OBJECT(win), "decor", d);

    connect_window(win);

    xid = wnck_window_get_xid(win);

    if (get_window_prop(xid, select_window_atom, &window)) {
        d->prop_xid = wnck_window_get_xid(win);
        update_switcher_window(win, window);
    } else if (get_window_prop(xid, frame_window_atom, &window)) {
        add_frame_window(win, window);
    }
}

void window_closed(WnckScreen* screen, WnckWindow* win)
{
    Display* xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    decor_t* d = g_object_get_data(G_OBJECT(win), "decor");
    Window window;

    gdk_error_trap_push();
    XDeleteProperty(xdisplay, wnck_window_get_xid(win), win_decor_atom);
    XSync(xdisplay, false);
    gdk_error_trap_pop();

    if (!get_window_prop(wnck_window_get_xid(win), select_window_atom, &window)) {
        remove_frame_window(win);
    }

    g_object_set_data(G_OBJECT(win), "decor", NULL);

    g_free(d);
}

void connect_screen(WnckScreen* screen)
{
    GList* windows;

    g_signal_connect_object(G_OBJECT(screen), "active_window_changed",
                            G_CALLBACK(active_window_changed), 0, 0);
    g_signal_connect_object(G_OBJECT(screen), "window_opened",
                            G_CALLBACK(window_opened), 0, 0);
    g_signal_connect_object(G_OBJECT(screen), "window_closed",
                            G_CALLBACK(window_closed), 0, 0);

    windows = wnck_screen_get_windows(screen);
    while (windows != NULL) {
        window_opened(screen, windows->data);
        windows = windows->next;
    }
}

void move_resize_window(WnckWindow* win, int direction, XEvent* xevent)
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
        gtk_object_destroy(GTK_OBJECT(action_menu));
        action_menu_mapped = false;
        action_menu = NULL;
        return;
    }

    ev.xclient.type = ClientMessage;
    ev.xclient.display = xdisplay;

    ev.xclient.serial = 0;
    ev.xclient.send_event = true;

    ev.xclient.window = wnck_window_get_xid(win);
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

void restack_window(WnckWindow* win, int stack_mode)
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
        gtk_object_destroy(GTK_OBJECT(action_menu));
        action_menu_mapped = false;
        action_menu = NULL;
        return;
    }

    ev.xclient.type = ClientMessage;
    ev.xclient.display = xdisplay;

    ev.xclient.serial = 0;
    ev.xclient.send_event = true;

    ev.xclient.window = wnck_window_get_xid(win);
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

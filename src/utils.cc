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

#include "utils.h"
#include <X11/Xatom.h>

int get_b_offset(int b)
{
    static int boffset[B_COUNT + 1];
    int i, b_t = 0;

    for (i = 0; i < B_COUNT; i++) {
        boffset[i] = b_t;
        if (btbistate[b_t]) {
            boffset[i + 1] = b_t;
            i++;
        }
        b_t++;
    }
    return boffset[b];
}

int get_b_t_offset(int b_t)
{
    static int btoffset[B_T_COUNT];
    int i, b = 0;

    for (i = 0; i < B_T_COUNT; i++) {
        btoffset[i] = b;
        b++;
        if (btbistate[i]) {
            b++;
        }
    }
    return btoffset[b_t];
}

int get_real_pos(window_settings* ws, int tobj, decor_t* d)
{
    switch (d->tobj_item_state[tobj]) {
    case 1:
        return ((d->width + ws->left_space - ws->right_space +
                 d->tobj_size[0] - d->tobj_size[1] - d->tobj_size[2]) / 2 +
                d->tobj_item_pos[tobj]);
    case 2:
        return (d->width - ws->right_space - d->tobj_size[2] +
                d->tobj_item_pos[tobj]);
    case 3:
        return -1;
    default:
        return (ws->left_space + d->tobj_item_pos[tobj]);
    }
}

int get_title_object_type(char obj)
{
    switch (obj) {
    case ':':                                        // state separator
        return -1;
    case 'C':                                        // close
        return TBT_CLOSE;
    case 'N':                                        // miNimize
        return TBT_MINIMIZE;
    case 'X':                                        // maXimize/Restore
    case 'R':                                        // ""
        return TBT_MAXIMIZE;
    case 'H':                                        // Help
        return TBT_HELP;
    case 'M':                                        // not implemented menu
        return TBT_MENU;
    case 'T':                                        // Text
        return TBT_TITLE;
    case 'I':                                        // Icon
        return TBT_ICON;
    case 'S':                                        // Shade
        return TBT_SHADE;
    case 'U':                                        // on-top(Up)
    case 'A':                                        // (Above)
        return TBT_ONTOP;
    case 'D':                                        // not implemented on-bottom(Down)
        return TBT_ONBOTTOM;
    case 'Y':
        return TBT_STICKY;
    default:
        return -2;
    }
    return -2;
}

int get_title_object_width(char obj, window_settings* ws, decor_t* d)
{
    int i = get_title_object_type(obj);

    switch (i) {
    case -1:                                        // state separator
        return -1;
    case TBT_TITLE:
        if (d->layout) {
            int w, h;
            d->layout->get_pixel_size(w, h);
            return w + 2;
        } else {
            return 2;
        }
    case TBT_ICON:
        return 18;
    default:
        if (i < B_T_COUNT)
            return (d->actions & button_actions[i]) ?
                   (ws->use_pixmap_buttons ? ws->c_icon_size[i].w : 18) : 0;
        else {
            return 0;
        }
    }
}

unsigned int get_mwm_prop(Window xwindow)
{
    Display* xdisplay;
    Atom actual;
    int err, result, format;
    unsigned long n, left;
    unsigned char* hints_ret;
    MwmHints* mwm_hints;
    unsigned int decor = MWM_DECOR_ALL;

    xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());

    gdk_error_trap_push();

    result = XGetWindowProperty(xdisplay, xwindow, mwm_hints_atom,
                                0L, 20L, false, mwm_hints_atom,
                                &actual, &format, &n, &left, &hints_ret);
    mwm_hints = (MwmHints*) hints_ret;

    err = gdk_error_trap_pop();
    if (err != Success || result != Success) {
        return decor;
    }

    if (n && mwm_hints) {
        if (n >= PROP_MOTIF_WM_HINT_ELEMENTS &&
                mwm_hints->flags & MWM_HINTS_DECORATIONS) {
            decor = mwm_hints->decorations;
        }

        XFree(mwm_hints);
    }

    return decor;
}

bool get_window_prop(Window xwindow, Atom atom, Window* val)
{
    Atom type;
    int format;
    unsigned long nitems;
    unsigned long bytes_after;
    Window* w;
    int err, result;

    *val = 0;

    gdk_error_trap_push();

    type = None;
    result = XGetWindowProperty(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()),
                                xwindow,
                                atom,
                                0, G_MAXLONG,
                                False, XA_WINDOW, &type, &format, &nitems,
                                &bytes_after, reinterpret_cast<unsigned char**>(&w));
    err = gdk_error_trap_pop();
    if (err != Success || result != Success) {
        return false;
    }

    if (type != XA_WINDOW) {
        XFree(w);
        return false;
    }

    *val = *w;
    XFree(w);

    return true;
}


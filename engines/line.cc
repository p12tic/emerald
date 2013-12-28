/*
 * Line theme engine
 *
 * line.c
 *
 * Copyright (C) 2006 Quinn Storm <livinglatexkali@gmail.com>
 * Copyright (C) 2007 Patrick Niklaus <patrick.niklaus@googlemail.com>
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

#include <libengine/emerald.h>
#include <libengine/engine.h>

#include <legacy_icon.h>

#define SECT "line_settings"

#define SHADOW_FIX

/*
 * color privates
 */
struct private_fs {
    alpha_color border;
    alpha_color title_bar;
};

/*
 * settings privates
 */
struct private_ws {
};

void get_meta_info(EngineMetaInfo* emi)
{
    emi->version = g_strdup("0.1");
    emi->description = g_strdup(_("Based on original legacy"));
    emi->last_compat = g_strdup("0.0"); // old themes still compatible
    emi->icon = Gdk::Pixbuf::create_from_inline(sizeof(my_pixbuf), my_pixbuf, true);
}

#ifdef SHADOW_FIX
static void draw_shadow_background(decor_t* d, cairo_t* cr)
{
    cairo_matrix_t matrix;
    double w, x2;
    int width, height;
    int left, right, top, bottom;
    window_settings* ws = d->fs->ws;

    if (!ws->large_shadow_pixmap) {
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
        cairo_paint(cr);

        return;
    }

    gdk_drawable_get_size(ws->large_shadow_pixmap, &width, &height);

    left = ws->left_space + ws->left_corner_space;
    right = ws->right_space + ws->right_corner_space;
    top = ws->top_space + ws->top_corner_space;
    bottom = ws->bottom_space + ws->bottom_corner_space;

    if (d->width - left - right < 0) {
        left = d->width / 2;
        right = d->width - left;
    }

    if (d->height - top - bottom < 0) {
        top = d->height / 2;
        bottom = d->height - top;
    }

    w = d->width - left - right;

    x2 = d->width - right;

    /* top left */
    cairo_matrix_init_identity(&matrix);
    cairo_pattern_set_matrix(ws->shadow_pattern, &matrix);
    cairo_set_source(cr, ws->shadow_pattern);
    cairo_rectangle(cr, 0.0, 0.0, left, top);
    cairo_fill(cr);

    /* top */
    if (w > 0) {
        cairo_matrix_init_translate(&matrix, left, 0.0);
        cairo_matrix_scale(&matrix, 1.0 / w, 1.0);
        cairo_matrix_translate(&matrix, -left, 0.0);
        cairo_pattern_set_matrix(ws->shadow_pattern, &matrix);
        cairo_set_source(cr, ws->shadow_pattern);
        cairo_rectangle(cr, left, 0.0, w, top);
        cairo_fill(cr);
    }


    /* top right */
    cairo_matrix_init_translate(&matrix, width - right - x2, 0.0);
    cairo_pattern_set_matrix(ws->shadow_pattern, &matrix);
    cairo_set_source(cr, ws->shadow_pattern);
    cairo_rectangle(cr, x2, 0.0, right, top);
    cairo_clip_preserve(cr);
    cairo_fill(cr);
}
#endif

extern "C"
void engine_draw_frame(decor_t* d, cairo_t* cr)
{
    frame_settings* fs = d->fs;
    private_fs* pfs = fs->engine_fs;
    window_settings* ws = fs->ws;

    double x1, y1, x2, y2;

    x1 = ws->left_space - ws->win_extents.left;
    y1 = ws->top_space - ws->win_extents.top;
    x2 = d->width  - ws->right_space  + ws->win_extents.right;
    y2 = d->height - ws->bottom_space + ws->win_extents.bottom;
    int top;
    top = ws->win_extents.top + ws->titlebar_height;

    double m1 = MIN(ws->win_extents.left, ws->win_extents.right);
    double m2 = MIN(ws->win_extents.top, ws->win_extents.bottom);

    double border_width = MIN(m1, m2);
    double border_offset = border_width / 2.0;

    cairo_set_line_width(cr, border_width);

    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

    rounded_rectangle(cr,
                      x1 + border_offset,
                      y1 + top - border_offset,
                      x2 - x1 - border_width,
                      y2 - y1 - top,
                      0, ws, 0);
    cairo_set_source_alpha_color(cr, &pfs->border);
    cairo_stroke(cr);

    // title bar
    if (pfs->title_bar.alpha != 0.0) {
        rounded_rectangle(cr,
                          x1,
                          y1,
                          x2 - x1,
                          top,
                          0, ws, 0);
        cairo_set_source_alpha_color(cr, &pfs->title_bar);
        cairo_fill(cr);
    } else {
        cairo_save(cr);
        cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
        cairo_rectangle(cr, 0.0, 0.0, d->width, top + y1 - border_width);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
        cairo_fill(cr);
        cairo_restore(cr);

// FIXME => find a proper solution for this
#ifdef SHADOW_FIX
        cairo_rectangle(cr, 0.0, 0.0, d->width, top + y1 - border_width);
        cairo_clip(cr);
        cairo_translate(cr, 0.0, ws->top_space + ws->win_extents.top);
        draw_shadow_background(d, cr);
        cairo_translate(cr, 0.0, -ws->top_space - ws->win_extents.top);
#endif
    }
}

extern "C"
void load_engine_settings(const KeyFile& f, window_settings* ws)
{
    PFACS(f, ws, border, SECT);
    PFACS(f, ws, title_bar, SECT);
}

extern "C"
void init_engine(window_settings* ws)
{
    private_fs* pfs;
    private_ws* pws;

    pws = new private_ws;
    ws->engine_ws = pws;

    pfs = new private_fs;
    ws->fs_act->engine_fs = pfs;
    ACOLOR(pfs, border, 0.0, 0.0, 0.0, 1.0);
    ACOLOR(pfs, title_bar, 0.0, 0.0, 0.0, 0.3);

    pfs = new private_fs;
    ws->fs_inact->engine_fs = pfs;
    ACOLOR(pfs, border, 0.0, 0.0, 0.0, 1.0);
    ACOLOR(pfs, title_bar, 0.0, 0.0, 0.0, 0.0);
}

extern "C"
void fini_engine(window_settings* ws)
{
    delete ((private_fs*)ws->fs_act->engine_fs);
    delete ((private_fs*)ws->fs_inact->engine_fs);
}

void my_engine_settings(Gtk::Box& hbox, bool active)
{
    auto& vbox = *Gtk::manage(new Gtk::VBox(false, 2));
    hbox.pack_start(vbox, true, true);
    vbox.pack_start(*Gtk::manage(new Gtk::Label(active ? "Active Window" : "Inactive Window")),
                    false, false);
    vbox.pack_start(*Gtk::manage(new Gtk::HSeparator()), false, false);
    auto& scroller = *Gtk::manage(new Gtk::ScrolledWindow());
    scroller.set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    vbox.pack_start(scroller, true, true);

    table_new(3, false, false);

    scroller.add(get_current_table());

    make_labels(_("Colors"));
    table_append_separator();
    add_color_alpha_value(_("Outer Frame Blend"), "border", SECT, active);
    add_color_alpha_value(_("Title Bar"), "title_bar", SECT, active);
}

void layout_engine_colors(Gtk::Box& vbox)
{
    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, true, true);
    my_engine_settings(hbox, true);
    hbox.pack_start(*Gtk::manage(new Gtk::VSeparator()), false, false);
    my_engine_settings(hbox, false);
}

extern "C"
void layout_engine_settings(Gtk::Box& vbox)
{
    auto* note = Gtk::manage(new Gtk::Notebook());
    vbox.pack_start(*note, true, true);
    layout_engine_colors(*build_notebook_page(_("Colors"), *note));
}

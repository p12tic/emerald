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
    emi->version = "0.1";
    emi->description = _("Based on original legacy");
    emi->last_compat = "0.0"; // old themes still compatible
    emi->icon = Gdk::Pixbuf::create_from_inline(sizeof(my_pixbuf), my_pixbuf, true);
}

#ifdef SHADOW_FIX
void draw_shadow_background(decor_t* d, Cairo::RefPtr<Cairo::Context>& cr)
{
    double w, x2;
    int width, height;
    int left, right, top, bottom;
    window_settings* ws = d->fs->ws;

    if (!ws->large_shadow_pixmap) {
        cr->set_source_rgba(0.0, 0.0, 0.0, 0.0);
        cr->paint();

        return;
    }

    ws->large_shadow_pixmap->get_size(width, height);

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

    ws->shadow_pattern->set_matrix(Cairo::identity_matrix());
    cr->set_source(ws->shadow_pattern);
    cr->rectangle(0.0, 0.0, left, top);
    cr->fill();

    /* top */
    if (w > 0) {
        auto mat = Cairo::translation_matrix(left, 0.0);
        mat.scale(1.0 / w, 1.0);
        mat.translate(-left, 0.0);
        ws->shadow_pattern->set_matrix(mat);
        cr->set_source(ws->shadow_pattern);
        cr->rectangle(left, 0.0, w, top);
        cr->fill();
    }


    /* top right */
    ws->shadow_pattern->set_matrix(
                Cairo::translation_matrix(width - right - x2, 0.0));
    cr->set_source(ws->shadow_pattern);
    cr->rectangle(x2, 0.0, right, top);
    cr->clip_preserve();
    cr->fill();
}
#endif

extern "C"
void engine_draw_frame(decor_t* d, Cairo::RefPtr<Cairo::Context>& cr)
{
    frame_settings* fs = d->fs;
    private_fs* pfs = reinterpret_cast<private_fs*>(fs->engine_fs);
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

    cr->set_line_width(border_width);

    cr->set_operator(Cairo::OPERATOR_SOURCE);

    rounded_rectangle(cr,
                      x1 + border_offset,
                      y1 + top - border_offset,
                      x2 - x1 - border_width,
                      y2 - y1 - top,
                      0, ws, 0);
    cairo_set_source_alpha_color(cr, &pfs->border);
    cr->stroke();

    // title bar
    if (pfs->title_bar.alpha != 0.0) {
        rounded_rectangle(cr,
                          x1,
                          y1,
                          x2 - x1,
                          top,
                          0, ws, 0);
        cairo_set_source_alpha_color(cr, &pfs->title_bar);
        cr->fill();
    } else {
        cr->save();
        cr->set_operator(Cairo::OPERATOR_CLEAR);
        cr->rectangle(0.0, 0.0, d->width, top + y1 - border_width);
        cr->set_source_rgba(1.0, 1.0, 1.0, 1.0);
        cr->fill();
        cr->restore();

// FIXME => find a proper solution for this
#ifdef SHADOW_FIX
        cr->rectangle(0.0, 0.0, d->width, top + y1 - border_width);
        cr->clip();
        cr->translate(0.0, ws->top_space + ws->win_extents.top);
        draw_shadow_background(d, cr);
        cr->translate(0.0, -ws->top_space - ws->win_extents.top);
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
    pfs->border = alpha_color(0.0, 0.0, 0.0, 1.0);
    pfs->title_bar = alpha_color(0.0, 0.0, 0.0, 0.3);

    pfs = new private_fs;
    ws->fs_inact->engine_fs = pfs;
    pfs->border = alpha_color(0.0, 0.0, 0.0, 1.0);
    pfs->title_bar = alpha_color(0.0, 0.0, 0.0, 0.0);
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

    SettingsTable tbl(3, false, false);

    scroller.add(tbl.table());

    tbl.append_header(_("Colors"));
    tbl.append_separator();
    tbl.append_acolor(_("Outer Frame Blend"), "border", SECT, active);
    tbl.append_acolor(_("Title Bar"), "title_bar", SECT, active);
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

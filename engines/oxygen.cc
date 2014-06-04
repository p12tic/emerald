/*
 * oxygen theme engine
 *
 * oxygen.c
 *
 * Copyright (C) 2006 Quinn Storm <livinglatexkali@gmail.com> (original legacy theme engine)
 * Copyright (C) 2006 Alain <big_al326@hotmail.com>
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
 */

//legacy engine
#include <libengine/emerald.h>
#include <libengine/engine.h>
#include <oxygen_icon.h>

#define SECT "oxygen_settings"

struct private_fs {
    alpha_color sides;
    alpha_color base;
    alpha_color glow;
    alpha_color window_halo;
    alpha_color window_highlight;
    alpha_color window_shadow;
    alpha_color separator_line;
    alpha_color contents_highlight;
    alpha_color contents_shadow;
    alpha_color contents_halo;
};

struct private_ws {
    bool round_top_left;
    bool round_top_right;
    bool round_bottom_left;
    bool round_bottom_right;
    double  corner_radius;
};

extern "C"
void get_meta_info(EngineMetaInfo* emi)
{
    emi->version = "0.1";
    emi->description = _("Designed to be vista-ish in nature");
    emi->last_compat = "0.0"; // old themes marked still compatible for testing-NeOS
    emi->icon = Gdk::Pixbuf::create_from_inline(sizeof(my_pixbuf), my_pixbuf, true);
}

extern "C"
void engine_draw_frame(decor_t* d, Cairo::RefPtr<Cairo::Context>& cr)
{
    double x1, y1, x2, y2, h;
    int top;
    frame_settings* fs = d->fs;
    private_fs* pfs = reinterpret_cast<private_fs*>(fs->engine_fs);
    window_settings* ws = fs->ws;
    private_ws* pws = reinterpret_cast<private_ws*>(ws->engine_ws);
    double pleft;
    double ptop;
    double pwidth;
    double pheight;
    top = ws->win_extents.top + ws->titlebar_height;

    x1 = ws->left_space - ws->win_extents.left;
    y1 = ws->top_space - ws->win_extents.top;
    x2 = d->width - ws->right_space + ws->win_extents.right;
    y2 = d->height - ws->bottom_space + ws->win_extents.bottom;
    pleft   = x1 + ws->win_extents.left - 0.5;
    ptop    = y1 + top - 0.5;
    pwidth  = x2 - x1 - ws->win_extents.left - ws->win_extents.right + 1;
    pheight = y2 - y1 - top - ws->win_extents.bottom + 1;

    h = d->height - ws->top_space - ws->titlebar_height - ws->bottom_space;

    int corners =
        ((pws->round_top_left) ? CORNER_TOPLEFT : 0) |
        ((pws->round_top_right) ? CORNER_TOPRIGHT : 0) |
        ((pws->round_bottom_left) ? CORNER_BOTTOMLEFT : 0) |
        ((pws->round_bottom_right) ? CORNER_BOTTOMRIGHT : 0);

    // maximize work-a-round
    if (d->state & (Wnck::WINDOW_STATE_MAXIMIZED_HORIZONTALLY |
                    Wnck::WINDOW_STATE_MAXIMIZED_VERTICALLY)) {
        corners = 0;
    }

    cr->set_operator(Cairo::OPERATOR_SOURCE);
    cr->set_line_width(1.0);

    //Center
    fill_rounded_rectangle(cr,
                           x1 + ws->win_extents.left,
                           y1 + 0.5,
                           x2 - x1 - ws->win_extents.left -
                           ws->win_extents.right,
                           top - 0.5,
                           0,
                           &pfs->base, &pfs->base,
                           SHADE_TOP, ws,
                           pws->corner_radius);

    //Top Left Gradient
    fill_rounded_rectangle(cr,
                           x1 + ws->win_extents.left,
                           y1 + 0.5,
                           (x2 - x1 - ws->win_extents.left -
                            ws->win_extents.right) / 3,
                           top - 0.5,
                           0,
                           &pfs->glow, &pfs->base,
                           SHADE_RIGHT, ws,
                           pws->corner_radius);

    //Top Right Gradient
    fill_rounded_rectangle(cr,
                           x1 + ws->win_extents.left + 2 * (x2 - x1 - ws->win_extents.left -
                                   ws->win_extents.right) / 3,
                           y1 + 0.5,
                           (x2 - x1 - ws->win_extents.left -
                            ws->win_extents.right) / 3,
                           top - 0.5,
                           0,
                           &pfs->base, &pfs->glow,
                           SHADE_RIGHT, ws,
                           pws->corner_radius);

    //Top Right
    fill_rounded_rectangle(cr,
                           x2 - ws->win_extents.right,
                           y1 + 0.5,
                           ws->win_extents.right - 0.5,
                           top - 0.5,
                           CORNER_TOPRIGHT & corners,
                           &pfs->glow, &pfs->glow,
                           SHADE_TOP | SHADE_RIGHT, ws,
                           pws->corner_radius);

    //Top Left
    fill_rounded_rectangle(cr,
                           x1 + 0.5,
                           y1 + 0.5,
                           ws->win_extents.left - 0.5,
                           top - 0.5,
                           CORNER_TOPLEFT & corners,
                           &pfs->glow, &pfs->glow,
                           SHADE_TOP | SHADE_LEFT, ws,
                           pws->corner_radius);

    //Left Top
    fill_rounded_rectangle(cr,
                           x1 + 0.5,
                           y1 + top,
                           ws->win_extents.left - 0.5,
                           h / 4,
                           0,
                           &pfs->glow, &pfs->sides,
                           SHADE_BOTTOM, ws,
                           pws->corner_radius);

    //Left Bottom
    fill_rounded_rectangle(cr,
                           x1 + 0.5,
                           y1 + top + h / 4 + 2,
                           ws->win_extents.left - 0.5,
                           h - (h / 4 + 2),
                           0,
                           &pfs->base, &pfs->base,
                           SHADE_BOTTOM, ws,
                           pws->corner_radius);

    //Left Middle
    fill_rounded_rectangle(cr,
                           x1 + 0.5,
                           y1 + top + h / 4 - 0.5,
                           ws->win_extents.left - 0.5,
                           3,
                           0,
                           &pfs->sides, &pfs->base,
                           SHADE_BOTTOM, ws,
                           pws->corner_radius);

    //Right Top
    fill_rounded_rectangle(cr,
                           x2 - ws->win_extents.right,
                           y1 + top,
                           ws->win_extents.right - 0.5,
                           h / 4,
                           0,
                           &pfs->glow, &pfs->sides,
                           SHADE_BOTTOM, ws,
                           pws->corner_radius);

    //Right Bottom
    fill_rounded_rectangle(cr,
                           x2 - ws->win_extents.right,
                           y1 + top + h / 4 + 2,
                           ws->win_extents.left - 0.5,
                           h - (h / 4 + 2),
                           0,
                           &pfs->base, &pfs->base,
                           SHADE_BOTTOM, ws,
                           pws->corner_radius);

    //Right Middle
    fill_rounded_rectangle(cr,
                           x2 - ws->win_extents.right,
                           y1 + top + h / 4 - 0.5,
                           ws->win_extents.right - 0.5,
                           3,
                           0,
                           &pfs->sides, &pfs->base,
                           SHADE_BOTTOM, ws,
                           pws->corner_radius);

    //Bottom Left
    fill_rounded_rectangle(cr,
                           x1 + 0.5,
                           y2 - ws->win_extents.bottom,
                           ws->win_extents.left - 0.5,
                           ws->win_extents.bottom - 0.5,
                           CORNER_BOTTOMLEFT & corners,
                           &pfs->base, &pfs->base,
                           SHADE_BOTTOM | SHADE_LEFT, ws,
                           pws->corner_radius);

    //Bottom
    fill_rounded_rectangle(cr,
                           x1 + ws->win_extents.left,
                           y2 - ws->win_extents.bottom,
                           x2 - x1 - ws->win_extents.left -
                           ws->win_extents.right,
                           ws->win_extents.bottom - 0.5,
                           0,
                           &pfs->base, &pfs->base,
                           SHADE_BOTTOM, ws,
                           pws->corner_radius);

    //Bottom Right
    fill_rounded_rectangle(cr,
                           x2 - ws->win_extents.right,
                           y2 - ws->win_extents.bottom,
                           ws->win_extents.right - 0.5,
                           ws->win_extents.bottom - 0.5,
                           CORNER_BOTTOMRIGHT & corners,
                           &pfs->base, &pfs->base,
                           SHADE_BOTTOM | SHADE_RIGHT, ws,
                           pws->corner_radius);

    // ======= NEW LAYER =======
    cr->set_operator(Cairo::OPERATOR_OVER);

    // titlebar separator line
    cairo_set_source_alpha_color(cr, &pfs->separator_line);
    cr->move_to(x1 + 0.5, y1 + top - 0.5);
    cr->rel_line_to(x2 - x1 - 1.0, 0.0);
    cr->stroke();


    // do not draw outside the decoration area
    rounded_rectangle(cr,
                      x1 + 0.5, y1 + 0.5,
                      x2 - x1 - 1.0, y2 - y1 - 1.0,
                      (CORNER_TOPLEFT | CORNER_TOPRIGHT |
                       CORNER_BOTTOMLEFT | CORNER_BOTTOMRIGHT) & corners,
                      ws, pws->corner_radius);
    cr->clip();

    cr->translate(1.0, 1.0);

    // highlight
    rounded_rectangle(cr,
                      x1 + 0.5, y1 + 0.5,
                      x2 - x1 - 1.0, y2 - y1 - 1.0,
                      (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                       CORNER_BOTTOMRIGHT) & corners, ws,
                      pws->corner_radius);

    cairo_set_source_alpha_color(cr, &pfs->window_highlight);
    cr->stroke();

    cr->translate(-2.0, -2.0);


    // shadow
    rounded_rectangle(cr,
                      x1 + 0.5, y1 + 0.5,
                      x2 - x1 - 1.0, y2 - y1 - 1.0,
                      (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                       CORNER_BOTTOMRIGHT) & corners, ws,
                      pws->corner_radius);

    cairo_set_source_alpha_color(cr, &pfs->window_shadow);
    cr->stroke();

    cr->translate(1.0, 1.0);

    cr->reset_clip();

    // halo
    rounded_rectangle(cr,
                      x1 + 0.5, y1 + 0.5,
                      x2 - x1 - 1.0, y2 - y1 - 1.0,
                      (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                       CORNER_BOTTOMRIGHT) & corners, ws,
                      pws->corner_radius);

    cairo_set_source_alpha_color(cr, &pfs->window_halo);
    cr->stroke();

    // inner border
    //TODO - make this a bit more pixel-perfect...but it works for now

    cr->set_line_width(1.0);

    cr->move_to(pleft + pwidth + 1.5, ptop - 1);
    cr->rel_line_to(-pwidth - 2.5, 0);
    cr->rel_line_to(0, pheight + 2.5);
    cairo_set_source_alpha_color(cr, &pfs->contents_shadow);
    cr->stroke();

    cr->move_to(pleft + pwidth + 1, ptop - 1.5);
    cr->rel_line_to(0, pheight + 2.5);
    cr->rel_line_to(-pwidth - 2.5, 0);
    cairo_set_source_alpha_color(cr, &pfs->contents_highlight);
    cr->stroke();

    cr->move_to(pleft, ptop);
    cr->rel_line_to(pwidth, 0);
    cr->rel_line_to(0, pheight);
    cr->rel_line_to(-pwidth, 0);
    cr->rel_line_to(0, -pheight);
    cairo_set_source_alpha_color(cr, &pfs->contents_halo);
    cr->stroke();
}

extern "C"
void load_engine_settings(const KeyFile& f, window_settings* ws)
{
    private_ws* pws = reinterpret_cast<private_ws*>(ws->engine_ws);

    // parse color settings
    PFACS(f, ws, base, SECT);
    PFACS(f, ws, glow, SECT);
    PFACS(f, ws, sides, SECT);
    PFACS(f, ws, window_halo, SECT);
    PFACS(f, ws, window_highlight, SECT);
    PFACS(f, ws, window_shadow, SECT);
    PFACS(f, ws, separator_line, SECT);
    PFACS(f, ws, contents_shadow, SECT);
    PFACS(f, ws, contents_highlight, SECT);
    PFACS(f, ws, contents_halo, SECT);

    // parse border settings
    load_bool_setting(f, &pws->round_top_left, "round_top_left", SECT);
    load_bool_setting(f, &pws->round_top_right, "round_top_right", SECT);
    load_bool_setting(f, &pws->round_bottom_left, "round_bottom_left", SECT);
    load_bool_setting(f, &pws->round_bottom_right, "round_bottom_right", SECT);
    load_float_setting(f, &pws->corner_radius, "radius", SECT);

}

extern "C"
void init_engine(window_settings* ws)
{
    private_fs* pfs;
    private_ws* pws;

    // private window settings
    pws = new private_ws;
    ws->engine_ws = pws;
    pws->round_top_left = true;
    pws->round_top_right = true;
    pws->round_bottom_left = true;
    pws->round_bottom_right = true;
    pws->corner_radius = 5.0;

    // private frame settings for active frames
    pfs = new private_fs;
    ws->fs_act->engine_fs = pfs;
    pfs->base = alpha_color(0.8, 0.8, 0.8, 0.5);
    pfs->glow = alpha_color(0.8, 0.8, 0.8, 0.5);
    pfs->sides = alpha_color(0.8, 0.8, 0.8, 0.5);
    pfs->window_highlight = alpha_color(1.0, 1.0, 1.0, 0.8);
    pfs->window_shadow = alpha_color(0.6, 0.6, 0.6, 0.8);
    pfs->window_halo = alpha_color(0.8, 0.8, 0.8, 0.8);
    pfs->separator_line = alpha_color(0.0, 0.0, 0.0, 0.0);
    pfs->contents_highlight = alpha_color(1.0, 1.0, 1.0, 0.8);
    pfs->contents_shadow = alpha_color(0.6, 0.6, 0.6, 0.8);
    pfs->contents_halo = alpha_color(0.8, 0.8, 0.8, 0.8);

    // private frame settings for inactive frames
    pfs = new private_fs;
    ws->fs_inact->engine_fs = pfs;
    pfs->base = alpha_color(0.8, 0.8, 0.8, 0.3);
    pfs->glow = alpha_color(0.8, 0.8, 0.8, 0.3);
    pfs->sides = alpha_color(0.8, 0.8, 0.8, 0.3);
    pfs->window_highlight = alpha_color(1.0, 1.0, 1.0, 0.7);
    pfs->window_shadow = alpha_color(0.6, 0.6, 0.6, 0.7);
    pfs->window_halo = alpha_color(0.8, 0.8, 0.8, 0.7);
    pfs->separator_line = alpha_color(0.0, 0.0, 0.0, 0.0);
    pfs->contents_highlight = alpha_color(1.0, 1.0, 1.0, 0.8);
    pfs->contents_shadow = alpha_color(0.6, 0.6, 0.6, 0.8);
    pfs->contents_halo = alpha_color(0.8, 0.8, 0.8, 0.8);
}

extern "C"
void fini_engine(window_settings* ws)
{
    delete ((private_fs*)ws->fs_act->engine_fs);
    delete ((private_fs*)ws->fs_inact->engine_fs);
}

void layout_corners_frame(Gtk::Box& vbox)
{
    Gtk::CheckButton* btn;
    btn = Gtk::manage(new Gtk::CheckButton(_("Round Top Left Corner")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "round_top_left");

    btn = Gtk::manage(new Gtk::CheckButton(_("Round Top Right Corner")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "round_top_right");

    btn = Gtk::manage(new Gtk::CheckButton(_("Round Bottom Left Corner")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "round_bottom_left");

    btn = Gtk::manage(new Gtk::CheckButton(_("Round Bottom Right Corner")));
    vbox.pack_start(*btn, false, false);
    SettingItem::create(*btn, SECT, "round_bottom_right");

    auto hbox = Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(*hbox, false, false);
    hbox->pack_start(*Gtk::manage(new Gtk::Label(_("Rounding Radius"))),
                     false, false);

    Gtk::Scale* scaler = scaler_new(0, 20, 0.5);
    hbox->pack_start(*scaler, true, true);
    SettingItem::create(*scaler, SECT, "radius");
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
    tbl.append_acolor(_("Base"), "base", SECT, active);
    tbl.append_acolor(_("Glow"), "glow", SECT, active);
    tbl.append_acolor(_("Sides"), "sides", SECT, active);
    tbl.append_separator();
    tbl.append_acolor(_("Titlebar Separator"), "separator_line", SECT, active);
    tbl.append_separator();
    tbl.append_acolor(_("Frame Outline"), "window_halo", SECT, active);
    tbl.append_acolor(_("Frame Highlight"), "window_highlight", SECT, active);
    tbl.append_acolor(_("Frame Shadow"), "window_shadow", SECT, active);
    tbl.append_separator();
    tbl.append_acolor(_("Contents Outline"), "contents_halo", SECT, active);
    tbl.append_acolor(_("Contents Highlight"), "contents_highlight", SECT, active);
    tbl.append_acolor(_("Contents Shadow"), "contents_shadow", SECT, active);
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
    layout_corners_frame(*build_notebook_page(_("Frame"), *note));
}

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
#include <emerald.h>
#include <engine.h>
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
    emi->version = g_strdup("0.1");
    emi->description = g_strdup(_("Designed to be vista-ish in nature"));
    emi->last_compat = g_strdup("0.0"); // old themes marked still compatible for testing-NeOS
    emi->icon = Gdk::Pixbuf::create_from_inline(sizeof(my_pixbuf), my_pixbuf, true);
}

extern "C"
void engine_draw_frame(decor_t* d, cairo_t* cr)
{
    double x1, y1, x2, y2, h;
    int top;
    frame_settings* fs = d->fs;
    private_fs* pfs = fs->engine_fs;
    window_settings* ws = fs->ws;
    private_ws* pws = ws->engine_ws;
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
    if (d->state & (WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY |
                    WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY)) {
        corners = 0;
    }

    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_line_width(cr, 1.0);

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
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    // titlebar separator line
    cairo_set_source_alpha_color(cr, &pfs->separator_line);
    cairo_move_to(cr, x1 + 0.5, y1 + top - 0.5);
    cairo_rel_line_to(cr, x2 - x1 - 1.0, 0.0);
    cairo_stroke(cr);


    // do not draw outside the decoration area
    rounded_rectangle(cr,
                      x1 + 0.5, y1 + 0.5,
                      x2 - x1 - 1.0, y2 - y1 - 1.0,
                      (CORNER_TOPLEFT | CORNER_TOPRIGHT |
                       CORNER_BOTTOMLEFT | CORNER_BOTTOMRIGHT) & corners,
                      ws, pws->corner_radius);
    cairo_clip(cr);

    cairo_translate(cr, 1.0, 1.0);

    // highlight
    rounded_rectangle(cr,
                      x1 + 0.5, y1 + 0.5,
                      x2 - x1 - 1.0, y2 - y1 - 1.0,
                      (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                       CORNER_BOTTOMRIGHT) & corners, ws,
                      pws->corner_radius);

    cairo_set_source_alpha_color(cr, &pfs->window_highlight);
    cairo_stroke(cr);

    cairo_translate(cr, -2.0, -2.0);


    // shadow
    rounded_rectangle(cr,
                      x1 + 0.5, y1 + 0.5,
                      x2 - x1 - 1.0, y2 - y1 - 1.0,
                      (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                       CORNER_BOTTOMRIGHT) & corners, ws,
                      pws->corner_radius);

    cairo_set_source_alpha_color(cr, &pfs->window_shadow);
    cairo_stroke(cr);

    cairo_translate(cr, 1.0, 1.0);

    cairo_reset_clip(cr);

    // halo
    rounded_rectangle(cr,
                      x1 + 0.5, y1 + 0.5,
                      x2 - x1 - 1.0, y2 - y1 - 1.0,
                      (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                       CORNER_BOTTOMRIGHT) & corners, ws,
                      pws->corner_radius);

    cairo_set_source_alpha_color(cr, &pfs->window_halo);
    cairo_stroke(cr);

    // inner border
    //TODO - make this a bit more pixel-perfect...but it works for now

    cairo_set_line_width(cr, 1.0);

    cairo_move_to(cr, pleft + pwidth + 1.5, ptop - 1);
    cairo_rel_line_to(cr, -pwidth - 2.5, 0);
    cairo_rel_line_to(cr, 0, pheight + 2.5);
    cairo_set_source_alpha_color(cr, &pfs->contents_shadow);
    cairo_stroke(cr);

    cairo_move_to(cr, pleft + pwidth + 1, ptop - 1.5);
    cairo_rel_line_to(cr, 0, pheight + 2.5);
    cairo_rel_line_to(cr, -pwidth - 2.5, 0);
    cairo_set_source_alpha_color(cr, &pfs->contents_highlight);
    cairo_stroke(cr);

    cairo_move_to(cr, pleft, ptop);
    cairo_rel_line_to(cr, pwidth, 0);
    cairo_rel_line_to(cr, 0, pheight);
    cairo_rel_line_to(cr, -pwidth, 0);
    cairo_rel_line_to(cr, 0, -pheight);
    cairo_set_source_alpha_color(cr, &pfs->contents_halo);
    cairo_stroke(cr);
}

extern "C"
void load_engine_settings(GKeyFile* f, window_settings* ws)
{
    private_ws* pws = ws->engine_ws;

    // parse color settings
    PFACS(base);
    PFACS(glow);
    PFACS(sides);
    PFACS(window_halo);
    PFACS(window_highlight);
    PFACS(window_shadow);
    PFACS(separator_line);
    PFACS(contents_shadow);
    PFACS(contents_highlight);
    PFACS(contents_halo);

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
    ACOLOR(base, 0.8, 0.8, 0.8, 0.5);
    ACOLOR(glow, 0.8, 0.8, 0.8, 0.5);
    ACOLOR(sides, 0.8, 0.8, 0.8, 0.5);
    ACOLOR(window_highlight, 1.0, 1.0, 1.0, 0.8);
    ACOLOR(window_shadow, 0.6, 0.6, 0.6, 0.8);
    ACOLOR(window_halo, 0.8, 0.8, 0.8, 0.8);
    ACOLOR(separator_line, 0.0, 0.0, 0.0, 0.0);
    ACOLOR(contents_highlight, 1.0, 1.0, 1.0, 0.8);
    ACOLOR(contents_shadow, 0.6, 0.6, 0.6, 0.8);
    ACOLOR(contents_halo, 0.8, 0.8, 0.8, 0.8);

    // private frame settings for inactive frames
    pfs = new private_fs;
    ws->fs_inact->engine_fs = pfs;
    ACOLOR(base, 0.8, 0.8, 0.8, 0.3);
    ACOLOR(glow, 0.8, 0.8, 0.8, 0.3);
    ACOLOR(sides, 0.8, 0.8, 0.8, 0.3);
    ACOLOR(window_highlight, 1.0, 1.0, 1.0, 0.7);
    ACOLOR(window_shadow, 0.6, 0.6, 0.6, 0.7);
    ACOLOR(window_halo, 0.8, 0.8, 0.8, 0.7);
    ACOLOR(separator_line, 0.0, 0.0, 0.0, 0.0);
    ACOLOR(contents_highlight, 1.0, 1.0, 1.0, 0.8);
    ACOLOR(contents_shadow, 0.6, 0.6, 0.6, 0.8);
    ACOLOR(contents_halo, 0.8, 0.8, 0.8, 0.8);
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

    table_new(3, false, false);

    scroller.add(get_current_table());

    make_labels(_("Colors"));
    table_append_separator();
    add_color_alpha_value(_("Base"), "base", SECT, active);
    add_color_alpha_value(_("Glow"), "glow", SECT, active);
    add_color_alpha_value(_("Sides"), "sides", SECT, active);
    table_append_separator();
    add_color_alpha_value(_("Titlebar Separator"), "separator_line", SECT, active);
    table_append_separator();
    add_color_alpha_value(_("Frame Outline"), "window_halo", SECT, active);
    add_color_alpha_value(_("Frame Highlight"), "window_highlight", SECT, active);
    add_color_alpha_value(_("Frame Shadow"), "window_shadow", SECT, active);
    table_append_separator();
    add_color_alpha_value(_("Contents Outline"), "contents_halo", SECT, active);
    add_color_alpha_value(_("Contents Highlight"), "contents_highlight", SECT, active);
    add_color_alpha_value(_("Contents Shadow"), "contents_shadow", SECT, active);
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

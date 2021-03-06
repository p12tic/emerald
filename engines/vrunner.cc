/*
 * vrunner theme engine
 *
 * vrunner.c
 *
 * Copyright (C) 2006 Quinn Storm <livinglatexkali@gmail.com> (original legacy theme engine)
 * Copyright (C) 2006 Varun <varunratnakar@gmail.com>
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

//vrunner engine
#include <libengine/emerald.h>
#include <libengine/engine.h>
#include <vrunner_icon.h>

#define SECT "vrunner_settings"

#define FLTS(flt) \
    load_float_setting(f,&((private_fs *)ws->fs_act->engine_fs)->flt,"active_" #flt ,SECT);\
    load_float_setting(f,&((private_fs *)ws->fs_inact->engine_fs)->flt,"inactive_" #flt ,SECT);

/*
 * settings structs
 */
struct private_fs {
    alpha_color title_left;
    alpha_color title_left_lower;
    alpha_color title_middle;
    alpha_color title_middle_lower;
    alpha_color title_right;
    alpha_color title_right_lower;
    alpha_color window_halo;
    alpha_color window_highlight;
    alpha_color window_shadow;
    alpha_color separator_line;
    alpha_color contents_highlight;
    alpha_color contents_shadow;
    alpha_color contents_halo;
    double  title_notch_position;
    double  curve_offset;
    double  frame_notch_position;
    double  color_contrast;
    double  alpha_contrast;
    double  glow_radius;
    alpha_color glow_inner;
    bool    use_glow;
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
    emi->version = "0.2";
    emi->description = _("Multiple gradients with somewhat glassy features too");
    emi->last_compat = "0.0"; // old themes marked still compatible for now
    emi->icon = Gdk::Pixbuf::create_from_inline(sizeof(my_pixbuf), my_pixbuf, true);
}

void
draw_closed_curve(Cairo::RefPtr<Cairo::Context>& cr,
                  double  x,
                  double  y,
                  double  w,
                  double  h,
                  double ch,
                  bool tophalf)
{
    cr->move_to(x, y);
    if (tophalf || ch == 0) {
        cr->line_to(x + w, y);
    } else {
        cr->curve_to(x, y, x + w / 2, y + ch, x + w, y);
    }

    cr->line_to(x + w, y + h);

    if (tophalf && ch != 0) {
        cr->curve_to(x + w, y + h, x + w / 2, y + h + ch, x, y + h);
    } else {
        cr->line_to(x, y + h);
    }

    cr->line_to(x, y);
}

void
draw_filled_closed_curve(Cairo::RefPtr<Cairo::Context>& cr,
                         double        x,
                         double        y,
                         double        w,
                         double        h,
                         double        ch,
                         bool      tophalf,
                         alpha_color* c0,
                         alpha_color* c1,
                         alpha_color* c2)
{
    draw_closed_curve(cr, x, y, w, h, ch, tophalf);

    auto pat = Cairo::LinearGradient::create(x, y, x + w, y + h);
    pat->add_color_stop_rgba(0.0, c0->color.r, c0->color.g, c0->color.b, c0->alpha);
    pat->add_color_stop_rgba(0.5, c1->color.r, c1->color.g, c1->color.b, c1->alpha);
    pat->add_color_stop_rgba(1.0, c2->color.r, c2->color.g, c2->color.b, c2->alpha);
    cr->set_source(pat);
    cr->fill();
}

void create_glow(decor_t* d, Cairo::RefPtr<Cairo::Context>& cr,
                 double x, double y, double w, double h,
                 alpha_color* c0, double p)
{
    double r = 0.0;
    double wp = p;
    double hp = p;
    //cr->rectangle(x1, y1, w, h);
    frame_settings* fs = d->fs;
    // private_fs * pfs = reinterpret_cast<private_fs*>(fs->engine_fs);
    window_settings* ws = fs->ws;
    if (wp * 2 > w) {
        wp = w / 2;
    }
    if (hp * 2 > h) {
        hp = h / 2;
    }

    x = x + wp;
    y = y + hp;
    wp = wp * 2;
    hp = hp * 2;
    w = w - wp;
    h = h - hp;

    alpha_color c1;
    c1.color = c0->color;
    c1.alpha = 0;
    cr->set_operator(Cairo::OPERATOR_OVER);

    fill_rounded_rectangle(cr,
                           x - wp, y - hp, wp, hp, CORNER_TOPLEFT,
                           c0, &c1, (SHADE_LEFT | SHADE_TOP), ws, r);
    fill_rounded_rectangle(cr,
                           x, y - hp, w, hp, 0,
                           c0, &c1, SHADE_TOP, ws, r);
    fill_rounded_rectangle(cr,
                           x + w, y - hp, wp, hp, CORNER_TOPRIGHT,
                           c0, &c1, (SHADE_RIGHT | SHADE_TOP), ws, r);
    fill_rounded_rectangle(cr,
                           x - wp, y, wp, h, 0,
                           c0, &c1, SHADE_LEFT, ws, r);
    fill_rounded_rectangle(cr,
                           x + w, y, wp, h, 0,
                           c0, &c1, SHADE_RIGHT, ws, r);
    fill_rounded_rectangle(cr,
                           x - wp, y + h, wp, hp, CORNER_BOTTOMLEFT,
                           c0, &c1, (SHADE_LEFT | SHADE_BOTTOM), ws, r);
    fill_rounded_rectangle(cr,
                           x, y + h, w, hp, 0,
                           c0, &c1, SHADE_BOTTOM, ws, r);
    fill_rounded_rectangle(cr,
                           x + w, y + h, wp, hp, CORNER_BOTTOMRIGHT,
                           c0, &c1, (SHADE_RIGHT | SHADE_BOTTOM), ws, r);

    cr->rectangle(x, y, w, h);
    cairo_set_source_alpha_color(cr, c0);
    cr->fill();
}

int get_real_pos(window_settings* ws, int tobj, decor_t* d)
{
    int width = d->width;
    int base = ws->left_space;
    switch (d->tobj_item_state[tobj]) {
    case 1:
        base = (width - ws->left_space - ws->right_space - d->tobj_size[0] - d->tobj_size[2]) / 2
               - d->tobj_size[1] / 2 + ws->left_space + d->tobj_size[0];
        break;
    case 2:
        base = width - ws->right_space - d->tobj_size[2];
        break;
    case 3:
        return -1;
    default:
        break;
    }
    return base + d->tobj_item_pos[tobj];
}

extern "C"
void engine_draw_frame(decor_t* d, Cairo::RefPtr<Cairo::Context>& cr)
{
    double        x1, y1, x2, y2, h,
                  top_title_height, bottom_title_height,
                  top_left, top_right;
    int           top, title_width = 0, title_height = 0, title_pos;
    // double        curve_offset;
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
        ((pws->round_top_left)     ? CORNER_TOPLEFT     : 0) |
        ((pws->round_top_right)    ? CORNER_TOPRIGHT    : 0) |
        ((pws->round_bottom_left)  ? CORNER_BOTTOMLEFT  : 0) |
        ((pws->round_bottom_right) ? CORNER_BOTTOMRIGHT : 0);

    // maximize work-a-round
    if (d->state & (Wnck::WINDOW_STATE_MAXIMIZED_HORIZONTALLY |
                    Wnck::WINDOW_STATE_MAXIMIZED_VERTICALLY)) {
        corners = 0;
    }

    cr->set_operator(Cairo::OPERATOR_SOURCE);
    cr->set_line_width(0.0);

    top_title_height    = (top - 0.5) * pfs->title_notch_position + 1;
    bottom_title_height = (top - 0.5) * (1 - pfs->title_notch_position) + 1;

    top_left = ws->win_extents.left;
    top_right = ws->win_extents.right;
    if ((top_left < pws->corner_radius) && (corners & CORNER_TOPLEFT)) {
        top_left = pws->corner_radius;
    }
    if ((top_right < pws->corner_radius) && (corners & CORNER_TOPRIGHT)) {
        top_right = pws->corner_radius;
    }

    // Main Top Titlebar
    rounded_rectangle(cr,
                      x1 + 0.5,
                      y1 + 0.5,
                      x2 - x1 - 1,
                      top + 1,
                      (CORNER_TOPLEFT | CORNER_TOPRIGHT) & corners, ws,
                      pws->corner_radius);
    cr->clip();

    draw_filled_closed_curve(cr,
                             x1 + 0.5,
                             y1 + 0.5,
                             x2 - x1 - 1,
                             top_title_height + 1,
                             pfs->curve_offset,
                             true,
                             &pfs->title_left,
                             &pfs->title_middle,
                             &pfs->title_right);

    draw_filled_closed_curve(cr,
                             x1 + 0.5,
                             y1 + top_title_height - 1,
                             x2 - x1 - 1,
                             bottom_title_height + 2,
                             pfs->curve_offset,
                             false,
                             &pfs->title_left_lower,
                             &pfs->title_middle_lower,
                             &pfs->title_right_lower);

    cr->reset_clip();

    // Left and Right
    fill_rounded_rectangle(cr,
                           x1 + 0.5,
                           y1 + top - 0.5,
                           ws->win_extents.left,
                           h + 0.5,
                           0,
                           &pfs->title_left_lower, &pfs->title_left_lower,
                           SHADE_BOTTOM, ws,
                           pws->corner_radius);

    fill_rounded_rectangle(cr,
                           x2 - ws->win_extents.right - 0.5,
                           y1 + top - 0.5,
                           ws->win_extents.right,
                           h + 0.5,
                           0,
                           &pfs->title_right_lower, &pfs->title_right_lower,
                           SHADE_BOTTOM, ws,
                           pws->corner_radius);

    // Bottom Frame
    rounded_rectangle(cr,
                      x1 + 0.5,
                      y2 - ws->win_extents.bottom,
                      x2 - x1 - 1,
                      ws->win_extents.bottom,
                      (CORNER_BOTTOMLEFT | CORNER_BOTTOMRIGHT) & corners, ws,
                      pws->corner_radius);

    cr->clip();

    draw_filled_closed_curve(cr,
                             x1 + 0.5,
                             y2 - ws->win_extents.bottom,
                             x2 - x1 - 1,
                             ws->win_extents.bottom,
                             0,
                             true,
                             &pfs->title_left_lower,
                             &pfs->title_middle_lower,
                             &pfs->title_right_lower);

    cr->reset_clip();

    // Title Glow
    if (pfs->use_glow) {
        cr->set_operator(Cairo::OPERATOR_ATOP);

        if (d->layout) {
            d->layout->get_pixel_size(title_width, title_height);
        }
        title_pos = get_real_pos(ws, TBT_TITLE, d);
        create_glow(d, cr, title_pos, y1 + top / 2.0 - title_height / 2.0,
                    title_width, title_height,
                    &pfs->glow_inner, pfs->glow_radius);
    }


    // ======= NEW LAYER ======
    cr->set_operator(Cairo::OPERATOR_OVER);
    cr->set_line_width(1.0);

    // Titlebar Separator
    if (pfs->separator_line.alpha > 0) {
        cairo_set_source_alpha_color(cr, &pfs->separator_line);
        cr->move_to(x1 + 0.5, y1 + top - 0.5);
        cr->rel_line_to(x2 - x1 - 1, 0.0);
        cr->stroke();
    }

    // Frame Clip
    rounded_rectangle(cr,
                      x1 + 0.5, y1 + 0.5,
                      x2 - x1 - 1.0, y2 - y1 - 1.0,
                      (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                       CORNER_BOTTOMRIGHT) & corners, ws,
                      pws->corner_radius);
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

    // color settings
    PFACS(f, ws, title_left, SECT);
    PFACS(f, ws, title_middle, SECT);
    PFACS(f, ws, title_right, SECT);
    PFACS(f, ws, window_halo, SECT);
    PFACS(f, ws, window_highlight, SECT);
    PFACS(f, ws, window_shadow, SECT);
    PFACS(f, ws, separator_line, SECT);
    PFACS(f, ws, contents_shadow, SECT);
    PFACS(f, ws, contents_highlight, SECT);
    PFACS(f, ws, contents_halo, SECT);
    PFACS(f, ws, glow_inner, SECT);
    FLTS(color_contrast);
    FLTS(glow_radius);
    FLTS(alpha_contrast);
    FLTS(curve_offset);
    FLTS(title_notch_position);

    private_fs* pfs = reinterpret_cast<private_fs*>(ws->fs_act->engine_fs);
    pfs->title_left_lower = apply_contrast(pfs->title_left, pfs->color_contrast, pfs->alpha_contrast);
    pfs->title_middle_lower = apply_contrast(pfs->title_middle, pfs->color_contrast, pfs->alpha_contrast);
    pfs->title_right_lower = apply_contrast(pfs->title_right, pfs->color_contrast, pfs->alpha_contrast);
    load_bool_setting(f, &pfs->use_glow, "active_use_glow", SECT);

    pfs = reinterpret_cast<private_fs*>(ws->fs_inact->engine_fs);
    pfs->title_left_lower = apply_contrast(pfs->title_left, pfs->color_contrast, pfs->alpha_contrast);
    pfs->title_middle_lower = apply_contrast(pfs->title_middle, pfs->color_contrast, pfs->alpha_contrast);
    pfs->title_right_lower = apply_contrast(pfs->title_right, pfs->color_contrast, pfs->alpha_contrast);
    load_bool_setting(f, &pfs->use_glow, "inactive_use_glow", SECT);

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

    pws = new private_ws{};
    ws->engine_ws = pws;
    pws->round_top_left = true;
    pws->round_top_right = true;
    pws->round_bottom_left = true;
    pws->round_bottom_right = true;
    pws->corner_radius = 5.0;

    pfs = new private_fs{};
    ws->fs_act->engine_fs = pfs;
    pfs->title_notch_position = 0.5;
    pfs->curve_offset = 0.0;
    pfs->color_contrast = 0.9;
    pfs->use_glow = false;
    pfs->alpha_contrast = 0.9;
    pfs->glow_radius = 7.0;
    pfs->title_left = alpha_color(0.8, 0.8, 0.8, 0.8);
    pfs->title_left_lower = apply_contrast(pfs->title_left, pfs->color_contrast, pfs->alpha_contrast);
    pfs->title_middle = alpha_color(0.8, 0.8, 0.8, 0.8);
    pfs->title_middle_lower = apply_contrast(pfs->title_middle, pfs->color_contrast, pfs->alpha_contrast);
    pfs->title_right = alpha_color(0.8, 0.8, 0.8, 0.8);
    pfs->title_right_lower = apply_contrast(pfs->title_right, pfs->color_contrast, pfs->alpha_contrast);
    pfs->window_highlight = alpha_color(1.0, 1.0, 1.0, 0.8);
    pfs->window_shadow = alpha_color(0.6, 0.6, 0.6, 0.8);
    pfs->window_halo = alpha_color(0.8, 0.8, 0.8, 0.8);
    pfs->separator_line = alpha_color(0.0, 0.0, 0.0, 0.0);
    pfs->contents_highlight = alpha_color(1.0, 1.0, 1.0, 0.8);
    pfs->contents_shadow = alpha_color(0.6, 0.6, 0.6, 0.8);
    pfs->contents_halo = alpha_color(0.8, 0.8, 0.8, 0.8);
    pfs->glow_inner = alpha_color(0.9, 0.9, 0.9, 0.9);

    pfs = new private_fs{};
    ws->fs_inact->engine_fs = pfs;
    pfs->title_notch_position = 0.5;
    pfs->curve_offset = 0.0;
    pfs->color_contrast = 0.9;
    pfs->alpha_contrast = 0.9;
    pfs->use_glow = false;
    pfs->glow_radius = 7.0;
    pfs->title_left = alpha_color(0.8, 0.8, 0.8, 0.6);
    pfs->title_left_lower = apply_contrast(pfs->title_left, pfs->color_contrast, pfs->alpha_contrast);
    pfs->title_middle = alpha_color(0.8, 0.8, 0.8, 0.6);
    pfs->title_middle_lower = apply_contrast(pfs->title_middle, pfs->color_contrast, pfs->alpha_contrast);
    pfs->title_right = alpha_color(0.8, 0.8, 0.8, 0.6);
    pfs->title_right_lower = apply_contrast(pfs->title_right, pfs->color_contrast, pfs->alpha_contrast);
    pfs->window_highlight = alpha_color(1.0, 1.0, 1.0, 0.7);
    pfs->window_shadow = alpha_color(0.6, 0.6, 0.6, 0.7);
    pfs->window_halo = alpha_color(0.8, 0.8, 0.8, 0.7);
    pfs->separator_line = alpha_color(0.0, 0.0, 0.0, 0.0);
    pfs->contents_highlight = alpha_color(1.0, 1.0, 1.0, 0.8);
    pfs->contents_shadow = alpha_color(0.6, 0.6, 0.6, 0.8);
    pfs->contents_halo = alpha_color(0.8, 0.8, 0.8, 0.8);
    pfs->glow_inner = alpha_color(0.9, 0.9, 0.9, 0.9);
}

extern "C"
void fini_engine(window_settings* ws)
{
    delete ((private_fs*)ws->fs_act->engine_fs);
    delete ((private_fs*)ws->fs_inact->engine_fs);
}

void layout_corners_frame(Gtk::Box& vbox)
{
    Gtk::ToggleButton* btn;
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

    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, false, false);

    hbox.pack_start(*Gtk::manage(new Gtk::Label(_("Rounding Radius"))),
                    false, false);

    auto& scaler = *scaler_new(0, 20, 0.5);
    hbox.pack_start(scaler, true, true);
    SettingItem::create(scaler, SECT, "radius");

}
void my_engine_settings(Gtk::Box& hbox, bool active)
{
    Gtk::Scale* scaler;

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
    tbl.append_acolor(_("Left Blend"), "title_left", SECT, active);
    tbl.append_acolor(_("Middle Blend"), "title_middle", SECT, active);
    tbl.append_acolor(_("Right Blend"), "title_right", SECT, active);

    tbl.append_separator();
    tbl.append(*Gtk::manage(new Gtk::Label(_("Contrast"))), false);
    tbl.append(*Gtk::manage(new Gtk::Label(_("Color"))), false);
    scaler = scaler_new(0, 1, 0.01);
    tbl.append(*scaler, false);
    if (active) {
        SettingItem::create(*scaler, SECT, "active_color_contrast");
    } else {
        SettingItem::create(*scaler, SECT, "inactive_color_contrast");
    }

    tbl.append(*Gtk::manage(new Gtk::Label(_("Contrast"))), false);
    tbl.append(*Gtk::manage(new Gtk::Label(_("(Alpha)"))), false);
    scaler = scaler_new(0, 1, 0.01);
    tbl.append(*scaler, false);

    if (active) {
        SettingItem::create(*scaler, SECT, "active_alpha_contrast");
    } else {
        SettingItem::create(*scaler, SECT, "inactive_alpha_contrast");
    }

    tbl.append(*Gtk::manage(new Gtk::Label(_("Notch"))), false);
    tbl.append(*Gtk::manage(new Gtk::Label(_("Position"))), false);
    scaler = scaler_new(0, 1, 0.01);
    scaler->set_value(0.5);
    tbl.append(*scaler, false);
    if (active) {
        SettingItem::create(*scaler, SECT, "active_title_notch_position");
    } else {
        SettingItem::create(*scaler, SECT, "inactive_title_notch_position");
    }

    tbl.append(*Gtk::manage(new Gtk::Label(_("Curve"))), false);
    tbl.append(*Gtk::manage(new Gtk::Label(_("Offset"))), false);
    scaler = scaler_new(-100, 100, 0.1);
    scaler->set_value(0);
    tbl.append(*scaler, false);
    if (active) {
        SettingItem::create(*scaler, SECT, "active_curve_offset");
    } else {
        SettingItem::create(*scaler, SECT, "inactive_curve_offset");
    }

    tbl.append_separator();

    Gtk::CheckButton* btn;
    btn = Gtk::manage(new Gtk::CheckButton(_("Use Glow")));
    vbox.pack_start(*btn, false, false);
    btn->set_active(false);
    if (active) {
        SettingItem::create(*btn, SECT, "active_use_glow");
    } else {
        SettingItem::create(*btn, SECT, "inactive_use_glow");
    }

    tbl.append_acolor(_("Title Glow"), "glow_inner", SECT, active);
    tbl.append(*Gtk::manage(new Gtk::Label("Glow")), false);
    tbl.append(*Gtk::manage(new Gtk::Label(_("Radius"))), false);
    scaler = scaler_new(0, 25, 0.1);
    scaler->set_value(7.0);
    tbl.append(*scaler, false);
    if (active) {
        SettingItem::create(*scaler, SECT, "active_glow_radius");
    } else {
        SettingItem::create(*scaler, SECT, "inactive_glow_radius");
    }

    tbl.append_separator();
    tbl.append_acolor(_("Titlebar Separator"), "separator_line", SECT, active);

    tbl.append_separator();
    tbl.append_acolor(_("Frame Outline"), "window_halo", SECT, active);
    tbl.append_acolor(_("Frame Highlight"), "window_highlight", SECT, active);
    tbl.append_acolor("Frame Shadow", "window_shadow", SECT, active);

    tbl.append_separator();
    tbl.append_acolor(_("Contents Outline"), "contents_halo", SECT, active);
    tbl.append_acolor(_("Contents Highlight"), "contents_highlight", SECT, active);
    tbl.append_acolor("Contents Shadow", "contents_shadow", SECT, active);
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

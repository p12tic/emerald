/*
 * oxygen theme engine
 *
 * oxygen.c
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

//pixmap engine
#include <libengine/emerald.h>
#include <libengine/engine.h>
#include <pixmap_icon.h>

#define SECT "pixmap_settings"

std::pair<Cairo::RefPtr<Cairo::ImageSurface>, bool>
    read_png(const std::string& fn)
{
    try {
        return {Cairo::ImageSurface::create_from_png(fn), true};
    } catch (...) {
        return {Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 0, 0), false};
    }
}

/*
 * pixmap data structs
 */
const char* p_types[] = {
    "top",
    "top_left",
    "top_right",
    "left",
    "right",
    "bottom",
    "bottom_left",
    "bottom_right",
    "title",
    "title_left",
    "title_right"
};
const char* names[] = {
    "Top",
    "Top Left",
    "Top Right",
    "Left",
    "Right",
    "Bottom",
    "Bottom Left",
    "Bottom Right",
    "Title",
    "Title Left",
    "Title Right"
};
enum {
    TOP = 0,
    TOP_LEFT,
    TOP_RIGHT,
    LEFT,
    RIGHT,
    BOTTOM,
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    TITLE,
    TITLE_LEFT,
    TITLE_RIGHT
};

struct pixmap_data {
    Cairo::RefPtr<Cairo::ImageSurface> surface;
    bool is_valid;
    bool use_scaled;
    bool use_width;
    bool use_height;
    double width;
    double height;
};

/*
 * settings structs
 */
struct private_fs {
    alpha_color inner;
    alpha_color outer;
    alpha_color title_inner;
    alpha_color title_outer;
    pixmap_data pixmaps[11];
};

struct private_ws {
    bool round_top_left;
    bool round_top_right;
    bool round_bottom_left;
    bool round_bottom_right;
    bool inactive_use_active_pixmaps;
    double  top_corner_radius;
    double  bottom_corner_radius;
};

extern "C"
void get_meta_info(EngineMetaInfo* emi)
{
    emi->version = "0.2";
    emi->description = _("Everything done with customizable pixmaps!");
    emi->last_compat = "0.0"; // old themes marked still compatible
    emi->icon = Gdk::Pixbuf::create_from_inline(sizeof(my_pixbuf), my_pixbuf, true);
}

void
fill_rounded_rectangle_pixmap_blend(Cairo::RefPtr<Cairo::Context>&       cr,
                                    double        x, double        y,
                                    double        w, double        h,
                                    int           corner,
                                    alpha_color* c0,
                                    alpha_color* c1,
                                    int           gravity,
                                    pixmap_data* pix,
                                    window_settings* ws,
                                    double    radius,
                                    bool blend_only_if_pixmaps_available)
{
    bool blend = true;
    int iw, ih;

    if (pix->is_valid) {
        iw = pix->surface->get_width();
        ih = pix->surface->get_height();

        cr->set_operator(Cairo::OPERATOR_SOURCE);
        cr->set_line_width(0.0);

        // While using scaled pixmaps
        if (pix->use_scaled) {
            auto mat = Cairo::scaling_matrix(iw / w, ih / h);
            mat.translate(-x, -y);

            auto pat = Cairo::SurfacePattern::create(pix->surface);
            pat->set_matrix(mat);
            cr->set_source(pat);
            pat->set_extend(Cairo::EXTEND_REPEAT);
        } else {
            // While using tiled pixmaps
            cr->set_source(pix->surface, x, y);
            cairo_pattern_set_extend(cr->get_source()->cobj(), CAIRO_EXTEND_REPEAT);
        }

        rounded_rectangle(cr, x, y, w, h, corner, ws, radius);
        cr->fill();

    } else if (blend_only_if_pixmaps_available) {
        blend = false;
    }

    cr->set_operator(Cairo::OPERATOR_OVER);
    if (w > 0 && blend) {
        // Now blend in the gradients
        fill_rounded_rectangle(cr, x, y, w, h, corner, c0, c1, gravity, ws, radius);
    }
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
    double        x1, y1, x2, y2, h;
    double        top_left_width, top_right_width;
    double        top_left_height, top_right_height;
    double        left_width, right_width;
    double        bottom_left_width, bottom_right_width;
    double        bottom_left_height, bottom_right_height;
    int           top, title_width, title_pos;
    int           title_left_width, title_right_width;
    frame_settings* fs = d->fs;
    private_fs* pfs = reinterpret_cast<private_fs*>(fs->engine_fs);
    window_settings* ws = fs->ws;
    private_ws* pws = reinterpret_cast<private_ws*>(ws->engine_ws);

    top = ws->win_extents.top + ws->titlebar_height;
    x1 = ws->left_space - ws->win_extents.left;
    y1 = ws->top_space - ws->win_extents.top;
    x2 = d->width - ws->right_space + ws->win_extents.right;
    y2 = d->height - ws->bottom_space + ws->win_extents.bottom;

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

    left_width = top_left_width = bottom_left_width = ws->win_extents.left;
    right_width = top_right_width = bottom_right_width = ws->win_extents.right;
    title_width = title_left_width = title_right_width = 0;

    if (pfs->pixmaps[TITLE].is_valid) {
        title_left_width = pfs->pixmaps[TITLE].surface->get_width();
    }
    if (pfs->pixmaps[TITLE_LEFT].is_valid) {
        title_right_width = pfs->pixmaps[TITLE_LEFT].surface->get_width();
    }

    top_left_height = top_right_height = top;
    bottom_left_height = bottom_right_height = ws->win_extents.bottom;

    // Adjustments of the (top/bottom)-(left/right) bar
    // if the radius is bigger than left/right extents
    if ((ws->win_extents.left < pws->top_corner_radius) &&
            (CORNER_TOPLEFT & corners)) {
        top_left_width = pws->top_corner_radius;
    }
    if ((ws->win_extents.left < pws->bottom_corner_radius) &&
            (CORNER_BOTTOMLEFT & corners)) {
        bottom_left_width = pws->bottom_corner_radius;
    }
    if ((ws->win_extents.right < pws->top_corner_radius) &&
            (CORNER_TOPRIGHT & corners)) {
        top_right_width = pws->top_corner_radius;
    }
    if ((ws->win_extents.right < pws->bottom_corner_radius) &&
            (CORNER_BOTTOMRIGHT & corners)) {
        bottom_right_width = pws->bottom_corner_radius;
    }

    // Manual Width overrides
    if (pfs->pixmaps[TOP_LEFT].use_width) {
        top_left_width = pfs->pixmaps[TOP_LEFT].width;
    }
    if (pfs->pixmaps[TOP_RIGHT].use_width) {
        top_right_width = pfs->pixmaps[TOP_RIGHT].width;
    }
    if (pfs->pixmaps[LEFT].use_width) {
        left_width = pfs->pixmaps[LEFT].width;
    }
    if (pfs->pixmaps[RIGHT].use_width) {
        right_width = pfs->pixmaps[RIGHT].width;
    }
    if (pfs->pixmaps[BOTTOM - LEFT].use_width) {
        bottom_left_width = pfs->pixmaps[BOTTOM_LEFT].width;
    }
    if (pfs->pixmaps[BOTTOM_RIGHT].use_width) {
        bottom_right_width = pfs->pixmaps[BOTTOM_RIGHT].width;
    }

    if (pfs->pixmaps[TITLE_LEFT].use_width) {
        title_left_width = pfs->pixmaps[TITLE_LEFT].width;
    }
    if (pfs->pixmaps[TITLE_RIGHT].use_width) {
        title_right_width = pfs->pixmaps[TITLE_RIGHT].width;
    }

    if (pfs->pixmaps[TOP_LEFT].use_height) {
        top_left_height = pfs->pixmaps[TOP_LEFT].height;
    }
    if (pfs->pixmaps[TOP_RIGHT].use_height) {
        top_right_height = pfs->pixmaps[TOP_RIGHT].height;
    }
    if (pfs->pixmaps[BOTTOM_LEFT].use_height) {
        bottom_left_height = pfs->pixmaps[BOTTOM_LEFT].height;
    }
    if (pfs->pixmaps[BOTTOM_RIGHT].use_height) {
        bottom_right_height = pfs->pixmaps[BOTTOM_RIGHT].height;
    }

    // Top Left Bar
    fill_rounded_rectangle_pixmap_blend(cr,
                                        x1,
                                        y1,
                                        top_left_width,
                                        top_left_height + 1,
                                        CORNER_TOPLEFT & corners,
                                        &pfs->title_inner, &pfs->title_outer,
                                        SHADE_TOP | SHADE_LEFT, &pfs->pixmaps[TOP_LEFT], ws,
                                        pws->top_corner_radius, true);

    // Top Bar
    fill_rounded_rectangle_pixmap_blend(cr,
                                        x1 + top_left_width,
                                        y1,
                                        x2 - x1 - top_left_width - top_right_width,
                                        top + 1,
                                        0,
                                        &pfs->title_inner, &pfs->title_outer,
                                        SHADE_TOP, &pfs->pixmaps[TOP], ws,
                                        pws->top_corner_radius, true);

    // Top Right Bar
    fill_rounded_rectangle_pixmap_blend(cr,
                                        x2 - top_right_width,
                                        y1,
                                        top_right_width,
                                        top_right_height + 1,
                                        CORNER_TOPRIGHT & corners,
                                        &pfs->title_inner, &pfs->title_outer,
                                        SHADE_TOP | SHADE_RIGHT, &pfs->pixmaps[TOP_RIGHT], ws,
                                        pws->top_corner_radius, true);

    // Left Bar
    fill_rounded_rectangle_pixmap_blend(cr,
                                        x1 + ws->win_extents.left - left_width,
                                        y1 + top_left_height - 1,
                                        left_width,
                                        h + 1 - (top_left_height - top),
                                        0,
                                        &pfs->inner, &pfs->outer,
                                        SHADE_LEFT, &pfs->pixmaps[LEFT], ws,
                                        pws->top_corner_radius, false);

    // Right Bar
    fill_rounded_rectangle_pixmap_blend(cr,
                                        x2 - ws->win_extents.right,
                                        y1 + top_right_height - 1,
                                        right_width,
                                        h + 1 - (top_right_height - top),
                                        0,
                                        &pfs->inner, &pfs->outer,
                                        SHADE_RIGHT, &pfs->pixmaps[RIGHT], ws,
                                        pws->top_corner_radius, false);

    // Bottom Left Bar
    fill_rounded_rectangle_pixmap_blend(cr,
                                        x1,
                                        y2 - bottom_left_height,
                                        bottom_left_width,
                                        bottom_left_height,
                                        CORNER_BOTTOMLEFT & corners,
                                        &pfs->inner, &pfs->outer,
                                        SHADE_BOTTOM | SHADE_LEFT, &pfs->pixmaps[BOTTOM_LEFT], ws,
                                        pws->bottom_corner_radius, false);

    // Bottom Bar
    fill_rounded_rectangle_pixmap_blend(cr,
                                        x1 + bottom_left_width,
                                        y2 - ws->win_extents.bottom,
                                        x2 - x1 - bottom_left_width - bottom_right_width,
                                        ws->win_extents.bottom, 0,
                                        &pfs->inner, &pfs->outer,
                                        SHADE_BOTTOM, &pfs->pixmaps[BOTTOM], ws,
                                        pws->bottom_corner_radius, false);

    // Bottom Right Bar
    fill_rounded_rectangle_pixmap_blend(cr,
                                        x2 - bottom_right_width,
                                        y2 - bottom_right_height,
                                        bottom_right_width,
                                        bottom_right_height,
                                        CORNER_BOTTOMRIGHT & corners,
                                        &pfs->inner, &pfs->outer,
                                        SHADE_BOTTOM | SHADE_RIGHT, &pfs->pixmaps[BOTTOM_RIGHT], ws,
                                        pws->bottom_corner_radius, false);

    cr->set_operator(Cairo::OPERATOR_OVER);

    // Draw Title pixmaps
    if (d->layout) {
        int dummy;
        d->layout->get_pixel_size(title_width, dummy);
    }
    title_pos = get_real_pos(ws, TBT_TITLE, d);

    // Check that it doesn't overflow
    if ((title_width + title_left_width + title_right_width) >
            (x2 - x1 - top_left_width - top_right_width - 10)) {
        double scaledown = (x2 - x1 - top_left_width - top_right_width - 10) /
                           (title_width + title_left_width + title_right_width);
        title_width = scaledown * title_width;
        title_left_width = scaledown * title_left_width - 1;
        title_right_width = scaledown * title_right_width;
    }

    // Title Left
    fill_rounded_rectangle_pixmap_blend(cr,
                                        title_pos - title_left_width - 1,
                                        y1,
                                        title_left_width + 1,
                                        top,
                                        0,
                                        &pfs->title_inner, &pfs->title_outer,
                                        SHADE_TOP, &pfs->pixmaps[TITLE_LEFT], ws,
                                        pws->top_corner_radius, true);

    // Title
    fill_rounded_rectangle_pixmap_blend(cr,
                                        title_pos - 0.5,
                                        y1,
                                        title_width + 0.5,
                                        top,
                                        0,
                                        &pfs->title_inner, &pfs->title_outer,
                                        SHADE_TOP, &pfs->pixmaps[TITLE], ws,
                                        pws->top_corner_radius, true);

    // Title Right
    fill_rounded_rectangle_pixmap_blend(cr,
                                        title_pos + title_width - 1,
                                        y1,
                                        title_right_width + 1,
                                        top,
                                        0,
                                        &pfs->title_inner, &pfs->title_outer,
                                        SHADE_TOP, &pfs->pixmaps[TITLE_RIGHT], ws,
                                        pws->top_corner_radius, true);

    cr->stroke();
}

extern "C"
void load_engine_settings(const KeyFile& f, window_settings* ws)
{
    private_ws* pws = reinterpret_cast<private_ws*>(ws->engine_ws);
    std::string pre = "active";
    PFACS(f, ws, outer, SECT);
    PFACS(f, ws, inner, SECT);
    PFACS(f, ws, title_outer, SECT);
    PFACS(f, ws, title_inner, SECT);
    load_bool_setting(f, &pws->round_top_left, "round_top_left", SECT);
    load_bool_setting(f, &pws->round_top_right, "round_top_right", SECT);
    load_bool_setting(f, &pws->round_bottom_left, "round_bottom_left", SECT);
    load_bool_setting(f, &pws->round_bottom_right, "round_bottom_right", SECT);
    load_bool_setting(f, &pws->inactive_use_active_pixmaps, "inactive_use_active_pixmaps", SECT);
    load_float_setting(f, &pws->top_corner_radius, "top_radius", SECT);
    load_float_setting(f, &pws->bottom_corner_radius, "bottom_radius", SECT);

    // Active window
    private_fs* pfs = reinterpret_cast<private_fs*>(ws->fs_act->engine_fs);
    for (int i = 0; i < 11; i++) {
        std::string key = pre + "_" + p_types[i];
        auto png = read_png(make_filename("pixmaps", key, "png"));
        pfs->pixmaps[i].surface = png.first;
        pfs->pixmaps[i].is_valid = png.second;

        load_bool_setting(f, &pfs->pixmaps[i].use_scaled, key + "_use_scaled", SECT);
        load_bool_setting(f, &pfs->pixmaps[i].use_width, key + "_use_width", SECT);
        load_float_setting(f, &pfs->pixmaps[i].width, key + "_width", SECT);
        load_bool_setting(f, &pfs->pixmaps[i].use_height, key + "_use_height", SECT);
        load_float_setting(f, &pfs->pixmaps[i].height, key + "_height", SECT);
    }

    // Inactive window
    pfs = reinterpret_cast<private_fs*>(ws->fs_inact->engine_fs);
    if (!pws->inactive_use_active_pixmaps) {
        pre = "inactive";
    }
    for (int i = 0; i < 11; i++) {
        std::string key = pre + "_" + p_types[i];

        auto png = read_png(make_filename("pixmaps", key, "png"));
        pfs->pixmaps[i].surface = png.first;
        pfs->pixmaps[i].is_valid = png.second;

        load_bool_setting(f, &pfs->pixmaps[i].use_scaled, key + "_use_scaled", SECT);
        load_bool_setting(f, &pfs->pixmaps[i].use_width, key + "_use_width", SECT);
        load_float_setting(f, &pfs->pixmaps[i].width, key + "_width", SECT);
        load_bool_setting(f, &pfs->pixmaps[i].use_height, key + "_use_height", SECT);
        load_float_setting(f, &pfs->pixmaps[i].height, key + "_height", SECT);
    }
}

extern "C"
void init_engine(window_settings* ws)
{
    private_fs* pfs;
    private_ws* pws;

    // private window settings
    pws = new private_ws{};
    ws->engine_ws = pws;
    pws->round_top_left = true;
    pws->round_top_right = true;
    pws->round_bottom_left = true;
    pws->round_bottom_right = true;
    pws->top_corner_radius = 5.0;
    pws->bottom_corner_radius = 5.0;

    // private frame settings for active frames
    pfs = new private_fs{};
    ws->fs_act->engine_fs = pfs;
    pfs->inner = alpha_color(0.8, 0.8, 0.8, 0.5);
    pfs->outer = alpha_color(0.8, 0.8, 0.8, 0.5);
    pfs->title_inner = alpha_color(0.8, 0.8, 0.8, 0.8);
    pfs->title_outer = alpha_color(0.8, 0.8, 0.8, 0.8);

    // private frame settings for inactive frames
    pfs = new private_fs{};
    ws->fs_inact->engine_fs = pfs;
    pfs->inner = alpha_color(0.8, 0.8, 0.8, 0.3);
    pfs->outer = alpha_color(0.8, 0.8, 0.8, 0.3);
    pfs->title_inner = alpha_color(0.8, 0.8, 0.8, 0.6);
    pfs->title_outer = alpha_color(0.8, 0.8, 0.8, 0.6);
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

    {
    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, false, false);
    hbox.pack_start(*Gtk::manage(new Gtk::Label(_("Top Rounding Radius"))),
                    false, false, 0);
    auto& scaler = *scaler_new(0, 20, 0.5);
    hbox.pack_start(scaler, true, true);
    SettingItem::create(scaler, SECT, "top_radius");
    }{
    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, false, false);
    hbox.pack_start(*Gtk::manage(new Gtk::Label(_("Bottom Rounding Radius"))),
                    false, false, 0);
    auto& scaler = *scaler_new(0, 20, 0.5);
    hbox.pack_start(scaler, true, true);
    SettingItem::create(scaler, SECT, "bottom_radius");
    }
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

    tbl.append_header("Colors");
    tbl.append_separator();
    tbl.append_acolor(_("Outer Frame Blend"), "outer", SECT, active);
    tbl.append_acolor(_("Inner Frame Blend"), "inner", SECT, active);
    tbl.append_separator();
    tbl.append_acolor(_("Outer Titlebar Blend"), "title_outer", SECT, active);
    tbl.append_acolor(_("Inner Titlebar Blend"), "title_inner", SECT, active);
    tbl.append_separator();
    tbl.append_acolor(_("Titlebar Separator"), "separator_line", SECT, active);
}

void layout_engine_colors(Gtk::Box& vbox)
{
    auto& hbox = *Gtk::manage(new Gtk::HBox(false, 2));
    vbox.pack_start(hbox, true, true);
    my_engine_settings(hbox, true);
    hbox.pack_start(*Gtk::manage(new Gtk::VSeparator()), false, false);
    my_engine_settings(hbox, false);
}

void layout_pixmap_box(SettingsTable& tbl, Gtk::Box& vbox, int b_t, bool active)
{
    (void) vbox;
    std::string pre = active ? "active" : "inactive";
    std::string key = pre + "_" + p_types[b_t];

    tbl.append(*Gtk::manage(new Gtk::Label(names[b_t])), false);
    std::string fc_title = std::string(names[b_t]) + " Pixmap";
    auto& filesel = *Gtk::manage(new Gtk::FileChooserButton(fc_title, Gtk::FILE_CHOOSER_ACTION_OPEN));
    tbl.append(filesel, false);

    Gtk::FileFilter filter;
    filter.set_name("Images");
    filter.add_pixbuf_formats();
    filesel.add_filter(filter);

    auto& scroller = *Gtk::manage(new Gtk::ScrolledWindow());
    scroller.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scroller.set_size_request(150, 50);

    auto& image = *Gtk::manage(new Gtk::Image());
    auto* item = SettingItem::register_img_file_setting(filesel, "pixmaps",
                                                        key.c_str(), &image);
    scroller.add(image);
    tbl.append(scroller, true);

    auto& clearer = *Gtk::manage(new Gtk::Button(Gtk::Stock::CLEAR));
    clearer.signal_clicked().connect([item](){ cb_clear_file(item); });
    tbl.append(clearer, false);

    // Style : Use Tiled or Scaled pixmaps
    auto& use_scaled = *Gtk::manage(new Gtk::ToggleButton(_("Scaled")));
    SettingItem::create(use_scaled, SECT, (key + "_use_scaled").c_str());
    tbl.append(use_scaled, false);

    // Width : Checkbox (Use my width) + Number (0-500)
    if (b_t == 0 || b_t == 5 || b_t == 8) {
        tbl.append(*Gtk::manage(new Gtk::Label(_("Not adjustable"))), false);
    } else {
        auto& width = *Gtk::manage(new Gtk::SpinButton(1));
        width.set_range(0, 500);
        SettingItem::create(width, SECT, (key + "_%s_width").c_str());

        auto& use_my_width = *Gtk::manage(new Gtk::CheckButton(""));
        SettingItem::create(use_my_width, SECT, (key + "_use_width").c_str());

        auto& tbox = *Gtk::manage(new Gtk::HBox(false, 2));
        tbox.pack_start(width, false, false);
        tbox.pack_start(use_my_width, false, false);
        tbl.append(tbox, false);
    }

    // Height : Checkbox (Use my width) + Number (0-500)
    if (b_t == 1 || b_t == 2 || b_t == 6 || b_t == 7) {
        auto& height = *Gtk::manage(new Gtk::SpinButton(1));
        height.set_range(0, 500);
        std::string key = pre + "_" + p_types[b_t] + "_height";
        SettingItem::create(height, SECT, key);

        auto& use_my_height = *Gtk::manage(new Gtk::CheckButton(""));
        key = pre + "_" + p_types[b_t] + "_use_height";
        SettingItem::create(use_my_height, SECT, key);

        auto& tbox = *Gtk::manage(new Gtk::HBox(false, 2));
        tbox.pack_start(height, false, false);
        tbox.pack_start(use_my_height, false, false);
        tbl.append(tbox, false);
    } else {
        tbl.append(*Gtk::manage(new Gtk::Label(_("Not adjustable"))), false);
    }
}

void layout_engine_pixmaps(Gtk::Box& vbox, bool active)
{
    auto& hbox = *Gtk::manage(new Gtk::HBox(true, 2));
    vbox.pack_start(hbox, false, false);

    if (!active) {
        auto& btn = *Gtk::manage(new Gtk::CheckButton(_("Same as Active")));
        hbox.pack_start(btn, true, true);
        SettingItem::create(btn, SECT, "inactive_use_active_pixmaps");
    }

    auto& scroller = *Gtk::manage(new Gtk::ScrolledWindow());
    scroller.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    vbox.pack_start(scroller, true, true);

    SettingsTable tbl(7, false, false);
    scroller.add(tbl.table());

    tbl.append(*Gtk::manage(new Gtk::Label(_("Pixmap"))), false);
    tbl.append(*Gtk::manage(new Gtk::Label(_("File"))), false);
    tbl.append(*Gtk::manage(new Gtk::Label(_("Preview"))), false);
    tbl.append(*Gtk::manage(new Gtk::Label(_("Clear"))), false);
    tbl.append(*Gtk::manage(new Gtk::Label(_("Tiled/Scaled"))), false);
    tbl.append(*Gtk::manage(new Gtk::Label(_("Width Override"))), false);
    tbl.append(*Gtk::manage(new Gtk::Label(_("Height Override"))), false);

    for (int i = 0; i < 11; i++) {
        layout_pixmap_box(tbl, vbox, i, active);
    }
}

extern "C"
void layout_engine_settings(Gtk::Box& vbox)
{
    auto* note = Gtk::manage(new Gtk::Notebook());
    vbox.pack_start(*note, true, true);
    layout_engine_pixmaps(*build_notebook_page("Pixmaps (Active)", *note), true);
    layout_engine_pixmaps(*build_notebook_page("Pixmaps (Inactive)", *note), false);
    layout_engine_colors(*build_notebook_page("Colors", *note));
    layout_corners_frame(*build_notebook_page("Frame", *note));
}

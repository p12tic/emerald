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

#include <libengine/engine.h>
#include <libengine/filesystem.h>

window_settings::window_settings() : // value-initialize everything
    engine_ws{},
    button_offset{},
    button_hoffset{},
    double_click_action{},
    button_hover_cursor{},
    round_top_left{},
    round_top_right{},
    round_bottom_left{},
    round_bottom_right{},
    fs_act{},
    fs_inact{},
    min_titlebar_height{},
    use_pixmap_buttons{},
    corner_radius{},
    title_text_align{},
    ButtonPix{},
    ButtonArray{},
    use_button_glow{},
    use_button_inactive_glow{},
    use_decoration_cropping{},
    use_button_fade{},
    ButtonGlowPix{},
    ButtonGlowArray{},
    ButtonInactiveGlowArray{},
    ButtonInactiveGlowPix{},
    button_fade_num_steps{},
    button_fade_step_duration{},
    button_fade_pulse_len_steps{},
    button_fade_pulse_wait_steps{},
    shadow_radius{},
    shadow_opacity{},
    shadow_color{},
    shadow_offset_x{},
    shadow_offset_y{},
    shadow_extents{},
    win_extents{},
    pos{},
    left_space{},
    right_space{},
    top_space{},
    bottom_space{},
    left_corner_space{},
    right_corner_space{},
    top_corner_space{},
    bottom_corner_space{},
    titlebar_height{},
    normal_top_corner_space{},
    shadow_left_space{},
    shadow_right_space{},
    shadow_top_space{},
    shadow_bottom_space{},
    shadow_left_corner_space{},
    shadow_right_corner_space{},
    shadow_top_corner_space{},
    shadow_bottom_corner_space{},
    shadow_pixmap{},
    large_shadow_pixmap{},
    decor_normal_pixmap{},
    decor_active_pixmap{},
    shadow_pattern{},
    text_height{},
    font_desc{},
    pango_context{},
    switcher_extents{},
    switcher_pixmap{},
    switcher_buffer_pixmap{},
    switcher_width{},
    switcher_height{},
    switcher_top_corner_space{},
    switcher_bottom_corner_space{},
    c_icon_size{},
    c_glow_size{},
    stretch_sides{},
    blur_type{}
{
}

decor_t::decor_t() : // value-initialize everything
    event_windows{},
    button_windows{},
    button_states{},
    tobj_pos{},
    tobj_size{},
    tobj_item_pos{},
    tobj_item_state{},
    tobj_item_width{},
    pixmap{},
    buffer_pixmap{},
    gc{},
    width{},
    height{},
    client_width{},
    client_height{},
    decorated{},
    active{},
    layout{},
    icon{},
    icon_pixmap{},
    icon_pixbuf{},
    state{},
    actions{},
    prop_xid{},
    force_quit_dialog{},
    fs{},
    draw{},
    button_region{},
    min_drawn_buttons_region{},
    draw_only_buttons_region{},
    button_last_drawn_state{},
    button_fade_info{},
    p_active{},
    p_active_buffer{},
    p_inactive{},
    p_inactive_buffer{},
    button_region_inact{},
    only_change_active{}
{
}

void copy_from_defaults_if_needed()
{
    fs::path homedir{g_get_home_dir()};

    fs::path cur_themes = homedir / ".emerald/theme";
    fs::create_directories(cur_themes);

    // copy the default settings.ini
    fs::path opath = homedir / ".emerald/settings.ini";
    fs::path orig_settings = DEFSETTINGSFILE;
    if (!fs::is_regular_file(opath)) {
        fs::copy_file(orig_settings, opath, fs::copy_option::overwrite_if_exists);
    }

    // copy the default themes
    opath = homedir / ".emerald/theme/theme.ini";
    if (!fs::is_regular_file(opath)) {
        fs::path def_themes = DEFTHEMEDIR;
        for (auto& entry : fs::directory_iterator{def_themes}) {
            fs::path dest = cur_themes / entry.path().filename();
            fs::copy_file(entry.path(), dest);
        }
    }
}

std::string make_filename(const std::string& sect, const std::string& key,
                          const std::string& ext)
{
    return std::string{g_get_home_dir()} + "/.emerald/theme/" + sect + '.' + key + '.' + ext;
}

void cairo_set_source_alpha_color(Cairo::RefPtr<Cairo::Context>& cr, alpha_color* c)
{
    cr->set_source_rgba(c->color.r, c->color.g, c->color.b, c->alpha);
}

void load_color_setting(const KeyFile& f, decor_color_t* color,
                        const std::string& key, const std::string& sect)
{
    GdkColor c;
    auto s = f.get_string_opt(sect, key);
    if (s) {
        gdk_color_parse(s->c_str(), &c);
        color->r = c.red / 65536.0;
        color->g = c.green / 65536.0;
        color->b = c.blue / 65536.0;
    }
}

void load_shadow_color_setting(const KeyFile& f, int sc[3], const std::string& key,
                               const std::string& sect)
{
    GdkColor c;
    auto s = f.get_string_opt(sect, key);
    if (s) {
        gdk_color_parse(s->c_str(), &c);
        sc[0] = c.red;
        sc[1] = c.green;
        sc[2] = c.blue;
    }
}

void load_float_setting(const KeyFile& f, double* d, const std::string& key,
                        const std::string& sect)
{
    auto s = f.get_string_opt(sect, key);
    if (s) {
        *d = g_ascii_strtod(s->c_str(), NULL);
    }
}

void load_int_setting(const KeyFile& f, int* i, const std::string& key,
                      const std::string& sect)
{
    auto ii = f.get_integer_opt(sect, key);
    if (!ii) {
        *i = *ii;
    }
}

void load_bool_setting(const KeyFile& f, bool* b, const std::string& key,
                       const std::string& sect)
{
    auto bb = f.get_boolean_opt(sect, key);
    if (bb) {
        *b = *bb;
    }
}

void load_font_setting(const KeyFile& f, PangoFontDescription** fd,
                       const std::string& key, const std::string& sect)
{
    auto s = f.get_string_opt(sect, key);
    if (s) {
        if (*fd) {
            pango_font_description_free(*fd);
        }
        *fd = pango_font_description_from_string(s->c_str());
    }
}

void load_string_setting(const KeyFile& f, std::string& s, const std::string& key,
                         const std::string& sect)
{
    auto ss = f.get_string_opt(sect, key);
    if (ss) {
        s = *ss;
    }
}

void
rounded_rectangle(Cairo::RefPtr<Cairo::Context>& cr,
                  double  x,
                  double  y,
                  double  w,
                  double  h,
                  int    corner,
                  window_settings* ws,
                  double  radius)
{
    (void) ws;
    if (radius == 0) {
        corner = 0;
    }

    if (corner & CORNER_TOPLEFT) {
        cr->move_to(x + radius, y);
    } else {
        cr->move_to(x, y);
    }

    if (corner & CORNER_TOPRIGHT)
        cr->arc(x + w - radius, y + radius, radius, M_PI * 1.5, M_PI * 2.0);
    else {
        cr->line_to(x + w, y);
    }

    if (corner & CORNER_BOTTOMRIGHT)
        cr->arc(x + w - radius, y + h - radius, radius, 0.0, M_PI * 0.5);
    else {
        cr->line_to(x + w, y + h);
    }

    if (corner & CORNER_BOTTOMLEFT)
        cr->arc(x + radius, y + h - radius, radius, M_PI * 0.5, M_PI);
    else {
        cr->line_to(x, y + h);
    }

    if (corner & CORNER_TOPLEFT) {
        cr->arc(x + radius, y + radius, radius, M_PI, M_PI * 1.5);
    } else {
        cr->line_to(x, y);
    }
}

void
fill_rounded_rectangle(Cairo::RefPtr<Cairo::Context>& cr,
                       double        x,
                       double        y,
                       double        w,
                       double        h,
                       int       corner,
                       alpha_color* c0,
                       alpha_color* c1,
                       int       gravity,
                       window_settings* ws,
                       double    radius)
{

    rounded_rectangle(cr, x, y, w, h, corner, ws, radius);

    if (gravity & SHADE_RIGHT) {
        x = x + w;
        w = -w;
    } else if (!(gravity & SHADE_LEFT)) {
        x = w = 0;
    }

    if (gravity & SHADE_BOTTOM) {
        y = y + h;
        h = -h;
    } else if (!(gravity & SHADE_TOP)) {
        y = h = 0;
    }

    Cairo::RefPtr<Cairo::Gradient> pat;
    if (w && h) {
        pat = Cairo::RadialGradient::create(0.0, 0.0, 0.0, 0.0, 0.0, w);

        auto mat = Cairo::scaling_matrix(1.0, w/h);
        mat.translate(-(x + w), -(y + h));
        pat->set_matrix(mat);
    } else {
        pat = Cairo::LinearGradient::create(x + w, y + h, x, y);
    }

    pat->add_color_stop_rgba(0.0, c0->color.r, c0->color.g, c0->color.b, c0->alpha);
    pat->add_color_stop_rgba(1.0, c1->color.r, c1->color.g, c1->color.b, c1->alpha);

    cr->set_source(pat);
    cr->fill();
}



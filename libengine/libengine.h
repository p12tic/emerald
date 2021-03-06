/*  This file is part of emerald

    Copyright (C) 2006  Novell, Inc.
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

#ifndef EMERALD_LIBENGINE_LIBENGINE_H
#define EMERALD_LIBENGINE_LIBENGINE_H

#include <libengine/emerald.h>
#include <libengine/setting_item.h>
#include <list>

void copy_from_defaults_if_needed();
void load_color_setting(const KeyFile& f, decor_color_t* color,
                        const std::string& key, const std::string& sect);
void load_shadow_color_setting(const KeyFile& f, int sc[3],
                               const std::string& key, const std::string& sect);
void load_float_setting(const KeyFile& f, double* d, const std::string& key,
                        const std::string& sect);
void load_int_setting(const KeyFile& f, int* i, const std::string& key,
                      const std::string& sect);
void load_bool_setting(const KeyFile& f, bool* b, const std::string& key,
                       const std::string& sect);
void load_font_setting(const KeyFile& f, Pango::FontDescription& fd,
                       const std::string& key, const std::string& sect);
void load_string_setting(const KeyFile& f, std::string& s, const std::string& key,
                         const std::string& sect);
void cairo_set_source_alpha_color(Cairo::RefPtr<Cairo::Context>& cr, alpha_color* c);
#define PFACS(f,ws,zc,SECT) \
    load_color_setting((f),&((private_fs *)(ws)->fs_act->engine_fs)->zc.color,"active_" #zc ,SECT);\
    load_color_setting((f),&((private_fs *)(ws)->fs_inact->engine_fs)->zc.color,"inactive_" #zc ,SECT);\
    load_float_setting((f),&((private_fs *)(ws)->fs_act->engine_fs)->zc.alpha,"active_" #zc "_alpha",SECT);\
    load_float_setting((f),&((private_fs *)(ws)->fs_inact->engine_fs)->zc.alpha,"inactive_" #zc "_alpha",SECT);

void
fill_rounded_rectangle(Cairo::RefPtr<Cairo::Context>&       cr,
                       double        x,
                       double        y,
                       double        w,
                       double        h,
                       int       corner,
                       alpha_color* c0,
                       alpha_color* c1,
                       int       gravity,
                       window_settings* ws,
                       double        radius);
void
rounded_rectangle(Cairo::RefPtr<Cairo::Context>& cr,
                  double  x,
                  double  y,
                  double  w,
                  double  h,
                  int    corner,
                  window_settings* ws,
                  double  radius);

//////////////////////////////////////////////////////
//themer stuff

#include <libengine/titlebar.h>

bool get_engine_meta_info(const std::string& engine, EngineMetaInfo* inf);   // returns false if couldn't find engine

Gtk::Scale* scaler_new(double low, double high, double prec);

Gtk::Box* build_frame(Gtk::Box& vbox, const std::string& title, bool is_hbox);

class SettingsTable {
public:
    SettingsTable(int width, bool same, bool labels);

    void append_acolor(const std::string& caption, const std::string& basekey,
                       const std::string& sect, bool active);
    void append_header(const std::string& header);

    void append(Gtk::Widget& child, bool stretch);
    void append_separator();

    Gtk::Table& table() { return *table_; }

private:
    Gtk::Table* table_;
    int width_;
    int col_;
    int row_;
};

void send_reload_signal();
void apply_settings();
void cb_apply_setting(SettingItem* item);

#ifdef USE_DBUS
void setup_dbus();
#endif

void update_preview(Gtk::FileChooser* fc, const std::string& filename,
                    Gtk::Image* img);
void update_preview_cb(Gtk::FileChooser* chooser, Gtk::Image* img);

void init_settings();
void set_changed(bool schanged);
void set_apply(bool sapply);
void cb_clear_file(SettingItem* item);
void init_key_files();
std::list<SettingItem>& get_setting_list();
void do_engine(const std::string& name);
Gtk::Box* build_notebook_page(const std::string& title, Gtk::Notebook& notebook);
std::string make_filename(const std::string& sect, const std::string& key,
                          const std::string& ext);
void layout_engine_list(Gtk::Box& vbox);
void init_engine_list();
#endif

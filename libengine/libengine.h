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
void load_font_setting(const KeyFile& f, PangoFontDescription** fd,
                       const std::string& key, const std::string& sect);
void load_string_setting(const KeyFile& f, std::string& s, const std::string& key,
                         const std::string& sect);
void cairo_set_source_alpha_color(cairo_t* cr, alpha_color* c);
#define PFACS(f,ws,zc,SECT) \
    load_color_setting((f),&((private_fs *)(ws)->fs_act->engine_fs)->zc.color,"active_" #zc ,SECT);\
    load_color_setting((f),&((private_fs *)(ws)->fs_inact->engine_fs)->zc.color,"inactive_" #zc ,SECT);\
    load_float_setting((f),&((private_fs *)(ws)->fs_act->engine_fs)->zc.alpha,"active_" #zc "_alpha",SECT);\
    load_float_setting((f),&((private_fs *)(ws)->fs_inact->engine_fs)->zc.alpha,"inactive_" #zc "_alpha",SECT);

void
fill_rounded_rectangle(cairo_t*       cr,
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
rounded_rectangle(cairo_t* cr,
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
void add_color_alpha_value(const std::string& caption, const std::string& basekey,
                           const std::string& sect, bool active);

void make_labels(const std::string& header);
Gtk::Box* build_frame(Gtk::Box& vbox, const std::string& title, bool is_hbox);

void table_new(int width, bool same, bool labels);
void table_append(Gtk::Widget& child, bool stretch);
void table_append_separator();
Gtk::Table& get_current_table();

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

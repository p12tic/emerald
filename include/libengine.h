#ifndef EMERALD_LIBENGINE_H
#define EMERALD_LIBENGINE_H

#include <emerald.h>
#include "setting_item.h"

void copy_from_defaults_if_needed();
void load_color_setting(GKeyFile* f, decor_color_t* color, const char* key,
                        const char* sect);
void load_shadow_color_setting(GKeyFile* f, int sc[3], const char* key,
                               const char* sect);
void load_float_setting(GKeyFile* f, double* d, const char* key,
                        const char* sect);
void load_int_setting(GKeyFile* f, int* i, const char* key, const char* sect);
void load_bool_setting(GKeyFile* f, bool* b, const char* key, const char* sect);
void load_font_setting(GKeyFile* f, PangoFontDescription** fd,
                       const char* key, const char* sect);
void load_string_setting(GKeyFile* f, char** s, const char* key, const char* sect);
void cairo_set_source_alpha_color(cairo_t* cr, alpha_color* c);
#define PFACS(zc) \
    load_color_setting(f,&((private_fs *)ws->fs_act->engine_fs)->zc.color,"active_" #zc ,SECT);\
    load_color_setting(f,&((private_fs *)ws->fs_inact->engine_fs)->zc.color,"inactive_" #zc ,SECT);\
    load_float_setting(f,&((private_fs *)ws->fs_act->engine_fs)->zc.alpha,"active_" #zc "_alpha",SECT);\
    load_float_setting(f,&((private_fs *)ws->fs_inact->engine_fs)->zc.alpha,"inactive_" #zc "_alpha",SECT);

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

#include <titlebar.h>

#define gtk_box_pack_startC(a,b,c,d,e) gtk_box_pack_start(GTK_BOX(a),GTK_WIDGET(b),c,d,e)
#define gtk_box_pack_endC(a,b,c,d,e) gtk_box_pack_end(GTK_BOX(a),b,c,d,e)
#define gtk_container_addC(a,b) gtk_container_add(GTK_CONTAINER(a),b)
#define gtk_container_set_border_widthC(a,b) gtk_container_set_border_width(GTK_CONTAINER(a),b)

bool get_engine_meta_info(const char* engine, EngineMetaInfo* inf);   // returns FALSE if couldn't find engine

GtkWidget* scaler_new(double low, double high, double prec);
void add_color_alpha_value(const char* caption, const char* basekey,
                           const char* sect, bool active);

void make_labels(const char* header);
GtkWidget* build_frame(GtkWidget* vbox, const char* title, bool is_hbox);
SettingItem* register_setting(GtkWidget* widget, SettingType type, char* section, char* key);
SettingItem* register_img_file_setting(GtkWidget* widget, const char* section,
                                       const char* key, GtkImage* image);
void table_new(int width, bool same, bool labels);
void table_append(GtkWidget* child, bool stretch);
void table_append_separator();
GtkTable* get_current_table();

void send_reload_signal();
void apply_settings();
void cb_apply_setting(GtkWidget* w, void* p);

#ifdef USE_DBUS
void setup_dbus();
#endif

void update_preview(GtkFileChooser* fc, char* filename, GtkImage* img);
void update_preview_cb(GtkFileChooser* file_chooser, void* data);

void init_settings();
void set_changed(bool schanged);
void set_apply(bool sapply);
void cb_clear_file(GtkWidget* button, void* p);
void init_key_files();
GSList* get_setting_list();
void do_engine(const char* nam);
GtkWidget* build_notebook_page(char* title, GtkWidget* notebook);
char* make_filename(const char* sect, const char* key, const char* ext);
void layout_engine_list(GtkWidget* vbox);
void init_engine_list();
#endif

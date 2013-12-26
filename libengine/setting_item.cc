/*  This file is part of emerald

    Copyright (C) 2006  Novell Inc.
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

#include "setting_item.h"
#include "libengine.h"
#include <list>

extern GKeyFile* global_theme_file;
extern GKeyFile* global_settings_file;
extern std::list<SettingItem> g_setting_list;
extern std::list<EngineData> g_engine_list;
extern GtkListStore* EngineModel;

char* globalStr = NULL;
char globalFloatStr[G_ASCII_DTOSTR_BUF_SIZE + 1];

void SettingItem::write_setting(void* p)
{
    GKeyFile* f = (GKeyFile*) p;
    switch (type_) {
    case ST_BOOL:
        g_key_file_set_boolean(f, section_, key_, get_bool());
        break;
    case ST_INT:
        g_key_file_set_integer(f, section_, key_, get_int());
        break;
    case ST_FLOAT:
        g_key_file_set_string(f, section_, key_, get_float_str());
        break;
    case ST_COLOR:
        g_key_file_set_string(f, section_, key_, get_color());
        break;
    case ST_FONT:
        g_key_file_set_string(f, section_, key_, get_font());
        break;
    case ST_META_STRING:
        g_key_file_set_string(f, section_, key_, get_string());
        break;
    case ST_STRING_COMBO:
        g_key_file_set_string(f, section_, key_, get_string_combo());
        break;
    case ST_IMG_FILE:
        //g_key_file_set_string(f,section_,key_,get_img_file(item));
    {
        char* s = g_strdup_printf("%s/.emerald/theme/%s.%s.png", g_get_home_dir(), section_, key_);
        GdkPixbuf* pbuf = gtk_image_get_pixbuf(image_);
        if (pbuf) {
            gdk_pixbuf_savev(pbuf, s, "png", NULL, NULL, NULL);
        } else {
            g_unlink(s); // to really clear out a clear'd image
        }
        g_free(s);
    }
    break;
    case ST_ENGINE_COMBO: {
        EngineMetaInfo emi;
        const char* active_engine = get_engine_combo();
        if (get_engine_meta_info(active_engine, &emi)) {
            g_key_file_set_string(f, "engine_version", active_engine, emi.version);
        }
        g_key_file_set_string(f, section_, key_, active_engine);
        do_engine(active_engine);
    }
    break;
    case ST_SFILE_INT:
        if (f == global_theme_file) {
            g_key_file_set_integer(global_settings_file, section_,
                                   key_, get_int());
            write_setting_file();
        }
        break;
    case ST_SFILE_BOOL:
        if (f == global_theme_file) {
            g_key_file_set_boolean(global_settings_file, section_,
                                   key_, get_bool());
            write_setting_file();
        }
        break;
    case ST_SFILE_INT_COMBO:
        if (f == global_theme_file) {
            g_key_file_set_integer(global_settings_file, section_,
                                   key_, get_sf_int_combo());
            write_setting_file();
        }
        break;
    default:
        break;
        //unhandled types
    }
}
void SettingItem::write_setting_file()
{
    char* file = g_strjoin("/", g_get_home_dir(), ".emerald/settings.ini", NULL);
    char* path = g_strjoin("/", g_get_home_dir(), ".emerald/", NULL);
    char* at;
    g_mkdir_with_parents(path, 00755);
    at = g_key_file_to_data(global_settings_file, NULL, NULL);
    if (at) {
        g_file_set_contents(file, at, -1, NULL);
        g_free(at);
    }
    g_free(file);
    g_free(path);
}
bool SettingItem::get_bool()
{
    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget_));
}
double SettingItem::get_float()
{
    if (!strcmp(G_OBJECT_TYPE_NAME(widget_), "GtkSpinButton")) {
        return gtk_spin_button_get_value((GtkSpinButton*)widget_);
    } else {
        return gtk_range_get_value(GTK_RANGE(widget_));
    }
}
int SettingItem::get_int()
{
    return get_float();
}
const char* SettingItem::get_float_str()
{
    g_ascii_dtostr(globalFloatStr, G_ASCII_DTOSTR_BUF_SIZE,
                   get_float());
    return globalFloatStr;
}
const char* SettingItem::get_color()
{
    GdkColor c;
    if (globalStr) {
        g_free(globalStr);
    }
    gtk_color_button_get_color(GTK_COLOR_BUTTON(widget_), &c);
    globalStr = g_strdup_printf("#%02x%02x%02x", c.red >> 8, c.green >> 8, c.blue >> 8);
    return globalStr;
}
const char* SettingItem::get_font()
{
    return gtk_font_button_get_font_name(GTK_FONT_BUTTON(widget_));
}
const char* SettingItem::get_string()
{
    return gtk_entry_get_text(GTK_ENTRY(widget_));
}
void SettingItem::check_file(const char* f)
{
    GdkPixbuf* p;
    p = gdk_pixbuf_new_from_file(f, NULL);
    if (p) {
        gtk_image_set_from_pixbuf(image_, p);
        gtk_image_set_from_pixbuf(preview_, p);
    } else {
        gtk_image_clear(image_);
        gtk_image_clear(preview_);
    }
    if (p) {
        g_object_unref(p);
    }
}
const char* SettingItem::get_img_file()
{
    return fvalue_;
}
const char* SettingItem::get_string_combo()
{
    const char* s;
    s = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(widget_))));
    if (strlen(s)) {
        return s;
    }
    s = "IT::HNXC:Default Layout (Blank Entry)";
    return s;
}

void SettingItem::set_engine_combo(const char* val)
{
    int i = 0;
    bool found = false;
    for (auto& item : g_engine_list) {
        if (strcmp(item.canname, val) == 0) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(widget_), i);
            found = true;
            break;
        }
        i++;
    }
    if (found) {
        do_engine(val);
    } else {
        i = 0;
        for (auto& item : g_engine_list) {
            if (strcmp(item.canname, "legacy") == 0) {
                gtk_combo_box_set_active(GTK_COMBO_BOX(widget_), i);
                break;
            }
            i++;
        }
        do_engine("legacy");
    }
}
const char* SettingItem::get_engine_combo()
{
    static char* s = NULL;
    GtkTreeIter i;
    if (s) {
        g_free(s);
    }
    //s = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget_));
    if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget_), &i)) {
        gtk_tree_model_get(GTK_TREE_MODEL(EngineModel), &i, ENGINE_COL_NAME, &s, -1);
        if (!strlen(s)) {
            g_free(s);
            s = g_strdup("legacy");
        }
    }
    return s;
}
int SettingItem::get_sf_int_combo()
{
    return gtk_combo_box_get_active(GTK_COMBO_BOX(widget_));
}
void SettingItem::set_img_file(const char* f)
{
    g_free(fvalue_);
    fvalue_ = g_strdup(f);
    gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(widget_), f);
    check_file(f);
}
void SettingItem::set_bool(bool b)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget_), b);
}
void SettingItem::set_float(double f)
{
    if (!strcmp(G_OBJECT_TYPE_NAME(widget_), "GtkSpinButton")) {
        gtk_spin_button_set_value((GtkSpinButton*)widget_, f);
    } else {
        gtk_range_set_value(GTK_RANGE(widget_), f);
    }
}
void SettingItem::set_int(int i)
{
    set_float(i);
}
void SettingItem::set_float_str(const char* s)
{
    set_float(g_ascii_strtod(s, NULL));
}
void SettingItem::set_color(const char* s)
{
    GdkColor c;
    gdk_color_parse(s, &c);
    gtk_color_button_set_color(GTK_COLOR_BUTTON(widget_), &c);
}
void SettingItem::set_font(const char* f)
{
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(widget_), f);
}
void SettingItem::set_string(const char* s)
{
    gtk_entry_set_text(GTK_ENTRY(widget_), s);
}
void SettingItem::set_string_combo(const char* s)
{
    gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(widget_))), s);
}
void SettingItem::set_sf_int_combo(int i)
{
    gtk_combo_box_set_active(GTK_COMBO_BOX(widget_), i);
}
void SettingItem::read_setting(void** p)
{
    GKeyFile* f = (GKeyFile*) p;
    GError* e = NULL;
    bool b;
    int i;
    char* s;
    switch (type_) {
    case ST_BOOL:
        b = g_key_file_get_boolean(f, section_, key_, &e);
        if (!e) {
            set_bool(b);
        }
        break;
    case ST_INT:
        i = g_key_file_get_integer(f, section_, key_, &e);
        if (!e) {
            set_int(i);
        }
        break;
    case ST_FLOAT:
        s = g_key_file_get_string(f, section_, key_, &e);
        if (!e && s) {
            set_float_str(s);
            g_free(s);
        }
        break;
    case ST_COLOR:
        s = g_key_file_get_string(f, section_, key_, &e);
        if (!e && s) {
            set_color(s);
            g_free(s);
        }
        break;
    case ST_FONT:
        s = g_key_file_get_string(f, section_, key_, &e);
        if (!e && s) {
            set_font(s);
            g_free(s);
        }
        break;
    case ST_META_STRING:
        s = g_key_file_get_string(f, section_, key_, &e);
        if (!e && s) {
            set_string(s);
            g_free(s);
        }
        break;
    case ST_STRING_COMBO:
        s = g_key_file_get_string(f, section_, key_, &e);
        if (!e && s) {
            set_string_combo(s);
            g_free(s);
        }
        break;
    case ST_IMG_FILE:
        /*s = g_key_file_get_string(f,section_,key_,&e);
        if (!e && s)
        {
            set_img_file(item,s);
            g_free(s);
        }*/
        s = g_strdup_printf("%s/.emerald/theme/%s.%s.png", g_get_home_dir(), section_, key_);
        set_img_file(s);
        g_free(s);
        break;
    case ST_ENGINE_COMBO:
        s = g_key_file_get_string(f, section_, key_, &e);
        if (!e && s) {
            set_engine_combo(s);
            g_free(s);
        }
        break;
    case ST_SFILE_INT:
        if (f == global_theme_file) {
            i = g_key_file_get_integer(global_settings_file,
                                       section_, key_, &e);
            if (!e) {
                set_int(i);
            }
        }
        break;
    case ST_SFILE_BOOL:
        if (f == global_theme_file) {
            b = g_key_file_get_boolean(global_settings_file,
                                       section_, key_, &e);
            if (!e) {
                set_bool(b);
            }
        }
        break;
    case ST_SFILE_INT_COMBO:
        if (f == global_theme_file) {
            i = g_key_file_get_integer(global_settings_file,
                                       section_, key_, &e);
            if (!e) {
                set_sf_int_combo(i);
            }
        }
        break;
    default:
        break;
        //unhandled types
    }
}

SettingItem* SettingItem::register_img_file_setting(GtkWidget* widget, const char* section,
                                                    const char* key, GtkImage* image)
{
    SettingItem* item = register_setting(widget, ST_IMG_FILE, section, key);
    gtk_file_chooser_button_set_width_chars(GTK_FILE_CHOOSER_BUTTON(widget), 0);
    item->image_ = image;
    item->preview_ = GTK_IMAGE(gtk_image_new());
    gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER(widget), GTK_WIDGET(item->preview_));
    g_signal_connect(widget, "update-preview", G_CALLBACK(update_preview_cb),
                     item->preview_);
    return item;
}
SettingItem* SettingItem::register_setting(GtkWidget* widget, SettingType type, const char* section, const char* key)
{
    g_setting_list.push_front(SettingItem());
    SettingItem* item = &g_setting_list.front();
    item->type_ = type;
    item->key_ = g_strdup(key);
    item->section_ = g_strdup(section);
    item->widget_ = widget;
    item->fvalue_ = g_strdup("");
    switch (item->type_) {
    case ST_BOOL:
    case ST_SFILE_BOOL:
        g_signal_connect(widget, "toggled",
                         G_CALLBACK(cb_apply_setting),
                         item);
        break;
    case ST_INT:
    case ST_SFILE_INT:
        g_signal_connect(widget, "value-changed",
                         G_CALLBACK(cb_apply_setting),
                         item);
        break;
    case ST_FLOAT:
        g_signal_connect(widget, "value-changed",
                         G_CALLBACK(cb_apply_setting),
                         item);
        break;
    case ST_COLOR:
        g_signal_connect(widget, "color-set",
                         G_CALLBACK(cb_apply_setting),
                         item);
        break;
    case ST_FONT:
        g_signal_connect(widget, "font-set",
                         G_CALLBACK(cb_apply_setting),
                         item);
        break;
    case ST_IMG_FILE:
        g_signal_connect(widget, "selection-changed",
                         G_CALLBACK(cb_apply_setting),
                         item);
        break;
    case ST_STRING_COMBO:
        g_signal_connect(gtk_bin_get_child(GTK_BIN(widget)), "changed",
                         G_CALLBACK(cb_apply_setting),
                         item);
        break;
    case ST_SFILE_INT_COMBO:
        g_signal_connect(widget, "changed",
                         G_CALLBACK(cb_apply_setting),
                         item);
        break;
    case ST_ENGINE_COMBO:
        g_signal_connect(widget, "changed",
                         G_CALLBACK(cb_apply_setting),
                         item);
    default:
        break;
        //unconnected types
    }
    return item;
}

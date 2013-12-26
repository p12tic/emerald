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

extern GKeyFile* global_theme_file;
extern GKeyFile* global_settings_file;
extern GSList* SettingList;
extern GSList* EngineList;
extern GtkListStore* EngineModel;

char* globalStr = NULL;
char globalFloatStr[G_ASCII_DTOSTR_BUF_SIZE + 1];

void write_setting(SettingItem* item, void* p)
{
    GKeyFile* f = (GKeyFile*) p;
    switch (item->type) {
    case ST_BOOL:
        g_key_file_set_boolean(f, item->section, item->key, get_bool(item));
        break;
    case ST_INT:
        g_key_file_set_integer(f, item->section, item->key, get_int(item));
        break;
    case ST_FLOAT:
        g_key_file_set_string(f, item->section, item->key, get_float_str(item));
        break;
    case ST_COLOR:
        g_key_file_set_string(f, item->section, item->key, get_color(item));
        break;
    case ST_FONT:
        g_key_file_set_string(f, item->section, item->key, get_font(item));
        break;
    case ST_META_STRING:
        g_key_file_set_string(f, item->section, item->key, get_string(item));
        break;
    case ST_STRING_COMBO:
        g_key_file_set_string(f, item->section, item->key, get_string_combo(item));
        break;
    case ST_IMG_FILE:
        //g_key_file_set_string(f,item->section,item->key,get_img_file(item));
    {
        char* s = g_strdup_printf("%s/.emerald/theme/%s.%s.png", g_get_home_dir(), item->section, item->key);
        GdkPixbuf* pbuf = gtk_image_get_pixbuf(item->image);
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
        const char* active_engine = get_engine_combo(item);
        if (get_engine_meta_info(active_engine, &emi)) {
            g_key_file_set_string(f, "engine_version", active_engine, emi.version);
        }
        g_key_file_set_string(f, item->section, item->key, active_engine);
        do_engine(active_engine);
    }
    break;
    case ST_SFILE_INT:
        if (f == global_theme_file) {
            g_key_file_set_integer(global_settings_file, item->section,
                                   item->key, get_int(item));
            write_setting_file();
        }
        break;
    case ST_SFILE_BOOL:
        if (f == global_theme_file) {
            g_key_file_set_boolean(global_settings_file, item->section,
                                   item->key, get_bool(item));
            write_setting_file();
        }
        break;
    case ST_SFILE_INT_COMBO:
        if (f == global_theme_file) {
            g_key_file_set_integer(global_settings_file, item->section,
                                   item->key, get_sf_int_combo(item));
            write_setting_file();
        }
        break;
    default:
        break;
        //unhandled types
    }
}
void write_setting_file()
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
bool get_bool(SettingItem* item)
{
    return gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(item->widget));
}
double get_float(SettingItem* item)
{
    if (!strcmp(G_OBJECT_TYPE_NAME(item->widget), "GtkSpinButton")) {
        return gtk_spin_button_get_value((GtkSpinButton*)item->widget);
    } else {
        return gtk_range_get_value(GTK_RANGE(item->widget));
    }
}
int get_int(SettingItem* item)
{
    return get_float(item);
}
const char* get_float_str(SettingItem* item)
{
    g_ascii_dtostr(globalFloatStr, G_ASCII_DTOSTR_BUF_SIZE,
                   get_float(item));
    return globalFloatStr;
}
const char* get_color(SettingItem* item)
{
    GdkColor c;
    if (globalStr) {
        g_free(globalStr);
    }
    gtk_color_button_get_color(GTK_COLOR_BUTTON(item->widget), &c);
    globalStr = g_strdup_printf("#%02x%02x%02x", c.red >> 8, c.green >> 8, c.blue >> 8);
    return globalStr;
}
const char* get_font(SettingItem* item)
{
    return gtk_font_button_get_font_name(GTK_FONT_BUTTON(item->widget));
}
const char* get_string(SettingItem* item)
{
    return gtk_entry_get_text(GTK_ENTRY(item->widget));
}
void check_file(SettingItem* item, char* f)
{
    GdkPixbuf* p;
    p = gdk_pixbuf_new_from_file(f, NULL);
    if (p) {
        gtk_image_set_from_pixbuf(item->image, p);
        gtk_image_set_from_pixbuf(item->preview, p);
    } else {
        gtk_image_clear(item->image);
        gtk_image_clear(item->preview);
    }
    if (p) {
        g_object_unref(p);
    }
}
const char* get_img_file(SettingItem* item)
{
    return item->fvalue;
}
const char* get_string_combo(SettingItem* item)
{
    const char* s;
    s = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(item->widget))));
    if (strlen(s)) {
        return s;
    }
    s = "IT::HNXC:Default Layout (Blank Entry)";
    return s;
}

void set_engine_combo(SettingItem* item, char* val)
{
    FindEngine fe;
    fe.canname = val;
    fe.found = FALSE;
    fe.i = 0;
    g_slist_foreach(EngineList, (GFunc) search_engine, &fe);
    if (fe.found) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(item->widget), fe.i);
    } else {
        fe.canname = "legacy";
        fe.found = FALSE;
        fe.i = 0;
        g_slist_foreach(EngineList, (GFunc) search_engine, &fe);
        if (fe.found) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(item->widget), fe.i);
        }
    }
    do_engine(fe.canname);
}
const char* get_engine_combo(SettingItem* item)
{
    static char* s = NULL;
    GtkTreeIter i;
    if (s) {
        g_free(s);
    }
    //s = gtk_combo_box_get_active_text(GTK_COMBO_BOX(item->widget));
    if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(item->widget), &i)) {
        gtk_tree_model_get(GTK_TREE_MODEL(EngineModel), &i, ENGINE_COL_NAME, &s, -1);
        if (!strlen(s)) {
            g_free(s);
            s = g_strdup("legacy");
        }
    }
    return s;
}
int get_sf_int_combo(SettingItem* item)
{
    return gtk_combo_box_get_active(GTK_COMBO_BOX(item->widget));
}
void set_img_file(SettingItem* item, char* f)
{
    g_free(item->fvalue);
    item->fvalue = g_strdup(f);
    gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(item->widget), f);
    check_file(item, f);
}
void set_bool(SettingItem* item, bool b)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(item->widget), b);
}
void set_float(SettingItem* item, double f)
{
    if (!strcmp(G_OBJECT_TYPE_NAME(item->widget), "GtkSpinButton")) {
        gtk_spin_button_set_value((GtkSpinButton*)item->widget, f);
    } else {
        gtk_range_set_value(GTK_RANGE(item->widget), f);
    }
}
void set_int(SettingItem* item, int i)
{
    set_float(item, i);
}
void set_float_str(SettingItem* item, char* s)
{
    set_float(item, g_ascii_strtod(s, NULL));
}
void set_color(SettingItem* item, char* s)
{
    GdkColor c;
    gdk_color_parse(s, &c);
    gtk_color_button_set_color(GTK_COLOR_BUTTON(item->widget), &c);
}
void set_font(SettingItem* item, char* f)
{
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(item->widget), f);
}
void set_string(SettingItem* item, char* s)
{
    gtk_entry_set_text(GTK_ENTRY(item->widget), s);
}
void set_string_combo(SettingItem* item, char* s)
{
    gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(item->widget))), s);
}
void set_sf_int_combo(SettingItem* item, int i)
{
    gtk_combo_box_set_active(GTK_COMBO_BOX(item->widget), i);
}
void read_setting(SettingItem* item, void** p)
{
    GKeyFile* f = (GKeyFile*) p;
    GError* e = NULL;
    bool b;
    int i;
    char* s;
    switch (item->type) {
    case ST_BOOL:
        b = g_key_file_get_boolean(f, item->section, item->key, &e);
        if (!e) {
            set_bool(item, b);
        }
        break;
    case ST_INT:
        i = g_key_file_get_integer(f, item->section, item->key, &e);
        if (!e) {
            set_int(item, i);
        }
        break;
    case ST_FLOAT:
        s = g_key_file_get_string(f, item->section, item->key, &e);
        if (!e && s) {
            set_float_str(item, s);
            g_free(s);
        }
        break;
    case ST_COLOR:
        s = g_key_file_get_string(f, item->section, item->key, &e);
        if (!e && s) {
            set_color(item, s);
            g_free(s);
        }
        break;
    case ST_FONT:
        s = g_key_file_get_string(f, item->section, item->key, &e);
        if (!e && s) {
            set_font(item, s);
            g_free(s);
        }
        break;
    case ST_META_STRING:
        s = g_key_file_get_string(f, item->section, item->key, &e);
        if (!e && s) {
            set_string(item, s);
            g_free(s);
        }
        break;
    case ST_STRING_COMBO:
        s = g_key_file_get_string(f, item->section, item->key, &e);
        if (!e && s) {
            set_string_combo(item, s);
            g_free(s);
        }
        break;
    case ST_IMG_FILE:
        /*s = g_key_file_get_string(f,item->section,item->key,&e);
        if (!e && s)
        {
            set_img_file(item,s);
            g_free(s);
        }*/
        s = g_strdup_printf("%s/.emerald/theme/%s.%s.png", g_get_home_dir(), item->section, item->key);
        set_img_file(item, s);
        g_free(s);
        break;
    case ST_ENGINE_COMBO:
        s = g_key_file_get_string(f, item->section, item->key, &e);
        if (!e && s) {
            set_engine_combo(item, s);
            g_free(s);
        }
        break;
    case ST_SFILE_INT:
        if (f == global_theme_file) {
            i = g_key_file_get_integer(global_settings_file,
                                       item->section, item->key, &e);
            if (!e) {
                set_int(item, i);
            }
        }
        break;
    case ST_SFILE_BOOL:
        if (f == global_theme_file) {
            b = g_key_file_get_boolean(global_settings_file,
                                       item->section, item->key, &e);
            if (!e) {
                set_bool(item, b);
            }
        }
        break;
    case ST_SFILE_INT_COMBO:
        if (f == global_theme_file) {
            i = g_key_file_get_integer(global_settings_file,
                                       item->section, item->key, &e);
            if (!e) {
                set_sf_int_combo(item, i);
            }
        }
        break;
    default:
        break;
        //unhandled types
    }
}

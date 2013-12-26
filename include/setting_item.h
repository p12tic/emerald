/*  This file is part of emerald

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

#ifndef EMERALD_SETTING_ITEM_H
#define EMERALD_SETTING_ITEM_H

#include "emerald.h"

enum SettingType {
    ST_BOOL,
    ST_INT,
    ST_FLOAT,
    ST_COLOR,
    ST_FONT,
    ST_META_STRING,
    ST_IMG_FILE,
    ST_STRING_COMBO,
    ST_SFILE_INT_COMBO,
    ST_ENGINE_COMBO,
    ST_SFILE_BOOL,
    ST_SFILE_INT,
    ST_NUM
};

class SettingItem {
public:

    void write_setting(void* p);
    static void write_setting_file();
    bool get_bool();
    double get_float();
    int get_int();
    const char* get_float_str();
    const char* get_color();
    const char* get_font();
    const char* get_string();
    void check_file(char* f);
    const char* get_file();
    const char* get_string_combo();
    int get_sf_int_combo();
    void set_file(char* f);
    void set_bool(bool b);
    void set_float(double f);
    void set_int(int i);
    void set_float_str(char* s);
    void set_color(char* s);
    void set_font(char* f);
    void set_string(char* s);
    void set_string_combo(char* s);
    void set_sf_int_combo(int i);
    void read_setting(void** p);
    const char* get_engine_combo();

    static SettingItem* register_img_file_setting(GtkWidget* widget, const char* section,
                                                  const char* key, GtkImage* image);
    static SettingItem* register_setting(GtkWidget* widget, SettingType type,
                                         char* section, char* key);

private:
    const char* get_img_file();
    void set_engine_combo(char* val);
    void set_img_file(char* f);

public:
    SettingType type_;
    char* key_;
    char* section_;
    GtkWidget* widget_;

    char* fvalue_;
    GtkImage* image_;
    GtkImage* preview_;
};

struct EngineMetaInfo {// TODO: move to a separate file
    char* description;
    char* version;
    char* last_compat;
    GdkPixbuf* icon;
};
struct EngineData { // TODO: move to a separate file
    const char* canname;
    char* dlname;
    GtkWidget* vbox;
    EngineMetaInfo meta;
};
struct FindEngine { // TODO: move to a separate file
    const char* canname;
    bool found;
    int i;
    EngineData* d;
};
enum EngineCol { // TODO: move to a separate file
    ENGINE_COL_DLNAME,
    ENGINE_COL_NAME,
    ENGINE_COL_VER,
    ENGINE_COL_LAST_COMPAT,
    ENGINE_COL_MARKUP,
    ENGINE_COL_ICON,
    ENGINE_COL_COUNT
};

void search_engine(EngineData& d, FindEngine& fe);

#endif

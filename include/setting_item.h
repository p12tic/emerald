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

struct SettingItem {
    SettingType type;
    char* key;
    char* section;
    GtkWidget* widget;

    char* fvalue;
    GtkImage* image;
    GtkImage* preview;
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

void write_setting(SettingItem* item, void* p);
void write_setting_file();
bool get_bool(SettingItem* item);
double get_float(SettingItem* item);
int get_int(SettingItem* item);
const char* get_float_str(SettingItem* item);
const char* get_color(SettingItem* item);
const char* get_font(SettingItem* item);
const char* get_string(SettingItem* item);
void check_file(SettingItem* item, char* f);
const char* get_file(SettingItem* item);
const char* get_string_combo(SettingItem* item);
int get_sf_int_combo(SettingItem* item);
void set_file(SettingItem* item, char* f);
void set_bool(SettingItem* item, bool b);
void set_float(SettingItem* item, double f);
void set_int(SettingItem* item, int i);
void set_float_str(SettingItem* item, char* s);
void set_color(SettingItem* item, char* s);
void set_font(SettingItem* item, char* f);
void set_string(SettingItem* item, char* s);
void set_string_combo(SettingItem* item, char* s);
void set_sf_int_combo(SettingItem* item, int i);
void read_setting(SettingItem* item, void** p);
const char* get_engine_combo(SettingItem* item);

void search_engine(EngineData* d, void* p);

#endif

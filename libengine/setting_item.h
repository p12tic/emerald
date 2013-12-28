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

#ifndef EMERALD_LIBENGINE_SETTING_ITEM_H
#define EMERALD_LIBENGINE_SETTING_ITEM_H

#include <libengine/emerald.h>

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

    void write_setting(KeyFile& f);
    static void write_setting_file();
    bool get_bool();
    double get_float();
    int get_int();
    std::string get_float_str();
    std::string get_color();
    std::string get_font();
    std::string get_string();
    void check_file(const std::string& f);
    std::string get_file();
    std::string get_string_combo();
    int get_sf_int_combo();
    void set_file(const std::string& f);
    void set_bool(bool b);
    void set_float(double f);
    void set_int(int i);
    void set_float_str(const std::string& s);
    void set_color(const std::string& s);
    void set_font(const std::string& f);
    void set_string(const std::string& s);
    void set_string_combo(const std::string& s);
    void set_sf_int_combo(int i);
    void read_setting(const KeyFile& f);
    std::string get_engine_combo();

    static SettingItem* register_img_file_setting(Gtk::FileChooserButton& widget, const std::string& section,
                                                  const std::string& key, Gtk::Image* image);

    static SettingItem* create_global(Gtk::ComboBoxText& widget,
                                      const std::string& section, const std::string& key);
    static SettingItem* create_engine(Gtk::ComboBox& widget,
                                      const std::string& section, const std::string& key);
    static SettingItem* create(Gtk::Entry& widget,
                               const std::string& section, const std::string& key);
    static SettingItem* create(Gtk::ComboBoxEntryText& widget,
                               const std::string& section, const std::string& key);
    static SettingItem* create(Gtk::FontButton& widget,
                               const std::string& section, const std::string& key);
    static SettingItem* create(Gtk::ColorButton& widget,
                               const std::string& section, const std::string& key);
    static SettingItem* create(Gtk::SpinButton& widget,
                               const std::string& section, const std::string& key);
    static SettingItem* create(Gtk::Range& widget,
                               const std::string& section, const std::string& key);
    static SettingItem* create_global(Gtk::Range& widget,
                                      const std::string& section, const std::string& key);
    static SettingItem* create(Gtk::ToggleButton& widget,
                               const std::string& section, const std::string& key);
    static SettingItem* create_global(Gtk::ToggleButton& widget,
                                      const std::string& section, const std::string& key);

private:
    static SettingItem* create_impl(Gtk::Widget& widget, SettingType type,
                                    const std::string& section, const std::string& key);

    std::string get_img_file();
    void set_engine_combo(const std::string& val);
    void set_img_file(const std::string& f);

public:
    SettingType type_;
    std::string key_;
    std::string section_;
    Gtk::Widget* widget_;

    std::string fvalue_;
    Gtk::Image* image_;
    Gtk::Image* preview_;
private:
    bool is_spin_button_; // if type is ST_FLOAT
};

struct EngineMetaInfo {// TODO: move to a separate file
    char* description;
    char* version;
    char* last_compat;
    Glib::RefPtr<Gdk::Pixbuf> icon;
};
struct EngineData { // TODO: move to a separate file
    const char* canname;
    char* dlname;
    Gtk::Box* vbox;
    EngineMetaInfo meta;
};

class EngineColumns :
    public Gtk::TreeModel::ColumnRecord	{
public:
    EngineColumns()
    {
        add(dlname);
        add(name);
        add(version);
        add(last_compat);
        add(markup);
        add(icon);
    }
    Gtk::TreeModelColumn<std::string> dlname;
    Gtk::TreeModelColumn<std::string> name;
    Gtk::TreeModelColumn<std::string> version;
    Gtk::TreeModelColumn<std::string> last_compat;
    Gtk::TreeModelColumn<std::string> markup;
    Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> icon;
};

extern EngineColumns g_engine_columns;

enum EngineCol { // TODO: move to a separate file
    ENGINE_COL_DLNAME,
    ENGINE_COL_NAME,
    ENGINE_COL_VER,
    ENGINE_COL_LAST_COMPAT,
    ENGINE_COL_MARKUP,
    ENGINE_COL_ICON,
    ENGINE_COL_COUNT
};

#endif

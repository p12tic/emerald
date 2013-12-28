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

#include <libengine/setting_item.h>
#include <libengine/libengine.h>
#include <libengine/filesystem.h>
#include <list>
#include <cstdlib>
#include <memory>

namespace fs = boost::filesystem;

extern std::shared_ptr<KeyFile> global_theme_file;
extern std::shared_ptr<KeyFile> global_settings_file;
extern std::list<SettingItem> g_setting_list;
extern std::list<EngineData> g_engine_list;

char* globalStr = NULL;
char globalFloatStr[G_ASCII_DTOSTR_BUF_SIZE + 1];

void SettingItem::write_setting(KeyFile& f)
{
    switch (type_) {
    case ST_BOOL:
        f.set_boolean(section_, key_, get_bool());
        break;
    case ST_INT:
        f.set_integer(section_, key_, get_int());
        break;
    case ST_FLOAT:
        f.set_string(section_, key_, get_float_str());
        break;
    case ST_COLOR:
        f.set_string(section_, key_, get_color());
        break;
    case ST_FONT:
        f.set_string(section_, key_, get_font());
        break;
    case ST_META_STRING:
        f.set_string(section_, key_, get_string());
        break;
    case ST_STRING_COMBO:
        f.set_string(section_, key_, get_string_combo());
        break;
    case ST_IMG_FILE:
        //g_key_file_set_string(f,section_,key_,get_img_file(item));
    {
        std::string fn = std::string{g_get_home_dir()} + "/.emerald/theme/" +
                         section_ + "." + key_ + ".png";
        auto pbuf = image_->get_pixbuf();
        if (pbuf) {
            pbuf->save(fn, "png");
        } else {
            std::remove(fn.c_str());
        }
    }
    break;
    case ST_ENGINE_COMBO: {
        EngineMetaInfo emi;
        std::string active_engine = get_engine_combo();
        if (get_engine_meta_info(active_engine, &emi)) {
            f.set_string("engine_version", active_engine, emi.version);
        }
        f.set_string(section_, key_, active_engine);
        do_engine(active_engine);
    }
    break;
    case ST_SFILE_INT:
        if (&f == global_theme_file.get()) {
            f.set_integer(section_, key_, get_int());
            write_setting_file();
        }
        break;
    case ST_SFILE_BOOL:
        if (&f == global_theme_file.get()) {
            f.set_boolean(section_, key_, get_bool());
            write_setting_file();
        }
        break;
    case ST_SFILE_INT_COMBO:
        if (&f == global_theme_file.get()) {
            f.set_integer(section_, key_, get_sf_int_combo());
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
    fs::path homedir = g_get_home_dir();
    fs::create_directories(homedir / ".emerald");

    fs::path file = homedir / ".emerald/settings.ini";
    Glib::file_set_contents(file.native(), global_settings_file->to_data());
}

bool SettingItem::get_bool()
{
    return ((Gtk::ToggleButton*) widget_)->get_active();
}

double SettingItem::get_float()
{
    if (is_spin_button_) {
        return ((Gtk::SpinButton*) widget_)->get_value();
    } else {
        return ((Gtk::Range*) widget_)->get_value();
    }
}

int SettingItem::get_int()
{
    return get_float();
}

std::string SettingItem::get_float_str()
{
    g_ascii_dtostr(globalFloatStr, G_ASCII_DTOSTR_BUF_SIZE,
                   get_float());
    return globalFloatStr;
}

std::string SettingItem::get_color()
{
    if (globalStr) {
        g_free(globalStr);
    }
    Gdk::Color c = ((Gtk::ColorButton*) widget_)->get_color();
    globalStr = g_strdup_printf("#%02x%02x%02x", c.get_red() >> 8,
                                c.get_green() >> 8, c.get_blue() >> 8);
    return globalStr;
}

std::string SettingItem::get_font()
{
    return ((Gtk::FontButton*) widget_)->get_font_name();
}

std::string SettingItem::get_string()
{
    return ((Gtk::Entry*) widget_)->get_text();
}

void SettingItem::check_file(const std::string& f)
{
    try { //FIXME: bad idea
        auto p = Gdk::Pixbuf::create_from_file(f); // throws on error
        image_->set(p);
        preview_->set(p);
    } catch (...) {
        image_->clear();
        preview_->clear();
    }
}

std::string SettingItem::get_img_file()
{
    return fvalue_.c_str();
}

std::string SettingItem::get_string_combo()
{
    auto *w = ((Gtk::Bin*) widget_)->get_child();
    std::string s = ((Gtk::Entry*) w)->get_text();
    if (s.empty()) {
        s = "IT::HNXC:Default Layout (Blank Entry)";
    }
    return s;
}

void SettingItem::set_engine_combo(const std::string& val)
{
    int i = 0;
    bool found = false;
    for (auto& item : g_engine_list) {
        if (item.canname == val) {
            ((Gtk::ComboBox*) widget_)->set_active(i);
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
                ((Gtk::ComboBox*) widget_)->set_active(i);
                break;
            }
            i++;
        }
        do_engine("legacy");
    }
}

std::string SettingItem::get_engine_combo()
{
    static std::string s;
    GtkTreeIter i;

    //s = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget_));
    Gtk::ComboBox* cv = widget_;
    auto it = cv->get_active();
    if (it) {
        s = (*it)[g_engine_columns.name];
        if (s == "") {
            s = "legacy";
        }
    }
    return s.c_str();
}

int SettingItem::get_sf_int_combo()
{
    return ((Gtk::ComboBox*) widget_)->get_active_row_number();
}

void SettingItem::set_img_file(const std::string& f)
{
    fvalue_ = f;
    ((Gtk::FileChooser*) widget_)->select_filename(f);
    check_file(f);
}

void SettingItem::set_bool(bool b)
{
    ((Gtk::ToggleButton*) widget_)->set_active(b);
}

void SettingItem::set_float(double f)
{
    if (is_spin_button_) {
        ((Gtk::SpinButton*) widget_)->set_value(f);
    } else {
        ((Gtk::Range*) widget_)->set_value(f);
    }
}

void SettingItem::set_int(int i)
{
    set_float(i);
}

void SettingItem::set_float_str(const std::string& s)
{
    set_float(g_ascii_strtod(s.c_str(), NULL));
}

void SettingItem::set_color(const std::string& s)
{
    Gdk::Color color;
    if (color.parse(s)) {
        ((Gtk::ColorButton*) widget_)->set_color(color);
    }
}

void SettingItem::set_font(const std::string& f)
{
    ((Gtk::FontButton*) widget_)->set_font_name(f);
}

void SettingItem::set_string(const std::string& s)
{
    ((Gtk::Entry*) widget_)->set_text(s);
}

void SettingItem::set_string_combo(const std::string& s)
{
    auto child = ((Gtk::Bin*) widget_)->get_child();
    ((Gtk::Entry*) child)->set_text(s);
}

void SettingItem::set_sf_int_combo(int i)
{
    ((Gtk::ComboBox*) widget_)->set_active(i);
}

void SettingItem::read_setting(const KeyFile& f)
{
    GError* e = NULL;
    bool b;
    int i;
    char* s;
    switch (type_) {
    case ST_BOOL: {
        auto opt = f.get_boolean_opt(section_, key_);
        if (opt) {
            set_bool(*opt);
        }
        break;
    }
    case ST_INT:
    case ST_FLOAT: {
        auto opt = f.get_integer_opt(section_, key_);
        if (opt) {
            set_int(*opt);
            break;
        }
        auto opt2 = f.get_string_opt(section_, key_);
        if (opt2) {
            set_float_str(opt2->c_str());
        }
        break;
    }
    case ST_COLOR: {
        auto opt = f.get_string_opt(section_, key_);
        if (opt) {
            set_color(opt->c_str());
        }
        break;
    }
    case ST_FONT: {
        auto opt = f.get_string_opt(section_, key_);
        if (opt) {
            set_font(opt->c_str());
        }
        break;
    }
    case ST_META_STRING: {
        auto opt = f.get_string_opt(section_, key_);
        if (opt) {
            set_string(opt->c_str());
        }
        break;
    }
    case ST_STRING_COMBO: {
        auto opt = f.get_string_opt(section_, key_);
        if (opt) {
            set_string_combo(opt->c_str());
        }
        break;
    }
    case ST_IMG_FILE: {
        /*s = g_key_file_get_string(f,section_,key_,&e);
        if (!e && s)
        {
            set_img_file(item,s);
            g_free(s);
        }*/
        std::string file = std::string{g_get_home_dir()} + "/.emerald/theme/"
                + section_ + "." + key_ + ".png";
        set_img_file(s);
        break;
    }
    case ST_ENGINE_COMBO: {
        auto opt = f.get_string_opt(section_, key_);
        if (opt) {
            set_engine_combo(opt->c_str());
        }
        break;
    }
    case ST_SFILE_INT: {
        if (&f == global_theme_file.get()) {
            auto opt = f.get_integer_opt(section_, key_);
            if (opt) {
                set_int(*opt);
            }
        }
        break;
    }
    case ST_SFILE_BOOL: {
        if (&f == global_theme_file.get()) {
            auto opt = f.get_boolean_opt(section_, key_);
            if (opt) {
                set_bool(*opt);
            }
        }
        break;
    }
    case ST_SFILE_INT_COMBO: {
        if (&f == global_theme_file.get()) {
            auto opt = f.get_integer_opt(section_, key_);
            if (opt) {
                set_sf_int_combo(*opt);
            }
        }
        break;
    }
    default:
        break;
        //unhandled types
    }
}

SettingItem* SettingItem::register_img_file_setting(Gtk::FileChooserButton& widget, const std::string& section,
                                                    const std::string& key, Gtk::Image* image)
{
    SettingItem* item = create_impl(widget, ST_IMG_FILE, section, key);
    widget.signal_selection_changed().connect(sigc::bind(&cb_apply_setting, item));
    widget.set_width_chars(0);
    item->image_ = image;
    item->preview_ = Gtk::manage(new Gtk::Image());
    widget.set_preview_widget(*(item->preview_));
    widget.signal_update_preview().connect(sigc::bind(&update_preview_cb, &widget, item->preview_));
    return item;
}

SettingItem* SettingItem::create_global(Gtk::ComboBoxText& widget,
                                        const std::string& section, const std::string& key)
{
    auto item = SettingItem::create_impl(widget, ST_SFILE_INT_COMBO, section, key);
    widget.signal_changed().connect(sigc::bind(&cb_apply_setting, item));
    return item;
}

SettingItem* SettingItem::create(Gtk::ComboBoxEntryText& widget,
                                 const std::string& section, const std::string& key)
{
    auto item = SettingItem::create_impl(widget, ST_STRING_COMBO, section, key);
    widget.signal_changed().connect(sigc::bind(&cb_apply_setting, item));
    return item;
}

SettingItem* SettingItem::create_engine(Gtk::ComboBox& widget,
                                        const std::string& section, const std::string& key)
{
    auto item = SettingItem::create_impl(widget, ST_ENGINE_COMBO, section, key);
    widget.signal_changed().connect(sigc::bind(&cb_apply_setting, item));
    return item;
}

SettingItem* SettingItem::create(Gtk::Entry& widget,
                                 const std::string& section, const std::string& key)
{
    auto item = SettingItem::create_impl(widget, ST_META_STRING, section, key);
    return item;
}

SettingItem* SettingItem::create(Gtk::FontButton& widget,
                                 const std::string& section, const std::string& key)
{
    auto item = SettingItem::create_impl(widget, ST_COLOR, section, key);
    widget.signal_font_set().connect(sigc::bind(&cb_apply_setting, item));
    return item;
}

SettingItem* SettingItem::create(Gtk::ColorButton& widget,
                                 const std::string& section, const std::string& key)
{
    auto item = SettingItem::create_impl(widget, ST_COLOR, section, key);
    widget.signal_color_set().connect(sigc::bind(&cb_apply_setting, item));
    return item;
}

SettingItem* SettingItem::create(Gtk::SpinButton& widget,
                                 const std::string& section, const std::string& key)
{
    auto item = SettingItem::create_impl(widget, ST_FLOAT, section, key);
    item->is_spin_button_ = true;
    widget.signal_value_changed().connect(sigc::bind(&cb_apply_setting, item));
    return item;
}

SettingItem* SettingItem::create(Gtk::Range& widget,
                                 const std::string& section, const std::string& key)
{
    auto item = SettingItem::create_impl(widget, ST_FLOAT, section, key);
    item->is_spin_button_ = false;
    widget.signal_value_changed().connect(sigc::bind(&cb_apply_setting, item));
    return item;
}

SettingItem* SettingItem::create_global(Gtk::Range& widget,
                                        const std::string& section, const std::string& key)
{
    auto item = SettingItem::create_impl(widget, ST_SFILE_INT, section, key);
    widget.signal_value_changed().connect(sigc::bind(&cb_apply_setting, item));
    return item;
}

SettingItem* SettingItem::create(Gtk::ToggleButton& widget,
                                 const std::string& section, const std::string& key)
{
    auto item = SettingItem::create_impl(widget, ST_BOOL, section, key);
    widget.signal_toggled().connect(sigc::bind(&cb_apply_setting, item));
    return item;
}

SettingItem* SettingItem::create_global(Gtk::ToggleButton& widget,
                                        const std::string& section, const std::string& key)
{
    auto item = SettingItem::create_impl(widget, ST_SFILE_BOOL, section, key);
    widget.signal_toggled().connect(sigc::bind(&cb_apply_setting, item));
    return item;
}


SettingItem* SettingItem::create_impl(Gtk::Widget& widget, SettingType type,
                                      const std::string& section, const std::string& key)
{
    g_setting_list.push_front(SettingItem());
    SettingItem* item = &g_setting_list.front();
    item->type_ = type;
    item->key_ = key;
    item->section_ = section;
    item->widget_ = &widget;
    item->fvalue_ = "";
    return item;
}


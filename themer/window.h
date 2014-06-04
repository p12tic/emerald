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

#ifndef EMERALD_THEMER_WINDOW_H
#define EMERALD_THEMER_WINDOW_H

#include <libengine/emerald.h>

struct FetcherInfo;

class ThemerWindow {
public:

    ThemerWindow(int install_file, const std::string& input_file);

    void theme_list_append(const std::string& value, const std::string& dir,
                           const std::string& fil);

    void theme_scan_dir(const std::string& dir, bool writable);
    void scroll_to_theme(const std::string& theme);
    void refresh_theme_list(const std::string& theme);
    void cb_refresh();
    bool confirm_dialog(const std::string& fmt, const std::string& val);
    void info_dialog(const std::string& val);
    void error_dialog(const std::string& val);
    void cb_load();
    std::string import_theme(const std::string& file_s);
    void export_theme(const std::string& file_s);
    void cb_save();
    void cb_delete(Gtk::Widget* w);
    bool cb_main_destroy(GdkEventAny*);
    void layout_button_box(Gtk::Box& vbox, int b_t);
    void layout_general_buttons_frame(Gtk::Box& hbox);
    void layout_button_pane(Gtk::Box& vbox);
    void layout_window_frame(Gtk::Box& vbox, bool active);
    void add_row(Gtk::Box& vbox, Gtk::Widget& item, const std::string& title);
    void add_color_button_row(Gtk::Box& vbox, const std::string& title,
                              const std::string& key, const std::string& sect);
    void add_int_range_row(Gtk::Box& vbox, const std::string& title,
                           const std::string& key,
                           int start, int end, const std::string& sect);
    void add_float_range_row(Gtk::Box& vbox, const std::string& title,
                             const std::string& key,
                             double start, double end, double prec,
                             const std::string& sect);
    void layout_shadows_frame(Gtk::Box& vbox);
    void layout_title_frame(Gtk::Box& vbox);
    void add_meta_string_value(const std::string& title,
                               const std::string& key);
    void cb_export();
    void layout_file_frame(Gtk::Box& vbox);
    void layout_info_frame(Gtk::Box& vbox);

    void add_border_slider(const std::string& text, const std::string& key,
                           int value);
    void layout_borders_frame(Gtk::Box& vbox);
    void layout_left_frame_pane(Gtk::Box& hbox);
    void layout_right_frame_pane(Gtk::Box& hbox);
    void layout_frame_pane(Gtk::Box& vbox);
    void layout_left_global_pane(Gtk::Box& hbox);
    void layout_right_global_pane(Gtk::Box& hbox);
    void layout_global_pane(Gtk::Box& vbox);
    void layout_screenshot_frame(Gtk::Box& vbox);
    void layout_left_theme_pane(Gtk::Box& hbox);
    void layout_right_theme_pane(Gtk::Box& hbox);
    void layout_theme_pane(Gtk::Box& vbox);
    void layout_settings_pane(Gtk::Box& vbox);
    void layout_engine_pane(Gtk::Box& vbox);
    void layout_lower_pane(Gtk::Box& vbox);
    Gtk::Box* build_lower_pane(Gtk::Box& vbox);
    void cb_refilter(Glib::RefPtr<Gtk::TreeModelFilter> filt);
    bool is_visible(const Gtk::TreeModel::const_iterator& iter, Gtk::Entry& e);
    void cb_clearbox(Gtk::Entry& w);
    void cb_import();
    Gtk::Widget* build_tree_view();
    void import_cache(Gtk::ProgressBar& progbar);
    bool watcher_func(FetcherInfo* fe);
    void fetch_svn();
    void fetch_gpl_svn();
    void fetch_ngpl_svn();
    void cb_quit();
    void layout_upper_pane(Gtk::Box& vbox);
    void layout_repo_pane(Gtk::Box& vbox);
    void layout_themes_pane(Gtk::Box& vbox);
    void create_filechooserdialog1(std::string input);
    void layout_main_window();

private:

    class ThemeColumns :
        public Gtk::TreeModel::ColumnRecord	{
    public:
        ThemeColumns()
        {
            add(name);
            add(engine);
            add(engine_version);
            add(creator);
            add(description);
            add(theme_version);
            add(suggested);
            add(markup);
            add(pixbuf);
        }

        Gtk::TreeModelColumn<std::string> name;
        Gtk::TreeModelColumn<std::string> engine;
        Gtk::TreeModelColumn<std::string> engine_version;
        Gtk::TreeModelColumn<std::string> creator;
        Gtk::TreeModelColumn<std::string> description;
        Gtk::TreeModelColumn<std::string> theme_version;
        Gtk::TreeModelColumn<std::string> suggested;
        Gtk::TreeModelColumn<std::string> markup;
        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> pixbuf;
    };

    ThemeColumns theme_columns_;
    Gtk::TreeView* theme_selector_;
    Glib::RefPtr<Gtk::ListStore> theme_model_;
    Glib::RefPtr<Gtk::TreeSelection> theme_select_;
    Gtk::Window* main_window_;
    Gtk::Entry* entry_box_;
    Gtk::Entry* version_entry_;
    Gtk::Button* reload_button_;
    Gtk::Button* delete_button_;
    Gtk::Button* import_button_;
    Gtk::Button* fetch_button_;
    Gtk::Button* fetch_button2_;
    Gtk::Button* export_button_;
    std::string svnpath;
    std::string themecache;

};

#endif

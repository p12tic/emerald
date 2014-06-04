/*  This file is part of emerald

    Copyright (C) 2006  Novell Inc.
    Copyright (C) 2013-2014  Povilas Kanapickas <povilas@radix.lt>

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

#ifndef EMERALD_THEMER_THEME_LIST_H
#define EMERALD_THEMER_THEME_LIST_H

#include <gtkmm.h>
#include <boost/optional.hpp>

/** Represents a theme list GUI element.
 */
class ThemeList : public Gtk::ScrolledWindow {
public:
    ThemeList();
    ~ThemeList();

    struct ThemeDesc {
        // if starts by '*' it's system theme, see how it's used in window.cc
        // FIXME: fix this hack
        std::string name;
        std::string engine;
        std::string engine_version;
        std::string creator;
        std::string description;
        std::string theme_version;
        std::string suggested;
        std::string markup;
        Glib::RefPtr<Gdk::Pixbuf> pixbuf;
    };

    /// Adds a new theme described by @a desc
    void add_theme(const ThemeDesc& desc);

    /// Removes all themes
    void clear();

    /// Returns the name of the selected theme, if any
    boost::optional<std::string> get_selected_theme_name() const;

    /** This signal is used to retrieve the text to filter the theme list by.
        By default the theme list is not filtered.
    */
    sigc::signal<std::string>& signal_get_filter_text();

    /// Signal, emitted whenever the theme selection changes
    Glib::SignalProxy0<void> signal_selection_changed();

    /// Scrolls to theme with @a name
    void scroll_to_theme(const std::string& name);

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
    Glib::RefPtr<Gtk::ListStore> theme_model_;
    Gtk::TreeView theme_selector_;
    Glib::RefPtr<Gtk::TreeSelection> theme_select_;
    sigc::signal<std::string> signal_filter_text_;

    bool is_visible(const Gtk::TreeModel::const_iterator& iter);

    void cb_refilter(Glib::RefPtr<Gtk::TreeModelFilter> filt);

};

#endif

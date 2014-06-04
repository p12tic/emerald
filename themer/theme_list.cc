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

#include "theme_list.h"
#include <cctype>

std::string default_filter_text()
{
    return "";
}

ThemeList::ThemeList() :
    theme_model_(Gtk::ListStore::create(theme_columns_)),
    theme_selector_(theme_model_)
{

    //auto filt = Gtk::TreeModelFilter::create(theme_model_);
    //filt->set_visible_func([=](){ return is_visible(); });
    //auto sort = Gtk::TreeModelSort::create(filt);
    //searchbox_->signal_changed().connect([=](){ cb_refilter(filt); });

    {
    auto &renderer = *Gtk::manage(new Gtk::CellRendererText());
    renderer.property_ellipsize().set_value(Pango::ELLIPSIZE_END);
    int id = theme_selector_.append_column("Theme", renderer);
    auto &column = *(theme_selector_.get_column(id-1));
    column.add_attribute(renderer.property_markup(), theme_columns_.markup);
    column.set_sort_column_id(theme_columns_.markup);
    column.set_resizable();
    column.set_reorderable();
    } {
    auto &renderer = *Gtk::manage(new Gtk::CellRendererPixbuf());
    renderer.set_alignment(0, 0.5);
    int id = theme_selector_.append_column("Screenshot", renderer);
    auto &column = *(theme_selector_.get_column(id-1));
    column.add_attribute(renderer.property_pixbuf(), theme_columns_.pixbuf);
    column.set_sort_column_id(theme_columns_.pixbuf);
    column.set_max_width(400);
    column.set_resizable();
    column.set_reorderable();
    } {
    auto &renderer = *Gtk::manage(new Gtk::CellRendererText());
    int id = theme_selector_.append_column("Up-to-Date", renderer);
    auto &column = *(theme_selector_.get_column(id-1));
    column.add_attribute(renderer.property_text(), theme_columns_.engine_version);
    column.set_sort_column_id(theme_columns_.engine_version);
    column.set_resizable();
    column.set_reorderable();
    } {
    auto &renderer = *Gtk::manage(new Gtk::CellRendererText());
    int id = theme_selector_.append_column("Engine", renderer);
    auto &column = *(theme_selector_.get_column(id-1));
    column.add_attribute(renderer.property_text(), theme_columns_.engine);
    column.set_sort_column_id(theme_columns_.engine);
    column.set_resizable();
    column.set_reorderable();
    }

    signal_filter_text_.connect(&default_filter_text);

    theme_select_ = theme_selector_.get_selection();
    theme_select_->set_mode(Gtk::SELECTION_SINGLE);

    theme_selector_.set_headers_clickable();
    theme_selector_.set_reorderable();
    add(theme_selector_);
}

ThemeList::~ThemeList() {}

void ThemeList::scroll_to_theme(const std::string& name)
{
    auto it = theme_model_->children().begin();
    auto it_end = theme_model_->children().end();
    for (; it != it_end; ++it) {
        std::string s = (*it)[theme_columns_.name];
        if (s == name) {
            Gtk::TreePath p = theme_model_->get_path(it);
            theme_selector_.scroll_to_row(p);
        }
    }
}

void ThemeList::add_theme(const ThemeDesc& desc)
{
    Gtk::TreeModel::Row row = *(theme_model_->append());
    row[theme_columns_.name] = desc.name;
    row[theme_columns_.engine] = desc.engine;
    row[theme_columns_.engine_version] = desc.engine_version;
    row[theme_columns_.creator] = desc.creator;
    row[theme_columns_.description] = desc.description;
    row[theme_columns_.theme_version] = desc.theme_version;
    row[theme_columns_.suggested] = desc.suggested;
    row[theme_columns_.markup] = desc.markup;
    row[theme_columns_.pixbuf] = desc.pixbuf;
}

void ThemeList::clear()
{
    theme_model_.clear();
}

boost::optional<std::string> ThemeList::get_selected_theme_name() const
{
    auto it = theme_select_->get_selected();
    if (!it) {
        return boost::none;
    }
    return boost::optional<std::string>((*it)[theme_columns_.name]);
}


sigc::signal<std::string>& ThemeList::signal_get_filter_text()
{
    return signal_filter_text_;
}

Glib::SignalProxy0<void> ThemeList::signal_selection_changed()
{
    return theme_select_->signal_changed();
}

void ThemeList::cb_refilter(Glib::RefPtr<Gtk::TreeModelFilter> filt)
{
    filt->refilter();
}

bool ThemeList::is_visible(const Gtk::TreeModel::const_iterator& iter)
{
    std::string tx = signal_filter_text_();
    if (tx.empty()) {
        return true;
    }
    for (auto& ch : tx) { ch = std::tolower(ch); }

    std::vector<std::string> strs;
    strs.push_back((*iter)[theme_columns_.name]);
    strs.push_back((*iter)[theme_columns_.engine]);
    strs.push_back((*iter)[theme_columns_.creator]);
    strs.push_back((*iter)[theme_columns_.description]);
    strs.push_back((*iter)[theme_columns_.suggested]);

    for (auto& str : strs) {
        for (auto& ch : str) { ch = std::tolower(ch); }
        if (str.find(tx) != std::string::npos) {
            return true;
        }
    }
    return false;
}

/*  This file is part of emerald

    Copyright (C) 2013  Povilas Kanapickas <povilas@radix.lt>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef EMERALD_KEYFILE_H
#define EMERALD_KEYFILE_H

#include <glibmm/keyfile.h>
#include <boost/optional.hpp>
#include <cstdint>

class KeyFile : public Glib::KeyFile {
public:

    /** Gets the value associated to the specific key. Equivalent to the
        corresponding function of Glib::KeyFile, the differences being that
        this function does not throw any exceptions and reports error conditions
        via an empty optional object.
    */
    boost::optional<Glib::ustring> get_value_opt(const Glib::ustring& key) const;
    boost::optional<Glib::ustring> get_value_opt(const Glib::ustring& group,
                                                 const Glib::ustring& key) const;
    boost::optional<Glib::ustring> get_string_opt(const Glib::ustring& key) const;
    boost::optional<Glib::ustring> get_string_opt(const Glib::ustring& group,
                                                  const Glib::ustring& key) const;
    boost::optional<int> get_integer_opt(const Glib::ustring& key) const;
    boost::optional<int> get_integer_opt(const Glib::ustring& group,
                                         const Glib::ustring& key) const;
    boost::optional<bool> get_boolean_opt(const Glib::ustring& key) const;
    boost::optional<bool> get_boolean_opt(const Glib::ustring& group,
                                          const Glib::ustring& key) const;
    boost::optional<std::int64_t> get_int64_opt(const Glib::ustring& key) const;
    boost::optional<std::int64_t> get_int64_opt(const Glib::ustring& group,
                                                const Glib::ustring& key) const;
    boost::optional<std::uint64_t> get_uint64_opt(const Glib::ustring& key) const;
    boost::optional<std::uint64_t> get_uint64_opt(const Glib::ustring& group,
                                                  const Glib::ustring& key) const;
    boost::optional<double> get_double_opt(const Glib::ustring& key) const;
    boost::optional<double> get_double_opt(const Glib::ustring& group,
                                           const Glib::ustring& key) const;

private:
    GKeyFile* cgobj() const { return const_cast<GKeyFile*>(gobj()); }
};

#endif

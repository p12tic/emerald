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

#include <libengine/keyfile.h>

boost::optional<Glib::ustring> char_to_opt(char* r)
{
    if (!r) {
        return boost::none;
    }
    Glib::ustring rs{r};
    g_free(r);
    return rs;
}

boost::optional<Glib::ustring> KeyFile::get_value_opt(const Glib::ustring& key) const
{
    GError* err = nullptr;
    char* r = g_key_file_get_value(cgobj(), nullptr, key.c_str(), &err);
    if (err) {
        g_error_free(err);
        return boost::none;
    }
    return char_to_opt(r);
}

boost::optional<Glib::ustring> KeyFile::get_value_opt(const Glib::ustring& group,
                                                      const Glib::ustring& key) const
{
    GError* err = nullptr;
    char* r = g_key_file_get_value(cgobj(), group.c_str(), key.c_str(), &err);
    if (err) {
        g_error_free(err);
        return boost::none;
    }
    return char_to_opt(r);
}

boost::optional<Glib::ustring> KeyFile::get_string_opt(const Glib::ustring& key) const
{
    GError* err = nullptr;
    char* r = g_key_file_get_string(cgobj(), nullptr, key.c_str(), &err);
    if (err) {
        g_error_free(err);
        return boost::none;
    }
    return char_to_opt(r);
}

boost::optional<Glib::ustring> KeyFile::get_string_opt(const Glib::ustring& group,
                                                       const Glib::ustring& key) const
{
    GError* err = nullptr;
    char* r = g_key_file_get_string(cgobj(), group.c_str(), key.c_str(), &err);
    if (err) {
        g_error_free(err);
        return boost::none;
    }
    return char_to_opt(r);
}

boost::optional<int> KeyFile::get_integer_opt(const Glib::ustring& key) const
{
    GError* err = nullptr;
    int r = g_key_file_get_integer(cgobj(), nullptr, key.c_str(), &err);
    if (err) {
        g_error_free(err);
        return boost::none;
    }
    return r;
}

boost::optional<int> KeyFile::get_integer_opt(const Glib::ustring& group,
                                              const Glib::ustring& key) const
{
    GError* err = nullptr;
    int r = g_key_file_get_integer(cgobj(), group.c_str(), key.c_str(), &err);
    if (err) {
        g_error_free(err);
        return boost::none;
    }
    return r;
}

boost::optional<bool> KeyFile::get_boolean_opt(const Glib::ustring& key) const
{
    GError* err = nullptr;
    bool r = g_key_file_get_boolean(cgobj(), nullptr, key.c_str(), &err);
    if (err) {
        g_error_free(err);
        return boost::none;
    }
    return r;
}

boost::optional<bool> KeyFile::get_boolean_opt(const Glib::ustring& group,
                                               const Glib::ustring& key) const
{
    GError* err = nullptr;
    bool r = g_key_file_get_boolean(cgobj(), group.c_str(), key.c_str(), &err);
    if (err) {
        g_error_free(err);
        return boost::none;
    }
    return r;
}

boost::optional<std::int64_t> KeyFile::get_int64_opt(const Glib::ustring& key) const
{
    GError* err = nullptr;
    std::int64_t r = g_key_file_get_int64(cgobj(), nullptr, key.c_str(), &err);
    if (err) {
        g_error_free(err);
        return boost::none;
    }
    return r;
}

boost::optional<std::int64_t> KeyFile::get_int64_opt(const Glib::ustring& group,
                                                     const Glib::ustring& key) const
{
    GError* err = nullptr;
    std::int64_t r = g_key_file_get_int64(cgobj(), group.c_str(), key.c_str(), &err);
    if (err) {
        g_error_free(err);
        return boost::none;
    }
    return r;
}

boost::optional<std::uint64_t> KeyFile::get_uint64_opt(const Glib::ustring& key) const
{
    GError* err = nullptr;
    std::uint64_t r = g_key_file_get_uint64(cgobj(), nullptr, key.c_str(), &err);
    if (err) {
        g_error_free(err);
        return boost::none;
    }
    return r;
}

boost::optional<std::uint64_t> KeyFile::get_uint64_opt(const Glib::ustring& group,
                                                       const Glib::ustring& key) const
{
    GError* err = nullptr;
    std::uint64_t r = g_key_file_get_uint64(cgobj(), group.c_str(), key.c_str(), &err);
    if (err) {
        g_error_free(err);
        return boost::none;
    }
    return r;
}

boost::optional<double> KeyFile::get_double_opt(const Glib::ustring& key) const
{
    GError* err = nullptr;
    double r = g_key_file_get_double(cgobj(), nullptr, key.c_str(), &err);
    if (err) {
        g_error_free(err);
        return boost::none;
    }
    return r;
}

boost::optional<double> KeyFile::get_double_opt(const Glib::ustring& group,
                                                const Glib::ustring& key) const
{
    GError* err = nullptr;
    double r = g_key_file_get_double(cgobj(), group.c_str(), key.c_str(), &err);
    if (err) {
        g_error_free(err);
        return boost::none;
    }
    return r;
}


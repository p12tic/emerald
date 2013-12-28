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

#ifndef EMERALD_FILESYSTEM_H
#define EMERALD_FILESYSTEM_H

// Ubuntu compiles the boost libraries in C++03 mode, thus we must not use
// some C++11 features to avoid linker errors
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace std {
// The following is needed to support C++11 range-for loop if using boost < 1.55
inline const fs::directory_iterator& begin(const fs::directory_iterator& it)
{
    return it;
}
inline fs::directory_iterator& begin(fs::directory_iterator& it)
{
    return it;
}

inline fs::directory_iterator end(const fs::directory_iterator&)
{
    return fs::directory_iterator{};
}
inline fs::directory_iterator end(fs::directory_iterator&)
{
    return fs::directory_iterator{};
}

} // namespace std

#endif

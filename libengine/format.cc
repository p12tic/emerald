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

#include <libengine/format.h>

template class boost::basic_format<char>;
template class FormatWrapper<char, std::char_traits<char>, std::allocator<char>>;
template std::basic_ostream<char>&
    operator<<(std::basic_ostream<char>& ost, const FormatWrapper<char>& tr);
template boost::basic_format<char>& boost::basic_format<char>::operator%(const char&);
template boost::basic_format<char>& boost::basic_format<char>::operator%(const char*&);
template boost::basic_format<char>& boost::basic_format<char>::operator%(const short&);
//template boost::basic_format<char>& boost::basic_format<char>::operator%(const int&);
template boost::basic_format<char>& boost::basic_format<char>::operator%(const long&);
template boost::basic_format<char>& boost::basic_format<char>::operator%(const long long&);
template boost::basic_format<char>& boost::basic_format<char>::operator%(const unsigned short&);
template boost::basic_format<char>& boost::basic_format<char>::operator%(const unsigned int&);
template boost::basic_format<char>& boost::basic_format<char>::operator%(const unsigned long&);
template boost::basic_format<char>& boost::basic_format<char>::operator%(const unsigned long long&);
template boost::basic_format<char>& boost::basic_format<char>::operator%(const float&);
template boost::basic_format<char>& boost::basic_format<char>::operator%(const double&);
template boost::basic_format<char>& boost::basic_format<char>::operator%(const std::string&);

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

#ifndef EMERALD_LIBENGINE_FORMAT_H
#define EMERALD_LIBENGINE_FORMAT_H

#include <boost/format.hpp>

// Implicit conversion to std::string sounds convenient
template<class CharT, class Traits = std::char_traits<CharT>,
         class Alloc = std::allocator<CharT> >
class FormatWrapper {
public:
    using Format = boost::basic_format<CharT,Traits,Alloc>;

    FormatWrapper(const Format& x) : wrapped_{x} {}
    Format& format() { return wrapped_; }
    const Format& format() const { return wrapped_; }

    operator std::string() { return wrapped_.str(); }

private:
    Format wrapped_;
};

template<class CharT, class Traits, class Alloc>
std::basic_ostream<CharT,Traits>&
    operator<<(std::basic_ostream<CharT,Traits>& ost,
               const FormatWrapper<CharT,Traits,Alloc>& tr)
{
    ost << tr.format();
    return ost;
}

// Wraps boost::format into printf-like function (up to 5 const arguments)
// std::string overloads
template<class CharT, class Traits, class Alloc>
FormatWrapper<CharT,Traits,Alloc>
    format(const std::string& str)
{
    return boost::basic_format<CharT,Traits,Alloc>{str};
}

template<class CharT, class Traits, class Alloc, class T1>
FormatWrapper<CharT,Traits,Alloc>
    format(const std::basic_string<CharT,Traits,Alloc>& str,
           const T1& t1)
{
    return boost::basic_format<CharT,Traits,Alloc>{str} % t1;
}

template<class CharT, class Traits, class Alloc, class T1, class T2>
FormatWrapper<CharT,Traits,Alloc>
    format(const std::basic_string<CharT,Traits,Alloc>& str,
           const T1& t1, const T2& t2)
{
    return boost::basic_format<CharT,Traits,Alloc>{str} % t1 % t2;
}

template<class CharT, class Traits, class Alloc, class T1, class T2, class T3>
FormatWrapper<CharT,Traits,Alloc>
    format(const std::basic_string<CharT,Traits,Alloc>& str,
           const T1& t1, const T2& t2, const T3& t3)
{
    return boost::basic_format<CharT,Traits,Alloc>{str} % t1 % t2 % t3;
}

template<class CharT, class Traits, class Alloc,
         class T1, class T2, class T3, class T4>
FormatWrapper<CharT,Traits,Alloc>
    format(const std::basic_string<CharT,Traits,Alloc>& str,
           const T1& t1, const T2& t2, const T3& t3, const T4& t4)
{
    return boost::basic_format<CharT,Traits,Alloc>{str} % t1 % t2 % t3 % t4;
}

template<class CharT, class Traits, class Alloc,
         class T1, class T2, class T3, class T4, class T5>
FormatWrapper<CharT,Traits,Alloc>
    format(const std::basic_string<CharT,Traits,Alloc>& str,
           const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5)
{
    return boost::basic_format<CharT,Traits,Alloc>{str} % t1 % t2 % t3 % t4 % t5;
}

// char* overloads
template<class CharT, class Traits = std::char_traits<CharT>,
         class Alloc = std::allocator<CharT>,
         class T1, class T2, class T3>
FormatWrapper<CharT,Traits,Alloc>
    format(const CharT* str)
{
    return boost::basic_format<CharT,Traits,Alloc>{str};
}

template<class CharT, class Traits = std::char_traits<CharT>,
         class Alloc = std::allocator<CharT>,
         class T1>
FormatWrapper<CharT,Traits,Alloc>
    format(const CharT* str,
           const T1& t1)
{
    return boost::basic_format<CharT,Traits,Alloc>{str} % t1;
}
template<class CharT, class Traits = std::char_traits<CharT>,
         class Alloc = std::allocator<CharT>,
         class T1, class T2>
FormatWrapper<CharT,Traits,Alloc>
    format(const CharT* str,
           const T1& t1, const T2& t2)
{
    return boost::basic_format<CharT,Traits,Alloc>{str} % t1 % t2;
}

template<class CharT, class Traits = std::char_traits<CharT>,
         class Alloc = std::allocator<CharT>,
         class T1, class T2, class T3>
FormatWrapper<CharT,Traits,Alloc>
    format(const CharT* str,
           const T1& t1, const T2& t2, const T3& t3)
{
    return boost::basic_format<CharT,Traits,Alloc>{str} % t1 % t2 % t3;
}

template<class CharT, class Traits = std::char_traits<CharT>,
         class Alloc = std::allocator<CharT>,
         class T1, class T2, class T3, class T4>
FormatWrapper<CharT,Traits,Alloc>
    format(const CharT* str,
           const T1& t1, const T2& t2, const T3& t3, const T4& t4)
{
    return boost::basic_format<CharT,Traits,Alloc>{str} % t1 % t2 % t3 % t4;
}

template<class CharT, class Traits = std::char_traits<CharT>,
         class Alloc = std::allocator<CharT>,
         class T1, class T2, class T3, class T4, class T5>
FormatWrapper<CharT,Traits,Alloc>
    format(const CharT* str,
           const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5)
{
    return boost::basic_format<CharT,Traits,Alloc>{str} % t1 % t2 % t3 % t4 % t5;
}

// Instantiate some templates elsewhere to reduce compile times
extern template class boost::basic_format<char>;
extern template class FormatWrapper<char>;
extern template std::basic_ostream<char>&
    operator<<(std::basic_ostream<char>& ost, const FormatWrapper<char>& tr);
extern template boost::basic_format<char>& boost::basic_format<char>::operator%(const char&);
extern template boost::basic_format<char>& boost::basic_format<char>::operator%(const char*&);
extern template boost::basic_format<char>& boost::basic_format<char>::operator%(const short&);
// Boost has a 'workaround' for GNUC
//extern template boost::basic_format<char>& boost::basic_format<char>::operator%(const int&);
extern template boost::basic_format<char>& boost::basic_format<char>::operator%(const long&);
extern template boost::basic_format<char>& boost::basic_format<char>::operator%(const long long&);
extern template boost::basic_format<char>& boost::basic_format<char>::operator%(const unsigned short&);
extern template boost::basic_format<char>& boost::basic_format<char>::operator%(const unsigned int&);
extern template boost::basic_format<char>& boost::basic_format<char>::operator%(const unsigned long&);
extern template boost::basic_format<char>& boost::basic_format<char>::operator%(const unsigned long long&);
extern template boost::basic_format<char>& boost::basic_format<char>::operator%(const float&);
extern template boost::basic_format<char>& boost::basic_format<char>::operator%(const double&);
extern template boost::basic_format<char>& boost::basic_format<char>::operator%(const std::string&);

#endif

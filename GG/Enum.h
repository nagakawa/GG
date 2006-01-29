// -*- C++ -*-
/* GG is a GUI for SDL and OpenGL.
   Copyright (C) 2003 T. Zachary Laine

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1
   of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
    
   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA

   If you do not wish to comply with the terms of the LGPL please
   contact the author as other terms are available for a fee.
    
   Zach Laine
   whatwasthataddress@hotmail.com */
   
/* $Header$ */

/** \file Enum.h
    Contains the utility classes and macros that allow for easy conversion to and from an enum 
    value and its textual representation. */

#ifndef _GG_Enum_h_
#define _GG_Enum_h_

#include <map>
#include <string>

namespace GG {

/** A base type for all templated EnumMap types. */
struct EnumMapBase
{
    enum {BAD_VALUE = -5000000};

    virtual ~EnumMapBase() {}
    virtual const std::string& FromEnum(int) const = 0;
    virtual int FromString (const std::string&) const = 0;
};

/** A mapping between the values of an enum and the string representations of the enum's values.  A specialization
    should be declared for each enumerated type for which an EnumMap is desired. */
template <class E> struct EnumMap : EnumMapBase
{
    virtual ~EnumMap() {}
    virtual const std::string& FromEnum(int) const
    { static std::string empty; return empty; }
    virtual int FromString (const std::string&) const {return 0;}
};

/** Returns a map of the values of an enum to the corresponding string representation of that value. */
template <class E> EnumMap<E> GetEnumMap()
{
    static EnumMap<E> enum_map;
    return enum_map;
}

/** Declares the beginning of a template specialization of EnumMap, for enumerated type \a name.
    Text-to-enum conversion is one of those places that calls for macro magic.  To use these for 
    e.g. "enum Foo {FOO, BAR};", call: 
    \verbatim 
    GG_ENUM_MAP_BEGIN( Foo ) 
        GG_ENUM_MAP_INSERT( FOO )
        GG_ENUM_MAP_INSERT( BAR )
        ...
    GG_ENUM_MAP_END \endverbatim */
#define GG_ENUM_MAP_BEGIN( name )                                               \
template <> struct EnumMap< name > : EnumMapBase                                \
{                                                                               \
    typedef name EnumType;                                                      \
    typedef std::map<EnumType, std::string> MapType;                            \
    EnumMap ()                                                                  \
    {

/** Adds a single value from an enumerated type, and its corresponding string representation, to an EnumMap. */
#define GG_ENUM_MAP_INSERT( value ) m_map[ value ] = #value ;

/** Declares the end of a template specialization of EnumMap, for enumerated type \a name. */
#define GG_ENUM_MAP_END                                                         \
    }                                                                           \
    virtual const std::string& FromEnum(int i) const                            \
    {                                                                           \
        static const std::string error_str;                                     \
        std::map<EnumType, std::string>::const_iterator it =                    \
            m_map.find(EnumType(i));                                            \
        return it == m_map.end() ? error_str : it->second;                      \
    }                                                                           \
    int FromString (const std::string &str) const                               \
    {                                                                           \
        for (MapType::const_iterator it = m_map.begin();                        \
             it != m_map.end();                                                 \
             ++it) {                                                            \
            if (it->second == str) {                                            \
                return it->first;                                               \
            }                                                                   \
        }                                                                       \
        return BAD_VALUE;                                                       \
    }                                                                           \
    MapType m_map;                                                              \
};

/** Defines an input stream operator for enumerated type \a name.  Note that the generated function requires that EnumMap<name> be defined. */
#define GG_ENUM_STREAM_IN( name )                                               \
inline std::istream& operator>>(std::istream& is, name& v)                      \
{                                                                               \
    std::string str;                                                            \
    is >> str;                                                                  \
    v = name (GG::GetEnumMap< name >().FromString(str));                        \
    return is;                                                                  \
}

/** Defines an output stream operator for enumerated type \a name.  Note that the generated function requires that EnumMap<name> be defined. */
#define GG_ENUM_STREAM_OUT( name )                                              \
inline std::ostream& operator<<(std::ostream& os, name v)                       \
{                                                                               \
    os << GG::GetEnumMap< name >().FromEnum(v);                                 \
    return os;                                                                  \
}

} // namespace GG

#endif // _GG_Enum_h_
/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_UTF16_UTIL_H___
#define __INCLUDED_UTF16_UTIL_H___

/**
 * \file utf16_util.h
 * \author Victor Schappert
 * \since 20140406
 * \brief Utility types for working with 16-bit characters.
 */

#include <ostream>

namespace jsdi {

//==============================================================================
//                           typedef utf16char_t
//==============================================================================

/**
 * \brief Guaranteed to be a 16-bit wide character type.
 * \author Victor Schappert
 * \since 20140406
 *
 * There are two reasons to use this <dfn>typedef</dfn> instead of
 * <dfn>char16_t</dfn>: first, <dfn>char16_t</dfn> and the <dfn>u""</dfn>
 * notation is basically unsupported by Microsoft as late as Visual Studio 2013;
 * second, the 
 */
typedef wchar_t utf16char_t;

//==============================================================================
//                              #define UTF16
//==============================================================================

/** \cond internal */
#define UTF16_(string_literal) L ## string_literal
/** \endcond */

/**
 * \brief Converts a symbol representing an ordinary string literal to a
 *        UTF-16 string literal
 * \author Victor Schappert
 * \since 20140410
 *
 * If <code>string_literal</code> is the name of an unexpanded macro, it will be
 * expanded. Thus, for example, <code>UTF16(__DATE__)</code> gives the expected
 * result.
 */
#define UTF16(string_literal) UTF16_(string_literal)

//==============================================================================
//                          typedef utf16_streambuf
//==============================================================================

/**
 * \brief Type name of a stream buffer which accepts 16-bit characters.
 * \author Victor Schappert
 * \since 20140115
 */
typedef std::basic_streambuf<utf16char_t> utf16_streambuf;

//==============================================================================
//                           typedef utf16_ostream
//==============================================================================

/**
 * \brief Type name of an output stream which accepts 16-bit characters.
 * \author Victor Schappert
 * \since 20131103
 */
typedef std::basic_ostream<utf16char_t, std::char_traits<utf16char_t>>
    utf16_ostream;

utf16_ostream& operator<<(utf16_ostream&, const char *);

} // jsdi

#endif // __INCLUDED_UTF16_UTIL_H___

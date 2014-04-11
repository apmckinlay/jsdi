/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: utf16_util.cpp
// auth: Victor Schappert
// date: 20140408
// desc: Implementation of utility types for working with 16-bit characters
//==============================================================================

#include "utf16_util.h"

namespace jsdi {

//==============================================================================
//                           typedef utf16char_t
//==============================================================================

static_assert(2 == sizeof(utf16char_t), "character size mismatch");

//==============================================================================
//                            class utf16_ostream
//==============================================================================

utf16_ostream& operator<<(utf16_ostream& o, const char * str)
{
    utf16_ostream::sentry sentry(o);
    if (sentry)
    {
        if (! str) o << static_cast<void *>(nullptr);
        else
        {
            utf16_streambuf * buf(o.rdbuf());
            if (buf) do
            {
                const char c = *str;
                if (! c) break;
                buf->sputc(o.widen(c));
                ++str;
            }
            while (true);
        }
    }
    return o;
}

} // namespace jsdi

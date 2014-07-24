/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#include "version.h"

namespace jsdi {

//==============================================================================
//                              struct version
//==============================================================================

char const * const version::BUILD_DATE = __DATE__ " " __TIME__;

char const * const version::PLATFORM =
#if defined(_M_AMD64)
"amd64"
#elif defined(_M_IX86)
"x86"
#else
#error "unknown platform"
#endif
;

} // namespace jsdi


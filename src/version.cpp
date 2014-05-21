/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#include "version.h"

namespace jsdi {

//==============================================================================
//                              struct version
//==============================================================================

utf16char_t const * const version::BUILD_DATE =
    UTF16(__DATE__) UTF16(" ") UTF16(__TIME__);

} // namespace jsdi
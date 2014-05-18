/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: jsdi_ole2.h
// auth: Victor Schappert
// date: 20140406
// desc: Don't #include <ole2.h> directly. Instead include this file.
//==============================================================================

#ifndef __INCLUDED_JSDI_OLE2_H__
#define __INCLUDED_JSDI_OLE2_H__

#include "jsdi_windef.h"

#include <ole2.h>

#undef min
#undef max
#undef ERROR

#endif // __INCLUDED_JSDI_OLE2_H__

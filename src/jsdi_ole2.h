/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_JSDI_OLE2_H__
#define __INCLUDED_JSDI_OLE2_H__

/**
 * \file jsdi_ole2.h
 * \author Victor Schappert
 * \since 20140406
 * \brief Don't <dfn>#include &lt;ole2.h&gt;</dfn> directly. Instead include
 *        this file.
 */

#include "jsdi_windef.h"

#include <ole2.h>

#undef min
#undef max
#undef ERROR

#endif // __INCLUDED_JSDI_OLE2_H__

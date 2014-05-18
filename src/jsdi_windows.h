/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_JSDI_WINDOWS_H__
#define __INCLUDED_JSDI_WINDOWS_H__

/**
 * \file jsdi_windows.h
 * \author Victor Schappert
 * \since 20130618
 * \brief Don't <dfn>#include &lt;windows.h&gt;</dfn> directly. Instead include
 *        this file.
 */

#include "jsdi_windef.h"

#include <windows.h>

#undef min
#undef max
#undef ERROR

#endif // __INCLUDED_JSDI_WINDOWS_H__

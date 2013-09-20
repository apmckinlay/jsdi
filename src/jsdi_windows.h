/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: jsdi_windows.h
// auth: Victor Schappert
// date: 20130618
// desc: Don't #include <windows.h> directly. Instead include this file.
//==============================================================================

#ifndef __INCLUDED_JSDI_WINDOWS_H__
#define __INCLUDED_JSDI_WINDOWS_H__

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#undef min
#undef max

#endif // __INCLUDED_JSDI_WINDOWS_H__

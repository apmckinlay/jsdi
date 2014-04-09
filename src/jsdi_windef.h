/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: jsdi_windef.h
// auth: Victor Schappert
// date: 20140406
// desc: Standard macros to #define before including a Windows header file
//==============================================================================

#ifndef __INCLUDED_JSDI_WINDEF_H__
#define __INCLUDED_JSDI_WINDEF_H__

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif

#if _WIN32_WINNT < 0x0501
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501 // Windows XP or higher
#endif

#endif // __INCLUDED_JSDI_WINDEF_H__
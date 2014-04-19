/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_TEST_COM_H___
#define __INCLUDED_TEST_COM_H___

/**
 * \file test_com.h
 * \author Victor Schappert
 * \since 20131020
 * \brief Interface to function for creating an object implementing IJSDITestCom
 */

#include "../jsdi_windows.h"

// Somehow including ole2.h before midl.h fixes some MinGW-related include
// dependency problems which otherwise produce compiler errors of the form:
//     error: 'DISPID' does not name a type
//     error: 'CALLCONV' does not name a type
// See here: http://mingw.5.n7.nabble.com/Errors-in-oleauto-h-td16631.html
#include <ole2.h>

#include "midl.h"

extern "C"
{

#define EXPORT_STDCALL(return_type) __declspec(dllexport) return_type __stdcall

EXPORT_STDCALL(ITestJSDICom *) TestCreateComObject();

} // extern "C"

#endif // __INCLUDED_TEST_COM_H___

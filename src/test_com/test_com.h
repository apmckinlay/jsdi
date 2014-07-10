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
#include "../jsdi_ole2.h"
#include "../test_exports.h" // For EXPORT_STDCALL

// This file is generated from test_com.idl by the MIDL compiler.
#include "midl.h"

extern "C"
{

EXPORT_STDCALL(ITestJSDICom *) TestCreateComObject();

} // extern "C"

#endif // __INCLUDED_TEST_COM_H___

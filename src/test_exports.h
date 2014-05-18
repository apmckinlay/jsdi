/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_TEST_EXPORTS_H___
#define __INCLUDED_TEST_EXPORTS_H___

/**
 * \file test_exports.h
 * \author Victor Schappert
 * \since 20130726
 * \brief Simple "C" functions exported from the DLL for testing purposes
 */

/** \cond internal */
#include <cstdint>

extern "C"
{

// NOTE: All of these calls should be structured so that passing 0 to any of
//       their parameters is safe. As long as you push the correct number of
//       zeroes onto the stack, calling any of these functions should behave in
//       a safe, predictable manner.

#define EXPORT_STDCALL(return_type) __declspec(dllexport) return_type __stdcall

struct Packed_CharCharShortLong
{
    signed char  a;
    signed char  b;
    short        c;
    long         d;
};

struct Recursive_CharCharShortLong
{
    struct Packed_CharCharShortLong x;
    Recursive_CharCharShortLong   * inner;
};

struct Recursive_StringSum
{
    struct Packed_CharCharShortLong x[2];
    const char *                    str;
    char *                          buffer;
    long                            len;
    Recursive_StringSum *           inner;
};

struct Swap_StringLongLong
{
    const char * str;
    long         a;
    long         b;
};

typedef long(__stdcall * TestCallback_Long1)(long);

typedef long(__stdcall * TestCallback_Long2)(long, long);

typedef long(__stdcall * TestCallback_Packed_CharCharShortLong)(
    struct Packed_CharCharShortLong);

typedef long(__stdcall * TestCallback_Recursive_StringSum)(
    struct Recursive_StringSum *);

EXPORT_STDCALL(void) TestVoid();

EXPORT_STDCALL(signed char) TestChar(signed char a);

EXPORT_STDCALL(short) TestShort(short a);

EXPORT_STDCALL(long) TestLong(long a);

EXPORT_STDCALL(int64_t) TestInt64(int64_t a);

EXPORT_STDCALL(float) TestReturn1_0Float();

EXPORT_STDCALL(double) TestReturn1_0Double();

EXPORT_STDCALL(float) TestFloat(float a);

EXPORT_STDCALL(double) TestDouble(double a);

EXPORT_STDCALL(int64_t) TestRemoveSignFromLong(long a);

EXPORT_STDCALL(signed char) TestSumTwoChars(signed char a, signed char b);

EXPORT_STDCALL(short) TestSumTwoShorts(short a, short b);

EXPORT_STDCALL(long) TestSumTwoLongs(long a, long b);

EXPORT_STDCALL(float) TestSumTwoFloats(float a, float b);

EXPORT_STDCALL(double) TestSumTwoDoubles(double a, double b);

EXPORT_STDCALL(long) TestSumThreeLongs(long a, long b, long c);

EXPORT_STDCALL(long) TestSumFourLongs(long a, long b, long c, long d);

EXPORT_STDCALL(int64_t) TestSumCharPlusInt64(signed char a, int64_t b);

EXPORT_STDCALL(long) TestSumPackedCharCharShortLong(Packed_CharCharShortLong x);

EXPORT_STDCALL(long) TestStrLen(const char * str);

EXPORT_STDCALL(const char *) TestHelloWorldReturn(long flag);

EXPORT_STDCALL(void) TestHelloWorldOutParam(const char ** str);

EXPORT_STDCALL(void) TestHelloWorldOutBuffer(char * buffer, long size);

EXPORT_STDCALL(void) TestNullPtrOutParam(const char ** ptr);

EXPORT_STDCALL(uint64_t) TestReturnPtrPtrPtrDoubleAsUInt64(
    const double * const * const * ptr);

EXPORT_STDCALL(long) TestSumString(Recursive_StringSum * ptr);

EXPORT_STDCALL(long) TestSumResource(const char * res, const char ** pres);

EXPORT_STDCALL(long) TestSwap(Swap_StringLongLong * ptr);

EXPORT_STDCALL(const char *) TestReturnString(const char * str);

EXPORT_STDCALL(const char *) TestReturnPtrString(const char * const * ptr);

EXPORT_STDCALL(char *) TestReturnStringOutBuffer(const char * str,
                                                 char * buffer, long size);

EXPORT_STDCALL(const struct Packed_CharCharShortLong *)
TestReturnStatic_Packed_CharCharShortLong(
    const struct Packed_CharCharShortLong * ptr);

EXPORT_STDCALL(const struct Recursive_CharCharShortLong *)
TestReturnStatic_Recursive_CharCharShortLong(
    const struct Recursive_CharCharShortLong * ptr);

EXPORT_STDCALL(const struct Recursive_StringSum *)
TestReturnStatic_Recursive_StringSum(const struct Recursive_StringSum * ptr);

EXPORT_STDCALL(long) TestInvokeCallback_Long1(TestCallback_Long1 f, long a);

EXPORT_STDCALL(long) TestInvokeCallback_Long1_2(TestCallback_Long1 f, long a,
                                                TestCallback_Long1 g, long b);

EXPORT_STDCALL(long) TestInvokeCallback_Long2(TestCallback_Long2 f, long a,
                                              long b);

EXPORT_STDCALL(long) TestInvokeCallback_Packed_CharCharShortLong(
    TestCallback_Packed_CharCharShortLong f, struct Packed_CharCharShortLong a);

EXPORT_STDCALL(long) TestInvokeCallback_Recursive_StringSum(
    TestCallback_Recursive_StringSum f, struct Recursive_StringSum * ptr);

} // extern "C"

/** \endcond */

#endif // __INCLUDED_TEST_EXPORTS_H___

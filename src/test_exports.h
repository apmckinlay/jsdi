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

#if defined(_M_IX86)
// For x86, don't specify __declspec(dllexport) because these exports are
// declared in test_exports.defs
#define EXPORT_STDCALL(return_type) return_type __stdcall
#else if defined(_M_AMD64)
// For x64, don't specify __stdcall because, really, there's no such thing.
#define EXPORT_STDCALL(return_type) __declspec(dllexport) return_type
#endif

struct Packed_Int8Int8Int16Int32
{
    int8_t  a;
    int8_t  b;
    int16_t c;
    int32_t d;
};

struct Packed_Int8x3
{
    int8_t a;
    int8_t b;
    int8_t c;
};

struct Recursive_Int8Int8Int16Int32
{
    struct Packed_Int8Int8Int16Int32 x;
    Recursive_Int8Int8Int16Int32   * inner;
};

struct Recursive_StringSum
{
    struct Packed_Int8Int8Int16Int32 x[2];
    const char *                     str;
    char *                           buffer;
    int32_t                          len;
    Recursive_StringSum *            inner;
};

struct Swap_StringInt32Int32
{
    const char * str;
    int32_t      a;
    int32_t      b;
};

typedef int32_t(__stdcall * TestCallback_Int32_1)(int32_t);

typedef int32_t(__stdcall * TestCallback_Int32_2)(int32_t, int32_t);

typedef int32_t(__stdcall * TestCallback_Mixed_6)(double, int8_t, float,
                                                  int16_t, float, int64_t);

typedef int32_t(__stdcall * TestCallback_Packed_Int8Int8Int16Int32)(
    struct Packed_Int8Int8Int16Int32);

typedef int32_t(__stdcall * TestCallback_Recursive_StringSum)(
    struct Recursive_StringSum *);

EXPORT_STDCALL(void) TestVoid();

EXPORT_STDCALL(int8_t) TestInt8(int8_t a);

EXPORT_STDCALL(int16_t) TestInt16(int16_t a);

EXPORT_STDCALL(int32_t) TestInt32(int32_t a);

EXPORT_STDCALL(int64_t) TestInt64(int64_t a);

EXPORT_STDCALL(float) TestReturn1_0Float();

EXPORT_STDCALL(double) TestReturn1_0Double();

EXPORT_STDCALL(float) TestFloat(float a);

EXPORT_STDCALL(double) TestDouble(double a);

EXPORT_STDCALL(int64_t) TestRemoveSignFromInt32(int32_t a);

EXPORT_STDCALL(void) TestCopyInt32Value(const int32_t * src, int32_t * dst);

EXPORT_STDCALL(int8_t) TestSumTwoInt8s(int8_t a, int8_t b);

EXPORT_STDCALL(int16_t) TestSumTwoInt16s(int16_t a, int16_t b);

EXPORT_STDCALL(int32_t) TestSumTwoInt32s(int32_t a, int32_t b);

EXPORT_STDCALL(float) TestSumTwoFloats(float a, float b);

EXPORT_STDCALL(double) TestSumTwoDoubles(double a, double b);

EXPORT_STDCALL(int32_t) TestSumThreeInt32s(int32_t a, int32_t b, int32_t c);

EXPORT_STDCALL(int32_t) TestSumFourInt32s(int32_t a, int32_t b, int32_t c,
                                          int32_t d);

EXPORT_STDCALL(int32_t) TestSumFiveInt32s(int32_t a, int32_t b, int32_t c,
                                          int32_t d, int32_t e);

EXPORT_STDCALL(int32_t) TestSumSixInt32s(int32_t a, int32_t b, int32_t c,
                                         int32_t d, int32_t e, int32_t f);

EXPORT_STDCALL(int32_t) TestSumSixMixed(double a, int8_t b, float c, int16_t d,
                                        float e, int64_t f);

EXPORT_STDCALL(int32_t) TestSumSevenInt32s(int32_t a, int32_t b, int32_t c,
                                           int32_t d, int32_t e, int32_t f,
                                           int32_t g);

EXPORT_STDCALL(int32_t) TestSumEightInt32s(int32_t a, int32_t b, int32_t c,
                                           int32_t d, int32_t e, int32_t f,
                                           int32_t g, int32_t h);

EXPORT_STDCALL(int32_t) TestSumNineInt32s(int32_t a, int32_t b, int32_t c,
                                          int32_t d, int32_t e, int32_t f,
                                          int32_t g, int32_t h, int32_t i);

EXPORT_STDCALL(int64_t) TestSumInt8PlusInt64(int8_t a, int64_t b);

EXPORT_STDCALL(int32_t) TestSumPackedInt8Int8Int16Int32(
    struct Packed_Int8Int8Int16Int32 x);

EXPORT_STDCALL(int32_t) TestSumPackedInt8x3(struct Packed_Int8x3 x);

EXPORT_STDCALL(int64_t) TestSumManyInts(int8_t a, int16_t b, int32_t c,
                                        Swap_StringInt32Int32 d, int64_t e,
                                        Packed_Int8Int8Int16Int32 f,
                                        Packed_Int8x3 g,
                                        Recursive_StringSum h,
                                        Recursive_StringSum * i);

EXPORT_STDCALL(int32_t) TestStrLen(const char * str);

EXPORT_STDCALL(const char *) TestHelloWorldReturn(int32_t flag);

EXPORT_STDCALL(void) TestHelloWorldOutParam(const char ** str);

EXPORT_STDCALL(void) TestHelloWorldOutBuffer(char * buffer, int32_t size);

EXPORT_STDCALL(void) TestNullPtrOutParam(const char ** ptr);

EXPORT_STDCALL(uint64_t) TestReturnPtrPtrPtrDoubleAsUInt64(
    const double * const * const * ptr);

EXPORT_STDCALL(int32_t) TestSumString(Recursive_StringSum * ptr);

EXPORT_STDCALL(int32_t) TestSumResource(const char * res, const char ** pres);

EXPORT_STDCALL(int32_t) TestSwap(Swap_StringInt32Int32 * ptr);

EXPORT_STDCALL(const char *) TestReturnString(const char * str);

EXPORT_STDCALL(const char *) TestReturnPtrString(const char * const * ptr);

EXPORT_STDCALL(char *) TestReturnStringOutBuffer(const char * str,
                                                 char * buffer, int32_t size);

EXPORT_STDCALL(const struct Packed_Int8Int8Int16Int32 *)
TestReturnStatic_Packed_Int8Int8Int16Int32(
    const struct Packed_Int8Int8Int16Int32 * ptr);

EXPORT_STDCALL(const struct Recursive_Int8Int8Int16Int32 *)
TestReturnStatic_Recursive_Int8Int8Int16Int32(
    const struct Recursive_Int8Int8Int16Int32 * ptr);

EXPORT_STDCALL(const struct Recursive_StringSum *)
TestReturnStatic_Recursive_StringSum(const struct Recursive_StringSum * ptr);

EXPORT_STDCALL(int32_t) TestInvokeCallback_Int32_1(TestCallback_Int32_1 f,
                                                   int32_t a);

EXPORT_STDCALL(int32_t) TestInvokeCallback_Int32_1_2(TestCallback_Int32_1 f,
                                                     int32_t a,
                                                     TestCallback_Int32_1 g,
                                                     int32_t b);

EXPORT_STDCALL(int32_t) TestInvokeCallback_Int32_2(TestCallback_Int32_2 f,
                                                   int32_t a, int32_t b);

EXPORT_STDCALL(int32_t) TestInvokeCallback_Mixed_6(TestCallback_Mixed_6 g,
                                                   double a, int8_t b, float c,
                                                   int16_t d, float e,
                                                   int64_t f);

EXPORT_STDCALL(int32_t) TestInvokeCallback_Packed_Int8Int8Int16Int32(
    TestCallback_Packed_Int8Int8Int16Int32 f,
    struct Packed_Int8Int8Int16Int32 a);

EXPORT_STDCALL(int32_t) TestInvokeCallback_Recursive_StringSum(
    TestCallback_Recursive_StringSum f, struct Recursive_StringSum * ptr);

} // extern "C"

/** \endcond */

#endif // __INCLUDED_TEST_EXPORTS_H___

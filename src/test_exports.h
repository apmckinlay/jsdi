#ifndef __INCLUDED_TEST_EXPORTS_H___
#define __INCLUDED_TEST_EXPORTS_H___

/**
 * \file test_exports.h
 * \author Victor Schappert
 * \since 20130726
 * \brief Simple "C" functions exported from the DLL for testing purposes
 */

#include <cstdint>

extern "C"
{

// NOTE: All of these calls should be structured so that passing 0 to any of
//       their parameters is safe. As long as you push the correct number of
//       zeroes onto the stack, calling any of these functions should behave in
//       a safe, predictable manner.

#define EXPORT_STDCALL __declspec(dllexport) __stdcall

struct Packed_CharCharShortLong
{
    signed char  a;
    signed char  b;
    short        c;
    long         d;
};

EXPORT_STDCALL void TestVoid();

EXPORT_STDCALL signed char TestChar(signed char a);

EXPORT_STDCALL short TestShort(short a);

EXPORT_STDCALL long TestLong(long a);

EXPORT_STDCALL int64_t TestInt64(int64_t a);

EXPORT_STDCALL signed char TestSumTwoChars(signed char a, signed char b);

EXPORT_STDCALL short TestSumTwoShorts(short a, short b);

EXPORT_STDCALL long TestSumTwoLongs(long a, long b);

EXPORT_STDCALL long TestSumThreeLongs(long a, long b, long c);

EXPORT_STDCALL long TestSumFourLongs(long a, long b, long c, long d);

EXPORT_STDCALL int64_t TestSumCharPlusInt64(signed char a, int64_t b);

EXPORT_STDCALL long TestSumPackedCharCharShortLong(Packed_CharCharShortLong x);

EXPORT_STDCALL long TestStrLen(const char * str);

EXPORT_STDCALL const char * TestHelloWorldReturn(long flag);

EXPORT_STDCALL void TestHelloWorldOutParam(const char ** str);

EXPORT_STDCALL void TestNullPtrOutParam(const char ** ptr);

EXPORT_STDCALL double TestReturnPtrPtrPtrDouble(const double *** ptr);

} // extern "C"

#endif // __INCLUDED_TEST_EXPORTS_H___

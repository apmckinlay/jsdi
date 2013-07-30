//==============================================================================
// file: test.cpp
// auth: Victor Schappert
// date: 20130709
// desc: Exports functions which can be used to test dll functionality
//==============================================================================

#include "test_exports.h"

#include <cstring>

extern "C" {

EXPORT_STDCALL void TestVoid()
{ }

EXPORT_STDCALL signed char TestChar(signed char a)
{ return a; }

EXPORT_STDCALL short TestShort(short a)
{ return a; }

EXPORT_STDCALL long TestLong(long a)
{ return a; }

EXPORT_STDCALL int64_t TestInt64(int64_t a)
{ return a; }

EXPORT_STDCALL signed char TestSumTwoChars(signed char a, signed char b)
{ return a + b; }

EXPORT_STDCALL short TestSumTwoShorts(short a, short b)
{ return a + b; }

EXPORT_STDCALL long TestSumTwoLongs(long a, long b)
{ return a + b; }

EXPORT_STDCALL long TestSumThreeLongs(long a, long b, long c)
{ return a + b + c; }

EXPORT_STDCALL long TestSumFourLongs(long a, long b, long c, long d)
{ return a + b + c + d; }

EXPORT_STDCALL int64_t TestSumCharPlusInt64(signed char a, int64_t b)
{ return a + b; }

EXPORT_STDCALL long TestSumPackedCharCharShortLong(Packed_CharCharShortLong x)
{ return x.a + x.b + x.c + x.d; }

EXPORT_STDCALL long TestStrLen(const char * str)
{ return str ? std::strlen(str) : 0; }

EXPORT_STDCALL const char * TestHelloWorldReturn(long flag)
{ return flag ? "hello world" : 0; }

EXPORT_STDCALL void TestHelloWorldOutParam(const char ** str)
{ if (str) *str = "hello world"; }

EXPORT_STDCALL void TestNullPtrOutParam(const char ** ptr)
{ if (ptr) *ptr = 0; }

EXPORT_STDCALL uint64_t TestReturnPtrPtrPtrDoubleAsUInt64(
    const double * const * const * ptr)
{
    // The parameter type is because: http://stackoverflow.com/a/1404795/1911388
    // The return value is uint64_t because floating-point values are returned
    // in ST0, which we can't handle at the moment.
    union
    {
        volatile double   as_dbl;
        volatile uint64_t as_uint64;
    };
    as_dbl = ptr && *ptr && **ptr ? ***ptr : 0.0;
    return as_uint64;
}

} // extern "C"


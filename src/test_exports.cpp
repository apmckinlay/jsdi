//==============================================================================
// file: test.cpp
// auth: Victor Schappert
// date: 20130709
// desc: Exports functions which can be used to test dll functionality
//==============================================================================

#include "test_exports.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>

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

EXPORT_STDCALL void TestHelloWorldOutBuffer(char * buffer, long size)
{ if (buffer) std::strncpy(buffer, "hello world", size); }

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

EXPORT_STDCALL long TestSumString(Recursive_StringSum * ptr)
{
    if (! ptr) return 0;
    long sum = TestSumPackedCharCharShortLong(ptr->x[0]) +
               TestSumPackedCharCharShortLong(ptr->x[1]);
    if (ptr->str) sum += std::atol(ptr->str);
    sum += TestSumString(ptr->inner);
    if (ptr->buffer) std::snprintf(ptr->buffer, ptr->len, "%ld", sum);
    return sum;
}

} // extern "C"

//==============================================================================
//                                  TESTS
//==============================================================================

#ifndef __TEST_H_NO_TESTS__

#include "test.h"
#include "util.h"

TEST(TestSumString,
    static Recursive_StringSum rss[3]; // zeroed automatically
    static char buffer[3][4];          // zeroed automatically
    assert_equals(0, TestSumString(0));
    assert_equals(0, TestSumString(rss));
    rss[0].x[0] = { 1, 2, 3, 4 };
    assert_equals(10, TestSumString(rss));
    rss[0].x[1] = { -5, -4, -3, -2 };
    assert_equals(-4, TestSumString(rss));
    rss[0].inner = &rss[1];
    assert_equals(-4, TestSumString(rss));
    std::copy(rss[0].x, rss[0].x + jsdi::array_length(rss[0].x), rss[1].x);
    assert_equals(-8, TestSumString(rss));
    rss[1].str = "999";
    assert_equals(991, TestSumString(rss));
    rss[0].str = "-992";
    assert_equals(-1, TestSumString(rss));
    rss[1].buffer = buffer[1];
    assert_equals(-1, TestSumString(rss));
    assert_equals(std::string(), rss[1].buffer);
    rss[1].len = jsdi::array_length(buffer[1]);
    assert_equals(-1, TestSumString(rss));
    assert_equals(std::string("995"), rss[1].buffer);
    rss[0].buffer = buffer[0];
    assert_equals(-1, TestSumString(rss));
    assert_equals(std::string(), rss[0].buffer);
    rss[0].len = jsdi::array_length(buffer[0]);
    assert_equals(-1, TestSumString(rss));
    assert_equals(std::string("-1"), buffer[0]);
    rss[1].inner = &rss[2];
    assert_equals(-1, TestSumString(rss));
    assert_equals(995, TestSumString(&rss[1]));
    rss[2].x[0] = { -90, 10, 110, 210 };
    rss[2].x[1] = { -1, -1, -1, -1 };
    assert_equals(236, TestSumString(&rss[2]));
    assert_equals(1231, TestSumString(&rss[1]));
    assert_equals(std::string("123"), buffer[1]);
    assert_equals(235, TestSumString(rss));
    assert_equals(std::string("235"), buffer[0]);
    rss[2].buffer = buffer[2];
    rss[2].len = 1;
    assert_equals(235, TestSumString(rss));
    assert_equals(std::string(), buffer[2]);
);

#endif // __TEST_H_NO_TESTS__

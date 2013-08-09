//==============================================================================
// file: test.cpp
// auth: Victor Schappert
// date: 20130709
// desc: Exports functions which can be used to test dll functionality
//==============================================================================

#include "test_exports.h"

#include "jsdi_windows.h"

#include <algorithm>
#include <cassert>
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

EXPORT_STDCALL float TestReturn1_0Float()
{ return 1.0f; }

EXPORT_STDCALL double TestReturn1_0Double()
{ return 1.0; }

EXPORT_STDCALL float TestFloat(float a)
{ return a; }

EXPORT_STDCALL double TestDouble(double a)
{ return a; }

EXPORT_STDCALL signed char TestSumTwoChars(signed char a, signed char b)
{ return a + b; }

EXPORT_STDCALL short TestSumTwoShorts(short a, short b)
{ return a + b; }

EXPORT_STDCALL long TestSumTwoLongs(long a, long b)
{ return a + b; }

EXPORT_STDCALL float TestSumTwoFloats(float a, float b)
{ return a + b; }

EXPORT_STDCALL double TestSumTwoDoubles(double a, double b)
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

EXPORT_STDCALL long TestSumResource(const char * res, const char ** pres)
{
    long sum(0);
    if (IS_INTRESOURCE(res))
        sum += reinterpret_cast<unsigned long>(res);
    else
    {
        assert(res);
        sum += std::atol(res);
    }
    if (pres)
    {
        // Finish calculating sum.
        if (IS_INTRESOURCE(*pres))
            sum += reinterpret_cast<unsigned long>(*pres);
        else
        {
            assert(*pres);
            sum += std::atol(*pres);
        }
        // If the sum fits in an INTRESOURCE, return it in *pres. Otherwise,
        // return a string.
        *pres = 0 <= sum && IS_INTRESOURCE(sum)
            ? MAKEINTRESOURCE(sum)
            : "sum is not an INTRESOURCE"
            ;
    }
    return sum;
}

EXPORT_STDCALL long TestSwap(Swap_StringLongLong * ptr)
{
    long result(0);
    if (ptr)
    {
        if (ptr->a != ptr->b)
        {
            ptr->str = "!=";
            std::swap(ptr->a, ptr->b);
        }
        else
        {
            ptr->str = "=";
            result = 1;
        }
    }
    return result;
}

EXPORT_STDCALL long TestInvokeCallback_Long1(TestCallback_Long1 f, long a)
{ return f ? f(a) : 0L; }

EXPORT_STDCALL long TestInvokeCallback_Long2(TestCallback_Long2 f, long a,
                                             long b)
{ return f ? f(a, b) : 0L; }

EXPORT_STDCALL long TestInvokeCallback_Packed_CharCharShortLong(
    TestCallback_Packed_CharCharShortLong f, Packed_CharCharShortLong a)
{ return f ? f(a) : 0L; }

EXPORT_STDCALL long TestInvokeCallback_Recursive_StringSum(
    TestCallback_Recursive_StringSum f, Recursive_StringSum * ptr)
{ return f ? f(ptr) : 0L; }

} // extern "C"

//==============================================================================
//                                  TESTS
//==============================================================================

#ifndef __NOTEST__

#include "test.h"
#include "util.h"

#include <limits>
#include <cstdint>

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

TEST(TestSumResource,
    const char * ptr;
    assert_equals(0, TestSumResource(0, 0));
    assert_equals(5, TestSumResource(MAKEINTRESOURCE(5), 0));
    ptr = MAKEINTRESOURCE(40000);
    assert_equals(40005, TestSumResource(MAKEINTRESOURCE(5), &ptr));
    assert_true(IS_INTRESOURCE(ptr));
    assert_equals(40005, reinterpret_cast<unsigned long>(ptr));
    assert_equals(255, TestSumResource("255", 0));
    ptr = "-1";
    assert_equals(255, TestSumResource("256", &ptr));
    assert_true(IS_INTRESOURCE(ptr));
    assert_equals(255, reinterpret_cast<unsigned long>(ptr));
    ptr = MAKEINTRESOURCE(1);
    assert_equals(
        std::numeric_limits<uint16_t>::max() + 1,
        TestSumResource(
            MAKEINTRESOURCE(std::numeric_limits<uint16_t>::max()),
            &ptr
        )
    )
    assert_false(IS_INTRESOURCE(ptr));
    assert_equals(std::string("sum is not an INTRESOURCE"), ptr);
);

#endif // __NOTEST__

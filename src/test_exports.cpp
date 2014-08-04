/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: test_exports.cpp
// auth: Victor Schappert
// date: 20130709
// desc: Exports functions which can be used to test dll functionality
//==============================================================================

#include "test_exports.h"

#include "jsdi_windows.h"
#include "util.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>

namespace {

template <typename T, typename U>
void assign(T& t, const U& u, T * next)
{
    t = u;
    t.inner = u.inner && next ? next : 0;
}

struct Recursive_StringSum_Storage
{
    struct Recursive_StringSum  rss;
    char                        buffer[32];
};

template <>
void assign(Recursive_StringSum_Storage& t, const struct Recursive_StringSum& u,
            Recursive_StringSum_Storage * next)
{
    std::copy(u.x, u.x + jsdi::array_length(u.x), t.rss.x);
    if (u.str)
    {
        TestReturnStringOutBuffer(u.str, t.buffer, sizeof(t.buffer));
        t.rss.str = t.buffer;
    }
    else
    {
        t.buffer[0] = '\0';
        t.rss.str = nullptr;
    }
    if (u.buffer && 0 < u.len)
    {
        size_t pos = std::strlen(t.buffer) + 1;
        assert(pos <= sizeof(t.buffer));
        t.rss.buffer = t.buffer + pos;
        t.rss.len = std::min(static_cast<int32_t>(sizeof(t.buffer) - pos),
                             u.len);
        std::memcpy(t.rss.buffer, u.buffer, t.rss.len);
    }
    else
    {
        t.rss.buffer = nullptr;
        t.rss.len = 0;
    }
    t.rss.inner = u.inner && next ? &next->rss : nullptr;
}

template <typename T, typename U, size_t N>
void recursive_copy(const T * ptr, U(&x)[N] )
{
    for (size_t k = 0; ptr && k < N; ++k)
    {
        assign(x[k], *ptr, k < N - 1 ? &x[k + 1] : nullptr);
        ptr = ptr->inner;
    }
}

} // anonymous namespace

extern "C" {

EXPORT_STDCALL(void) TestVoid()
{ }

EXPORT_STDCALL(int8_t) TestInt8(int8_t a)
{ return a; }

EXPORT_STDCALL(int16_t) TestInt16(int16_t a)
{ return a; }

EXPORT_STDCALL(int32_t) TestInt32(int32_t a)
{ return a; }

EXPORT_STDCALL(int64_t) TestInt64(int64_t a)
{ return a; }

EXPORT_STDCALL(float) TestReturn1_0Float()
{ return 1.0f; }

EXPORT_STDCALL(double) TestReturn1_0Double()
{ return 1.0; }

EXPORT_STDCALL(float) TestFloat(float a)
{ return a; }

EXPORT_STDCALL(double) TestDouble(double a)
{ return a; }

EXPORT_STDCALL(int64_t) TestRemoveSignFromInt32(int32_t a)
{
    return reinterpret_cast<int64_t>(reinterpret_cast<void *>(a)) &
           0xffffffffLL;
}

EXPORT_STDCALL(void) TestCopyInt32Value(const int32_t * src, int32_t * dst)
{ if (src && dst) *dst = *src; }

EXPORT_STDCALL(int8_t) TestSumTwoInt8s(int8_t a, int8_t b)
{ return a + b; }

EXPORT_STDCALL(int16_t) TestSumTwoInt16s(int16_t a, int16_t b)
{ return a + b; }

EXPORT_STDCALL(int32_t) TestSumTwoInt32s(int32_t a, int32_t b)
{ return a + b; }

EXPORT_STDCALL(float) TestSumTwoFloats(float a, float b)
{ return a + b; }

EXPORT_STDCALL(double) TestSumTwoDoubles(double a, double b)
{ return a + b; }

EXPORT_STDCALL(int32_t) TestSumThreeInt32s(int32_t a, int32_t b, int32_t c)
{ return a + b + c; }

EXPORT_STDCALL(int32_t) TestSumFourInt32s(int32_t a, int32_t b, int32_t c,
                                          int32_t d)
{ return a + b + c + d; }

EXPORT_STDCALL(int32_t) TestSumFiveInt32s(int32_t a, int32_t b, int32_t c,
                                          int32_t d, int32_t e)
{ return TestSumFourInt32s(a, b, c, d) + e; }

EXPORT_STDCALL(int32_t) TestSumSixInt32s(int32_t a, int32_t b, int32_t c,
                                         int32_t d, int32_t e, int32_t f)
{ return TestSumFiveInt32s(a, b, c, d, e) + f; }

EXPORT_STDCALL(int32_t) TestSumSixMixed(double a, int8_t b, float c, int16_t d,
                                        float e, int64_t f)
{
    return static_cast<int32_t>(a) + b + static_cast<int32_t>(c) + d +
           static_cast<int32_t>(e) + static_cast<int32_t>(f);
}

EXPORT_STDCALL(int32_t) TestSumSevenInt32s(int32_t a, int32_t b, int32_t c,
                                           int32_t d, int32_t e, int32_t f,
                                           int32_t g)
{ return TestSumSixInt32s(a, b, c, d, e, f) + g; }

EXPORT_STDCALL(int32_t) TestSumEightInt32s(int32_t a, int32_t b, int32_t c,
                                           int32_t d, int32_t e, int32_t f,
                                           int32_t g, int32_t h)
{ return TestSumSevenInt32s(a, b, c, d, e, f, g) + h; }

EXPORT_STDCALL(int32_t) TestSumNineInt32s(int32_t a, int32_t b, int32_t c,
                                          int32_t d, int32_t e, int32_t f,
                                          int32_t g, int32_t h, int32_t i)
{ return TestSumEightInt32s(a, b, c, d, e, f, g, h) + i; }

EXPORT_STDCALL(int64_t) TestSumInt8PlusInt64(int8_t a, int64_t b)
{ return a + b; }

EXPORT_STDCALL(int32_t) TestSumPackedInt8Int8Int16Int32(
    struct Packed_Int8Int8Int16Int32 x)
{ return x.a + x.b + x.c + x.d; }

EXPORT_STDCALL(int32_t) TestSumPackedInt8x3(struct Packed_Int8x3 x)
{ return x.a + x.b + x.c; }

EXPORT_STDCALL(int64_t) TestSumManyInts(int8_t a, int16_t b, int32_t c,
                                        Swap_StringInt32Int32 d, int64_t e,
                                        Packed_Int8Int8Int16Int32 f,
                                        Packed_Int8x3 g,
                                        Recursive_StringSum h,
                                        Recursive_StringSum * i)
{
    return static_cast<int64_t>(a) + static_cast<int64_t>(b) +
           static_cast<int64_t>(c) + static_cast<int64_t>(TestSwap(&d)) + e +
           static_cast<int64_t>(TestSumPackedInt8Int8Int16Int32(f)) +
           static_cast<int64_t>(TestSumPackedInt8x3(g)) +
           static_cast<int64_t>(TestSumString(&h)) +
           static_cast<int64_t>(TestSumString(i));
}

EXPORT_STDCALL(int32_t) TestStrLen(const char * str)
{ return str ? static_cast<int32_t>(std::strlen(str)) : 0; }

EXPORT_STDCALL(const char *) TestHelloWorldReturn(int32_t flag)
{ return flag ? "hello world" : 0; }

EXPORT_STDCALL(void) TestHelloWorldOutParam(const char ** str)
{ if (str) *str = "hello world"; }

EXPORT_STDCALL(void) TestHelloWorldOutBuffer(char * buffer, int32_t size)
{ if (buffer) std::strncpy(buffer, "hello world", size); }

EXPORT_STDCALL(void) TestNullPtrOutParam(const char ** ptr)
{ if (ptr) *ptr = 0; }

EXPORT_STDCALL(uint64_t) TestReturnPtrPtrPtrDoubleAsUInt64(
    double const * const * const * ptr)
{
    // The parameter type is because: http://stackoverflow.com/a/1404795/1911388
    union
    {
        volatile double   as_dbl;
        volatile uint64_t as_uint64;
    };
    as_dbl = ptr && *ptr && **ptr ? ***ptr : 0.0;
    return as_uint64;
}

EXPORT_STDCALL(int32_t) TestSumString(struct Recursive_StringSum * ptr)
{
    if (! ptr) return 0;
    int32_t sum = TestSumPackedInt8Int8Int16Int32(ptr->x[0]) +
                  TestSumPackedInt8Int8Int16Int32(ptr->x[1]);
    if (ptr->str) sum += std::atol(ptr->str);
    sum += TestSumString(ptr->inner);
    if (ptr->buffer)
    {
        // Pathetically, Visual C++ doesn't support snprintf(...), a C99
        // standard function, even in April 2014. This workaround is
        // adequate to our needs, but for a more general workaround, see here:
        // http://stackoverflow.com/a/8712996/1911388.
        char buffer[32];
        sprintf(buffer, "%ld", sum);
        strncpy(ptr->buffer, buffer,
                std::max(static_cast<int32_t>(0), ptr->len - 1));
        if (0 < ptr->len) ptr->buffer[ptr->len - 1] = '\0';
    }
    return sum;
}

EXPORT_STDCALL(int32_t) TestSumResource(const char * res, const char ** pres)
{
    int32_t sum(0);
    if (IS_INTRESOURCE(res))
        sum += static_cast<int32_t>(reinterpret_cast<uint16_t>(res));
    else
    {
        assert(res);
        sum += std::atol(res);
    }
    if (pres)
    {
        // Finish calculating sum.
        if (IS_INTRESOURCE(*pres))
            sum += static_cast<int32_t>(reinterpret_cast<uint16_t>(*pres));
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

EXPORT_STDCALL(int32_t) TestSwap(Swap_StringInt32Int32 * ptr)
{
    int32_t result(0);
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

EXPORT_STDCALL(const char *) TestReturnString(const char * str)
{ return str; }

EXPORT_STDCALL(const char *) TestReturnPtrString(char const * const * ptr)
{ return ptr ? *ptr : 0; }

EXPORT_STDCALL(char *) TestReturnStringOutBuffer(const char * str,
                                                 char * buffer, int32_t size)
{
    if (! str || ! buffer || size < 1) return 0;
    // This is more-or-less strncpy, except that we guarantee that any non-empty
    // buffer will always be zero-terminated.
    char * i(buffer), * e(buffer + size);
    while (i < e - 1)
    {
        const char x = *str++;
        *i = x;
        if (! x) return buffer;
        ++i;
    }
    assert(i < e);
    *i = '\0';
    return buffer;
}

EXPORT_STDCALL(const struct Packed_Int8Int8Int16Int32 *)
TestReturnStatic_Packed_Int8Int8Int16Int32(
    const struct Packed_Int8Int8Int16Int32 * ptr)
{
    static Packed_Int8Int8Int16Int32 x;
    if (ptr) x = *ptr;
    return &x;
}

EXPORT_STDCALL(const struct Recursive_Int8Int8Int16Int32 *)
TestReturnStatic_Recursive_Int8Int8Int16Int32(
    const struct Recursive_Int8Int8Int16Int32 * ptr)
{
    static Recursive_Int8Int8Int16Int32 x[3];
    recursive_copy(ptr, x);
    return x;
}

EXPORT_STDCALL(const struct Recursive_StringSum *)
TestReturnStatic_Recursive_StringSum(const struct Recursive_StringSum * ptr)
{
    static Recursive_StringSum_Storage x[3];
    recursive_copy(ptr, x);
    return &x[0].rss;
}

EXPORT_STDCALL(int32_t) TestInvokeCallback_Int32_1(TestCallback_Int32_1 f,
                                                   int32_t a)
{ return f ? f(a) : 0; }

EXPORT_STDCALL(int32_t) TestInvokeCallback_Int32_1_2(TestCallback_Int32_1 f,
                                                     int32_t a,
                                                     TestCallback_Int32_1 g,
                                                     int32_t b)
{
    int32_t count(0);
    if (f) f(a), ++count;
    if (g) g(b), ++count;
    return count;
}

EXPORT_STDCALL(int32_t) TestInvokeCallback_Int32_2(TestCallback_Int32_2 f,
                                                   int32_t a, int32_t b)
{ return f ? f(a, b) : 0; }

EXPORT_STDCALL(int32_t) TestInvokeCallback_Mixed_6(TestCallback_Mixed_6 g,
                                                   double a, int8_t b, float c,
                                                   int16_t d, float e,
                                                   int64_t f)
{ return g ? g(a, b, c, d, e, f) : 0; }

EXPORT_STDCALL(int32_t) TestInvokeCallback_Packed_Int8Int8Int16Int32(
    TestCallback_Packed_Int8Int8Int16Int32 f, Packed_Int8Int8Int16Int32 a)
{ return f ? f(a) : 0; }

EXPORT_STDCALL(int32_t) TestInvokeCallback_Recursive_StringSum(
    TestCallback_Recursive_StringSum f, struct Recursive_StringSum * ptr)
{ return f ? f(ptr) : 0; }

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
    rss[1].len = static_cast<int32_t>(jsdi::array_length(buffer[1]));
    assert_equals(-1, TestSumString(rss));
    assert_equals(std::string("995"), rss[1].buffer);
    rss[0].buffer = buffer[0];
    assert_equals(-1, TestSumString(rss));
    assert_equals(std::string(), rss[0].buffer);
    rss[0].len = static_cast<int32_t>(jsdi::array_length(buffer[0]));
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

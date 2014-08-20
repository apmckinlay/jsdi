/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: invoke64.cpp
// auth: Victor Schappert
// date: 20140707
// desc: Windows x64 ABI invocation translation unit
//==============================================================================

#include "invoke64.h"

#include "seh.h"

#include <type_traits>

using jsdi::abi_amd64::param_register_types;

extern "C" {

uint64_t invoke64_ll_basic(size_t args_size_bytes, const void * args_ptr,
                           void * func_ptr);

uint64_t invoke64_ll_fp(size_t args_size_bytes, const void * args_ptr,
                        void * func_ptr, param_register_types register_types);

} // extern "C"

static_assert(std::is_standard_layout<param_register_types>::value, 
              "must be standard layout to pass to invoke64_ll_* functions");

//==============================================================================
//                              struct invoke64
//==============================================================================

namespace jsdi {
namespace abi_amd64 {

uint64_t invoke64::basic(size_t args_size_bytes, const void * args_ptr,
                         void * func_ptr)
{
    SEH_CONVERT_TO_CPP_BEGIN
    return invoke64_ll_basic(args_size_bytes, args_ptr, func_ptr);
    SEH_CONVERT_TO_CPP_END
    return uint64_t(0); // Squelch compiler warning
}

uint64_t invoke64::fp(size_t args_size_bytes, const void * args_ptr,
                      void * func_ptr, param_register_types register_types)
{
    SEH_CONVERT_TO_CPP_BEGIN
    return invoke64_ll_fp(args_size_bytes, args_ptr, func_ptr, register_types);
    SEH_CONVERT_TO_CPP_END
    return uint64_t(0);
}

double invoke64::return_double(
    size_t args_size_bytes, const void * args_ptr, void * func_ptr,
    param_register_types register_types)
{
    SEH_CONVERT_TO_CPP_BEGIN
    return reinterpret_cast<
        double (*)(size_t, const void *, void *, param_register_types)
    >(invoke64_ll_fp)(args_size_bytes, args_ptr, func_ptr, register_types);
    SEH_CONVERT_TO_CPP_END
    return 0.0; // Squelch compiler warning
}

float invoke64::return_float(
    size_t args_size_bytes, const void * args_ptr, void * func_ptr,
    param_register_types register_types)
{
    SEH_CONVERT_TO_CPP_BEGIN
    return reinterpret_cast<
        float (*)(size_t, const void *, void *, param_register_types)
    >(invoke64_ll_fp)(args_size_bytes, args_ptr, func_ptr, register_types);
    SEH_CONVERT_TO_CPP_END
    return 0.0f; // Squelch compiler warning
}

} // namespace abi_amd64
} // namespace jsdi

//==============================================================================
//                                  TESTS
//==============================================================================

#ifndef __NOTEST__

#include "test.h"
#include "test_exports.h"

#include "jsdi_windows.h"
#include "util.h"

#include "test64.h"

using namespace jsdi::abi_amd64;
using namespace jsdi::abi_amd64::test64;

//==============================================================================
//                         TESTS : invoke64::basic()
//==============================================================================

namespace {

// EXTRA TEST FUNCTIONS

int32_t sum_string_byval(Recursive_StringSum x)
{ return TestSumString(&x); }

uint64_t divide(uint64_t a, uint64_t b)
{ return a / b; /* if b == 0, raises EXCEPTION_INT_DIVIDE_BY_ZERO */ }

} // anonymous namespace 

TEST(basic_int8,
    int8_t x = static_cast<int8_t>('J');
    uint64_t a;
    assert_true(copy_to(x, &a, 1));
    int8_t result = static_cast<int8_t>(invoke64::basic(sizeof(a), &a, TestInt8));
    assert_equals(result, static_cast<int8_t>('J'));
);

TEST(basic_int32,
    constexpr size_t N = 9;
    void * f[] = { TestInt32, TestSumTwoInt32s, TestSumThreeInt32s,
                   TestSumFourInt32s, TestSumFiveInt32s, TestSumSixInt32s,
                   TestSumSevenInt32s, TestSumEightInt32s, TestSumNineInt32s };
    uint64_t a[N];
    uint64_t sum;
    for (size_t i = 0; i < N; ++i)
    {
        sum = 0;
        for (size_t j = 0; j <= i; ++j)
        {
            a[j] = i + j;
            sum += a[j];
        }
        int32_t result = static_cast<int32_t>(invoke64::basic(
                                              (i + 1) * sizeof(uint64_t), &a, f[i]));
        assert_equals(static_cast<int32_t>(sum), result);
    }
);

TEST(basic_swap,
    // Simple test passing one argument by pointer.
    Swap_StringInt32Int32 s, * ps(&s);
    s.str = nullptr;
    s.a = 33;
    s.b = 33;
    assert_true(static_cast<int32_t>(invoke64::basic(sizeof(ps), &ps, TestSwap)));
    assert_equals(33, s.a);
    assert_equals(33, s.b);
    assert_equals(std::string("="), s.str);
    ++s.b;
    assert_false(static_cast<int32_t>(invoke64::basic(sizeof(ps), &ps, TestSwap)));
    assert_equals(34, s.a);
    assert_equals(33, s.b);
    assert_equals(std::string("!="), s.str);
);

TEST(basic_aligned_byval,
    // Pass an "aligned" struct by value. Because the size is a power of 2 and
    // it is 8 bytes or less, it actually goes by value.
    Packed_Int8Int8Int16Int32 a = { -1, -2, -3, -4 };
    static_assert(8 == sizeof(a), "test assumes wrong structure size");
    uint64_t b(0);
    assert_true(copy_to(a, &b, 1));
    int32_t result = static_cast<int32_t>(invoke64::basic(sizeof(b), &b,
                                          TestSumPackedInt8Int8Int16Int32));
    assert_equals(result, -10);
);

TEST(basic_unaligned_byval,
    // Pass an "unaligned" struct by value.  Because the size is not a power of
    // 2, it secretly has to be passed by pointer according to the x64 ABI.
    Packed_Int8x3 u = { -5, 13, -1 }, *pu(&u);
    static_assert(3 == sizeof(u), "test assumes wrong structure size");
    int32_t result = static_cast<int32_t>(invoke64::basic(sizeof(pu), &pu,
                                                          TestSumPackedInt8x3));
    assert_equals(result, 7);
);

TEST(basic_oversize_byval,
    // Pass an "oversized" struct by value. Because the size is greater than 8,
    // it secretly has to be passed by pointer according to the x64 ABI.
    Recursive_StringSum r, * pr(&r);
    static_assert(48 == sizeof(r), "test assumes wrong structure size");
    char buffer[16];
    r.x[0].a = 10; r.x[0].b =  20; r.x[0].c =  30; r.x[0].d =  40;
    r.x[1].a = -5; r.x[1].b = -15; r.x[1].c = -25; r.x[1].d = -35;
    r.str = "-45";
    r.buffer = buffer;
    r.len = static_cast<int32_t>(jsdi::array_length(buffer));
    r.inner = nullptr;
    int32_t result = static_cast<int32_t>(invoke64::basic(sizeof(pr), &pr,
                                                          sum_string_byval));
    assert_equals(-25, result);
    assert_equals(std::string("-25"), buffer);
);

TEST(basic_comprehensive,
    // This test has a number of purposes:
    //     1) It tests the potential boundary condition of 9 arguments.
    //     2) It tests pass-by-value structs both in the four register slots
    //        and in the stack slots.
    //     3) Of the pass-by-value structs, it tests both types that go on the
    //        stack and those that go by pointer.
    int8_t a(0);
    int16_t b(1);
    int32_t c(2);
    Swap_StringInt32Int32 d = { nullptr, -1, 1 }, * pd(&d); // This contributes 0 to sum
    int64_t e(3);
    Packed_Int8Int8Int16Int32 f = { 4, 5, 6, 7 };
    Packed_Int8x3 g = { 8, 9, 10 }, * pg(&g);
    Recursive_StringSum h = { { { 11, 12, 13, 14 }, { 15, 16, 17, 18 } }, "19", nullptr, 0, nullptr }, * ph(&h);
    Recursive_StringSum i = { { { 20, 21, 22, 23 }, { 24, 25, 26, 27 } }, "28", nullptr, 0, nullptr }, * pi(&i);
    uint64_t args[9];
    assert_true(copy_to(a,  &args[0], 1));
    assert_true(copy_to(b,  &args[1], 1));
    assert_true(copy_to(c,  &args[2], 1));
    static_assert(16 == sizeof(d), "test assumes wrong structure size");
    assert_true(copy_to(pd, &args[3], 1)); // Pointer because > 8 bytes
    assert_true(copy_to(e,  &args[4], 1));
    assert_true(copy_to(f,  &args[5], 1));
    static_assert(3 == sizeof(g), "test assumes wrong structure size");
    assert_true(copy_to(pg, &args[6], 1)); // Pointer because size not power of 2
    static_assert(48 == sizeof(h), "test assumes wrong structure size");
    assert_true(copy_to(ph, &args[7], 1)); // Pointer because > 8 bytes
    assert_true(copy_to(pi, &args[8], 1)); // Pointer because parameter is pass by pointer
    int64_t result = static_cast<int64_t>(invoke64::basic(sizeof(args), args,
                                                          TestSumManyInts));
    assert_equals(406, result);
);

TEST(basic_seh,
    // This test ensures that on a simple level, the invoke64::basic() function
    // works with Windows structured exception handling.
    uint64_t a[] = { 0, 0 };
    bool caught = false;
    try
    { invoke64::basic(sizeof(a), a, divide); } 
    catch (jsdi::seh_exception const& e)
    { caught = std::string("win32 exception: INT_DIVIDE_BY_ZERO") == e.what(); }
    assert_true(caught);
);

//==============================================================================
//                          TESTS : invoke64::fp()
//==============================================================================

#include <algorithm>

TEST(fp_noargs,
    invoke64::fp(0, nullptr, TestVoid, param_register_types());
);

TEST(fp_comprehensive,
    std::vector<uint64_t> args;
    std::for_each(
        test64::FP_FUNCTIONS.begin, test64::FP_FUNCTIONS.end,
        [this, &args](auto f)
        {
            assert_true(param_register_type::UINT64 == f->func.ret_type);
            size_t sum(0);
            args.resize(f->func.nargs);
            for (size_t i = 0; i < f->func.nargs; ++i)
            {
                sum += i;
                switch (f->func.arg_types[i])
                {
                    case param_register_type::UINT64:
                        args[i] = static_cast<uint64_t>(i);
                        break;
                    case param_register_type::DOUBLE:
                        assert_true(copy_to(static_cast<double>(i), &args[i], 1));
                        break;
                    case param_register_type::FLOAT:
                        assert_true(copy_to(static_cast<float>(i), &args[i], 1));
                        break;
                    default:
                        assert(false || !"control should never pass here");
                }
            } // for(args)
            uint64_t result = invoke64::fp(f->func.nargs * sizeof(uint64_t),
                                           &args[0], f->func.ptr,
                                           f->func.register_types);
            assert_equals(sum, result);
        } // lambda
    ); // std::for_each(FP_FUNCTIONS)
);

TEST(fp_seh,
    // This test ensures that on a basic level, the invoke64::fp() function
    // works with Windows structured exception handling.
    uint64_t a[] = { 0, 0 };
    bool caught = false;
    try
    { invoke64::fp(sizeof(a), a, divide, param_register_types()); } 
    catch(jsdi::seh_exception const& e)
    { caught = std::string("win32 exception: INT_DIVIDE_BY_ZERO") == e.what(); }
    assert_true(caught);
);

//==============================================================================
//                     TESTS : invoke64::return_double()
//==============================================================================

TEST(return_double,
    static double args_double[2] = { -2300.5, -2.0 };
    static uint64_t args[2];
    assert_true(copy_to(args_double[0], &args[0], 1));
    assert_true(copy_to(args_double[1], &args[1], 1));
    param_register_types rt(
        DOUBLE, DOUBLE,
        param_register_type::UINT64, param_register_type::UINT64);
    assert_equals(1.0, invoke64::return_double(0, args, TestReturn1_0Double, rt));
    assert_equals(
        -2300.5,
        invoke64::return_double(sizeof(uint64_t), args, TestDouble, rt));
    assert_equals(
        -2.0,
        invoke64::return_double(sizeof(uint64_t), args + 1, TestDouble, rt));
    assert_equals(
        -2302.5,
        invoke64::return_double(
            2 * sizeof(uint64_t), args, TestSumTwoDoubles, rt));
);

//==============================================================================
//                     TESTS : invoke64::return_float()
//==============================================================================

TEST(return_float,
    static float args_float[2] = { 10.0f, -1.0f };
    static uint64_t args[2];
    assert_true(copy_to(args_float[0], &args[0], 1));
    assert_true(copy_to(args_float[1], &args[1], 1));
    param_register_types rt(
        param_register_type::FLOAT, param_register_type::FLOAT,
        param_register_type::UINT64, param_register_type::UINT64);
    assert_equals(1.0f, invoke64::return_float(0, args, TestReturn1_0Float, rt));
    assert_equals(
        10.0f, invoke64::return_float(sizeof(uint64_t), args, TestFloat, rt));
    assert_equals(
        -1.0, invoke64::return_float(sizeof(uint64_t), args + 1, TestFloat, rt));
    assert_equals(
        9.0,
        invoke64::return_float(
            2 * sizeof(uint64_t), args, TestSumTwoFloats, rt));
);

#endif // __NOTEST__

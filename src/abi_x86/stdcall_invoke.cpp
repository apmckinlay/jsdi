/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: stdcall_invoke.cpp
// auth: Victor Schappert
// date: 20130805
// desc: stdcall invocation translation unit
//==============================================================================

#include "stdcall_invoke.h"

#ifndef __NOTEST__

#include "test.h"
#include "test_exports.h"

#include <limits>

namespace {

template<typename FuncPtr, typename ArgType>
int64_t basic_invoke(FuncPtr f, size_t nargs, ArgType * args)
{
    return static_cast<int64_t>(jsdi::abi_x86::stdcall_invoke::basic(
        nargs * sizeof(ArgType), args, reinterpret_cast<void *>(f)
    ));
}

template<typename FuncPtr, typename ArgType>
double double_invoke(FuncPtr f, size_t nargs, ArgType * args)
{
    return jsdi::abi_x86::stdcall_invoke::return_double(
        nargs * sizeof(ArgType), args, reinterpret_cast<void *>(f)
    );
}

} // anonymous namespace

TEST(basic,
    union
    {
        uint32_t a[4];
        const char * str;
        const char ** pstr;
        Packed_Int8Int8Int16Int32 p_ccsl;
        int64_t int64;
    };
    basic_invoke(TestVoid, 0, a);
    a[0] = static_cast<int32_t>('a');
    assert_equals('a', static_cast<char>(basic_invoke(TestInt8, 1, a)));
    a[0] = 0xf1;
    assert_equals(0xf1, static_cast<int16_t>(basic_invoke(TestInt16, 1, a)));
    a[0] = 0x20130725;
    assert_equals(0x20130725, static_cast<int32_t>(basic_invoke(TestInt32,
                                                                1, a)));
    int64 = std::numeric_limits<int64_t>::min();
    assert_equals(
        std::numeric_limits<int64_t>::min(),
        basic_invoke(TestInt64, 2, a)
    );
    a[0] = 0x80; // this is -128 as a char
    a[1] = 0x7f; // this is 127 as a char
    assert_equals(
        static_cast<int8_t>(0xff),
        static_cast<int8_t>(basic_invoke(TestSumTwoInt8s, 2, a))
    );
    a[0] = 0x8000;
    a[1] = 0x7fff;
    assert_equals(
        static_cast<int16_t>(0xffff),
        static_cast<int16_t>(basic_invoke(TestSumTwoInt16s, 2, a))
    );
    a[0] = std::numeric_limits<int32_t>::min() + 5;
    a[1] = -5;
    assert_equals(
        std::numeric_limits<int32_t>::min(),
        static_cast<int32_t>(basic_invoke(TestSumTwoInt32s, 2, a))
    );
    a[2] = std::numeric_limits<int32_t>::max();
    assert_equals(
        std::numeric_limits<int32_t>::max() +
            std::numeric_limits<int32_t>::min(),
        static_cast<int32_t>(basic_invoke(TestSumThreeInt32s, 3, a))
    );
    a[0] = -100;
    a[1] =   99;
    a[2] = -200;
    a[3] =  199;
    assert_equals(-2, static_cast<int32_t>(basic_invoke(TestSumFourInt32s, 4, a)));
    a[0] = -1;
    *reinterpret_cast<int64_t *>(a + 1) = std::numeric_limits<int64_t>::max() - 2;
    assert_equals(
        std::numeric_limits<int64_t>::max() - 3,
        basic_invoke(TestSumInt8PlusInt64, 3, a)
    );
    p_ccsl.a = -1;
    p_ccsl.b = -3;
    p_ccsl.c = -129;
    p_ccsl.d = -70000;
    assert_equals(
        -70133,
        static_cast<int32_t>(basic_invoke(
            TestSumPackedInt8Int8Int16Int32,
            (sizeof(p_ccsl) + sizeof(uint32_t) - 1) / sizeof(uint32_t),
            a
        ))
    );
    str =
        "From hence ye beauties, undeceived,     \n"
        "Know, one false step is ne'er retrieved,\n"
        "    And be with caution bold.           \n"
        "Not all that tempts your wandering eyes \n"
        "And heedless hearts is lawful prize;    \n"
        "    Nor all that glitters, gold.         ";
    assert_equals(41*6, static_cast<int32_t>(basic_invoke(TestStrLen, 1, a)));
    a[0] = 1; // true
    assert_equals(
        std::string("hello world"),
        reinterpret_cast<const char *>(basic_invoke(TestHelloWorldReturn, 1, a))
    );
    const char * tmp_str(0);
    pstr = &tmp_str;
    basic_invoke(TestHelloWorldOutParam, 1, a);
    assert_equals(std::string("hello world"), tmp_str);
    basic_invoke(TestNullPtrOutParam, 1, a);
    assert_equals(0, tmp_str);
);

TEST(return_double,
    {
        static float args[2] = { 10.0f, -1.0f };
        assert_equals(1.0, double_invoke(TestReturn1_0Float, 0, args));
        assert_equals(10.0, double_invoke(TestFloat, 1, args));
        assert_equals(-1.0, double_invoke(TestFloat, 1, args + 1));
        assert_equals(9.0, double_invoke(TestSumTwoFloats, 2, args));
    }
    {
        static double args[2] = { -2300.5, -2.0 };
        assert_equals(1.0, double_invoke(TestReturn1_0Double, 0, args));
        assert_equals(-2300.5, double_invoke(TestDouble, 1, args));
        assert_equals(-2.0, double_invoke(TestDouble, 1, args + 1));
        assert_equals(-2302.5, double_invoke(TestSumTwoDoubles, 2, args));
    }
);

#endif // __NOTEST__

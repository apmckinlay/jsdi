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

template<typename FuncPtr>
inline int64_t basic_invoke(FuncPtr f, int nlongs, long * args)
{
    return static_cast<int64_t>(jsdi::stdcall_invoke::basic(
        nlongs * sizeof(long),
        reinterpret_cast<const char *>(args),
        reinterpret_cast<void *>(f)
    ));
}

TEST(basic,
    union
    {
        long a[4];
        const char * str;
        const char ** pstr;
        Packed_CharCharShortLong p_ccsl;
        int64_t int64;
    };
    basic_invoke(TestVoid, 0, a);
    a[0] = static_cast<long>('a');
    assert_equals('a', static_cast<char>(basic_invoke(TestChar, 1, a)));
    a[0] = 0xf1;
    assert_equals(0xf1, static_cast<short>(basic_invoke(TestShort, 1, a)));
    a[0] = 0x20130725;
    assert_equals(0x20130725, static_cast<long>(basic_invoke(TestLong, 1, a)));
    int64 = std::numeric_limits<int64_t>::min();
    assert_equals(
        std::numeric_limits<int64_t>::min(),
        basic_invoke(TestInt64, 2, a)
    );
    a[0] = 0x80; // this is -128 as a char
    a[1] = 0x7f; // this is 127 as a char
    assert_equals(
        static_cast<char>(0xff),
        static_cast<char>(basic_invoke(TestSumTwoChars, 2, a))
    );
    a[0] = 0x8000;
    a[1] = 0x7fff;
    assert_equals(
        static_cast<short>(0xffff),
        static_cast<short>(basic_invoke(TestSumTwoShorts, 2, a))
    );
    a[0] = std::numeric_limits<long>::min() + 5;
    a[1] = -5;
    assert_equals(
        std::numeric_limits<long>::min(),
        static_cast<long>(basic_invoke(TestSumTwoLongs, 2, a))
    );
    a[2] = std::numeric_limits<long>::max();
    assert_equals(
        std::numeric_limits<long>::max() + std::numeric_limits<long>::min(),
        static_cast<long>(basic_invoke(TestSumThreeLongs, 3, a))
    );
    a[0] = -100;
    a[1] =   99;
    a[2] = -200;
    a[3] =  199;
    assert_equals(-2, static_cast<long>(basic_invoke(TestSumFourLongs, 4, a)));
    a[0] = -1;
    *reinterpret_cast<int64_t *>(a + 1) = std::numeric_limits<int64_t>::max() - 2;
    assert_equals(
        std::numeric_limits<int64_t>::max() - 3,
        basic_invoke(TestSumCharPlusInt64, 3, a)
    );
    p_ccsl.a = -1;
    p_ccsl.b = -3;
    p_ccsl.c = -129;
    p_ccsl.d = -70000;
    assert_equals(
        -70133,
        static_cast<long>(basic_invoke(
            TestSumPackedCharCharShortLong,
            (sizeof(p_ccsl) + sizeof(long) - 1) / sizeof(long),
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
    assert_equals(41*6, static_cast<long>(basic_invoke(TestStrLen, 1, a)));
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

#endif // __NOTEST__

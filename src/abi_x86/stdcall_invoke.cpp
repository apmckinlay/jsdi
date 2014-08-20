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

#include "seh.h"

namespace jsdi {
namespace abi_x86 {

uint64_t stdcall_invoke::basic(int args_size_bytes, const void * args_ptr,
                               void * func_ptr)
{
    SEH_CONVERT_TO_CPP_BEGIN
    assert(
        0 == args_size_bytes % 4
            || !"argument size must be a multiple of 4 bytes");
#if defined(__GNUC__)
    uint64_t result;
    asm volatile
    (
        /* OUTPUT PARAMS: %0 gets the 64-bit value EDX << 32 | EAX */
        /* INPUT PARAMS:  %1 is the number of BYTES to push onto the stack, */
        /*                   and during the copy loop it is the address of */
        /*                   the next word to push */
        /*                %2 is the base address of the array */
        /*                %3 is the address of the function to call */
            "testl %1, %1        # If zero argument bytes given, skip  \n\t"
            "je    2f            # right to the function call.         \n\t"
            "addl  %2, %1\n"
        "1:\n\t"
            "subl  $4, %1        # Push arguments onto the stack in    \n\t"
            "pushl (%1)          # reverse order. Keep looping while   \n\t"
            "cmp   %2, %1        # addr to push (%1) > base addr (%2). \n\t"
            "jg    1b            # Callee cleans up b/c __stdcall.     \n"
        "2:\n\t"
            "call  * %3"
        : "=A" (result)
        : "r" (args_size_bytes), "r" (args_ptr), "r" (func_ptr)
        : "%ecx" /* eax, ecx, edx are caller-save */, "cc", "memory"
    );
    return result;
#elif defined(_MSC_VER)
    uint32_t result[2];
    // The assembly code below tries to clobber only the registers that are
    // caller save under __stdcall anyway: eax, ecx, and edx. Hopefully this
    // will cause MSVC to generate the minimum amount of register preserving
    // code around this block.
    __asm
    {
        mov   eax, args_size_bytes
        test  eax, eax           // If zero argument bytes given, skip
        je    ms_asm_basic_call  // right to the funtion call.
        mov   edx, args_ptr
        add   eax, edx
    ms_asm_basic_loop:
        sub   eax, 4             // Push arguments onto the stack in
        push  dword ptr [eax]    // reverse order. Keep looping while
        cmp   eax, edx           // addr to push (eax) > base addr (edx).
        jg    ms_asm_basic_loop  // Callee cleans up b/c __stdcall.
    ms_asm_basic_call:
        mov   ecx, func_ptr      // Might as well clobber ecx since it is caller
        call  ecx                // save under __stdcall anyway.
        mov   result[0 * type int], eax
        mov   result[1 * type int], edx
    }
    return *reinterpret_cast<uint64_t *>(result);
#else
#error replacement for inline assembler required
#endif
    SEH_CONVERT_TO_CPP_END
    return 0; // Squelch compiler warning
}

double stdcall_invoke::return_double(int args_size_bytes,
                                     const void * args_ptr, void * func_ptr)
{
    double result;
    assert(
        0 == args_size_bytes % 4
            || !"argument size must be a multiple of 4 bytes");
    SEH_CONVERT_TO_CPP_BEGIN
#if defined(__GNUC__)
    asm volatile
    (
        /* INPUT PARAMS:  %0 is the variable where ST0 should be stored
         *                %1 is the number of BYTES to push onto the stack, */
        /*                   and during the copy loop it is the address of */
        /*                   the next word to push */
        /*                %2 is the base address of the array */
        /*                %3 is the address of the function to call */
            "testl %1, %1        # If zero argument bytes given, skip \n\t"
            "je    2f            # right to the function call.        \n\t"
            "addl  %2, %1\n"
        "1:\n\t"
            "subl  $4, %1        # Push arguments onto the stack in   \n\t"
            "pushl (%1)          # reverse order. Keep looping while  \n\t"
            "cmp   %2, %1        # addr to push (%1) > base addr (%2) \n\t"
            "jg    1b            # Callee cleans up b/c __stdcall.    \n"
        "2:\n\t"
            "call  * %3          # Callee will leave result in ST0."
        : "=t" (result)
        : "r" (args_size_bytes), "r" (args_ptr), "r" (func_ptr)
        : "%eax", "%edx", "%ecx" /* eax, ecx, edx are caller-save */, "cc",
          "memory"
    );
#elif defined(_MSC_VER)
    __asm
    {
        mov   eax, args_size_bytes
        test  eax, eax           // If zero argument bytes given, skip
        je    ms_asm_double_call // right to the funtion call.
        mov   edx, args_ptr
        add   eax, edx
    ms_asm_double_loop:
        sub   eax, 4             // Push arguments onto the stack in
        push  dword ptr [eax]    // reverse order. Keep looping while
        cmp   eax, edx           // addr to push (eax) > base addr (edx).
        jg    ms_asm_double_loop // Callee cleans up b/c __stdcall.
    ms_asm_double_call:
        mov   ecx, func_ptr      // Might as well clobber ecx since it is caller
        call  ecx                // save under __stdcall anyway.
        fstp  result             // Callee will leave result in ST0.
    }
#else
#error replacement for inline assembler required
#endif
    SEH_CONVERT_TO_CPP_END
    return result;
}

} // namespace abi_x86
} // namespace jsdi

//==============================================================================
//                                  TESTS
//==============================================================================

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
        static_cast<int8_t>(0xffu),
        static_cast<int8_t>(basic_invoke(TestSumTwoInt8s, 2, a))
    );
    a[0] = 0x8000;
    a[1] = 0x7fff;
    assert_equals(
        static_cast<int16_t>(0xffffu),
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
            // TODO: use marshalling.h's min_whole_words<uint32_t> for this
            //       (it is crashing MSVC November 2013 CTP as of 20140729)
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

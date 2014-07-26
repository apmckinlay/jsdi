/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_STDCALL_INVOKE_H___
#define __INCLUDED_STDCALL_INVOKE_H___

/**
 * \file stdcall_invoke.h
 * \author Victor Schappert
 * \since 20130805
 * \brief Generic system for invoking functions according to the __stdcall
 *        calling convention
 */

#include <cassert>
#include <cstdint>

namespace jsdi {
namespace abi_x86 {

/**
 * Contains generic functions for invoking other functions according to the
 * __stdcall calling convention.
 * \author Victor Schappert
 * \since 20130805
 * \see stdcall_thunk
 */
struct stdcall_invoke
{
        /**
         * \brief Invokes a <code>__stdcall</code> function which expects all
         * parameters on the stack and which places its return value in the
         * EAX/EDX pair.
         * \param args_size_bytes Number of bytes pointed to by
         * <code>args_ptr</code> &mdash; <em>must be a multiple of 4 bytes!</em>
         * \param args_ptr Pointer to the arguments to push on the stack
         * \param func_ptr Pointer to the <code>__stdcall</code> function to
         * call
         * \return A 64-bit unsigned integer in which the low-order 32 bits are
         * the contents of EAX after invoking <code>func_ptr</code> and the
         * high-order 32 bits are the content of EDX after invoking
         * <code>func_ptr</code>
         * \see #return_double(int, const void *, void *)
         * \since 20130805
         *
         * This function is very limited in capability. In particular, it cannot:
         *     - Call stdcall functions which return floating-point values. This
         *       is because such values are returned in ST(0), the top register
         *       of the floating-point stack, not in the EAX/EDX pair, under
         *       stdcall.
         *     - Call stdcall functions which return aggregates of 5-7, or 9+
         *       bytes in size, because these functions require the caller to
         *       pass the address of the return value as a silent parameter in
         *       EAX.
         *
         * See <a href="http://pic.dhe.ibm.com/infocenter/ratdevz/v7r5/index.jsp?topic=%2Fcom.ibm.etools.pl1.win.doc%2Ftopics%2Fxf6700.htm">here</a>
         * and <a href="http://stackoverflow.com/q/17912828/1911388">here</a>.
         */
        static uint64_t basic(int args_size_bytes, const void * args_ptr,
                              void * func_ptr);

        /**
         * \brief Invokes a <code>__stdcall</code> function which expects all
         * parameters on the stack and which places its return value at the
         * top of the floating-point stack (ST0).
         * \param args_size_bytes Number of bytes pointed to by
         * <code>args_ptr</code> &mdash; <em>must be a multiple of 4 bytes!</em>
         * \param args_ptr Pointer to the arguments to push on the stack
         * \param func_ptr Pointer to the <code>__stdcall</code> function to call
         * \return The `double` returned by calling <code>func_ptr</code>
         * \see #basic(int, const void *, void *)
         * \since 20130808
         *
         * Note that this invoker should be used for <code>stdcall</code>
         * functions which specify <code>float</code> as their return type as
         * well as those which indicate <code>double</code>. Since the return
         * value comes off of the top floating point register (which has higher
         * precision than either <code>float</code> or <code>double</code>), we
         * might as well take the <code>double</code> option.
         */
        static double return_double(int args_size_bytes, const void * args_ptr,
                                    void * func_ptr);
};

inline uint64_t stdcall_invoke::basic(int args_size_bytes,
                                      const void * args_ptr, void * func_ptr)
{
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
}

inline double stdcall_invoke::return_double(
    int args_size_bytes, const void * args_ptr, void * func_ptr)
{
    double result;
    assert(
        0 == args_size_bytes % 4
            || !"argument size must be a multiple of 4 bytes");
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
    return result;
}

} // namespace abi_x86
} // namespace jsdi

#endif // __INCLUDED_STDCALL_INVOKE_H___

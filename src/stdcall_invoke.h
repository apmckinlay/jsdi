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

/**
 * Contains generic functions for invoking other functions according to the
 * __stdcall calling convention.
 * \see stdcall_thunk
 */
struct stdcall_invoke
{
        /**
         * \brief Invokes a <dfn>__stdcall</dfn> function which expects all
         * parameters on the stack and which places its return value in the
         * EAX/EDX pair.
         * \param args_size_bytes Number of bytes pointed to by
         * <dfn>args_ptr</dfn> &mdash; <em>must be a multiple of 4 bytes!</em>
         * \param args_ptr Pointer to the arguments to push on the stack
         * \param func_ptr Pointer to the <dfn>__stdcall</dfn> function to call
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
        static uint64_t basic(int args_size_bytes, const char * args_ptr,
                              void * func_ptr);
};

inline uint64_t stdcall_invoke::basic(int args_size_bytes,
                                      const char * args_ptr, void * func_ptr)
{
    uint32_t hi32, lo32;
    assert(
        0 == args_size_bytes % 4
            || !"argument size must be a multiple of 4 bytes");
#if defined(__GNUC__)
    asm
    (
        /* OUTPUT PARAMS: %0 gets the high-order 32 bits of the return value */
        /*                %1 gets the low-order 32 bits of the return value */
        /* INPUT PARAMS:  %2 is the number of BYTES to push onto the stack, */
        /*                   and during the copy loop it is the address of */
        /*                   the next word to push */
        /*                %3 is the base address of the array */
        /*                %4 is the address of the function to call */
            "testl %2, %2    # If zero argument bytes given, skip \n\t"
            "je    2f        # right to the function call.        \n\t"
            "addl  %3, %2\n"
        "1:\n\t"
            "subl  $4, %2    # Push arguments onto the stack in   \n\t"
            "pushl (%2)      # reverse order. Keep looping while  \n\t"
            "cmp   %3, %2    # addr to push (%2) > base addr (%3) \n\t"
            "jg    1b        # Callee cleans up b/c __stdcall.    \n"
        "2:\n\t"
            "call  * %4"
        : "=d" (hi32), "=a" (lo32)
        : "0" (args_size_bytes), "1" (args_ptr), "mr" (func_ptr)
        : "%ecx" /* eax, ecx, edx are caller-save */, "cc"
    );
#else
#pragma error "Replacement for inline assembler required"
#endif
    return static_cast<uint64_t>(hi32) << 32 | lo32;
}

} // namespace jsdi

#endif // __INCLUDED_STDCALL_INVOKE_H___

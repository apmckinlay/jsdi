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

#include "util.h"

#include <cassert>
#include <cstdint>

namespace jsdi {
namespace abi_x86 {

/**
 * \brief Contains generic functions for invoking other functions according to
 *        the <code>__stdcall</code> calling convention.
 * \author Victor Schappert
 * \since 20130805
 * \see stdcall_thunk
 *
 * \remark
 * The functions in this namespace use \link SEH_CONVERT_TO_CPP_BEGIN\endlink
 * and \link SEH_CONVERT_TO_CPP_END\endlink to rethrow non-fatal structured
 * exception handling exceptions as jsdi::seh_exception.
 */
struct stdcall_invoke : private non_instantiable
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
         * \throws jsdi::seh_exception If a structure exception handling
         *         exception occurs during the invocation process
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
         * \throws jsdi::seh_exception If a structure exception handling
         *         exception occurs during the invocation process
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

} // namespace abi_x86
} // namespace jsdi

#endif // __INCLUDED_STDCALL_INVOKE_H___

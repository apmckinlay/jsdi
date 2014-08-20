/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_INVOKE64_H___
#define __INCLUDED_INVOKE64_H___

/**
 * \file invoke64.h
 * \author Victor Schappert
 * \since 20140707
 * \brief Generic system for invoking functions according to Microsoft Windows
 *        x64 ABI
 */

#include "register64.h"
#include "util.h"

#include <cstdint>

namespace jsdi {
namespace abi_amd64 {

/**
 * \brief Contains generic functions for invoking other functions using the
 *        Microsoft Windows x64 ABI.
 * \author Victor Schappert
 * \since 20140819
 * \see thunk64
 *
 * \remark
 * The functions in this namespace use \link SEH_CONVERT_TO_CPP_BEGIN\endlink
 * and \link SEH_CONVERT_TO_CPP_END\endlink to rethrow non-fatal structured
 * exception handling exceptions as jsdi::seh_exception.
 */
struct invoke64 : private non_instantiable
{
        /**
         * \brief Invokes a function using the Windows x64 ABI calling
         *        convention where none of the first four parameters or the
         *        return value have type <code>double</code> or
         *        <code>float</code>
         * \author Victor Schappert
         * \since 20140708
         * \param args_size_bytes Size of the arguments pointed-to by
         *                        <code>args_ptr</code> <em>must be a multiple
         *                        of 8</em>
         * \param args_ptr Pointer to the arguments <em>must be 8-byte
         *                 aligned</em>, may be <code>null</code> <em>only</em>
         *                 if <code>args_size_bytes</code> is 0
         * \param func_ptr Pointer to the function to call
         * \throws jsdi::seh_exception If a structure exception handling
         *         exception occurs during the invocation process
         * \see #fp(size_t, const void *, void *, param_register_types)
         *
         * \warning
         * Please use this function with care as failure to follow the contract
         * will crash the process.
         *
         * This function may be used to invoke a <code>func_ptr</code> meeting
         * the following constraints:
         *
         * - it does not expect floating-point primitives (<em>ie</em>
         *   <code>float</code>, <code>double</code> in its first four
         *   parameters); and
         * - it does not return a floating-point primitive.
         *
         * The arguments in <code>args_ptr</code> must meet the following
         * criteria:
         *
         * - <code>args_size_bytes</code> must be a multiple of 8;
         * - <code>args_ptr</code> may only be <code>null</code> if
         *   <code>args_size_bytes</code> is zero;
         * - the arguments must be organized in left-to-right order (<em>ie</em>
         *   <code>args_ptr[0]</code> is the first argument,
         *   <code>args_ptr[1]</code> is the second argument, and so on); and
         * - in all cases where <code>func_ptr</code> expects a pointer,
         *   <em>including</em> for pass-by-value types that the ABI specifies
         *   are actually passed by reference, the relevant argument must be a
         *   pointer; and
         * - in all cases, the value of an argument must be such that if the
         *   8-byte argument value is "cast" to the type expected by
         *   <code>func_ptr</code>, it contains a valid value.
         */
        static uint64_t basic(size_t args_size_bytes, const void * args_ptr,
                              void * func_ptr);

        /**
         * \brief Invokes a function using the Windows x64 ABI calling
         *        convention where one or more of the first four parameters have
         *        type <code>double</code> or <code>float</code> but the return
         *        type is not floating-point
         * \author Victor Schappert
         * \since 20140715
         * \param args_size_bytes Size of the arguments pointed-to by
         *                        <code>args_ptr</code> <em>must be a multiple
         *                        of 8</em>
         * \param args_ptr Pointer to the arguments <em>must be 8-byte
         *                 aligned</em>, may be <code>null</code> <em>only</em>
         *                 if <code>args_size_bytes</code> is 0
         * \param func_ptr Pointer to the function to call
         * \param register_types Register types required for the first four
         *        parameters
         * \throws jsdi::seh_exception If a structure exception handling
         *         exception occurs during the invocation process
         * \see #basic(size_t, const void *, void *)
         * \see #return_double(size_t, const void *, void *,
         *                     param_register_types)
         * \see #return_float(size_t, const void *, void *,
         *                    param_register_types)
         *
         * This function may be used to invoke a <code>func_ptr</code> meeting
         * the following constraints:
         *
         * - it does not return a floating-point primitive.
         *
         * The arguments in <code>args_ptr</code> must meet the criteria for
         * <code>args_ptr</code> described in the documentation for 
         * \link #basic(size_t, const void *, void *) basic()\endlink.
         */
        static uint64_t fp(size_t args_size_bytes, const void * args_ptr,
                           void * func_ptr,
                           param_register_types register_types);

        /**
         * \brief Invokes a function using the Windows x64 ABI calling
         *        convention where zero or more of the first four parameters
         *        have type <code>double</code> or <code>float</code> and the
         *        return type is <code>double</code>
         * \author Victor Schappert
         * \since 20140717
         * \param args_size_bytes Size of the arguments pointed-to by
         *                        <code>args_ptr</code> <em>must be a multiple
         *                        of 8</em>
         * \param args_ptr Pointer to the arguments <em>must be 8-byte
         *                 aligned</em>, may be <code>null</code> <em>only</em>
         *                 if <code>args_size_bytes</code> is 0
         * \param func_ptr Pointer to the function to call
         * \param register_types Register types required for the first four
         *        parameters
         * \throws jsdi::seh_exception If a structure exception handling
         *         exception occurs during the invocation process
         * \see #return_float(size_t, const void *, void *,
         *                    param_register_types)
         *
         * This function behaves identically to
         * \link #fp(size_t, const void *, void *, param_register_types)
         * fp()\endlink except that it must be used to invoke functions whose
         * return type is <code>double</code>.
         */
        static double return_double(
            size_t args_size_bytes, const void * args_ptr, void * func_ptr,
            param_register_types register_types);

        /**
         * \brief Invokes a function using the Windows x64 ABI calling
         *        convention where zero or more of the first four parameters
         *        have type <code>double</code> or <code>float</code> and the
         *        return type is <code>float</code>
         * \author Victor Schappert
         * \since 20140717
         * \param args_size_bytes Size of the arguments pointed-to by
         *                        <code>args_ptr</code> <em>must be a multiple
         *                        of 8</em>
         * \param args_ptr Pointer to the arguments <em>must be 8-byte
         *                 aligned</em>, may be <code>null</code> <em>only</em>
         *                 if <code>args_size_bytes</code> is 0
         * \param func_ptr Pointer to the function to call
         * \param register_types Register types required for the first four
         *        parameters
         * \throws jsdi::seh_exception If a structure exception handling
         *         exception occurs during the invocation process
         * \see return_double(size_t, const void *, void *,
         *                    param_register_types)
         *
         * This function behaves identically to
         * \link #fp(size_t, const void *, void *, param_register_types)
         * fp()\endlink except that it must be used to invoke functions whose
         * return type is <code>float</code>.
         */
        static float return_float(
            size_t args_size_bytes, const void * args_ptr, void * func_ptr,
            param_register_types register_types);

}; // struct invoke64

} // namespace abi_amd64
} // namespace jsdi

#endif // __INCLUDED_INVOKE64_H___

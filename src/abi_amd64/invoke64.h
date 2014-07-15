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

#include <cstdint>

extern "C" {

/**
 * \brief Invokes a function using the Windows x64 ABI calling convention
 * \author Victor Schappert
 * \since 20140708
 * \param args_size_bytes Size of the arguments pointed-to by
 *                        <code>args_ptr</code> <em>must be a multiple of 8</em>
 * \param args_ptr Pointer to the arguments <em>must be 8-byte aligned</em>
 * \param func_ptr Pointer to the function to call
 *
 * \warning
 * Please use this function with care as failure to follow the contract will
 * crash the process.
 *
 * This function may be used to invoke a <code>func_ptr</code> meeting the
 * following constraints:
 *
 * - it does not expect floating-point primitives (<em>ie</em>
 *   <code>float</code>, <code>double</code> in its first four parameters); and
 * - it does not return a floating-point primitive.
 *
 * The arguments in <code>args_ptr</code> must meet the following criteria:
 *
 * - <code>args_size_bytes</code> must be a multiple of 8;
 * - <code>args_ptr</code> may only be <code>null</code> if
 *   <code>args_size_bytes</code> is zero;
 * - the arguments must be organized in left-to-right order (<em>ie</em>
 *   <code>args_ptr[0]</code> is the first argument,
 *   <code>args_ptr[1]</code> is the second argument, and so on); and
 * - in all cases where <code>func_ptr</code> expects a pointer,
 *   <em>including</em> for pass-by-value types that the ABI specifies are
 *   actually passed by reference, the relevant argument must be a pointer;
 *   and
 * - in all cases, the value of an argument must be such that if the 8-byte
 *   argument value is "cast" to the type expected by <code>func_ptr</code>, it
 *   contains a valid value.
 */
uint64_t invoke64_basic(size_t args_size_bytes, const void * args_ptr,
                        void * func_ptr);

} // extern "C"

#endif // __INCLUDED_INVOKE64_H___

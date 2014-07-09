/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_ABI_AMD64_INVOKE64_H___
#define __INCLUDED_ABI_AMD64_INVOKE64_H___

/**
 * \file invoke64.h
 * \author Victor Schappert
 * \since 20140707
 * \brief Generic system for invoking functions according to Microsoft Windows
 *        x64 ABI
 */

#include <cstdint>

extern "C" {

uint64_t invoke64_basic(size_t args_size_bytes, const uint64_t * args_ptr,
                        void * func_ptr);

uint64_t invoke64_float(size_t args_size_bytes, const uint64_t * args_ptr,
                        void * func_ptr);

double invoke64_float_return(size_t args_size_bytes, const uint64_t * args_ptr,
                             void * func_ptr);

} // extern "C"

#endif // __INCLUDED_ABI_AMD64_INVOKE64_H___

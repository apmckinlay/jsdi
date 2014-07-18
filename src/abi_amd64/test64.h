/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_TEST64_H___
#define __INCLUDED_TEST64_H___

#ifndef __NOTEST__

/**
 * \file test64.h
 * \author Victor Schappert
 * \since 20140716
 * \brief Test functions used only in the x64 test builds
 */

/**\cond internal*/

#include "register64.h"

#include <vector>

namespace jsdi {
namespace abi_amd64 {
namespace test64 {

template <typename T>
static bool copy_to(const T& src, uint64_t * dst,
                    size_t capacity /* in uint64_t */)
{
    if (sizeof(src) <= sizeof(uint64_t) * capacity)
    {
        std::memcpy(dst, &src, sizeof(src));
        return true;
    }
    return false;
}

struct function_data
{
    void *                           ptr;
    size_t                           nargs;
    param_register_type              ret_type;
    std::vector<param_register_type> arg_types;
    param_register_types             register_types;
    function_data(void *, param_register_type,
                  const std::initializer_list<param_register_type>&);
};

struct function
{
    function_data   func;
    function_data   invoker;
    char            signature[256]; // signature of 'func'
    function(void *, param_register_type,
             const std::initializer_list<param_register_type>&, void *);
};

struct function_list
{
    function const ** begin;
    function const ** end;
};

extern const function_list FP_FUNCTIONS;

} // namespace test64
} // namespace abi_amd64
} // namespace jsdi

/**\endcond internal*/

#endif // __NOTEST__

#endif // __INCLUDED_TEST64_H___
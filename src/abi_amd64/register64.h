/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_REGISTER64_H___
#define __INCLUDED_REGISTER64_H___

/**
 * \file register64.h
 * \author Victor Schappert
 * \since 20140714
 * \brief Utility types for describing registers in x64 ABI
 */

#include <cassert>
#include <cstdint>

namespace jsdi {
namespace abi_amd64 {

/**
 * \brief Number of registers used for parameter passing in Windows x64 ABI
 * \see NUM_PARAM_REGISTER_TYPES 
 *
 * The parameter passing registers, by ordinal position, are:
 * <code>rcx</code> (<code>xmm0</code>); <code>rdx</code> (<code>xmm1</code>);
 * <code>r8</code> (<code>xmm2</code>); <code>r9</code> (<code>xmm3</code).
 */
constexpr size_t NUM_PARAM_REGISTERS      =  4;
/**
 * \brief Number of "types" of parameters passed by register under the Windows
 *        x64 ABI
 *
 * This number is equal to the number of elements in the
 * \link param_register_type\endlink enumeration.
 */
constexpr size_t NUM_PARAM_REGISTER_TYPES =  3;

/**
 * \brief Enumerates the "types" of parameters in the Windows x64 ABI
 * \see NUM_PARAM_REGISTER_TYPES
 *
 * At the lowest level, the parameters that can be passed by register fall into
 * one of the following three type categories:
 *
 * - any non-floating point value that can be represented in 64 bits or less and
 *   placed in a general-purpose register (#PARAM_REGISTER_TYPE_UINT64);
 * - a 64-bit double-precision floating point value
 *   (#PARAM_REGISTER_TYPE_DOUBLE); or
 * - a 32-bit single-precision floating point value
 *   (#PARAM_REGISTER_TYPE_FLOAT).
 */
enum param_register_type
{
    /** \brief Non-floating point value passed in a general-purpose register */
    PARAM_REGISTER_TYPE_UINT64 = 0x0,
    /** \brief 64-bit <code>double</code> value passed in an SSE register */
    PARAM_REGISTER_TYPE_DOUBLE = 0x1,
    /** \brief 32-bit <code>float</code> value passed in an SSE register */
    PARAM_REGISTER_TYPE_FLOAT  = 0x2,
};

/**
 * \brief Stores up to four register parameter types in an 8-bit data structure
 * \author Victor Schappert
 * \since 20140714
 */
class param_register_types
{
        //
        // DATA
        //

    public:

        uint8_t d_data;

        //
        // CONSTRUCTORS
        //

    public:

        param_register_types(param_register_type param0,
                             param_register_type param1,
                             param_register_type param2,
                             param_register_type param3);

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Returns the register "type" of the given parameter number
         * \param param_num Parameter number in the range [0 ..
         *        \link #NUM_PARAM_REGISTERS\endlink]
         * \return Register "type" corresponding to <code>param_num</code>
         */
        const param_register_type operator[](size_t param_num) const;
};

inline param_register_types::param_register_types(param_register_type param0,
                                                  param_register_type param1,
                                                  param_register_type param2,
                                                  param_register_type param3)
    : d_data(param0 | (param1 << 2) | (param2 << 4) | (param3 << 6))
{
    assert(0 <= param0 && param0 < NUM_PARAM_REGISTER_TYPES);
    assert(0 <= param1 && param1 < NUM_PARAM_REGISTER_TYPES);
    assert(0 <= param2 && param2 < NUM_PARAM_REGISTER_TYPES);
    assert(0 <= param3 && param3 < NUM_PARAM_REGISTER_TYPES);
}

inline const param_register_type param_register_types::operator[](
    size_t param_num) const
{
    switch (param_num)
    {
        case 0: return static_cast<param_register_type>(d_data & 0x3);
        case 1: return static_cast<param_register_type>((d_data >> 2) & 0x3);
        case 2: return static_cast<param_register_type>((d_data >> 4) & 0x3);
        case 3: return static_cast<param_register_type>((d_data >> 6) & 0x3);
        default:
            assert(!"control should never pass here");
            return static_cast<param_register_type>(0xffffffff);
    }
}

} // namespace abi_amd64
} // namespace jsdi
 
 #endif // __INCLUDED_REGISTER64_H___
 
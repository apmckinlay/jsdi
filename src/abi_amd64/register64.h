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
 * <code>r8</code> (<code>xmm2</code>); <code>r9</code> (<code>xmm3</code>).
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
 *   placed in a general-purpose register (#UINT64);
 * - a 64-bit double-precision floating point value (#DOUBLE); or
 * - a 32-bit single-precision floating point value (#FLOAT).
 */
enum param_register_type
{
    /** \brief Non-floating point value passed in a general-purpose register */
    UINT64 = 0x0,
    /** \brief 64-bit <code>double</code> value passed in an SSE register */
    DOUBLE = 0x1,
    /** \brief 32-bit <code>float</code> value passed in an SSE register */
    FLOAT  = 0x2,
};

/**
 * \brief Stores up to four register parameter types
 * \author Victor Schappert
 * \since 20140714
 */
class param_register_types
{
        //
        // DATA
        //

        uint32_t d_data;

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Explicitly initializes all four parameter registers
         * \param param0 Register type for the first parameter
         * \param param1 Register type for the second parameter
         * \param param2 Register type for the third parameter
         * \param param3 Register type for the fourth parameter
         * \see #param_register_types()
         */
        param_register_types(param_register_type param0,
                             param_register_type param1,
                             param_register_type param2,
                             param_register_type param3);

        /**
         * \brief Initializes all four register parameter types to
         *        param_register_type#UINT64
         * \see #param_register_types(param_register_type, param_register_type,
         *                            param_register_type, param_register_type)
         */
        param_register_types();

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
        param_register_type operator[](size_t param_num) const;

        /**
         * \brief Indicates whether any floating-point registers are required
         * \returns True iff at least one of the four registers has type
         *          param_register_type#DOUBLE or param_register_type#FLOAT
         */
        bool has_fp() const;
};

inline param_register_types::param_register_types(param_register_type param0,
                                                  param_register_type param1,
                                                  param_register_type param2,
                                                  param_register_type param3)
    : d_data((param0 << 030) | (param1 << 020) | (param2 << 010) | param3)
{
    assert(0 <= param0 && param0 < NUM_PARAM_REGISTER_TYPES);
    assert(0 <= param1 && param1 < NUM_PARAM_REGISTER_TYPES);
    assert(0 <= param2 && param2 < NUM_PARAM_REGISTER_TYPES);
    assert(0 <= param3 && param3 < NUM_PARAM_REGISTER_TYPES);
}

inline param_register_types::param_register_types()
    : param_register_types(UINT64, UINT64, UINT64, UINT64)
{ }

inline param_register_type param_register_types::operator[](
    size_t param_num) const
{
    switch (param_num)
    {
        case 3: return static_cast<param_register_type>(d_data & 0x3);
        case 2: return static_cast<param_register_type>((d_data >> 010) & 0x3);
        case 1: return static_cast<param_register_type>((d_data >> 020) & 0x3);
        case 0: return static_cast<param_register_type>((d_data >> 030) & 0x3);
        default:
            assert(!"control should never pass here");
            return static_cast<param_register_type>(-1);
    }
}

inline bool param_register_types::has_fp() const
{ return 0 < d_data; }

} // namespace abi_amd64
} // namespace jsdi
 
 #endif // __INCLUDED_REGISTER64_H___
 
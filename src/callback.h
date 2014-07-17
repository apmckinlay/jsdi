/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_CALLBACK_H___
#define __INCLUDED_CALLBACK_H___

/**
 * \file callback.h
 * \author Victor Schappert
 * \since 20130804
 * \brief Generic interface for a callback function
 */

#include <vector>
#include <memory>
#include <cassert>

namespace jsdi {

//==============================================================================
//                              class callback
//==============================================================================

/**
 * \brief Interface for a callback function
 * \author Victor Schappert
 * \since 20130804
 *
 * Specific implementations of this class should override
 * #call(const ParamType *).
 */
template<typename ParamType>
class callback
{
        //
        // DATA
        //

    protected:

        /** \cond internal */
        std::vector<int> d_ptr_array;
        int              d_size_direct;
        int              d_size_total;
        int              d_vi_count;
        /** \endcond internal */

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructs a callback with a given set of unmarshalling
         * parameters.
         * \param size_direct Size of the on-stack arguments
         * \param size_indirect Size of data indirectly accessible from
         * pointers passed on the stack
         * \param ptr_array Array of tuples indicating which positions in the
         * direct and indirect storage are pointers, and which positions they
         * point to
         * \param ptr_array_size Size of <dfn>ptr_array</dfn> (must be even)
         * \param vi_count Number of variable indirect pointers that must be
         * unmarshalled
         */
        callback(int size_direct, int size_indirect, const int * ptr_array,
                 int ptr_array_size, int vi_count);

        virtual ~callback();

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Returns the size of the callback's on-stack arguments in
         *  bytes
         * \return Direct argument size in bytes
         */
        int size_direct() const;

        //
        // MUTATORS
        //

    public:

        /**
         * \brief Unmarshalls the parameters, does whatever work is expected,
         * and returns the callback return value
         * \param args Points to the address on the execution stack that is the
         * base of the on-stack arguments in <dfn>stdcall</dfn> format
         * \return Return value of the callback function
         */
        virtual ParamType call(const ParamType * args) = 0;
};

template<typename ParamType>
inline callback<ParamType>::callback(int size_direct, int size_indirect,
                                     const int * ptr_array, int ptr_array_size,
                                     int vi_count)
    : d_ptr_array(ptr_array, ptr_array + ptr_array_size)
    , d_size_direct(size_direct)
    , d_size_total(size_direct + size_indirect)
    , d_vi_count(vi_count)
{
    assert(0 < size_direct || !"direct size must be positive");
    assert(0 <= size_indirect || !"indirect size cannot be negative");
    assert(0 <= ptr_array_size || !"pointer array size cannot be negative");
    assert(0 <= vi_count || !"variable indirect count cannot be negative");
}

template<typename ParamType>
inline callback<ParamType>::~callback()
{ } // anchor for virtual destructor

template<typename ParamType>
inline int callback<ParamType>::size_direct() const { return d_size_direct; }

} // namespace jsdi

#endif // __INCLUDED_CALLBACK_H___

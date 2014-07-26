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

#include "marshalling.h"

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
 * #call(const marshall_word_t *).
 */
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
         * \param size_direct Size of the on-stack arguments: <code>0 &lt;
         * size_direct &le; size_total</code> <em>must be a multiple of
         * <code>sizeof(marshall_word_t)</code></em>
         * \param size_total Size of the on-stack arguments plus size of any
         * data indirectly accessible via pointers from the on-stack arguments
         * <em>must be a multiple of <code>sizeof(marshall_word_t)</code></em>
         * \param ptr_array Array of tuples indicating which positions in the
         * direct and indirect storage are pointers, and which positions they
         * point to
         * \param ptr_array_size Size of <code>ptr_array</code> (must be even)
         * \param vi_count Number of variable indirect pointers that must be
         * unmarshalled
         */
        callback(int size_direct, int size_total, const int * ptr_array,
                 int ptr_array_size, int vi_count);

        virtual ~callback() = default;

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
         * base of the on-stack arguments in <code>stdcall</code> format
         * \return Return value of the callback function
         */
        virtual uint64_t call(const marshall_word_t * args) = 0;
};

inline int callback::size_direct() const { return d_size_direct; }

} // namespace jsdi

#endif // __INCLUDED_CALLBACK_H___

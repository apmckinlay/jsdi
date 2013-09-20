/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_HEAP_H___
#define __INCLUDED_HEAP_H___

/**
 * \file heap.h
 * \author Victor Schappert
 * \since 20130802
 * \brief Wraps Win32 heap object in C++ code
 */

#include "util.h"

namespace jsdi {

struct heap_impl;

/**
 * \brief Wraps a Windows heap
 * \author Victor Schappert
 * \since 20130802
 *
 * This is basically a copy of Andrew's CSuneido Heap class. It is necessary
 * for allocating executable stubs on the heap in order to implement callbacks
 * (because the default process heap should not be executable).
 */
class heap : private non_copyable
{
        //
        // DATA
        //

        heap_impl * d_impl;

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * Constructs a heap.
         * \param name Non-NULL zero-terminated name for the heap
         * \param is_executable Set <dfn>true</dfn> to allow blocks allocated
         * from this heap to be executable, <dfn>false</dfn> otherwise
         * \throw std::bad_alloc If a Windows heap cannot be created
         */
        heap(const char * name, bool is_executable) throw(std::bad_alloc);

        ~heap();

        //
        // ACECSSORS
        //

    public:

        /**
         * Returns the heap name.
         * \return Name of the heap
         */
        const std::string& name() const;

        //
        // MUTATORS
        //

    public:

        /**
         * Allocates a block of <dfn>n</dfn> bytes on the heap.
         * \param n Size of the desired block, in bytes
         * \throw std::bad_alloc If the heap allocation fails
         */
        void * alloc(size_t n) throw(std::bad_alloc);

        /**
         * Frees a block previously allocated using
         * #alloc(size_t) throw(std::bad_alloc)
         * \param ptr Pointer to the block of memory to free
         */
        void free(void * ptr) noexcept;
};

} // namespace jsdi

#endif // __INCLUDED_HEAP_H___

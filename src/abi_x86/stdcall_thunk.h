/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_STDCALL_THUNK_H___
#define __INCLUDED_STDCALL_THUNK_H___

/**
 * \file stdcall_thunk.h
 * \author Victor Schappert
 * \since 20130802
 * \brief Stub invoked according to the <dfn>stdcall</dfn> calling convention
 *        that wraps a callback function
 */

#include "util.h"

#include <memory>

namespace jsdi {

class callback; // TODO: Should this move to jsdi::abi_x86??

namespace abi_x86 {

struct stdcall_thunk_impl;

/**
 * \brief Enumerates the possible states in which a thunk can be.
 * \author Victor Schappert
 * \since 20130826
 * \see stdcall_thunk#state() const
 */
enum stdcall_thunk_state
{
    READY       /**< The only state in which a thunk may validly be called */,
    CLEARING    /**< The thunk is in the process of being cleared. It is an
                 *   error to attempt to call it. */,
    CLEARED     /**< The thunk has been cleared and is ready to delete. It is
                 *   an error to attempt to call it. */,
    DELETED     /**< The thunk has been deleted. No live pointer or reference
                 *   to a thunk should ever have this state. */
};

/**
 * \brief TODO
 * \author Victor Schappert
 * \since 20130802
 * \see stdcall_invoke
 * \see stdcall_thunk_state
 */
class stdcall_thunk : private non_copyable
{
        //
        // DATA
        //

        stdcall_thunk_impl * d_impl;

        //
        // CONSTRUCTORS
        //

    public:

        stdcall_thunk(const std::shared_ptr<callback>& callback_ptr);

        ~stdcall_thunk();

        //
        // ACCESSORS
        //

    public:

        void * func_addr();

        stdcall_thunk_state state() const;

        //
        // MUTATORS
        //

    public:

        stdcall_thunk_state clear();
};

} // namespace x86
} // namespace jsdi

#endif // __INCLUDED_STDCALL_THUNK_H___

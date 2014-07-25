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

#include "thunk.h"

namespace jsdi {

class callback;

namespace abi_x86 {

struct stdcall_thunk_impl;

/**
 * \brief Thunk for the x86 <code>__stdcall</code> calling convention
 * \author Victor Schappert
 * \since 20130802
 * \see stdcall_invoke
 */
class stdcall_thunk : public thunk
{
        //
        // DATA
        //

        std::unique_ptr<stdcall_thunk_impl> d_impl;

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructs a <code>__stdcall</code> thunk
         * \param callback_ptr Valid pointer to the callback to invoke when
         *        #func_addr() is called
         */
        stdcall_thunk(const std::shared_ptr<callback>& callback_ptr);

        //
        // ANCESTOR CLASS: thunk
        //

    public:

        /**
         * \brief Returns the address of the dynamically-generated
         *        <code>__stdcall</code> thunk function
         * \return Thunk function address
         */
        void * func_addr();
};

} // namespace x86
} // namespace jsdi

#endif // __INCLUDED_STDCALL_THUNK_H___

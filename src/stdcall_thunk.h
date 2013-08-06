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

class callback;

struct stdcall_thunk_impl;

/**
 * \brief TODO
 * \author Victor Schappert
 * \since 20130802
 * \see stdcall_invoke
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

        //
        // MUTATORS
        //

    public:

        void reset_callback(const std::shared_ptr<callback>& callback_ptr);
};

} // namespace jsdi


#endif // __INCLUDED_STDCALL_THUNK_H___

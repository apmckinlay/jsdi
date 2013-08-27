#ifndef __INCLUDED_CONCURRENT_H___
#define __INCLUDED_CONCURRENT_H___

/**
 * \file callback.h
 * \author Victor Schappert
 * \since 20130826
 * \brief Temporary utility functions for concurrency (ideally to be replaced
 *        by <dfn>std::mutex</dfn> and <dfn>std::lock_guard</dfn> when these are
 *        widely implemented)
 */

#include "util.h"
#include "jsdi_windows.h"

#include <cassert>

namespace jsdi {

/**
 * \brief Lightweight wrapper around a Windows <dfn>CRITICAL_SECTION</dfn>.
 * \author Victor Schappert
 * \since 20130826
 * \see lock_guard
 *
 * This class should ideally be removed and all uses replaced with a
 * <dfn>std::mutex</dfn>. However, this C++11 threading type is not yet
 * supported by MinGW.
 */

class critical_section : private non_copyable
{
        //
        // DATA
        //

        CRITICAL_SECTION d_critical;

        //
        // CONSTRUCTORS
        //

    public:

        critical_section();

        ~critical_section();

        //
        // MUTATORS
        //

    public:

        void lock();

        void unlock();
};

inline critical_section::critical_section()
{ InitializeCriticalSection(&d_critical); }

inline critical_section::~critical_section()
{ DeleteCriticalSection(&d_critical); }

inline void critical_section::lock()
{ EnterCriticalSection(&d_critical); }

inline void critical_section::unlock()
{ LeaveCriticalSection(&d_critical); }

/**
 * \brief Trivial RAII lock guard for any type supporting <dfn>lock()</dfn> and
 *        <dfn>unlock</dfn> operations.
 * \author Victor Schappert
 * \since 20130826
 * \see critical_section
 *
 * This class should ideally be removed and all uses replaced with
 * a <dfn>std::lock_guard</dfn>. However, this C++11 threading type is not yet
 * supported by MinGW.
 */
template<typename LockType>
class lock_guard : private non_copyable
{
        //
        // PUBLIC TYPES
        //

    public:

        typedef LockType    lock_type;
        typedef lock_type * lock_type_pointer;

        //
        // DATA
        //

    private:

        lock_type_pointer d_lock_ptr;

        //
        // CONSTRUCTORS
        //

    public:

        lock_guard(lock_type * lock);

        ~lock_guard();
};

template<typename LockType>
inline lock_guard<LockType>::lock_guard(lock_type_pointer lock_ptr)
    : d_lock_ptr(lock_ptr)
{
    assert(lock_ptr || !"lock object pointer cannot be NULL");
    lock_ptr->lock();
}

template<typename LockType>
inline lock_guard<LockType>::~lock_guard()
{ d_lock_ptr->unlock(); }

} // namespace jsdi

#endif // __INCLUDED_CONCURRENT_H___

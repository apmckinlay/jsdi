/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_THUNK_H___
#define __INCLUDED_THUNK_H___

/**
 * \file thunk.h
 * \author Victor Schappert
 * \since 20140714
 * \brief Reuseable thunk base class and management code
 */

#include "util.h"

#include <cassert>
#include <atomic>
#include <memory>

namespace jsdi {

class callback;

//==============================================================================
//                             enum thunk_state
//==============================================================================

/**
 * \brief Enumerates the possible states in which a thunk can be
 * \author Victor Schappert
 * \since 20130826
 * \see thunk#state() const
 *
 * \note
 * There are two motivations for tracking thunk states.
 * -# ensure the jsdi layer is independently able to detect and recover from
 *    some misuses by the Suneido programmer;
 * -# support the Suneido feature that a <code>callback</code> can clear itself.
 */
enum thunk_state
{
    /** The thunk has been deleted. No live pointer or reference to a thunk
     *  should ever have this state. */
    DELETED = -1,
    /** The thunk has been cleared and is ready to delete. It is an error to
     *  attempt to call it. */
    CLEARED = 0,
    /** The thunk is in the process of being cleared. It is an error to attempt
     *  to call it. */
    CLEARING = 1,
   /** The only state in which a thunk may validly be called */
    READY = 2
};

//==============================================================================
//                                class thunk
//==============================================================================

/**
 * \brief Base class for an ABI-neutral thunk
 * \author Victor Schappert
 * \since 20140714
 *
 * This is the base class for a thunk which can provide a pointer to a
 * dynamically-generated function that can then be supplied to a caller function
 * that expects a callback function pointer. When the caller function calls the
 * thunk, the thunk passes the function arguments supplied by the caller through
 * to an instance of\link callback\endlink known to the thunk. This callback
 * object then performs the action expected by the caller. If it returns a
 * value, the thunk passes the return value on to the caller.
 */
class thunk : private non_copyable
{
        //
        // DATA
        //

#ifndef NDEBUG
        int                         d_magic;
#endif // NDEBUG
        std::atomic_int_fast32_t    d_state;
        std::atomic<bool>           d_clearing;
        std::shared_ptr<callback>   d_callback;

        //
        // INTERNALS
        //

        void setup_bad_state(int_fast32_t state);
        void teardown_bad_state(int_fast32_t state);

    protected:

        /** \cond internal */
        void setup_call();
        void teardown_call();
        /** \endcond internal */

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructs a thunk
         * \param callback_ptr Non-<code>null</code> pointer to the callback
         *        to invoke when #func_addr() is called
         */
        thunk(const std::shared_ptr<callback>& callback_ptr);

        virtual ~thunk();

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Returns the address of the dynamically-generated thunk
         *        function
         * \return Thunk function address
         *
         * The caller function should invoke the function returned by
         * this member, which will in turn invoke the jsdi::callback attached
         * to this thunk.
         */
        virtual void * func_addr() = 0;

        /**
         * \brief Returns the thunk state
         * \return State of this thunk
         * \see #clear()
         */
        thunk_state state() const;

        //
        // MUTATORS
        //

    public:

        /** 
         * \brief Marks a thunk as uncallable
         * \see #state() const
         *
         * \warning
         * <em>As soon as</em> any thread calls this function, <em>and before
         * this function returns</em>, the thunk will be moved to
         * thunk_state#CLEARED and no other thread may attempt to call
         * #func_addr().
         *
         * \warning
         * No thread may call this function while #func_addr() is being called
         * by any thread.
         */
        thunk_state clear();
};

//==============================================================================
//                         class thunk_clearing_list
//==============================================================================

struct thunk_clearing_list_impl;

/**
 * \brief Clears thunks but temporarily delays their destruction to facilitate
 *        debugging
 * \author Victor Schappert
 * \since 20140714
 *
 * \warning
 * This class is <em>guaranteed</em> to <strong>leak</strong> a small amount of
 * memory as it will likely own a small number of undeleted thunks when it is
 * destroyed and it does not delete this small number of objects. Thus only one
 * instance of this class will preferably be instantiated over the lifetime of
 * the program.
 */
class thunk_clearing_list
{
        //
        // DATA
        //

        std::shared_ptr<thunk_clearing_list_impl> d_impl;
            // A std::unique_ptr would be better, but see http://goo.gl/NV3gZ2

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Creates an empty thunk clearing list
         */
        thunk_clearing_list();

        //
        // MUTATORS
        //

    public:

        /**
         * \brief Clears a thunk and queues it for deletion
         * \param thunk_ Valid pointer to a thunk to clear
         *
         * After this function is called, the thunk cleared list becomes the
         * owner of <code>thunk</code> and will delete it at some undefined time
         * in the future.
         *
         * \note
         * This operation is thread-safe with respect to this list. In other
         * words, two different threads may safely call this function (with
         * respect to <em>different</em> thunks!) simultaneously.
         */
        void clear_thunk(thunk * thunk_);
};

} // namespace jsdi

#endif // __INCLUDED_THUNK_H___

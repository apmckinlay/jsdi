/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: thunk.cpp
// auth: Victor Schappert
// date: 20140714
// desc: Reuseable thunk base class and management code
//==============================================================================

#include "thunk.h"

#include "log.h"

#include <deque>
#include <mutex>
#include <sstream>

namespace jsdi {

//==============================================================================
//                                class thunk
//==============================================================================

namespace {

#ifndef NDEBUG
constexpr int MAGIC = 0x1baddeed;
#endif // NDEBUG

std::string bad_state_to_str(int_fast32_t state)
{
    switch (state)
    {
        case thunk_state::DELETED:
            return std::string("DELETED");
        case thunk_state::CLEARED:
            return std::string("CLEARED");
        case thunk_state::CLEARING:
            return std::string("CLEARING");
        default:
            assert(state < thunk_state::DELETED);
            std::ostringstream o;
            o << "<unknown state " << state << '>';
            return o.str();
    }
}

} // anonymous namespace

/* NOTE: The meaning of the thunk.d_state member is the following:
 *    -1: deleted
 *     0: cleared
 *     1: clearing
 *     2..N: if (d_clearing) clearing, else ready
 * For the clearing and ready states, the number of threads concurrently
 * executing the thunk is (d_state-clearing) or (d_state-ready), as appropriate.
 * The reason for structuring it this way is so all the concurrency issues that
 * arise if the thunk function is being called on two or more threads at the
 * same time can be solved with a simple atomic test-and-set approach.
 */

void thunk::setup_bad_state(int_fast32_t state)
{
    LOG_FATAL("Bad state " << state << " detected in setup_call() for "
              "thunk " << this << " [func_addr() => " << func_addr() << ']');
    std::abort();
}

void thunk::teardown_bad_state(int_fast32_t state)
{
    LOG_FATAL("Bad state " << state << " detected in setup_call() for "
              "thunk " << this << " [func_addr() => " << func_addr() << ']');
    std::abort();
}

void thunk::setup_call()
{
    assert(MAGIC == d_magic);
    int_fast32_t state = std::atomic_fetch_add(&d_state, 1);
    if (state < thunk_state::READY)
        setup_bad_state(state);
}

void thunk::teardown_call()
{
    assert(MAGIC == d_magic);
    int_fast32_t state = std::atomic_fetch_sub(&d_state, 1) - 1;
    if (state < thunk_state::READY)
    {
        if (thunk_state::CLEARING == state)
            std::atomic_store(&d_state, thunk_state::CLEARED);
        else
            teardown_bad_state(state);
    }
}

thunk::thunk(const std::shared_ptr<callback>& callback_ptr)
    :
#ifndef NDEBUG
      d_magic(MAGIC),
#endif // NDEBUG
      d_state(thunk_state::READY)
    , d_clearing(false)
    , d_callback(callback_ptr)
{ LOG_DEBUG("New thunk " << static_cast<void *>(this)); }

thunk::~thunk()
{
#ifndef NDEBUG
    d_magic = ~MAGIC;
#endif // NDEBUG
    int_fast32_t state = std::atomic_exchange(&d_state, thunk_state::DELETED);
    assert(thunk_state::CLEARED == state || thunk_state::READY == state);
}

thunk_state thunk::clear()
{
    assert(MAGIC == d_magic);
    bool already_clearing = std::atomic_exchange(&d_clearing, true);
    assert(!already_clearing || !"clearing flag already set");
    int_fast32_t state = std::atomic_fetch_sub(&d_state, 1) - 1;
    assert(thunk_state::CLEARING <= state);
    if (thunk_state::CLEARING == state)
    {
        std::atomic_store(&d_state, thunk_state::CLEARED);
        return thunk_state::CLEARED;
    }
    else return thunk_state::CLEARING;
}

thunk_state thunk::state() const
{
    int_fast32_t state = std::atomic_load(&d_state);
    assert(thunk_state::CLEARED <= state || !"bad thunk state");
    if (state < thunk_state::READY)
        return static_cast<thunk_state>(state);
    else
        return d_clearing.load() ? thunk_state::CLEARING : thunk_state::READY;
}

//==============================================================================
//                         class thunk_clearing_list
//==============================================================================

struct thunk_clearing_list_impl
{
    std::deque<thunk *> d_cleared_list;
    std::deque<thunk *> d_clearing_list;
    std::mutex               d_mutex;
    // Use default destructor. Thus it will leak whatever is on the lists at
    // time of destruction.
    void clear_thunk(thunk * thunk_)
    {
        assert(thunk_ || !"thunk cannot be null");
        std::lock_guard<std::mutex> lock(d_mutex);
        switch (thunk_->clear())
        {
            case thunk_state::CLEARED:
                d_clearing_list.push_back(thunk_);
                break;
            case thunk_state::CLEARING:
                d_clearing_list.push_back(thunk_);
                break;
            default:
                assert(!"invalid thunk state");
                break;
        }
        // Don't let the cleared list grow indefinitely
        if (10 < d_clearing_list.size())
        {
            assert(thunk_ != d_clearing_list.front());
            LOG_DEBUG("Deleting thunk " << static_cast<void *>(
                                               d_clearing_list.front()));
            delete d_clearing_list.front();
            d_clearing_list.pop_front();
        }
        // Don't let the clearing list grow indefinitely
        if (1 < d_clearing_list.size() &&
            thunk_state::CLEARED == d_clearing_list.front()->state())
        {
            d_clearing_list.push_back(d_clearing_list.front());
            d_clearing_list.pop_front();
        }
    }
};

thunk_clearing_list::thunk_clearing_list()
    : d_impl(new thunk_clearing_list_impl)
{ }

void thunk_clearing_list::clear_thunk(thunk * thunk_)
{ d_impl->clear_thunk(thunk_); }

} // namespace jsdi

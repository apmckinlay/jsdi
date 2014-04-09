/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: stdcall_thunk.cpp
// auth: Victor Schappert
// date: 20130802
// desc: Implement a __stdcall stub wrapping a Suneido callback
//==============================================================================

#include "stdcall_thunk.h"

#include "callback.h"
#include "concurrent.h"
#include "heap.h"

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <vector>
#include <limits>
#include <cassert>
#include <cstring>
#include <cstdlib>

namespace jsdi {

//==============================================================================
//                             struct stub_code
//==============================================================================

namespace {

enum
{
    CODE_SIZE                = 18,
    CODE_IMPL_POINTER_OFFSET =  6,
    CODE_CALL_ADDR_OFFSET    = 11,
    CODE_RET_OFFSET          = 15,
    CODE_RET_POP_SIZE_OFFSET = CODE_RET_OFFSET + 1,
};

constexpr unsigned char INSTRUCTIONS[CODE_SIZE] =
{
    0x8d, 0x44, 0x24, 0x04,          // lea [esp+4], %eax
                                     //   Get pointer to start of arguments
    0x50,                            // push %eax
                                     //   Push pointer to arguments onto stack
    0x68, 0x55, 0x55, 0x55, 0x55,    // push 0x55555555
                                     //   Placeholder for pushing impl pointer
    0xe8, 0x66, 0x66, 0x66, 0x66,    // call 0x66666666
                                     //   Placeholder for call-relative operand
                                     //   to call wrapper which itself is a
                                     //   __stdcall function so it, not us, will
                                     //   clean up the two arguments we pushed
                                     //   above.
    0xc2, 0x77, 0x77,                // ret $0x7777
                                     //   Pop return value, then remove
                                     //   args passed by caller from stack
};

static_assert(0x55 == INSTRUCTIONS[CODE_IMPL_POINTER_OFFSET+0], "check code");
static_assert(0x55 == INSTRUCTIONS[CODE_IMPL_POINTER_OFFSET+1], "check code");
static_assert(0x55 == INSTRUCTIONS[CODE_IMPL_POINTER_OFFSET+2], "check code");
static_assert(0x55 == INSTRUCTIONS[CODE_IMPL_POINTER_OFFSET+3], "check code");
static_assert(0x66 == INSTRUCTIONS[CODE_CALL_ADDR_OFFSET+0], "check code");
static_assert(0x66 == INSTRUCTIONS[CODE_CALL_ADDR_OFFSET+1], "check code");
static_assert(0x66 == INSTRUCTIONS[CODE_CALL_ADDR_OFFSET+2], "check code");
static_assert(0x66 == INSTRUCTIONS[CODE_CALL_ADDR_OFFSET+3], "check code");
static_assert(0x77 == INSTRUCTIONS[CODE_RET_POP_SIZE_OFFSET+0], "check code");
static_assert(0x77 == INSTRUCTIONS[CODE_RET_POP_SIZE_OFFSET+1], "check code");

typedef __stdcall long (* wrapper_func)(stdcall_thunk_impl *, const char *);

struct stub_code
{
    unsigned char d_instructions[CODE_SIZE];
    stub_code(stdcall_thunk_impl *, wrapper_func, int);
};

// Compute the operand for the 'call relative' instruction. The operand is an
// offset which when added to the address of the next instruction gives you the
// jump address.
//     See e.g. here: http://stackoverflow.com/q/9438544/1911388
inline int call_relative_offset(wrapper_func thunk_wrapper, stub_code& code)
{
    unsigned char * near_jump_addr =
        reinterpret_cast<unsigned char *>(thunk_wrapper);
    unsigned char * next_inst_addr = &code.d_instructions[CODE_RET_OFFSET];
    return near_jump_addr - next_inst_addr;
}

stub_code::stub_code(stdcall_thunk_impl * impl_addr, wrapper_func thunk_wrapper,
                     int args_size_bytes)
{
    std::memcpy(d_instructions, INSTRUCTIONS, sizeof(INSTRUCTIONS));
    assert(impl_addr || !"thunk implementation cannot be NULL");
    assert(
        0 == args_size_bytes % 4
            || !"argument size must be a multiple of 4 bytes");
    static_assert(4 == sizeof(impl_addr), "check code");
    // Replace the placeholder bytes for the second argument to the wrapper
    // function with the address of the thunk implementation.
    std::memcpy(d_instructions + CODE_IMPL_POINTER_OFFSET, &impl_addr,
                sizeof(impl_addr));
    // Replace the placeholder bytes for the address of the wrapper function
    // itself with stdcall_thunk_impl::wrapper.
    int offset = call_relative_offset(thunk_wrapper, *this);
    static_assert(4 == sizeof(int), "check code");
    std::memcpy(d_instructions + CODE_CALL_ADDR_OFFSET, &offset, sizeof(int));
    // Finally, replace the placeholder bytes for the number of bytes by the
    // ret instruction with the number of bytes which the thunk containing this
    // stub code block must pop off of the execution stack in order to
    // successfully emulate the stdcall function which the caller believes it
    // invoked when it called the thunk.
    if (!(0 <= args_size_bytes
        && args_size_bytes <= std::numeric_limits<uint16_t>::max()))
    {
        std::ostringstream() << "Thunk argument size of " << args_size_bytes
                             << " cannot be represented in a 2-byte unsigned"
                                " RET instruction operand"
                             << throw_cpp<std::runtime_error>();
    }
    uint16_t ret_pop_size(args_size_bytes);
    std::memcpy(d_instructions + CODE_RET_POP_SIZE_OFFSET, &ret_pop_size,
                sizeof(ret_pop_size));
}

} // anonymous namespace

//==============================================================================
//                         struct stdcall_thunk_impl
//==============================================================================

namespace {

heap impl_heap("stdcall_thunk_impl", true);

enum { MAGIC1 = 0x18741130, MAGIC2 = 0x1baddeed };

} // anonymous namespace


struct stdcall_thunk_impl
{
    //
    // DATA
    //

    int                         d_magic_1;
    stub_code                   d_code;
    std::shared_ptr<callback>   d_callback;
    critical_section            d_critical_section;
    int                         d_num_ongoing_calls;
    stdcall_thunk_state         d_state;
    int                         d_magic_2;

    //
    // CONSTRUCTORS
    //

    stdcall_thunk_impl(const std::shared_ptr<callback>& callback_ptr);

    ~stdcall_thunk_impl();

    //
    // STATIC FUNCTIONS
    //

    static __stdcall long wrapper(stdcall_thunk_impl *, const char *);

    //
    // OPERATORS
    //

    void * operator new(size_t);

    void operator delete(void *);

    private: void * operator new[](size_t);
    private: void operator delete[](void *);
};

stdcall_thunk_impl::stdcall_thunk_impl(
    const std::shared_ptr<callback>& callback_ptr)
    : d_magic_1(MAGIC1)
    , d_code(this, wrapper, callback_ptr->size_direct())
    , d_callback(callback_ptr)
    , d_num_ongoing_calls(0)
    , d_state(READY)
    , d_magic_2(MAGIC2)
{ assert(callback_ptr || "callback pointer may not be null"); }

stdcall_thunk_impl::~stdcall_thunk_impl()
{
#ifndef NDEBUG
    {
        lock_guard<critical_section> lock(&d_critical_section);
        assert(CLEARED == d_state);
        assert(0 == d_num_ongoing_calls);
    }
#endif
    d_state = DELETED;
    d_magic_1 = ~MAGIC1;
    d_magic_2 = ~MAGIC2;
}

__stdcall long stdcall_thunk_impl::wrapper(stdcall_thunk_impl * impl,
                                           const char * args)
{
    long result;
    assert(MAGIC1 == impl->d_magic_1);
    assert(MAGIC2 == impl->d_magic_2);
    // SETUP
    {
        lock_guard<critical_section> lock(&impl->d_critical_section);
        assert(READY == impl->d_state || !"callback already cleared");
        ++impl->d_num_ongoing_calls; // No RAII guard is OK b/c of catch (...)
    }
    //
    // CALL
    //
    // NOTE: It is [C++] callback's responsibility to ensure that no C++
    //       exceptions propagate out to this level. Furthermore, C++ callback
    //       is responsible for stopping execution and returning the moment a
    //       JNI exception occurs.
    try
    { result = impl->d_callback->call(args); }
    catch (...)
    {
        std::cerr << "FATAL ERROR: exception caught in " << __FUNCTION__
                  << "( " << __FILE__ << " at line " << __LINE__ << ')'
                  << std::endl;
        std::abort();
        result = 0L; // To shut up the compiler warning
    }
    //
    // TEARDOWN
    //
    {
        lock_guard<critical_section> lock(&impl->d_critical_section);
        --impl->d_num_ongoing_calls;
        assert(MAGIC1 == impl->d_magic_1);
        assert(MAGIC2 == impl->d_magic_2);
        assert(READY == impl->d_state || CLEARING == impl->d_state);
        if (CLEARING == impl->d_state && ! impl->d_num_ongoing_calls)
            impl->d_state = CLEARED;
    }
    //
    // RETURN CALLBACK RESULT
    //
    return result;
}

void * stdcall_thunk_impl::operator new(size_t n)
{ return impl_heap.alloc(n); }

void stdcall_thunk_impl::operator delete(void * ptr)
{ impl_heap.free(ptr); }

//==============================================================================
//                            class stdcall_thunk
//==============================================================================

stdcall_thunk::stdcall_thunk(const std::shared_ptr<callback>& callback_ptr)
    : d_impl(new stdcall_thunk_impl(callback_ptr))
{ }

stdcall_thunk::~stdcall_thunk() { delete d_impl; }

void * stdcall_thunk::func_addr()
{ return d_impl->d_code.d_instructions; }

stdcall_thunk_state stdcall_thunk::state() const
{ return d_impl->d_state; }

stdcall_thunk_state stdcall_thunk::clear()
{
    lock_guard<critical_section> lock(&d_impl->d_critical_section);
    assert(READY == d_impl->d_state);
    assert(0 <= d_impl->d_num_ongoing_calls);
    d_impl->d_state = 0 == d_impl->d_num_ongoing_calls ? CLEARED : CLEARING;
    return d_impl->d_state;
}

} // namespace jsdi

//==============================================================================
//                                  TESTS
//==============================================================================

#ifndef __NOTEST__

#include "test.h"
#include "test_exports.h"
#include "marshalling.h"
#include "stdcall_invoke.h"

using namespace jsdi;

static const int EMPTY_PTR_ARRAY[0] = { };

// callback that can invoke a stdcall func and return its value
struct stdcall_invoke_basic_callback : public callback
{
    void * d_func_ptr;
    template<typename FuncPtr>
    stdcall_invoke_basic_callback(FuncPtr func_ptr, int size_direct,
                                  int size_indirect, const int * ptr_array,
                                  int ptr_array_size, int vi_count)
        : callback(size_direct, size_indirect, ptr_array, ptr_array_size,
                   vi_count)
        , d_func_ptr(reinterpret_cast<void *>(func_ptr))
    { }
    virtual long call(const char * args)
    {
        std::vector<char> data(d_size_total, 0);
        std::vector<int> vi_inst_array(
            d_vi_count,
            static_cast<int>(suneido_language_jsdi_VariableIndirectInstruction::RETURN_JAVA_STRING));
        unmarshaller_vi_test u(d_size_direct, d_size_total, d_ptr_array.data(),
                               d_ptr_array.data() + d_ptr_array.size(),
                               d_vi_count);
        u.unmarshall_vi(data.data(), args, 0, 0, vi_inst_array.data());
        return static_cast<long>(
            stdcall_invoke::basic(d_size_direct, data.data(), d_func_ptr) &
            0x00000000ffffffffLL
        );
    }
};

template<typename Param1, typename Param2>
struct Func
{
    typedef __stdcall long (*callback_function)(Param1, Param2);
    typedef __stdcall long (*function)(callback_function, Param1, Param2);
    static long call(function f, stdcall_thunk& t, Param1 arg1, Param2 arg2)
    {
        callback_function c = reinterpret_cast<callback_function>(t
            .func_addr());
        return f(c, arg1, arg2);
    }
};

template<typename Param>
struct Func<Param, void>
{
    typedef __stdcall long (*callback_function)(Param);
    typedef __stdcall long (*function)(callback_function, Param);
    static long call(function f, stdcall_thunk& t, Param arg)
    {
        callback_function c = reinterpret_cast<callback_function>(t
            .func_addr());
        return f(c, arg);
    }
};

TEST(one_long,
    std::shared_ptr<callback> cb(
        new stdcall_invoke_basic_callback(TestLong, sizeof(long), 0,
                                          EMPTY_PTR_ARRAY, 0, 0));
    stdcall_thunk thunk(cb);
    long result = Func<long, void>::call(TestInvokeCallback_Long1, thunk, 10);
    thunk.clear();
    assert_equals(10, result);
);

TEST(sum_two_longs,
    std::shared_ptr<callback> cb(
        new stdcall_invoke_basic_callback(TestSumTwoLongs, 2 * sizeof(long), 0,
                                           EMPTY_PTR_ARRAY, 0, 0));
    stdcall_thunk thunk(cb);
    long result = Func<long, long>::call(
        TestInvokeCallback_Long2, thunk, std::numeric_limits<long>::min() + 3,
        -2);
    thunk.clear();
    assert_equals(std::numeric_limits<long>::min() + 1, result);
);

TEST(sum_packed,
    Packed_CharCharShortLong p_ccsl = { -1, 2, 100, 54321 };
    std::shared_ptr<callback> cb(
        new stdcall_invoke_basic_callback(TestSumPackedCharCharShortLong,
                                          sizeof(p_ccsl), 0, EMPTY_PTR_ARRAY, 0,
                                          0));
    stdcall_thunk thunk(cb);
    long result = Func<Packed_CharCharShortLong, void>::call(
        TestInvokeCallback_Packed_CharCharShortLong, thunk, p_ccsl);
    thunk.clear();
    assert_equals(54422, result);
);

TEST(sum_string,
    static Recursive_StringSum r_ss[2]; // zeroed
    r_ss[0].x[0] = { 0, 1, 2, 3 };
    r_ss[0].x[1] = { 4, 5, 6, 7 };
    r_ss[0].str = "8";
    r_ss[0].inner = &r_ss[1];
    r_ss[1].x[0] = { 9, 10, 11, 12 };
    r_ss[1].x[1] = { 13, 14, 15, 16 };
    r_ss[1].str = "17";
    enum
    {
        SIZE_DIRECT = sizeof(Recursive_StringSum *),
        SIZE_INDIRECT = sizeof(r_ss),
        SIZE_TOTAL = SIZE_DIRECT + SIZE_INDIRECT,
        STR_OFFSET = reinterpret_cast<char *>(&r_ss[0].str) -
                     reinterpret_cast<char *>(&r_ss[0]),
        BUF_OFFSET = reinterpret_cast<char *>(&r_ss[0].buffer) -
                     reinterpret_cast<char *>(&r_ss[0]),
        INNER_OFFSET = reinterpret_cast<char *>(&r_ss[0].inner) -
                       reinterpret_cast<char *>(&r_ss[0]),
        VI_COUNT = 4,
    };
    static int PTR_ARRAY[12] =
    {
        0, SIZE_DIRECT,
        SIZE_DIRECT + STR_OFFSET, SIZE_TOTAL + 0,
        SIZE_DIRECT + BUF_OFFSET, SIZE_TOTAL + 1,
        SIZE_DIRECT + INNER_OFFSET, SIZE_DIRECT + sizeof(r_ss[0]),
        SIZE_DIRECT + sizeof(r_ss[0]) + STR_OFFSET, SIZE_TOTAL + 2,
        SIZE_DIRECT + sizeof(r_ss[0]) + BUF_OFFSET, SIZE_TOTAL + 3,
    };
    std::shared_ptr<callback> cb(
    new stdcall_invoke_basic_callback(TestSumString, SIZE_DIRECT, SIZE_INDIRECT,
                                      PTR_ARRAY, array_length(PTR_ARRAY),
                                      VI_COUNT));
    stdcall_thunk thunk(cb);
    long result = Func<Recursive_StringSum *, void>::call(
        TestInvokeCallback_Recursive_StringSum, thunk, r_ss);
    thunk.clear();
    assert_equals(0+1+2+3+4+5+6+7+8+9+10+11+12+13+14+15+16+17, result);
);

#endif // __NOTEST__

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
#include "heap.h"
#include "log.h"

#include <cassert>
#include <functional>
#include <limits>
#include <stdexcept>
#include <sstream>
#include <vector>

namespace jsdi {
namespace abi_x86 {

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
    0x68, 0x55, 0x55, 0x55, 0x55,    // push $0x55555555
                                     //   Placeholder for pushing impl pointer
    0xe8, 0x66, 0x66, 0x66, 0x66,    // call $0x66666666
                                     //   Placeholder for call-relative operand
                                     //   to call wrapper which itself is a
                                     //   __stdcall function so it, not us, will
                                     //   clean up the two arguments we pushed
                                     //   above.
    0xc2, 0x77, 0x77,                // ret $0x7777
                                     //   Pop return value, then remove
                                     //   args passed by caller from stack
};

#if !defined(_MSC_VER)
// TODO: Microsoft's Visual C++ compiler doesn't properly support 'constexpr'
//       so that static_assert(...)-ing against elements of constexpr arrays
//       fails with error C2057 - "Expected constant expression". This is as
//       of version 18.00.21114 for x86 (November 2013 CTP). These asserts
//       should be re-enabled as soon as 'constexpr' support is properly
//       available.
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
#endif

typedef uint32_t (__stdcall * wrapper_func)(stdcall_thunk_impl *, uint32_t *);

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
    // Replace the placeholder bytes for the first argument to the wrapper
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

} // anonymous namespace

struct stdcall_thunk_impl
{
    //
    // DATA
    //

    stub_code                                  d_code;
    std::function<void()>                      d_setup;
    std::shared_ptr<stdcall_thunk::callback_t> d_callback;
    std::function<void()>                      d_teardown;

    //
    // CONSTRUCTORS
    //

    stdcall_thunk_impl(const std::function<void()>&,
                       const std::shared_ptr<stdcall_thunk::callback_t>&,
                       const std::function<void()>&);

    //
    // STATIC FUNCTIONS
    //

    static uint32_t __stdcall wrapper(stdcall_thunk_impl *, uint32_t *);

    //
    // OPERATORS
    //

    void * operator new(size_t);

    void operator delete(void *);

    void * operator new[](size_t) = delete;
    void operator delete[](void *) = delete;
};

stdcall_thunk_impl::stdcall_thunk_impl(
    const std::function<void()>& setup,
    const std::shared_ptr<stdcall_thunk::callback_t>& callback,
    const std::function<void()>& teardown)
    : d_code(this, wrapper, callback->size_direct())
    , d_setup(setup)
    , d_callback(callback)
    , d_teardown(teardown)
{ }

uint32_t __stdcall stdcall_thunk_impl::wrapper(stdcall_thunk_impl * impl,
                                               uint32_t * args)
{
    uint32_t result(0);
    LOG_TRACE("stdcall_thunk_impl::wrapper( impl => " << impl << ", args => "
                                                      << args << " )");
    impl->d_setup();
    // NOTE: It is [C++] callback's responsibility to ensure that no C++
    //       exceptions propagate out to this level. Furthermore, C++ callback
    //       is responsible for stopping execution and returning the moment a
    //       JNI exception occurs.
    try
    {
        // TODO: Put in SEH blocks here (catch, teardown(), rethrow)
        result = impl->d_callback->call(args);
    }
    catch (const std::exception& e)
    {
        LOG_FATAL("Exception escaped callback: '" << e.what() << '\'');
        std::abort();
    }
    catch (...)
    {
        LOG_FATAL("Exception escaped callback");
        std::abort();
    }
    impl->d_teardown();
    return result;
}

void * stdcall_thunk_impl::operator new(size_t n)
{ return impl_heap.alloc(n); }

void stdcall_thunk_impl::operator delete(void * ptr)
{ impl_heap.free(ptr); }

//==============================================================================
//                            class stdcall_thunk
//==============================================================================

stdcall_thunk::stdcall_thunk(
    const std::shared_ptr<callback_t>& callback_ptr)
    : thunk(callback_ptr)
    , d_impl(new stdcall_thunk_impl(
          std::bind(std::mem_fn(&stdcall_thunk::setup_call), this),
          callback_ptr,
          std::bind(std::mem_fn(&stdcall_thunk::teardown_call), this)))
{ }

void * stdcall_thunk::func_addr()
{ return d_impl->d_code.d_instructions; }

} // namespace abi_x86
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
using namespace jsdi::abi_x86;

static const int EMPTY_PTR_ARRAY[1] = { };

typedef stdcall_thunk::callback_t callback_t;

// callback that can invoke a stdcall func and return its value
struct stdcall_invoke_basic_callback : public callback_t
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
    virtual uint32_t call(const uint32_t * args)
    {
        std::vector<char> data(d_size_total, 0);
        std::vector<int> vi_inst_array(
            d_vi_count,
            static_cast<int>(suneido_jsdi_VariableIndirectInstruction::RETURN_JAVA_STRING));
        unmarshaller_vi_test u(d_size_direct, d_size_total, d_ptr_array.data(),
                               d_ptr_array.data() + d_ptr_array.size(),
                               d_vi_count);
        u.unmarshall_vi(data.data(),
                        // FIXME: The reinterpret_cast below should go away
                        reinterpret_cast<const char *>(args), 0, 0,
                        vi_inst_array.data());
        return static_cast<uint32_t>(
            stdcall_invoke::basic(d_size_direct, data.data(), d_func_ptr) &
            0x00000000ffffffffLL
        );
    }
};

template<typename ... Params>
struct Func
{
    typedef int32_t(__stdcall * callback_function)(Params...);
    typedef int32_t(__stdcall * invoke_function)(callback_function, Params...);
    static int32_t call(invoke_function f, stdcall_thunk& t, Params ... args)
    {
        callback_function c = reinterpret_cast<callback_function>(t.
            func_addr());
        return f(c, args...);
    };
};

TEST(one_int32,
    std::shared_ptr<callback_t> cb(
        new stdcall_invoke_basic_callback(TestInt32, sizeof(int32_t), 0,
                                          EMPTY_PTR_ARRAY, 0, 0));
    stdcall_thunk thunk(cb);
    int32_t result = Func<int32_t>::call(TestInvokeCallback_Int32_1, thunk, 10);
    thunk.clear();
    assert_equals(10, int32_t(result));
);

TEST(sum_two_int32s,
    std::shared_ptr<callback_t> cb(
        new stdcall_invoke_basic_callback(TestSumTwoInt32s, 2 * sizeof(int32_t),
                                          0, EMPTY_PTR_ARRAY, 0, 0));
    stdcall_thunk thunk(cb);
    int32_t result = Func<int32_t, int32_t>::call(
        TestInvokeCallback_Int32_2, thunk,
        std::numeric_limits<int32_t>::min() + 3, -2);
    thunk.clear();
    assert_equals(std::numeric_limits<int32_t>::min() + 1, result);
);

TEST(sum_six_mixed,
    std::shared_ptr<callback_t> cb(
        new stdcall_invoke_basic_callback(TestSumSixMixed, 8 * sizeof(uint32_t),
                                          0, EMPTY_PTR_ARRAY, 0, 0));
    stdcall_thunk thunk(cb);
    int32_t result = Func<double, int8_t, float, int16_t, float, int64_t>::call(
        TestInvokeCallback_Mixed_6, thunk,
        -3.0, 5, -3.0f, 5, -3.0f, 5);
    thunk.clear();
    assert_equals(6, result);
);

TEST(sum_packed,
    Packed_Int8Int8Int16Int32 packed = { -1, 2, 100, 54321 };
    std::shared_ptr<callback_t> cb(
        new stdcall_invoke_basic_callback(TestSumPackedInt8Int8Int16Int32,
                                          sizeof(packed), 0, EMPTY_PTR_ARRAY, 0,
                                          0));
    stdcall_thunk thunk(cb);
    int32_t result = Func<Packed_Int8Int8Int16Int32>::call(
        TestInvokeCallback_Packed_Int8Int8Int16Int32, thunk, packed);
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
        VI_COUNT = 4,
    };
    const int STR_OFFSET =   reinterpret_cast<char *>(&r_ss[0].str) -
                             reinterpret_cast<char *>(&r_ss[0]),
              BUF_OFFSET =   reinterpret_cast<char *>(&r_ss[0].buffer) -
                             reinterpret_cast<char *>(&r_ss[0]),
              INNER_OFFSET = reinterpret_cast<char *>(&r_ss[0].inner) -
                             reinterpret_cast<char *>(&r_ss[0]);
    static int PTR_ARRAY[12] =
    {
        0, SIZE_DIRECT,
        SIZE_DIRECT + STR_OFFSET, SIZE_TOTAL + 0,
        SIZE_DIRECT + BUF_OFFSET, SIZE_TOTAL + 1,
        SIZE_DIRECT + INNER_OFFSET, SIZE_DIRECT + sizeof(r_ss[0]),
        SIZE_DIRECT + sizeof(r_ss[0]) + STR_OFFSET, SIZE_TOTAL + 2,
        SIZE_DIRECT + sizeof(r_ss[0]) + BUF_OFFSET, SIZE_TOTAL + 3,
    };
    std::shared_ptr<callback_t> cb(
    new stdcall_invoke_basic_callback(TestSumString, SIZE_DIRECT, SIZE_INDIRECT,
                                      PTR_ARRAY, array_length(PTR_ARRAY),
                                      VI_COUNT));
    stdcall_thunk thunk(cb);
    int32_t result = Func<Recursive_StringSum *>::call(
        TestInvokeCallback_Recursive_StringSum, thunk, r_ss);
    thunk.clear();
    assert_equals(0+1+2+3+4+5+6+7+8+9+10+11+12+13+14+15+16+17, result);
);

#endif // __NOTEST__

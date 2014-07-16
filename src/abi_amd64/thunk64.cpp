/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: thunk64.cpp
// auth: Victor Schappert
// date: 20140710
// desc: Implement an x64 ABI shim wrapping a Suneido callback
//==============================================================================

#include "thunk64.h"

#include "callback.h"
#include "jsdi_windows.h"
#include "log.h"
#include "heap.h"

#include <functional>
#include <sstream>

namespace jsdi {
namespace abi_amd64 {

//==============================================================================
//                             struct stub_code
//==============================================================================

namespace {

enum
{
    CODE_SIZE_PROLOGUE                  =  4,
    CODE_SIZE_FIXED_BODY                = 21,
    CODE_SIZE_EPILOGUE                  =  5,
    CODE_SIZE_MAX_MOV_STACK             =  6,
    CODE_SIZE_MAX_TOTAL                 = CODE_SIZE_PROLOGUE +
                                          (NUM_PARAM_REGISTERS * CODE_SIZE_MAX_MOV_STACK) +
                                          CODE_SIZE_FIXED_BODY +
                                          CODE_SIZE_EPILOGUE,
    
    CODE_OFFSET_FIXED_BODY_IMPL_POINTER =  2,
    CODE_OFFSET_FIXED_BODY_CALL_ADDR    = 17,
    UNWIND_INFO_SIZE                    =  6,
};

struct mov_stack
{
    uint8_t  instruction[CODE_SIZE_MAX_MOV_STACK];
    uint8_t  size_bytes;
};


constexpr uint8_t CODE_PROLOGUE[] =
{
   0x48, 0x83, 0xec, 0x20,                              // sub   rsp, 32
};
static_assert(sizeof(CODE_PROLOGUE) == CODE_SIZE_PROLOGUE, "check code");

constexpr mov_stack CODE_MOV_STACK_TABLE[NUM_PARAM_REGISTER_TYPES]
                                        [NUM_PARAM_REGISTERS] =
{
    { // uint64
        { { 0x48, 0x89, 0x4c, 0x24, 0x28 }, 5 },        // mov   [rsp+40], rcx
        { { 0x48, 0x89, 0x54, 0x24, 0x30 }, 5 },        // mov   [rsp+48], rdx
        { { 0x4c, 0x89, 0x44, 0x24, 0x38 }, 5 },        // mov   [rsp+56], r8
        { { 0x4c, 0x89, 0x4c, 0x24, 0x40 }, 5 },        // mov   [rsp+64], r9
    },
    { // double
        { { 0xf2, 0x0f, 0x11, 0x44, 0x24, 0x28 }, 6 },  // movsd [rsp+40], xmm0
        { { 0xf2, 0x0f, 0x11, 0x4c, 0x24, 0x30 }, 6 },  // movsd [rsp+48], xmm1
        { { 0xf2, 0x0f, 0x11, 0x54, 0x24, 0x38 }, 6 },  // movsd [rsp+56], xmm2
        { { 0xf2, 0x0f, 0x11, 0x5c, 0x24, 0x40 }, 6 },  // movsd [rsp+64], xmm3
    },
    { // float
        { { 0xf3, 0x0f, 0x11, 0x44, 0x24, 0x28 }, 6 },  // movss [rsp+40], xmm0
        { { 0xf3, 0x0f, 0x11, 0x4c, 0x24, 0x30 }, 6 },  // movss [rsp+48], xmm1
        { { 0xf3, 0x0f, 0x11, 0x54, 0x24, 0x38 }, 6 },  // movss [rsp+56], xmm2
        { { 0xf3, 0x0f, 0x11, 0x5c, 0x24, 0x38 }, 6 },  // movss [rsp+64], xmm3
    }
};

constexpr uint8_t CODE_FIXED_BODY[] =
{
    0x48, 0xb9, 0x55, 0x55, 0x55, 0x55,                 // mov   rcx, 0x5555555555555555
                0x55, 0x55, 0x55, 0x55,                 //    Placeholder for
                                                        //    impl. address
    0x48, 0x8d, 0x54, 0x24, 0x28,                       // lea   rdx, [rsp+40]
    0xff, 0x15, 0x66, 0x66, 0x66, 0x66                  // callq [rip+0x66666666]
                                                        //    Placeholder for
                                                        //    RIP-relative addr.
                                                        //    of wrapper addr.
};
static_assert(sizeof(CODE_FIXED_BODY) == CODE_SIZE_FIXED_BODY, "check code");

#if !defined(_MSC_VER)
// TODO: Enable these asserts as soon as Microsoft's Visual C++ compiler
//       properly supports constexpr. It does not as of version 18.00.21114
//       (November 2013 CTP).
static_assert(0x55 == CODE_FIXED_BODY[CODE_OFFSET_FIXED_BODY_IMPL_POINTER+0], "check code");
static_assert(0x55 == CODE_FIXED_BODY[CODE_OFFSET_FIXED_BODY_IMPL_POINTER+1], "check code");
static_assert(0x55 == CODE_FIXED_BODY[CODE_OFFSET_FIXED_BODY_IMPL_POINTER+1], "check code");
static_assert(0x55 == CODE_FIXED_BODY[CODE_OFFSET_FIXED_BODY_IMPL_POINTER+3], "check code");
static_assert(0x66 == CODE_FIXED_BODY[CODE_OFFSET_FIXED_BODY_CALL_ADDR+0], "check code");
static_assert(0x66 == CODE_FIXED_BODY[CODE_OFFSET_FIXED_BODY_CALL_ADDR+1], "check code");
static_assert(0x66 == CODE_FIXED_BODY[CODE_OFFSET_FIXED_BODY_CALL_ADDR+2], "check code");
static_assert(0x66 == CODE_FIXED_BODY[CODE_OFFSET_FIXED_BODY_CALL_ADDR+3], "check code");
#endif

constexpr uint8_t CODE_EPILOGUE[] =
{
    0x48, 0x83, 0xc4, 0x20,                             // add   rsp, 32
    0xc3                                                // retq
};
static_assert(sizeof(CODE_EPILOGUE) == CODE_SIZE_EPILOGUE, "check code");

constexpr uint8_t NOP = 0x90;

const uint8_t UNWIND_INFO[UNWIND_INFO_SIZE] =
{
    0x01 /* version 1, flags 0 */,
    CODE_SIZE_PROLOGUE,
    0x01 /* count of unwind codes = 1 */,
    0x00 /* no frame register needed */,
    0x32 /* UNWIND CODE #0: UWOP_ALLOC_SMALL, 3 * 8 + 8 = 32 bytes */,
    0x00 /* UNWIND CODE #1: Not used. (MSFT says we must allocate an even number
            of UNWIND_CODE's, the last being unused if unnecessary) */
};

struct stub_code
{
    //
    // DATA
    //
    alignas(16) uint8_t             d_instructions[CODE_SIZE_MAX_TOTAL];
    alignas(8)  RUNTIME_FUNCTION    d_exception_data;
                                    // MSFT says RUNTIME_FUNCTION must be DWORD
                                    // aligned: http://msdn.microsoft.com/en-us/library/ft9x1kdx.aspx
    alignas(8)  uint8_t             d_unwind_info[UNWIND_INFO_SIZE];
                                    // MSFT says UNWIND_INFO must be DWORD
                                    // aligned...
    //
    // CONSTRUCTORS
    //
    stub_code(thunk64_impl *, void * wrapper_func_indirect_addr, size_t,
              param_register_types);
    ~stub_code();
    //
    // HELPERS
    //
    int32_t rip_rel_addr_offset(uint8_t *, uint8_t *);
    void compile(thunk64_impl *, void * wrapper_func_indirect_addr, size_t,
                 param_register_types);
    void register_exception_data();
};

stub_code::stub_code(thunk64_impl * impl_addr,
                     void * wrapper_func_indirect_addr,
                     size_t num_param_registers,
                     param_register_types register_types)
{
    assert(impl_addr || !"thunk implementation cannot be NULL");
    assert(wrapper_func_indirect_addr ||
        !"wrapper function indirect address cannot be NULL");
    assert(0 <= num_param_registers && num_param_registers <= NUM_PARAM_REGISTERS);
    // "Compile" the stub code
    compile(impl_addr, wrapper_func_indirect_addr, num_param_registers,
            register_types);
    // Create Windows exception unwind data
    register_exception_data();
}

stub_code::~stub_code()
{ RtlDeleteFunctionTable(&d_exception_data); }

int32_t stub_code::rip_rel_addr_offset(uint8_t * addr_addr,
                                       uint8_t * epilogue_start)
{
    // 'addr_addr' is the address of the memory containing the jump address
    // 'epilogue_start' is the address of the first epilogue instruction, which
    // is also the first instruction after the 'call' instruction.
    assert(addr_addr && epilogue_start);
    ptrdiff_t offset = addr_addr - epilogue_start;
    if (!(static_cast<ptrdiff_t>(std::numeric_limits<int32_t>::min()) <= offset &&
        offset <= static_cast<ptrdiff_t>(std::numeric_limits<int32_t>::max())))
    {
        LOG_ERROR("Wrapper indirect offset " << offset << " exceeds 32 bits; "
                  "addr_addr => " << addr_addr << ", epilogue_start => "
                  << epilogue_start);
        std::ostringstream() << "Wrapper indirect address offset " << offset
                             << " exceeds 32 bits"
                             << throw_cpp<std::domain_error>();
    }
    return static_cast<int32_t>(offset);
}

void stub_code::compile(thunk64_impl * impl_addr,
                        void * wrapper_func_indirect_addr,
                        size_t num_param_registers,
                        param_register_types register_types)
{
    uint8_t * cursor(d_instructions);
    cursor = std::copy(CODE_PROLOGUE, CODE_PROLOGUE + CODE_SIZE_PROLOGUE,
                       cursor);
    // Insert up to four instructions of the correct type to move data passed
    // in registers into the stack homespace
    for (size_t k = 0; k < num_param_registers; ++k)
    {
        const mov_stack * inst = &CODE_MOV_STACK_TABLE[register_types[k]][k];
        cursor = std::copy(inst->instruction,
                           inst->instruction + inst->size_bytes, cursor);
    }
    // Finish copying basic instructions
    uint8_t * const fixed_start(cursor); // Needed to replace placeholders
    cursor = std::copy(CODE_FIXED_BODY, CODE_FIXED_BODY + CODE_SIZE_FIXED_BODY,
                       cursor);
    cursor = std::copy(CODE_EPILOGUE, CODE_EPILOGUE + CODE_SIZE_EPILOGUE,
                       cursor);
    // Fill the rest of the structure with nop instructions
    assert(cursor <= d_instructions + jsdi::array_length(d_instructions));
    std::fill(cursor, d_instructions + jsdi::array_length(d_instructions), NOP);
    // Replace the placeholder bytes for the first argument to the wrapper
    // function with the address of the thunk implementation.
    static_assert(8 == sizeof(impl_addr), "check code");
    std::copy(fixed_start + CODE_OFFSET_FIXED_BODY_IMPL_POINTER,
              fixed_start + CODE_OFFSET_FIXED_BODY_IMPL_POINTER + 8,
              reinterpret_cast<uint8_t *>(&impl_addr));
    // Replace the placeholder bytes for the rip-relative offset to the memory
    // location containing the wrapper function (thunk64_impl::wrapper)
    // with the actual offset.
    std::copy(fixed_start + CODE_OFFSET_FIXED_BODY_CALL_ADDR,
              fixed_start + CODE_OFFSET_FIXED_BODY_CALL_ADDR + 4,
              reinterpret_cast<uint8_t *>(
                  rip_rel_addr_offset(
                      static_cast<uint8_t *>(wrapper_func_indirect_addr),
                      fixed_start + CODE_OFFSET_FIXED_BODY_CALL_ADDR + 4)));
}

void stub_code::register_exception_data()
{
    std::copy(UNWIND_INFO, UNWIND_INFO + UNWIND_INFO_SIZE, d_unwind_info);
    d_exception_data.BeginAddress = static_cast<ULONG>(
                                        d_instructions -
                                        reinterpret_cast<uint8_t *>(this));
    d_exception_data.EndAddress = static_cast<ULONG>(
                                      d_instructions -
                                      reinterpret_cast<uint8_t *>(this) +
                                      jsdi::array_length(d_instructions)
                                  );
    d_exception_data.UnwindInfoAddress = static_cast<ULONG>(
                                             d_unwind_info -
                                             reinterpret_cast<uint8_t *>(this));
    if (! RtlAddFunctionTable(&d_exception_data, 1, reinterpret_cast<DWORD64>(this)))
    {
        LOG_ERROR("Unable to register exception data: RtlAddFunctionTable("
                  << &d_exception_data << ', ' << 1 << ', ' << this
                  << ") failed and GetLastError() returned " << GetLastError());
        throw std::runtime_error("Thunk cannot register exception data");
    }
}

} // anonymous namespace

//==============================================================================
//                           struct thunk64_impl
//==============================================================================

namespace {

heap impl_heap("thunk64_impl", true);

} // anonymous namespace

struct thunk64_impl
{
    //
    // DATA
    //

    stub_code                               d_code;
    std::function<void()>                   d_setup;
    std::shared_ptr<thunk64::callback_t>    d_callback;
    std::function<void()>                   d_teardown;

    //
    // CONSTRUCTORS
    //

    thunk64_impl(size_t, param_register_types, const std::function<void()>&,
                 const std::shared_ptr<thunk64::callback_t>&,
                 const std::function<void()>&);

    //
    // STATICS
    //

    static uint64_t wrapper(thunk64_impl *, const uint64_t *);
    static void * wrapper_addr;

    //
    // OPERATORS
    //

    void * operator new(size_t);

    void operator delete(void *);

    void * operator new[](size_t) = delete;
    void operator delete[](void *) = delete;
};

// Pointer-to-function-pointer required for indirect 'call' instruction.
void * thunk64_impl::wrapper_addr(thunk64_impl::wrapper);

thunk64_impl::thunk64_impl(
    size_t num_param_registers, param_register_types register_types,
    const std::function<void()>& setup,
    const std::shared_ptr<thunk64::callback_t>& callback,
    const std::function<void()>& teardown)
    : d_code(this, &wrapper_addr, num_param_registers, register_types)
    , d_setup(setup)
    , d_callback(callback)
    , d_teardown(teardown)
{ }

uint64_t thunk64_impl::wrapper(thunk64_impl * impl, const uint64_t * args)
{
    uint64_t result(0);
    LOG_TRACE("thunk64_impl::wrapper ( impl => " << impl << ", args => "
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

void * thunk64_impl::operator new(size_t n)
{ return impl_heap.alloc(n); }

void thunk64_impl::operator delete(void * ptr)
{ impl_heap.free(ptr); }

//==============================================================================
//                              class thunk64
//==============================================================================

thunk64::thunk64(const std::shared_ptr<callback_t>& callback_ptr,
                 size_t num_param_registers,
                 param_register_types register_types)
    : thunk(callback_ptr)
    , d_impl(new thunk64_impl(num_param_registers, register_types,
          std::bind(std::mem_fn(&thunk64::setup_call), this),
          callback_ptr,
          std::bind(std::mem_fn(&thunk64::teardown_call), this)))
{ }

void * thunk64::func_addr()
{ return d_impl->d_code.d_instructions; }

} // namespace abi_amd64
} // namespace jsdi

//==============================================================================
//                                  TESTS
//==============================================================================

#ifndef __NOTEST__

#include "test.h"
#include "invoke64.h"

using namespace jsdi::abi_amd64;

static const int EMPTY_PTR_ARRAY[1] = { };

typedef thunk64::callback_t callback_t;

// callback that can invoke a function using direct data (arguments) only and
// return its result
struct direct_callback : public callback_t
{
    void *               d_func_ptr;
    param_register_types d_register_types;
    template<typename FuncPtr>
    direct_callback(FuncPtr func_ptr, int size_direct,
                    param_register_types register_types)
        : callback(size_direct, 0, EMPTY_PTR_ARRAY, 0, 0)
        , d_func_ptr(reinterpret_cast<void *>(func_ptr))
    { }
    virtual uint64_t call(const uint64_t * args)
    {
        const size_t args_size_bytes(static_cast<size_t>(d_size_direct));
        return d_register_types.has_fp()
            ? invoke64_basic(args_size_bytes, args, d_func_ptr)
            : invoke64_fp(args_size_bytes, args, d_func_ptr, d_register_types);
    }
};


#endif // __NOTEST__

/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: seh.cpp
// auth: Victor Schappert
// date: 20130819
// desc: Code for catching SEH exceptions and rethrowing C++ exceptions
//==============================================================================

#include "seh.h"

#include <sstream>
#include <type_traits>

namespace jsdi {

namespace {

#define SEH_EXCEPTION_NAME(x) case EXCEPTION_##x: return #x;

#define SEH_FATAL_EXCEPTION(x) case EXCEPTION_##x: return nullptr;

inline char const * seh_exception_name(DWORD code)
{   // Similar to cSuneido unhandled.cpp 'exception_name()' function. This
    // function returns nullptr if the exception code corresponds to a fatal
    // exception that should not be caught.
    switch (code)
    {
        SEH_EXCEPTION_NAME(ACCESS_VIOLATION)
        SEH_EXCEPTION_NAME(DATATYPE_MISALIGNMENT)
        SEH_EXCEPTION_NAME(BREAKPOINT)
        SEH_EXCEPTION_NAME(SINGLE_STEP)
        SEH_EXCEPTION_NAME(ARRAY_BOUNDS_EXCEEDED)
        SEH_EXCEPTION_NAME(FLT_DENORMAL_OPERAND)
        SEH_EXCEPTION_NAME(FLT_DIVIDE_BY_ZERO)
        SEH_EXCEPTION_NAME(FLT_INEXACT_RESULT)
        SEH_EXCEPTION_NAME(FLT_INVALID_OPERATION)
        SEH_EXCEPTION_NAME(FLT_OVERFLOW)
        SEH_EXCEPTION_NAME(FLT_STACK_CHECK)
        SEH_EXCEPTION_NAME(FLT_UNDERFLOW)
        SEH_EXCEPTION_NAME(INT_DIVIDE_BY_ZERO)
        SEH_EXCEPTION_NAME(INT_OVERFLOW)
        SEH_EXCEPTION_NAME(PRIV_INSTRUCTION)
        SEH_EXCEPTION_NAME(ILLEGAL_INSTRUCTION)
        SEH_EXCEPTION_NAME(INVALID_HANDLE)

        SEH_FATAL_EXCEPTION(IN_PAGE_ERROR)
        SEH_FATAL_EXCEPTION(NONCONTINUABLE_EXCEPTION)
        SEH_FATAL_EXCEPTION(STACK_OVERFLOW)
        SEH_FATAL_EXCEPTION(INVALID_DISPOSITION)
        SEH_FATAL_EXCEPTION(GUARD_PAGE)

        default: return "???";
    } // switch
}

std::string seh_exception_message(EXCEPTION_RECORD const& record)
{
    DWORD const code(record.ExceptionCode);
    char const * name(seh_exception_name(code));
    assert(name || !"can't get message for a fatal error");
    std::ostringstream o;
    o << "win32 exception: " << name;
    if (EXCEPTION_ACCESS_VIOLATION == code)
    {
        o << " at address " << record.ExceptionAddress;
    }
    return o.str();
}

} // anonymous namespace

//==============================================================================
//                           class seh_exception
//==============================================================================

seh_exception::seh_exception(EXCEPTION_RECORD const& record)
    : runtime_error(seh_exception_message(record))
{ }

//==============================================================================
//                                struct seh
//==============================================================================

namespace {

// TODO: Change MSFT "__declspec(thread)" to C++ "thread_local" once Visual C++
//       supports the latter. It is not available as of November 2013 CTP.
__declspec(thread) EXCEPTION_RECORD seh_record;
static_assert(
    std::is_pod<decltype(seh_record)>::value,
    "static storage duration exception record must be plain old data");
// NOTE: The C++ thread_local storage duration keyword does support non-POD
//       types. However, we would like to be able to assume 'seh_record' is
//       POD, so we assert.

} // anonymous namespace

int seh::filter(struct _EXCEPTION_POINTERS const * info)
{
    char const * name = seh_exception_name(
        info->ExceptionRecord->ExceptionCode);
    if (! name) return 0; // Don't handle fatal errors.
    seh_record = *info->ExceptionRecord;
    return 1;
}

void seh::convert_last_filtered_to_cpp()
{ throw seh_exception(seh_record); }

} // namespace jsdi

//==============================================================================
//                                  TESTS
//==============================================================================

#ifndef __NOTEST__

#include "test.h"

#include <algorithm>

using namespace jsdi;

namespace {

int test_and_set(int * arr, int index, int new_val)
{
    int old_val(arr[index]);
    arr[index] = new_val;
    return old_val;
}

} // anonymous namespace

TEST(access_violation,
    int x(1);
    assert_equals(1, test_and_set(&x, 0, 0));
    assert_equals(0, x);
    std::string message;
    try
    {
        seh::convert_to_cpp_func_ptr_wrapper(test_and_set,
                                             static_cast<int *>(nullptr), 0,
                                             10);
    }
    catch (seh_exception const& e)
    { message = e.what(); }
    std::string const expected("win32 exception: ACCESS_VIOLATION at address ");
    auto m = std::mismatch(expected.begin(), expected.end(), message.begin());
    assert_true(expected.end() == m.first);
);

#endif // __NOTEST__

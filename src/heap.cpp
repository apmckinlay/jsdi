//==============================================================================
// file: heap.cpp
// auth: Victor Schappert
// date: 20130802
// desc: Implements C++ wrapper for a Win32 heap
//==============================================================================

#include "heap.h"

#include "jsdi_windows.h"

#include <cassert>
#include <memory>

namespace jsdi {

//==============================================================================
//                                  INTERNALS
//==============================================================================

namespace {

// For some reason, this flag isn't defined in many versions of windows.h
#ifndef HEAP_CREATE_ENABLE_EXECUTE
enum { HEAP_CREATE_ENABLE_EXECUTE = 0x00040000 };
#endif

inline DWORD flags(bool is_executable)
{ return is_executable ? HEAP_CREATE_ENABLE_EXECUTE : 0; }

} // anonymous namespace

//==============================================================================
//                                class heap
//==============================================================================

struct heap_impl
{
    HANDLE      d_hheap;
    std::string d_name;
};

heap::heap(const char * name, bool is_executable) throw(std::bad_alloc)
    : d_impl(0)
{
    assert(name || !"heap name cannot be null");
    std::unique_ptr<heap_impl> tmp(new heap_impl);
    tmp->d_hheap = HeapCreate(
        flags(is_executable),
        0 /* default size */,
        0 /* growable */
    );
    if (! tmp->d_hheap) throw std::bad_alloc();
    tmp->d_name.assign(name);
    d_impl = tmp.release();
}

heap::~heap()
{
    assert(d_impl && d_impl->d_hheap);
    BOOL success = HeapDestroy(d_impl->d_hheap);
    delete d_impl;
    assert(success || !"failed to destroy Win32 heap");
}

const std::string& heap::name() const
{ return d_impl->d_name; }

void * heap::alloc(size_t n) throw(std::bad_alloc)
{
    assert(d_impl && d_impl->d_hheap);
    void * ptr = HeapAlloc(d_impl->d_hheap, 0, n);
    if (! ptr) throw std::bad_alloc();
    return ptr;
}

void heap::free(void * ptr) noexcept
{
    assert(d_impl && d_impl->d_hheap);
    BOOL success = HeapFree(d_impl->d_hheap, 0, ptr);
    assert(success || !"failed to free heap memory");
}

} // namespace jsdi

//==============================================================================
//                                  TESTS
//==============================================================================

#ifndef __NOTEST__

#include "test.h"

#include <cstring>

using namespace jsdi;

TEST(heap,
    heap h1("my heap", false);
    char * str = reinterpret_cast<char *>(
                     h1.alloc(std::strlen("bonjour monde") + 1));
    assert_true(str);
    std::strcpy(str, "bonjour monde");
    assert_equals(std::string("bonjour monde"), str);
    h1.free(str);
    static unsigned char CODE[] =
    {
        0xb8, 0x1b, 0x00, 0x00, 0x00, // movl 0x0000001b, %eax
        0xc3                          // ret
    };
    heap h2("exeheap", true);
    void * code = h2.alloc(sizeof(CODE));
    std::memcpy(code, CODE, sizeof(CODE));
    int should_be_27(0xffffff00);
#ifdef __GNUC__
    asm (
        "call * %1"
        : "=a" (should_be_27)
        : "0" (code)
        :
    );
#else
#pragma error "Replacement for inline assembler required"
#endif
    assert_equals(27, should_be_27);
);

#endif // #ifndef __NOTEST__

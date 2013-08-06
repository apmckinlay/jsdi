//==============================================================================
// file: callback.cpp
// auth: Victor Schappert
// date: 20130804
// desc: Implementation of generic interface for a callback function
//==============================================================================

#include "callback.h"

#include <cstring>
#include <memory>

namespace jsdi {

//==============================================================================
//                                INTERNALS
//==============================================================================

namespace {

inline char ** to_char_ptr(char * data, int ptr_pos)
{ return reinterpret_cast<char **>(&data[ptr_pos]); }

} // anonymous namespace

//==============================================================================
//                            class callback_args
//==============================================================================

callback_args::~callback_args()
{ } // anchor for virtual destructor


//==============================================================================
//                              class callback
//==============================================================================

inline bool callback::is_vi_ptr(int ptd_to_pos) const
{ return d_size_total <= ptd_to_pos; }

void callback::vi_ptr(callback_args& args, int ptr_pos, int ptd_to_pos)
{
    int vi_index = ptd_to_pos - d_size_total;
    assert(0 <= vi_index && vi_index < d_vi_count);
    char ** pstr(to_char_ptr(args.data(), ptr_pos));
    if (*pstr) args.vi_string_ptr(*pstr, vi_index);
}

void callback::normal_ptr(callback_args& args,
                          int ptr_pos, int ptd_to_pos,
                          std::vector<int>::const_iterator& i,
                          std::vector<int>::const_iterator& e)
{
    // STAGE 1: Copy
    //     Look at the next pointer following this one.
    //     - If it is a vi pointer, skip and keep looking.
    //     - If it is a standard pointer, its ptd_to_pos sets the boundary of
    //       what needs to be copied.
    //     - If there is no next pointer, boundary of what needs to be copied is
    //       the end of the data.
    //
    //     NOTE: If this normal pointer is NULL, we don't copy, we zero out.
    int copy_end(d_size_total);
    if (i != e)
    {
        std::vector<int>::const_iterator j(i);
        do
        {
            ++j;
            int next_ptd_to_pos = *j++;
            if (! is_vi_ptr(next_ptd_to_pos))
            {
                copy_end = next_ptd_to_pos;
                break;
            }
        }
        while (j != e);
    }
    char ** ptr_addr(to_char_ptr(args.data(), ptr_pos));
    char ** ptd_to_addr(to_char_ptr(args.data(), ptd_to_pos));
    if (*ptr_addr)
        std::memcpy(ptd_to_addr, *ptr_addr, copy_end - ptd_to_pos);
    else
        std::memset(ptd_to_addr, 0, copy_end - ptd_to_pos);
    // STAGE 2: Handle sub-pointers
    //     For each subsequent pointer that is within the block just copied or
    //     zeroed:
    //     - If it's a vi pointer, just handle it, since it points to "flat"
    //       string data.
    //     - If it's a normal pointer, call this function recursively.
    while (i != e)
    {
        int next_ptr_pos = *(i + 1);
        if (! (ptd_to_pos <= next_ptr_pos && next_ptr_pos < copy_end)) break;
        ++i;
        int next_ptd_to_pos = *i++;
        if (is_vi_ptr(next_ptd_to_pos))
            vi_ptr(args, next_ptr_pos, next_ptd_to_pos);
        else
            normal_ptr(args, next_ptr_pos, next_ptd_to_pos, i, e);
    }
}

callback::callback(int size_direct, int size_indirect, const int * ptr_array,
                   int ptr_array_size, int vi_count)
    : d_ptr_array(ptr_array_size)
{
    set_unmarshall_params(size_direct, size_indirect, ptr_array, ptr_array_size,
                        vi_count);
}

callback::~callback()
{ } // anchor for virtual destructor

void callback::set_unmarshall_params(int size_direct, int size_indirect,
                                   const int * ptr_array, int ptr_array_size,
                                   int vi_count)
{
    d_size_direct = size_direct;
    d_size_indirect = size_indirect;
    d_size_total = size_direct + size_indirect;
    assert(ptr_array || !"ptr_array cannot be null");
    assert(0 == ptr_array_size % 2 || "ptr_array must have even size");
    d_ptr_array.assign(ptr_array, ptr_array + ptr_array_size);
    d_vi_count = vi_count;
}

long callback::call(const char * args)
{
    std::unique_ptr<callback_args> args_(alloc_args());
    char * data(args_->data());
    std::memcpy(data, args, d_size_direct);
    std::vector<int>::const_iterator ptr_i(d_ptr_array.begin()),
                                     ptr_e(d_ptr_array.end());
    while (ptr_i != ptr_e)
    {
        int ptr_pos = *ptr_i++;
        int ptd_to_pos = *ptr_i++;
        if (is_vi_ptr(ptd_to_pos))          // vi pointer
            vi_ptr(*args_, ptr_pos, ptd_to_pos);
        else                                // normal pointer
            normal_ptr(*args_, ptr_pos, ptd_to_pos, ptr_i, ptr_e);
    }
    return call(*args_);
}

#ifndef __NOTEST__

//==============================================================================
//                         class test_callback_args
//==============================================================================

namespace {

test_callback_args last_value_(0, 0);

} // anonymous namespace

test_callback_args::~test_callback_args()
{
    if (this != &last_value_) try
    {
        last_value_.d_data = d_data;
        last_value_.d_vi_array = d_vi_array;
        last_value_.set_data(last_value_.d_data.data());
    }
    catch (...)
    { }
}

void test_callback_args::vi_string_ptr(const char * str, int vi_index)
{
    assert(0 <= vi_index && static_cast<size_t>(vi_index) < d_vi_array.size());
    assert(str || !"str cannot be NULL");
    d_vi_array[vi_index].reset(new std::string(str));
}

const test_callback_args& test_callback_args::last_value()
{ return last_value_; }

//==============================================================================
//                            class test_callback
//==============================================================================

callback_args * test_callback::alloc_args() const
{ return new test_callback_args(d_size_total, d_vi_count); }

long test_callback::call(callback_args& args)
{ return 0L; }

#endif // __NOTEST__

} // namespace jsdi

//==============================================================================
//                                  TESTS
//==============================================================================

#ifndef __NOTEST__

#include "test.h"

#include <cstring>

using namespace jsdi;

static const int EMPTY_PTR_ARRAY[0] = { };

template<typename T>
inline void vector_append(std::vector<char>& dest, const T& ref)
{
    const char * i = reinterpret_cast<const char *>(&ref);
    const char * e = i + sizeof(T);
    dest.insert(dest.end(), i, e);
}

TEST(unmarshall_flat_args,
    static const char ARGS[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    test_callback cb(sizeof(ARGS), 0, EMPTY_PTR_ARRAY, 0, 0);
    assert_equals(0, cb.callback::call(ARGS));
    assert_equals(0, std::memcmp(ARGS, last_value_.data(), sizeof(ARGS)));
);

TEST(unmarshall_flat_args_vi,
    static const char * ARGS[] = { 0, "Hallo Welt", "xyz" };
    static const int PTR_ARRAY[] =
    { sizeof(char *), sizeof(ARGS), 2 * sizeof(char *), sizeof(ARGS) + 1 };
    test_callback cb(sizeof(ARGS), 0, PTR_ARRAY, array_length(PTR_ARRAY), 2);
    cb.callback::call(reinterpret_cast<const char *>(ARGS));
    assert_equals(0, std::memcmp(ARGS, last_value_.data(), sizeof(ARGS)));
    test_callback_args::vi_vector vi(last_value_.vi_array());
    assert_equals(2, vi.size());
    assert_equals("Hallo Welt", *vi[0]);
    assert_equals("xyz", *vi[1]);
);

TEST(unmarshall_level_one_simple,
    static const struct { int a; int b; char c; } s = { -1, 1, 'c' };
    static const void * ARGS[] = { &s };
    static const int PTR_ARRAY[] = { 0, sizeof(ARGS) };
    test_callback cb(sizeof(ARGS), sizeof(s), PTR_ARRAY,
                     array_length(PTR_ARRAY), 0);
    cb.callback::call(reinterpret_cast<const char *>(ARGS));
    std::vector<char> expected;
    vector_append(expected, ARGS);
    vector_append(expected, s);
    assert_equals(0, std::memcmp(expected.data(), last_value_.data(),
                                 expected.size()));
);

TEST(unmarshall_level_one_complex,
     static const struct S { double d; char c; int64_t i; } s1 = { 2.5, 'C',
         0xfffffffffffffffeLL }, s2 = { -2.5, 'D', 50LL }, szero { 0, 0, 0LL };
     static const S * ARGS[] { 0, &s1, 0, &s1, &s2, 0, &s1 };
     static const int PTR_ARRAY[] =
     {
         0 * sizeof(const S *), sizeof(ARGS) + 0 * sizeof(S),
         1 * sizeof(const S *), sizeof(ARGS) + 1 * sizeof(S),
         2 * sizeof(const S *), sizeof(ARGS) + 2 * sizeof(S),
         3 * sizeof(const S *), sizeof(ARGS) + 3 * sizeof(S),
         4 * sizeof(const S *), sizeof(ARGS) + 4 * sizeof(S),
         5 * sizeof(const S *), sizeof(ARGS) + 5 * sizeof(S),
         6 * sizeof(const S *), sizeof(ARGS) + 6 * sizeof(S)
     };
     test_callback cb(sizeof(ARGS), 7 * sizeof(S), PTR_ARRAY,
                      array_length(PTR_ARRAY), 0);
     cb.callback::call(reinterpret_cast<const char *>(ARGS));
     std::vector<char> expected;
     vector_append(expected, ARGS);
     vector_append(expected, szero);
     vector_append(expected, s1);
     vector_append(expected, szero);
     vector_append(expected, s1);
     vector_append(expected, s2);
     vector_append(expected, szero);
     vector_append(expected, s1);
     assert_equals(0, std::memcmp(expected.data(), last_value_.data(),
                                  expected.size()));
);

TEST(unmarshall_level_two,
    static const struct S { int level; const S * next; } s2 = { 2, 0 },
        s1 = { 1, &s2 }, szero = { 0, 0 };
    static const S * ARGS[] = { 0, &s1 };
    static const int PTR_ARRAY[] =
    {
        0 * sizeof(const S *),
            sizeof(ARGS) + 0 * sizeof(S),
        sizeof(ARGS) + 0 * sizeof(S) + sizeof(int),
            sizeof(ARGS) + 1 * sizeof(S),
        1 * sizeof(const S *),
            sizeof(ARGS) + 2 * sizeof(S),
        sizeof(ARGS) + 2 * sizeof(S) + sizeof(int),
            sizeof(ARGS) + 3 * sizeof(S)
    };
    test_callback cb(sizeof(ARGS), 4 * sizeof(S), PTR_ARRAY,
                     array_length(PTR_ARRAY), 0);
    cb.callback::call(reinterpret_cast<const char *>(ARGS));
    std::vector<char> expected;
    vector_append(expected, ARGS);
    vector_append(expected, szero);
    vector_append(expected, szero);
    vector_append(expected, s1);
    vector_append(expected, s2);
    assert_equals(0, std::memcmp(expected.data(), last_value_.data(),
                                 expected.size()));
);

TEST(unmarshall_level_three_with_vi,
    static const struct S { int level; const S * next; const char * str; }
        s3 = { 3, 0, "level3" }, s2 = { 2, &s3, "level2" },
        s1 = { 1, &s2, "level1" }, szero = { 0, 0, 0 };
    static const S * ARGS[] = { &szero, &s1, 0 };
    enum
    {
        SIZE_DIRECT = sizeof(ARGS),
        SIZE_INDIRECT = 4 * sizeof(S),
        SIZE_TOTAL = SIZE_DIRECT + SIZE_INDIRECT,
        NEXT_OFFSET = sizeof(int),
        STR_OFFSET = NEXT_OFFSET + sizeof(const S *)
    };
    static const int PTR_ARRAY[] =
    {
        0 * sizeof(const S *),
            SIZE_DIRECT + 0 * sizeof(S),
        SIZE_DIRECT + 0 * sizeof(S) + STR_OFFSET,     // NULL -> str
            SIZE_TOTAL + 0,
        1 * sizeof(const S *),                         // ARG -> s1
            SIZE_DIRECT + 1 * sizeof(S),              // s1
        SIZE_DIRECT + 1 * sizeof(S) + NEXT_OFFSET,    // s1.next -> s2
            SIZE_DIRECT + 2 * sizeof(S),              // s2
        SIZE_DIRECT + 2 * sizeof(S) + NEXT_OFFSET,    // s2.next -> s3
            SIZE_DIRECT + 3 * sizeof(S),              // s3
        SIZE_DIRECT + 3 * sizeof(S) + STR_OFFSET,     // s3.str
            SIZE_TOTAL + 1,
        SIZE_DIRECT + 2 * sizeof(S) + STR_OFFSET,     // s2.str
            SIZE_TOTAL + 2,
        SIZE_DIRECT + 1 * sizeof(S) + STR_OFFSET,     // s1.str
            SIZE_TOTAL + 3,
    };
    test_callback cb(sizeof(ARGS), SIZE_INDIRECT, PTR_ARRAY,
                     array_length(PTR_ARRAY), 4);
    cb.callback::call(reinterpret_cast<const char *>(ARGS));
    std::vector<char> expected;
    vector_append(expected, ARGS);
    vector_append(expected, szero);
    vector_append(expected, s1);
    vector_append(expected, s2);
    vector_append(expected, s3);
    assert_equals(0, std::memcmp(expected.data(), last_value_.data(),
                                 expected.size()));
    test_callback_args::vi_vector vi(last_value_.vi_array());
    assert_equals(4, vi.size());
    assert_true(! vi[0].get());
    assert_equals("level3", *vi[1]);
    assert_equals("level2", *vi[2]);
    assert_equals("level1", *vi[3]);
);

#endif // __NOTEST__

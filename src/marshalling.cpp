/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: marshalling.cpp
// auth: Victor Schappert
// date: 20130812
// desc: Functions for marshalling data structures between the format sent by
//       jSuneido and the format expected by C
//==============================================================================

#include "marshalling.h"

namespace jsdi {

//==============================================================================
//                      class marshalling_vi_container
//==============================================================================

marshalling_vi_container::~marshalling_vi_container()
{
    // NOTE: This destructor may be invoked in the context  of a C++ catch block
    //       cleanup operation triggered by a JNI_EXCEPTION_CHECK. So it may run
    //       when there is a JNI exception pending. Ergo, you can't call any JNI
    //       functions which do anything other than legitimate cleanup from
    //       within this destructor.
    try
    {
        const size_t N(d_arrays.size());
        for (size_t k = 0; k < N; ++k)
        {
            tuple& t(d_arrays[k]);
            if (! t.d_elems) continue;
            if (t.d_global)
            {
                d_env->ReleaseByteArrayElements(t.d_global, t.d_elems, 0);
                d_env->DeleteGlobalRef(t.d_global);
            }
        }
    }
    catch (...)
    { }
}

inline void marshalling_vi_container::put_not_null(jint pos, jbyteArray array,
                                                   jbyte ** pp_array)
{ // NORMAL USE (arguments)
    assert(0 <= pos && pos < d_arrays.size());
    tuple& t = d_arrays[pos];
    assert(! t.d_elems || !"duplicate variable indirect pointer");
    t.d_elems = d_env->GetByteArrayElements(array, &t.d_is_copy);
    JNI_EXCEPTION_CHECK(d_env);
    if (! t.d_elems) throw jni_bad_alloc("GetByteArrayElements", __FUNCTION__);
    assert(! t.d_global);
    // Save a global reference to the array. This allows us to call
    // ReleaseByteArrayElements from the destructor (A) regardless of whether
    // the corresponding entry in d_object_array is subsequently replaced by a
    // different value; and (B) without having to call GetObjectArrayElement(),
    // which isn't permitted if a JNI exception has been flagged.
    t.d_global = static_cast<jbyteArray>(d_env->NewGlobalRef(array));
    JNI_EXCEPTION_CHECK(d_env);
    t.d_pp_arr = pp_array;
    *pp_array = t.d_elems;
}

//==============================================================================
//                       struct marshalling_roundtrip
//==============================================================================

void marshalling_roundtrip::ptrs_init_vi(
    marshall_word_t * args, jsize args_size, const jint * ptr_array,
    jsize ptr_array_size, JNIEnv * env, jobjectArray vi_array_in,
    marshalling_vi_container& vi_array_out)
{
    assert(0 == ptr_array_size % 2 || !"pointer array must have even size");
    jint const * i(ptr_array), * e(ptr_array + ptr_array_size);
    jint const total_size = args_size * sizeof(marshall_word_t);
    while (i < e)
    {
        jint ptr_word_index = *i++;
        jint ptd_to_pos = *i++;
        if (UNKNOWN_LOCATION == ptd_to_pos) continue; // Leave a null pointer
        assert(0 <= ptd_to_pos || !"pointer offset must be non-negative");
        jbyte ** ptr_addr = reinterpret_cast<jbyte **>(&args[ptr_word_index]);
        if (ptd_to_pos < total_size)
        {
            // Normal pointer: points back into a location within args
            jbyte * ptd_to_addr = reinterpret_cast<jbyte *>(args) + ptd_to_pos;
            *ptr_addr = ptd_to_addr;
        }
        else
        {
            // Variable indirect pointer: points to the start of a byte[] which
            // is passed in from Java in vi_array_in, and marshalled into the
            // C++ environment in vi_array_out.
            ptd_to_pos -= total_size; // Convert to index into vi_array
            assert(
                static_cast<size_t>(ptd_to_pos) < vi_array_out.d_arrays.size()
                || !"pointer points outside of variable indirect array"
            );
            jni_auto_local<jobject> object(
                env, env->GetObjectArrayElement(vi_array_in, ptd_to_pos));
            JNI_EXCEPTION_CHECK(env);
            if (! object)
            {   // Note if this is a 'resource', the value at *ptr_addr is
                // actually a 16-bit integer which might not be NULL
                vi_array_out.put_null(ptd_to_pos, ptr_addr);
            }
            else
            {
                assert(env->IsInstanceOf(object, GLOBAL_REFS->byte_ARRAY()));
                vi_array_out.put_not_null(
                    ptd_to_pos,
                    static_cast<jbyteArray>(static_cast<jobject>(object)),
                    ptr_addr
                );
            }
        }
    }
}

void marshalling_roundtrip::ptrs_finish_vi(
    jobjectArray vi_array_java, marshalling_vi_container& vi_array_cpp,
    const jni_array_region<jint>& vi_inst_array)
{
    const jsize N(static_cast<jsize>(vi_array_cpp.d_arrays.size()));
    JNIEnv * const env(vi_array_cpp.d_env);
    assert(
        N == vi_inst_array.size() || !"variable indirect array size mismatch");
    for (jsize k = 0; k < N; ++k)
    {
        const marshalling_vi_container::tuple& tuple(vi_array_cpp.d_arrays[k]);
        switch (ordinal_enum_to_cpp<
            suneido_jsdi_VariableIndirectInstruction>(vi_inst_array[k])
        )
        {
            case NO_ACTION:
                break;
            case RETURN_JAVA_STRING:
                if (! *tuple.d_pp_arr)
                {   // null pointer, so return a null String ref
                    vi_array_cpp.replace_byte_array(k, nullptr);
                }
                else
                {
                    jni_auto_local<jstring> str(
                        env, make_jstring(env, *tuple.d_pp_arr));
                    vi_array_cpp.replace_byte_array(k, str);
                }
                break;
            case RETURN_RESOURCE:
                if (IS_INTRESOURCE(*tuple.d_pp_arr))
                {   // it's an INT resource, not a string, so return an Integer
                    jni_auto_local<jobject> int_resource(
                        env,
                        env->NewObject(
                            GLOBAL_REFS->java_lang_Integer(),
                            GLOBAL_REFS->java_lang_Integer__init(),
                            reinterpret_cast<int>(*tuple.d_pp_arr)
                        )
                    );
                    JNI_EXCEPTION_CHECK(env);
                    if (! int_resource)
                        throw jni_bad_alloc("NewObject", __FUNCTION__);
                    vi_array_cpp.replace_byte_array(k, int_resource);
                }
                else
                {
                    jni_auto_local<jstring> str(
                        env, make_jstring(env, *tuple.d_pp_arr));
                    vi_array_cpp.replace_byte_array(k, str);
                }
                break;
            default:
                assert(!"control should never pass here");
                break;
        }
    }
}

//==============================================================================
//                        class unmarshaller_indirect
//==============================================================================

void unmarshaller_indirect::normal_ptr(
    marshall_word_t * data, int ptr_word_index, int ptd_to_byte_offset,
    ptr_iterator_t& ptr_i) const
{
    // STAGE 1: Copy
    //     Look at the next pointer following this one.
    //     - Its ptd_to_byte_offset sets the boundary of what needs to be
    //       copied.
    //     - If there is no next pointer, boundary of what needs to be copied is
    //       the end of the data.
    //
    //     NOTE: If this normal pointer is NULL, we don't copy, we zero out.
    int copy_end_byte_offset(d_size_total);
    if (ptr_i != d_ptr_end) copy_end_byte_offset = *(ptr_i + 1);
    auto ptr_addr = addr_of_ptr(data, ptr_word_index);
    auto ptd_to_addr = addr_of_byte(data, ptd_to_byte_offset);
    if (*ptr_addr)
        std::memcpy(ptd_to_addr, *ptr_addr,
                    copy_end_byte_offset - ptd_to_byte_offset);
    else
        std::memset(ptd_to_addr, 0,
                    copy_end_byte_offset - ptd_to_byte_offset);
    // STAGE 2: Handle sub-pointers
    //     For each subsequent pointer that is within the block just copied or
    //     zeroed, call this function recursively.
    while (ptr_i != d_ptr_end)
    {
        const int next_ptr_word_index = *ptr_i;
        const int next_ptr_byte_offset = sizeof(marshall_word_t) *
                                         next_ptr_word_index;
        if (! (ptd_to_byte_offset <= next_ptr_byte_offset &&
               next_ptr_byte_offset < copy_end_byte_offset))
            break;
        ++ptr_i;
        const int next_ptd_to_byte_offset = *ptr_i++;
        normal_ptr(data, next_ptr_word_index, next_ptd_to_byte_offset, ptr_i);
    }
}

//==============================================================================
//                        class unmarshaller_vi_base
//==============================================================================

void unmarshaller_vi_base::vi_ptr(
    marshall_word_t * data, int ptr_word_index, int ptd_to_pos, JNIEnv * env,
    jobjectArray vi_array, const int * vi_inst_array)
{
    int vi_index = ptd_to_pos - unmarshaller_base::d_size_total;
    assert(0 <= vi_index && vi_index < d_vi_count);
    auto pstr = unmarshaller_base::addr_of_ptr(data, ptr_word_index);
    if (*pstr)
    {
        auto str = reinterpret_cast<const char *>(*pstr);
        vi_string_ptr(str, vi_index, env, vi_array, vi_inst_array[vi_index]);
    }
}

void unmarshaller_vi_base::normal_ptr(
    marshall_word_t * data, int ptr_word_index, int ptd_to_byte_offset,
    ptr_iterator_t& ptr_i, JNIEnv * env, jobjectArray vi_array,
    const int * vi_inst_array)
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
    int copy_end_byte_offset(unmarshaller_base::d_size_total);
    ptr_iterator_t ptr_e(unmarshaller_indirect::d_ptr_end);
    if (ptr_i != ptr_e)
    {
        ptr_iterator_t ptr_j(ptr_i);
        do
        {
            ++ptr_j;
            const int next_ptd_to_pos = *ptr_j++;
            if (! is_vi_ptr(next_ptd_to_pos))
            {
                copy_end_byte_offset = next_ptd_to_pos;
                break;
            }
        }
        while (ptr_j != ptr_e);
    }
    auto ptr_addr = addr_of_ptr(data, ptr_word_index);
    auto ptd_to_addr = addr_of_byte(data, ptd_to_byte_offset);
    if (*ptr_addr)
        std::memcpy(ptd_to_addr, *ptr_addr,
                    copy_end_byte_offset - ptd_to_byte_offset);
    else
        std::memset(ptd_to_addr, 0,
                    copy_end_byte_offset - ptd_to_byte_offset);
    // STAGE 2: Handle sub-pointers
    //     For each subsequent pointer that is within the block just copied or
    //     zeroed:
    //     - If it's a vi pointer, just handle it, since it points to "flat"
    //       string data.
    //     - If it's a normal pointer, call this function recursively.
    while (ptr_i != ptr_e)
    {
        const int next_ptr_word_index = *ptr_i;
        const int next_ptr_byte_offset = sizeof(marshall_word_t) *
                                         next_ptr_word_index;
        if (! (ptd_to_byte_offset <= next_ptr_byte_offset &&
               next_ptr_byte_offset < copy_end_byte_offset))
            break;
        ++ptr_i;
        const int next_ptd_to_pos = *ptr_i++;
        if (is_vi_ptr(next_ptd_to_pos))
            vi_ptr(data, next_ptr_word_index, next_ptd_to_pos, env, vi_array,
                   vi_inst_array);
        else
            normal_ptr(data, next_ptr_word_index, next_ptd_to_pos, ptr_i, env,
                       vi_array, vi_inst_array);
    }
}

//==============================================================================
//                        class unmarshaller_vi_test
//==============================================================================

#ifndef __NOTEST__

void unmarshaller_vi_test::vi_string_ptr(const char * str, int vi_index,
                                         JNIEnv * env, jobjectArray vi_array,
                                         int vi_inst)
{
    assert(
        0 <= vi_index && vi_index < static_cast<int>(d_vi_data.size()));
    assert(str || !"str cannot be NULL");
    d_vi_data[vi_index].reset(new std::string(str));
}

#endif // __NOTEST__

//==============================================================================
//                           class unmarshaller_vi
//==============================================================================

void unmarshaller_vi::vi_string_ptr(const char * str, int vi_index,
                                    JNIEnv * env, jobjectArray vi_array,
                                    int vi_inst)
{
    assert(env || !"JNI environment cannot be NULL");
    assert(env || !"vi_array cannot be NULL");
    switch (ordinal_enum_to_cpp<
        suneido_jsdi_VariableIndirectInstruction>(vi_inst)
    )
    {
        case NO_ACTION:
            break;
        case RETURN_RESOURCE:
            if (IS_INTRESOURCE(str))
            {   // it's an INT resource, not a string, so return an Integer
                jni_auto_local<jobject> int_resource(
                    env,
                    env->NewObject(
                        GLOBAL_REFS->java_lang_Integer(),
                        GLOBAL_REFS->java_lang_Integer__init(),
                        reinterpret_cast<int>(str)
                    )
                );
                JNI_EXCEPTION_CHECK(env);
                if (! int_resource)
                    throw jni_bad_alloc("NewObject", __FUNCTION__);
                env->SetObjectArrayElement(vi_array, vi_index, int_resource);
                JNI_EXCEPTION_CHECK(env);
                break;
            }
            // Deliberately fall through if not IS_INTRESOURCE, because if it's
            // not an INTRESOURCE, it's a string.
        case RETURN_JAVA_STRING:
            if (str)
            {
                jni_auto_local<jstring> jstr(env, make_jstring(env, str));
                env->SetObjectArrayElement(vi_array, vi_index, jstr);
                JNI_EXCEPTION_CHECK(env);
            }
            break;
        default:
            assert(!"control should never pass here");
            break;
    }
}

} // namespace jsdi

//==============================================================================
//                                  TESTS
//==============================================================================

#ifndef __NOTEST__

#include "test.h"
#include "test_exports.h"

#include <algorithm>
#include <array>

#include <iostream> // FIXME: delete this include TODO: delete me
#include <iomanip>  // FIXME: delete this include TODO: delete me
template<typename T> // TODO: delete me
std::ostream& dump_bytes_todo_deleteme(std::ostream& o, const T& t) // TODO: delete me
{
    uint8_t const * i(reinterpret_cast<const uint8_t *>(&t)),
                  * e(i + sizeof(T));
    if (i != e)
    {
        o << std::hex << std::setw(2) << std::setfill('0')
          << static_cast<int>(*i);
        for (++i; i != e; ++i)
            o << ' ' << std::setw(2) << static_cast<int>(*i);
    }
    return o;
} // TODO: this func needs to be deleted!!

#if defined(_M_IX86)
#include "abi_x86/stdcall_invoke.h"
#elif defined(_M_AMD64)
#include "abi_amd64/invoke64.h"
#else
#error no invocation program for this platform
#endif // if defined(_M_IX86)

using namespace jsdi;

namespace {

template<typename IntType>
constexpr int word_size(IntType x)
{
    return static_cast<int>(
        (x + sizeof(marshall_word_t)-1) / sizeof(marshall_word_t));
}

template<typename T>
constexpr int word_size()
{ return word_size(sizeof(T)); }

#define ARGS(member) marshall_word_t args[word_size<decltype(member)>()]

template<typename T, typename U>
constexpr int byte_offset(const T& t, const U& u)
{
    return static_cast<int>(reinterpret_cast<const char *>(&u) -
                            reinterpret_cast<const char *>(&t));
}

#pragma warning(push)
#pragma warning(disable: 4592) // MS bug 931288, http://goo.gl/SvVcbg
template<typename T, typename U>
constexpr int word_offset(const T& t, const U& u)
{ return word_size(byte_offset(t, u)); }
#pragma warning(pop)

template<typename FuncPtr>
inline uint64_t invoke_basic(FuncPtr f, size_t size_bytes,
                             const marshall_word_t * args)
{
    auto f_(reinterpret_cast<void *>(f));
#if defined(_M_IX86)
    return abi_x86::stdcall_invoke::basic(size_bytes, args, f);
#elif defined(_M_AMD64)
    return invoke64_basic(size_bytes, args, f);
#else
#error no invocation program for this platform
#endif // if defined(_M_IX86)
}

} // anonymous namespace

TEST(ptrs_init,
    // single indirection
    {
        union u_
        {
            struct x_
            {
                int * ptr;
                int   value;
            } x;
            ARGS(x);
        } u;
        const jint ptr_array[] =
        {
            word_offset(u.x, u.x.ptr), byte_offset(u.x, u.x.value)
        };
        marshalling_roundtrip::ptrs_init(
            u.args, ptr_array, static_cast<jsize>(array_length(ptr_array)));
        *u.x.ptr = 0x19820207;
        assert_equals(u.x.value, 0x19820207);
    }
    // double indirection
    {
        union u_
        {
            struct x_
            {
                int ** ptr_ptr;
                int *  ptr;
                int    value;
            } x;
            ARGS(x);
        } u;
        const jint ptr_array[] =
        {
            word_offset(u.x, u.x.ptr_ptr), byte_offset(u.x, u.x.ptr),
            word_offset(u.x, u.x.ptr),     byte_offset(u.x, u.x.value)
        };
        marshalling_roundtrip::ptrs_init(
            u.args, ptr_array, static_cast<jsize>(array_length(ptr_array)));
        **u.x.ptr_ptr = 0x19900606;
        assert_equals(0x19900606, u.x.value);
    }
    // triple indirection
    {
        union u_
        {
            struct x_
            {
                double *** ptr_ptr_ptr;
                double **  ptr_ptr;
                double *   ptr;
                double     value;
            } x;
            ARGS(x);
        } u;
        const jint ptr_array[] =
        {
            word_offset(u.x, u.x.ptr_ptr_ptr), byte_offset(u.x, u.x.ptr_ptr),
            word_offset(u.x, u.x.ptr_ptr),     byte_offset(u.x, u.x.ptr),
            word_offset(u.x, u.x.ptr),         byte_offset(u.x, u.x.value)
        };
        marshalling_roundtrip::ptrs_init(
            u.args, ptr_array, static_cast<jsize>(array_length(ptr_array)));
        const double expect = -123456789.0 + (1.0 / 32.0);
        ***u.x.ptr_ptr_ptr = expect;
        assert_equals(expect, u.x.value);
        for (int k = 0; k < 10; ++k)
        {
            std::unique_ptr<u_> u2(new u_);
            std::fill(u2->args, u2->args + array_length(u2->args), 0);
            std::vector<jint> ptr_vector(ptr_array,
                                         ptr_array + array_length(ptr_array));
            assert(array_length(ptr_array) == ptr_vector.size());
            marshalling_roundtrip::ptrs_init(
                u2->args, &ptr_vector[0], static_cast<jsize>(ptr_vector.size()));
            const double expect2 = double(k) * double(k) * double(k);
            ***u2->x.ptr_ptr_ptr = expect2;
            union
            {
                double   as_dbl;
                uint64_t as_uint64;
            };
            as_uint64 = TestReturnPtrPtrPtrDoubleAsUInt64(u2->x.ptr_ptr_ptr);
            assert_equals(expect2, as_dbl);
            as_uint64 = static_cast<uint64_t>(
                invoke_basic(TestReturnPtrPtrPtrDoubleAsUInt64,
                             sizeof(double ***), u2->args));
            assert_equals(expect2, as_dbl);
        }
    }
);

// TODO: add tests for ptrs_init_vi that uses a JVM instance (as of right now
//       there are no tests for ptrs_init_vi()!!

namespace {

static const int EMPTY_PTR_ARRAY[1] = { };

static const int ZEROED_VI_INST_ARRAY[10] = { };

constexpr JNIEnv * NULL_JNI_ENV = nullptr;

constexpr jobjectArray NULL_JOBJ_ARR = nullptr;

// TODO: Is this function actually being used by anything!?!?
template<typename T>
inline void vector_append(std::vector<char>& dest, const T& ref)
{
    const char * i = reinterpret_cast<const char *>(&ref);
    const char * e = i + sizeof(T);
    dest.insert(dest.end(), i, e);
}

} // anonymous namespace

TEST(unmarshall_flat_args,
    union
    {
        uint8_t data[8];
        ARGS(data);
    } static const STORAGE = { { 0, 1, 2, 3, 4, 5, 6, 7 } };
    unmarshaller_indirect x(sizeof(STORAGE), sizeof(STORAGE), EMPTY_PTR_ARRAY,
                            EMPTY_PTR_ARRAY);
    static decltype(STORAGE.args) result;
    x.unmarshall_indirect(STORAGE.args, result);
    assert_equals(0, std::memcmp(&STORAGE, result, sizeof(STORAGE)));
    unmarshaller_vi_test y(sizeof(STORAGE), sizeof(STORAGE), EMPTY_PTR_ARRAY,
                           EMPTY_PTR_ARRAY, 0);
    std::memset(result, 0xee, sizeof(result));
    y.unmarshall_vi(STORAGE.args, result, NULL_JNI_ENV, NULL_JOBJ_ARR,
                    ZEROED_VI_INST_ARRAY);
    assert_equals(0, std::memcmp(&STORAGE, result, sizeof(STORAGE)));
);

TEST(unmarshall_flat_args_vi,
   union
   {
       const char * data[3];
       ARGS(data);
   } static const STORAGE = { { nullptr, "Hallo Welt", "xyz" } };
   static int const PTR_ARRAY[] =
   {
       word_offset(STORAGE.args, STORAGE.args[1]), sizeof(STORAGE.args) + 0,
       word_offset(STORAGE.args, STORAGE.args[2]), sizeof(STORAGE.args) + 1
   };
   unmarshaller_vi_test y(sizeof(STORAGE), sizeof(STORAGE), PTR_ARRAY,
                          PTR_ARRAY + array_length(PTR_ARRAY), 2);
   static decltype(STORAGE.args) result;
   y.unmarshall_vi(STORAGE.args, result, NULL_JNI_ENV, NULL_JOBJ_ARR,
                   ZEROED_VI_INST_ARRAY);
   assert_equals(0, std::memcmp(&STORAGE, result, sizeof(STORAGE)));
   assert_equals("Hallo Welt", *y.vi_at(0));
   assert_equals("xyz", *y.vi_at(1));
);

TEST(unmarshall_level_one_simple,
    struct s { int a; int b; char c; };
    union indirect
    {
        s value;
        ARGS(value);
    } static const INDIRECT = { { -1, -1, 'c' } };
    union direct
    {
        s * ptr;
        ARGS(ptr);
    } static const DIRECT = { const_cast<s *>(&INDIRECT.value) };
    union combined
    {
        struct
        {
            direct d;
            indirect i;
        } data;
        ARGS(data);
    } static const EXPECTED = { { DIRECT, INDIRECT } };
    constexpr size_t SIZE_TOTAL = sizeof(DIRECT) + sizeof(INDIRECT);
    static_assert(sizeof(combined) == SIZE_TOTAL, "size mismatch");
    static int const PTR_ARRAY[] =
    {
        word_offset(EXPECTED.data, EXPECTED.data.d),
        byte_offset(EXPECTED.data, EXPECTED.data.i)
    };
    static combined result;
    unmarshaller_indirect x(sizeof(DIRECT), SIZE_TOTAL,
                            PTR_ARRAY, PTR_ARRAY + array_length(PTR_ARRAY));
    x.unmarshall_indirect(DIRECT.args, result.args);
    assert_equals(0, std::memcmp(EXPECTED.args, result.args, SIZE_TOTAL));
    unmarshaller_vi y(sizeof(DIRECT), SIZE_TOTAL,
                      PTR_ARRAY, PTR_ARRAY + array_length(PTR_ARRAY), 0);
    std::memset(result.args, 0xaa, sizeof(result));
    y.unmarshall_vi(DIRECT.args, result.args,
                    NULL_JNI_ENV, NULL_JOBJ_ARR, ZEROED_VI_INST_ARRAY);
    assert_equals(0, std::memcmp(EXPECTED.args, result.args, SIZE_TOTAL));
);

TEST(unmarshall_level_one_complex,
    constexpr size_t N = 7;
    static const struct S { double d; char c; int64_t i; }
        s1 = { 2.5, 'C', static_cast<int64_t>(0xfffffffffffffffeLL) },
        s2 = { -2.5, 'D', 50LL },
        szero { 0, 0, 0LL };
    union
    {
        std::array<S const *, N> data;
        ARGS(data);
    } static const DIRECT = { { nullptr, &s1, nullptr, &s1, &s2, nullptr, &s1 } };
    union combined
    {
        struct
        {
            std::array<S const *, N> direct;
            S                        indirect[N];
        } data;
        ARGS(data);
    } static const EXPECTED =
    { /* union */
        { /* struct */
            DIRECT.data /* direct */,
            { szero, s1, szero, s1, s2, szero, s1 } /* indirect */
        }
    };
    constexpr size_t SIZE_TOTAL = sizeof(combined);
    int PTR_ARRAY[2 * N];
    for (size_t k = 0; k < N; ++k)
    {
        PTR_ARRAY[2*k+0] = word_offset(EXPECTED, EXPECTED.data.direct[k]);
        PTR_ARRAY[2*k+1] = byte_offset(EXPECTED, EXPECTED.data.indirect[k]);
    };
    static combined result;
    unmarshaller_indirect x(sizeof(DIRECT), SIZE_TOTAL, PTR_ARRAY,
                            PTR_ARRAY + array_length(PTR_ARRAY));
    x.unmarshall_indirect(DIRECT.args, result.args);
    assert_equals(0, std::memcmp(&EXPECTED, &result, SIZE_TOTAL));
    unmarshaller_vi_test y(sizeof(DIRECT), SIZE_TOTAL, PTR_ARRAY,
                           PTR_ARRAY + array_length(PTR_ARRAY), 0);
    std::memset(&result, 0, sizeof(result));
    y.unmarshall_vi(DIRECT.args, result.args, NULL_JNI_ENV,
                    NULL_JOBJ_ARR, ZEROED_VI_INST_ARRAY);
    assert_equals(0, std::memcmp(&EXPECTED, &result, SIZE_TOTAL));
);

TEST(unmarshall_level_two,
    constexpr size_t N = 2;
    static const struct S { int level; S const * next; } s2 = { 2, nullptr },
        s1 = { 1, &s2 }, szero = { 0, nullptr };
    union
    {
        std::array<S const *, N> data;
        ARGS(data);
    } static const DIRECT = { { nullptr, &s1 } };
    union combined
    {
        struct
        {
            std::array<S const *, N> direct;
            S                        indirect[2 * N];
        } data;
        ARGS(data);
    } static const EXPECTED =
    { /* union */
        { /* struct */
            DIRECT.data /* direct */,
            { szero, szero, s1, s2 } /* indirect */
        }
    };
    constexpr size_t SIZE_TOTAL = sizeof(combined);
    static int const PTR_ARRAY[4 * N] =
    {
        word_offset(EXPECTED, EXPECTED.data.direct[0]),
            byte_offset(EXPECTED, EXPECTED.data.indirect[0]),
        word_offset(EXPECTED, EXPECTED.data.indirect[0].next),
            byte_offset(EXPECTED, EXPECTED.data.indirect[1]),
        word_offset(EXPECTED, EXPECTED.data.direct[1]),
            byte_offset(EXPECTED, EXPECTED.data.indirect[2]),
        word_offset(EXPECTED, EXPECTED.data.indirect[2].next),
            byte_offset(EXPECTED, EXPECTED.data.indirect[3])
    };
    static combined result;
    unmarshaller_indirect x(sizeof(DIRECT), SIZE_TOTAL, PTR_ARRAY,
                           PTR_ARRAY + array_length(PTR_ARRAY));
    x.unmarshall_indirect(DIRECT.args, result.args);
    assert_equals(0, std::memcmp(&EXPECTED, &result, SIZE_TOTAL));
// TODO: delete the commented lines below, and clean up the dependencies above
//std::cout << "DIRECT:" << std::endl << '\t';
//dump_bytes_todo_deleteme(std::cout, DIRECT) << std::endl;
//std::cout << "EXPECTED:" << std::endl << '\t';
//dump_bytes_todo_deleteme(std::cout, EXPECTED) << std::endl;
//std::cout << "result:" << std::endl << '\t';
//dump_bytes_todo_deleteme(std::cout, result) << std::endl;
    unmarshaller_vi_test y(sizeof(DIRECT), SIZE_TOTAL, PTR_ARRAY,
                           PTR_ARRAY + array_length(PTR_ARRAY), 0);
    std::memset(&result, 0, SIZE_TOTAL);
    y.unmarshall_vi(DIRECT.args, result.args,
                    NULL_JNI_ENV, NULL_JOBJ_ARR, ZEROED_VI_INST_ARRAY);
    assert_equals(0, std::memcmp(&EXPECTED, &result, SIZE_TOTAL));
);

TEST(unmarshall_level_three_with_vi,
    enum { NARGS = 3, NVALS = 4, VI_COUNT = 4 };
    static const struct S { int level; const S * next; const char * str; }
        s3 = { 3, nullptr, "level3" }, s2 = { 2, &s3, "level2" },
        s1 = { 1, &s2, "level1" }, szero = { 0, nullptr, nullptr };
    union
    {
        std::array<S const *, NARGS> data;
        ARGS(data);
    } static const DIRECT = { { &szero, &s1, nullptr } };
    union combined
    {
        struct
        {
            std::array<S const *, NARGS> direct;
            S                            indirect[NVALS];
        } data;
        ARGS(data);
    } static const EXPECTED =
    { /* union */
        { /* struct */
            DIRECT.data /* direct */,
            { szero, s1, s2, s3 } /* indirect */
        }
    };
    constexpr size_t SIZE_TOTAL = sizeof(combined);
    int const PTR_ARRAY[] =
    {
        // normal ptr : DIRECT[0] -> indirect[0] <=> DIRECT[0] -> szero
        word_offset(EXPECTED, EXPECTED.data.direct[0]),
            byte_offset(EXPECTED, EXPECTED.data.indirect[0]),

        // omitted: normal ptr : indirect[0].next <=> szero.next (not needed
        // because it is NULL anyway...)

        // vi ptr : indirect[0].str <=> szero.str
        word_offset(EXPECTED, EXPECTED.data.indirect[0].str), SIZE_TOTAL + 0,

        // normal ptr: DIRECT[1] -> indirect[1] <=> DIRECT[1] -> s1
        word_offset(EXPECTED, EXPECTED.data.direct[1]),
            byte_offset(EXPECTED, EXPECTED.data.indirect[1]),

        // normal ptr: indirect[1].next -> indirect[2] <=> s1.next -> s2
        word_offset(EXPECTED, EXPECTED.data.indirect[1].next),
            byte_offset(EXPECTED, EXPECTED.data.indirect[2]),

        // normal ptr: indirect[2].next -> indirect[3] <=> s2.next -> s3
        word_offset(EXPECTED, EXPECTED.data.indirect[2].next),
            byte_offset(EXPECTED, EXPECTED.data.indirect[3]),

        // vi ptr: indirect[3].str <=> s3.str
        word_offset(EXPECTED, EXPECTED.data.indirect[3].str), SIZE_TOTAL + 1,

        // vi ptr: indirect[2].str <=> s2.str
        word_offset(EXPECTED, EXPECTED.data.indirect[2].str), SIZE_TOTAL + 2,

        // vi ptr: indirect[2].str <=> s2.str
        word_offset(EXPECTED, EXPECTED.data.indirect[1].str), SIZE_TOTAL + 3,
    };
    unmarshaller_vi_test y(sizeof(DIRECT), SIZE_TOTAL, PTR_ARRAY,
                           PTR_ARRAY + array_length(PTR_ARRAY), VI_COUNT);
    static int VI_INST_ARRAY[VI_COUNT] = { };
    static combined result;
    y.unmarshall_vi(DIRECT.args, result.args, NULL_JNI_ENV,
                    NULL_JOBJ_ARR, VI_INST_ARRAY);
    assert_equals(0, std::memcmp(&EXPECTED, &result, SIZE_TOTAL));
    assert_false(y.vi_at(0));
    assert_equals("level3", *y.vi_at(1));
    assert_equals("level2", *y.vi_at(2));
    assert_equals("level1", *y.vi_at(3));
);

#endif // __NOTEST__

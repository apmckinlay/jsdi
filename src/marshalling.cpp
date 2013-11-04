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

inline void marshalling_vi_container::put_not_null(size_t pos, jbyteArray array,
                                                   jbyte ** pp_array)
{ // NORMAL USE (arguments)
    tuple& t = d_arrays[pos];
    assert(! t.d_elems || !"duplicate variable indirect pointer");
    t.d_elems = d_env->GetByteArrayElements(array, &t.d_is_copy);
    JNI_EXCEPTION_CHECK(d_env);
    if (! t.d_elems) throw jni_bad_alloc("GetByteArrayElements", __FUNCTION__);
    assert(! t.d_global);
    // Save a global reference to the array. This allows us to call
    // ReleaseByteArrayElements from the destructor (A) regardless of whether
    // this array is subsequently replaced by a different value; and (B) without
    // having to call GetObjectArrayElement(), which isn't permitted if a JNI
    // exception has been flagged.
    t.d_global = static_cast<jbyteArray>(d_env->NewGlobalRef(array));
    JNI_EXCEPTION_CHECK(d_env);
    t.d_pp_arr = pp_array;
    *pp_array = t.d_elems;

}

//==============================================================================
//                       struct marshalling_roundtrip
//==============================================================================

void marshalling_roundtrip::ptrs_init_vi(jbyte * args, jsize args_size,
                                         const jint * ptr_array,
                                         jsize ptr_array_size, JNIEnv * env,
                                         jobjectArray vi_array_in,
                                         marshalling_vi_container& vi_array_out)
{
    assert(0 == ptr_array_size % 2 || !"pointer array must have even size");
    jint const * i(ptr_array), * e(ptr_array + ptr_array_size);
    while (i < e)
    {
        jint ptr_pos = *i++;
        jint ptd_to_pos = *i++;
        if (UNKNOWN_LOCATION == ptd_to_pos) continue; // Leave a null pointer
        assert(0 <= ptd_to_pos || !"pointer position must be a non-negative index");
        jbyte ** ptr_addr = reinterpret_cast<jbyte **>(&args[ptr_pos]);
        if (ptd_to_pos < args_size)
        {
            // Normal pointer: points back into a location within args
            jbyte * ptd_to_addr = &args[ptd_to_pos];
            *ptr_addr = ptd_to_addr;
        }
        else
        {
            // Variable indirect pointer: points to the start of a byte[] which
            // is passed in from Java in vi_array_in, and marshalled into the
            // C++ environment in vi_array_out.
            ptd_to_pos -= args_size; // Convert to an index into vi_array
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
    JNIEnv * env, jobjectArray vi_array_java,
    marshalling_vi_container& vi_array_cpp,
    const jni_array_region<jint>& vi_inst_array)
{
    const jsize N(vi_array_cpp.d_arrays.size());
    assert(
        N == vi_inst_array.size() || !"variable indirect array size mismatch");
    for (jsize k = 0; k < N; ++k)
    {
        const marshalling_vi_container::tuple& tuple(vi_array_cpp.d_arrays[k]);
        switch (ordinal_enum_to_cpp<
            suneido_language_jsdi_VariableIndirectInstruction>(vi_inst_array[k])
        )
        {
            case NO_ACTION:
                break;
            case RETURN_JAVA_STRING:
                if (! *tuple.d_pp_arr)
                {   // null pointer, so return a null String ref
                    vi_array_cpp.replace_byte_array(k, 0);
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

void unmarshaller_indirect::normal_ptr(char * data, int ptr_pos, int ptd_to_pos,
                                       ptr_iterator_type& ptr_i) const
{
    // STAGE 1: Copy
    //     Look at the next pointer following this one.
    //     - If it is a standard pointer, its ptd_to_pos sets the boundary of
    //       what needs to be copied.
    //     - If there is no next pointer, boundary of what needs to be copied is
    //       the end of the data.
    //
    //     NOTE: If this normal pointer is NULL, we don't copy, we zero out.
    int copy_end(d_size_total);
    if (ptr_i != d_ptr_end) copy_end = *(ptr_i + 1);
    char ** ptr_addr(to_char_ptr_ptr(data, ptr_pos));
    char ** ptd_to_addr(to_char_ptr_ptr(data, ptd_to_pos));
    if (*ptr_addr)
        std::memcpy(ptd_to_addr, *ptr_addr, copy_end - ptd_to_pos);
    else
        std::memset(ptd_to_addr, 0, copy_end - ptd_to_pos);
    // STAGE 2: Handle sub-pointers
    //     For each subsequent pointer that is within the block just copied or
    //     zeroed, call this function recursively.
    while (ptr_i != d_ptr_end)
    {
        int next_ptr_pos = *ptr_i;
        if (! (ptd_to_pos <= next_ptr_pos && next_ptr_pos < copy_end)) break;
        ++ptr_i;
        int next_ptd_to_pos = *ptr_i++;
        normal_ptr(data, next_ptr_pos, next_ptd_to_pos, ptr_i);
    }
}

//==============================================================================
//                        class unmarshaller_vi_base
//==============================================================================

unmarshaller_vi_base::~unmarshaller_vi_base()
{ } // anchor for virtual destructor

void unmarshaller_vi_base::vi_ptr(char * data, int ptr_pos, int ptd_to_pos,
                                  JNIEnv * env, jobjectArray vi_array,
                                  const int * vi_inst_array)
{
    int vi_index = ptd_to_pos - unmarshaller_base::d_size_total;
    assert(0 <= vi_index && vi_index < d_vi_count);
    char ** pstr(unmarshaller_base::to_char_ptr_ptr(data, ptr_pos));
    if (*pstr)
        vi_string_ptr(*pstr, vi_index, env, vi_array, vi_inst_array[vi_index]);
}

void unmarshaller_vi_base::normal_ptr(char * data, int ptr_pos, int ptd_to_pos,
                                      ptr_iterator_type& ptr_i, JNIEnv * env,
                                      jobjectArray vi_array,
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
    int copy_end(unmarshaller_base::d_size_total);
    ptr_iterator_type ptr_e(unmarshaller_indirect::d_ptr_end);
    if (ptr_i != ptr_e)
    {
        ptr_iterator_type ptr_j(ptr_i);
        do
        {
            ++ptr_j;
            int next_ptd_to_pos = *ptr_j++;
            if (! is_vi_ptr(next_ptd_to_pos))
            {
                copy_end = next_ptd_to_pos;
                break;
            }
        }
        while (ptr_j != ptr_e);
    }
    char ** ptr_addr(unmarshaller_base::to_char_ptr_ptr(data, ptr_pos));
    char ** ptd_to_addr(unmarshaller_base::to_char_ptr_ptr(data, ptd_to_pos));
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
    while (ptr_i != ptr_e)
    {
        int next_ptr_pos = *ptr_i;
        if (! (ptd_to_pos <= next_ptr_pos && next_ptr_pos < copy_end)) break;
        ++ptr_i;
        int next_ptd_to_pos = *ptr_i++;
        if (is_vi_ptr(next_ptd_to_pos))
            vi_ptr(data, next_ptr_pos, next_ptd_to_pos, env, vi_array,
                   vi_inst_array);
        else
            normal_ptr(data, next_ptr_pos, next_ptd_to_pos, ptr_i, env,
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
        suneido_language_jsdi_VariableIndirectInstruction>(vi_inst)
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

#include "stdcall_invoke.h"
#include "test.h"
#include "test_exports.h"

#include <algorithm>

using namespace jsdi;

namespace {

template<typename FuncPtr>
inline jlong invoke_stdcall_(FuncPtr f, int nlongs, long * args)
{
    return stdcall_invoke::basic(nlongs * sizeof(long),
                                 reinterpret_cast<char *>(args),
                                 reinterpret_cast<void *>(f));
}

template<typename FuncPtr>
inline jlong invoke_stdcall_(FuncPtr f, int nbytes, jbyte * args)
{
    return stdcall_invoke::basic(nbytes,
                                 reinterpret_cast<char *>(args),
                                 reinterpret_cast<void *>(f));
}

} // anonymous namespace

TEST(ptrs_init,
    // single indirection
    {
        union u_
        {
            struct
            {
                long * ptr;
                long   value;
            } x;
            jbyte args[sizeof(x)];
        } u;
        jint ptr_array[] = { 0, sizeof(long *) };
        marshalling_roundtrip::ptrs_init(u.args, ptr_array,
                                         array_length(ptr_array));
        *u.x.ptr = 0x19820207;
        assert_equals(u.x.value, 0x19820207);
    }
    // double indirection
    {
        union u_
        {
            struct
            {
                long ** ptr_ptr;
                long *  ptr;
                long    value;
            } x;
            jbyte args[sizeof(x)];
        } u;
        jint ptr_array[] = { 0, sizeof(long **), sizeof(long **), sizeof(long **) + sizeof(long *) };
        marshalling_roundtrip::ptrs_init(u.args, ptr_array,
                                         array_length(ptr_array));
        **u.x.ptr_ptr = 0x19900606;
        assert_equals(0x19900606, u.x.value);
    }
    // triple indirection
    {
        union u_
        {
            struct
            {
                double *** ptr_ptr_ptr;
                double **  ptr_ptr;
                double *   ptr;
                double     value;
            } x;
            jbyte args[sizeof(x)];
        } u;
        jint ptr_array[] =
        {
            0,
                reinterpret_cast<char *>(&u.x.ptr_ptr) - reinterpret_cast<char *>(&u.args[0]),
            reinterpret_cast<char *>(&u.x.ptr_ptr) - reinterpret_cast<char *>(&u.args[0]),
                reinterpret_cast<char *>(&u.x.ptr) - reinterpret_cast<char *>(&u.args[0]),
            reinterpret_cast<char *>(&u.x.ptr) - reinterpret_cast<char *>(&u.args[0]),
                reinterpret_cast<char *>(&u.x.value) - reinterpret_cast<char *>(&u.args[0]),
        };
        marshalling_roundtrip::ptrs_init(u.args, ptr_array,
                                         array_length(ptr_array));
        const double expect = -123456789.0 + (1.0 / 32.0);
        ***u.x.ptr_ptr_ptr = expect;
        assert_equals(expect, u.x.value);
        for (int k = 0; k < 10; ++k)
        {
            std::shared_ptr<u_> u2(new u_);
            std::fill(u2->args, u2->args + array_length(u2->args), 0);
            std::vector<jint> ptr_vector(ptr_array,
                                         ptr_array + array_length(ptr_array));
            assert(array_length(ptr_array) == ptr_vector.size());
            marshalling_roundtrip::ptrs_init(u2->args, &ptr_vector[0],
                                             ptr_vector.size());
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
                invoke_stdcall_(TestReturnPtrPtrPtrDoubleAsUInt64,
                                sizeof (double ***), u2->args)
            );
            assert_equals(expect2, as_dbl);
        }
    }
);

namespace {

static const int EMPTY_PTR_ARRAY[0] = { };

static const int ZEROED_VI_INST_ARRAY[10] = { };

constexpr JNIEnv * NULL_JNI_ENV = nullptr;

constexpr jobjectArray NULL_JOBJ_ARR = nullptr;

template<typename T>
inline void vector_append(std::vector<char>& dest, const T& ref)
{
    const char * i = reinterpret_cast<const char *>(&ref);
    const char * e = i + sizeof(T);
    dest.insert(dest.end(), i, e);
}

} // anonymous namespace

TEST(unmarshall_flat_args,
    static const char ARGS[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    unmarshaller_indirect x(sizeof(ARGS), sizeof(ARGS), EMPTY_PTR_ARRAY,
                            EMPTY_PTR_ARRAY);
    static char result[sizeof(ARGS)];
    x.unmarshall_indirect(result, ARGS);
    assert_equals(0, std::memcmp(ARGS, result, sizeof(ARGS)));
    unmarshaller_vi_test y(sizeof(ARGS), sizeof(ARGS), EMPTY_PTR_ARRAY,
                           EMPTY_PTR_ARRAY, 0);
    std::memset(result, 0xee, sizeof(result));
    y.unmarshall_vi(result, ARGS, NULL_JNI_ENV, NULL_JOBJ_ARR,
                    ZEROED_VI_INST_ARRAY);
    assert_equals(0, std::memcmp(ARGS, result, sizeof(ARGS)));
);

TEST(unmarshall_flat_args_vi,
    static const char * ARGS[] = { 0, "Hallo Welt", "xyz" };
    static const int PTR_ARRAY[] =
    { sizeof(char *), sizeof(ARGS), 2 * sizeof(char *), sizeof(ARGS) + 1 };
    unmarshaller_vi_test y(sizeof(ARGS), sizeof(ARGS), PTR_ARRAY,
                           PTR_ARRAY + array_length(PTR_ARRAY), 2);
    static char result[sizeof(ARGS)];
    y.unmarshall_vi(result, reinterpret_cast<const char *>(ARGS), NULL_JNI_ENV,
                    NULL_JOBJ_ARR, ZEROED_VI_INST_ARRAY);
    assert_equals(0, std::memcmp(ARGS, result, sizeof(ARGS)));
    assert_equals("Hallo Welt", *y.vi_at(0));
    assert_equals("xyz", *y.vi_at(1));
);

TEST(unmarshall_level_one_simple,
    static const struct { int a; int b; char c; } s = { -1, 1, 'c' };
    static const void * ARGS[] = { &s };
    static const int PTR_ARRAY[] = { 0, sizeof(ARGS) };
    enum { SIZE_TOTAL = sizeof(ARGS) + sizeof(s) };
    unmarshaller_indirect x(sizeof(ARGS), SIZE_TOTAL, PTR_ARRAY,
                            PTR_ARRAY + array_length(PTR_ARRAY));
    static char result[SIZE_TOTAL];
    x.unmarshall_indirect(result, reinterpret_cast<const char *>(ARGS));
    std::vector<char> expected;
    vector_append(expected, ARGS);
    vector_append(expected, s);
    assert_equals(0, std::memcmp(expected.data(), result, expected.size()));
    unmarshaller_vi y(sizeof(ARGS), SIZE_TOTAL, PTR_ARRAY,
                      PTR_ARRAY + array_length(PTR_ARRAY), 0);
    std::memset(result, 0xaa, sizeof(result));
    y.unmarshall_vi(result, reinterpret_cast<const char *>(ARGS),
                    NULL_JNI_ENV, NULL_JOBJ_ARR, ZEROED_VI_INST_ARRAY);
    assert_equals(0, std::memcmp(expected.data(), result, expected.size()));
);

TEST(unmarshall_level_one_complex,
     static const struct S { double d; char c; int64_t i; }
         s1 = { 2.5, 'C', static_cast<int64_t>(0xfffffffffffffffeLL) },
         s2 = { -2.5, 'D', 50LL },
         szero { 0, 0, 0LL };
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
     enum { SIZE_TOTAL = sizeof(ARGS) + 7 * sizeof(S) };
     unmarshaller_indirect x(sizeof(ARGS), SIZE_TOTAL, PTR_ARRAY,
                             PTR_ARRAY + array_length(PTR_ARRAY));
     char result[SIZE_TOTAL];
     x.unmarshall_indirect(result, reinterpret_cast<const char *>(ARGS));
     std::vector<char> expected;
     vector_append(expected, ARGS);
     vector_append(expected, szero);
     vector_append(expected, s1);
     vector_append(expected, szero);
     vector_append(expected, s1);
     vector_append(expected, s2);
     vector_append(expected, szero);
     vector_append(expected, s1);
     assert_equals(SIZE_TOTAL, expected.size());
     assert_equals(0, std::memcmp(expected.data(), result, expected.size()));
     unmarshaller_vi_test y(sizeof(ARGS), SIZE_TOTAL, PTR_ARRAY,
                            PTR_ARRAY + array_length(PTR_ARRAY), 0);
     std::memset(result, 0xbb, sizeof(result));
     y.unmarshall_vi(result, reinterpret_cast<const char *>(ARGS), NULL_JNI_ENV,
                     NULL_JOBJ_ARR, ZEROED_VI_INST_ARRAY);
     assert_equals(0, std::memcmp(expected.data(), result, expected.size()));
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
    enum { SIZE_TOTAL = sizeof(ARGS) + 4 * sizeof(S) };
    unmarshaller_indirect x(sizeof(ARGS), SIZE_TOTAL, PTR_ARRAY,
                            PTR_ARRAY + array_length(PTR_ARRAY));
    static char result[SIZE_TOTAL];
    x.unmarshall_indirect(result, reinterpret_cast<const char *>(ARGS));
    std::vector<char> expected;
    vector_append(expected, ARGS);
    vector_append(expected, szero);
    vector_append(expected, szero);
    vector_append(expected, s1);
    vector_append(expected, s2);
    assert_equals(sizeof(result), expected.size());
    assert_equals(0, std::memcmp(expected.data(), result, expected.size()));
    unmarshaller_vi_test y(sizeof(ARGS), SIZE_TOTAL, PTR_ARRAY,
                           PTR_ARRAY + array_length(PTR_ARRAY), 0);
    std::memset(result, 0xcc, sizeof(result));
    y.unmarshall_vi(result, reinterpret_cast<const char *>(ARGS),
                    NULL_JNI_ENV, NULL_JOBJ_ARR, ZEROED_VI_INST_ARRAY);
    assert_equals(0, std::memcmp(expected.data(), result, expected.size()));
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
        STR_OFFSET = NEXT_OFFSET + sizeof(const S *),
        VI_COUNT = 4
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
    unmarshaller_vi_test y(SIZE_DIRECT, SIZE_TOTAL, PTR_ARRAY,
                           PTR_ARRAY + array_length(PTR_ARRAY), VI_COUNT);
    static int VI_INST_ARRAY[VI_COUNT] = { };
    char result[SIZE_TOTAL];
    y.unmarshall_vi(result, reinterpret_cast<const char *>(ARGS), NULL_JNI_ENV,
                    NULL_JOBJ_ARR, VI_INST_ARRAY);
    std::vector<char> expected;
    vector_append(expected, ARGS);
    vector_append(expected, szero);
    vector_append(expected, s1);
    vector_append(expected, s2);
    vector_append(expected, s3);
    assert_equals(sizeof(result), expected.size());
    assert_equals(0, std::memcmp(expected.data(), result, expected.size()));
    assert_true(! y.vi_at(0));
    assert_equals("level3", *y.vi_at(1));
    assert_equals("level2", *y.vi_at(2));
    assert_equals("level1", *y.vi_at(3));
);

#endif // __NOTEST__

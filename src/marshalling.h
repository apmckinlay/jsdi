/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_MARSHALLING_H___
#define __INCLUDED_MARSHALLING_H___

/**
 * \file marshalling.h
 * \author Victor Schappert
 * \since 20130812
 * \brief Functions for marshalling data structures between the format sent by
 *        jSuneido and the format expected by C
 */

#include "global_refs.h"
#include "java_enum.h"
#include "jni_util.h"
#include "jsdi_windows.h"
#include "util.h"

#include <vector>
#include <cassert>
#include <cstring>

#ifndef __NOTEST__
#include <memory>   // for unmarshaller_vi_test
#endif

namespace jsdi {
namespace abi_x86 {

//==============================================================================
//                      class marshalling_vi_container
//==============================================================================

struct marshalling_roundtrip;

// TODO: docs since 20130812 -- formerly jbyte_array_container in jsdi_jni.h/cpp
class marshalling_vi_container : private non_copyable
{
        //
        // FRIENDSHIPS
        //

        friend struct marshalling_roundtrip;

        //
        // TYPES
        //

    public:

        struct tuple
        {
                jbyte *    d_elems;     // ptr to the byte array elements, OWNED
                jbyte **   d_pp_arr;    // points to the addr in the marshalled
                                        // data array which contains the pointer
                                        // to the byte array
                jbyteArray d_global;    // non-NULL iff d_elems not NULL
                jboolean   d_is_copy;
        };

        //
        // INTERNAL TYPES
        //

    private:

        typedef std::vector<tuple> vector_type;

        //
        // DATA
        //

    private:

        vector_type  d_arrays;
        JNIEnv     * d_env;
        jobjectArray d_object_array;

        //
        // CONSTRUCTORS
        //

    public:

        marshalling_vi_container(size_t size, JNIEnv * env,
                                 jobjectArray object_array);

        ~marshalling_vi_container();

        //
        // ACCESSORS
        //

    public:

        size_t size() const;

        //
        // MUTATORS
        //

    public:

        void put_not_null(size_t pos, jbyteArray array, jbyte ** pp_array);

        void put_return_value(size_t pos, jbyte * str);

        void put_null(size_t pos, jbyte ** pp_array);

        void replace_byte_array(size_t pos, jobject new_object);
};

constexpr marshalling_vi_container::tuple NULL_TUPLE = { nullptr, nullptr,
                                                         nullptr, JNI_FALSE };
    // On gcc 4.6.2 (MinGW), you can't declare this inside the definition of
    // marshalling_vi_container or the compiler complains.

inline marshalling_vi_container::marshalling_vi_container(
    size_t size, JNIEnv * env, jobjectArray object_array)
    : d_arrays(size, NULL_TUPLE)
    , d_env(env)
    , d_object_array(object_array)
{ assert(env && object_array); }

inline size_t marshalling_vi_container::size() const
{ return d_arrays.size(); }

inline void marshalling_vi_container::put_return_value(size_t pos, jbyte * str)
{ // FOR USE BY RETURN VALUE
    tuple& t = d_arrays[pos];
    *t.d_pp_arr = str;
}

inline void marshalling_vi_container::put_null(size_t pos, jbyte ** pp_array)
{
    tuple& t = d_arrays[pos];
    assert(! t.d_elems || !"duplicate variable indirect pointer");
    t.d_pp_arr = pp_array;
}

inline void marshalling_vi_container::replace_byte_array(
    size_t pos, jobject new_object /* may be null */)
{
#ifndef NDEBUG
    tuple& t = d_arrays[pos];
    if (t.d_elems)
    {
        assert(t.d_global || !"no global reference allocated");
        jni_auto_local<jobject> prev_object(
            d_env, d_env->GetObjectArrayElement(d_object_array, pos));
        assert(d_env->IsInstanceOf(prev_object, GLOBAL_REFS->byte_ARRAY()));
    }
#endif
    d_env->SetObjectArrayElement(d_object_array, pos, new_object);
    JNI_EXCEPTION_CHECK(d_env);
}

//==============================================================================
//                       struct marshalling_roundtrip
//==============================================================================

/**
 * \brief Contains functions for marshalling data from the format sent by
 * jSuneido to the format expected by C and then back to the format expected by
 * jSuneido.
 * \author Victor Schappert
 * \since 20130812
 * \see unmarshaller_indirect
 * \see unmarshaller_vi
 */
struct marshalling_roundtrip
{
        static constexpr jint UNKNOWN_LOCATION = -1;

        static void ptrs_init(jbyte * args, const jint * ptr_array,
                              jsize ptr_array_size);

        static void ptrs_init_vi(jbyte * args, jsize args_size,
                                 const jint * ptr_array, jsize ptr_array_size,
                                 JNIEnv * env, jobjectArray vi_array_in,
                                 marshalling_vi_container& vi_array_out);

        static void ptrs_finish_vi(JNIEnv * env, jobjectArray vi_array_java,
                                   marshalling_vi_container& vi_array_cpp,
                                   const jni_array_region<jint>& vi_inst_array);
};

inline void marshalling_roundtrip::ptrs_init(jbyte * args,
                                             const jint * ptr_array,
                                             jsize ptr_array_size)
{
    assert(0 == ptr_array_size % 2 || !"pointer array must have even size");
    jint const * i(ptr_array), * e(ptr_array + ptr_array_size);
    while (i < e)
    {
        jint ptr_pos = *i++;
        jint ptd_to_pos = *i++;
        // If the Java-side marshaller put UNKNOWN_LOCATION as the location to
        // point to, just skip this pointer -- we will trust that the Java side
        // put a NULL value into the data array. Otherwise, set the pointer in
        // the data array to point to the appropriate location.
        if (UNKNOWN_LOCATION != ptd_to_pos)
        {
            jbyte ** ptr_addr = reinterpret_cast<jbyte **>(&args[ptr_pos]);
            jbyte * ptd_to_addr = &args[ptd_to_pos];
            // Possible alignment issue if the Java-side marshaller didn't set
            // things up so that the pointers are word-aligned. This is the
            // Java side's job, however, and we trust it was done properly.
            *ptr_addr = ptd_to_addr;
        }
    }
}

//==============================================================================
//                          class unmarshaller_base
//==============================================================================

class unmarshaller_base : private non_copyable
{
        //
        // DATA
        //

    protected:

        int         d_size_direct;
        int         d_size_total;

        //
        // INTERNALS
        //

    protected:

        static char ** to_char_ptr_ptr(char * data, int ptr_pos);

        //
        // CONSTRUCTORS
        //

    protected:

        unmarshaller_base(int size_direct, int size_total);
};

inline char ** unmarshaller_base::to_char_ptr_ptr(char * data, int ptr_pos)
{ return reinterpret_cast<char **>(&data[ptr_pos]); }

inline unmarshaller_base::unmarshaller_base(int size_direct, int size_total)
    : d_size_direct(size_direct)
    , d_size_total(size_total)
{ assert(0 <= size_direct && size_direct <= size_total); }

//==============================================================================
//                        class unmarshaller_indirect
//==============================================================================

/**
 * Generic class for marshalling indirect data out of a C structure and into the
 * format expected by jSuneido.
 * \author Victor Schappert
 * \since 20130812
 * \see marshalling_roundtrip
 * \see unmarshaller_vi_base
 * \see unmarshaller_vi
 */
class unmarshaller_indirect : public unmarshaller_base
{
        //
        // TYPES
        //

    public:

        typedef const int * ptr_iterator_type;

        //
        // DATA
        //

    protected:

        ptr_iterator_type d_ptr_begin;
        ptr_iterator_type d_ptr_end;

        //
        // INTERNALS
        //

    private:

        void normal_ptr(char * data, int ptr_pos, int ptd_to_pos,
                        ptr_iterator_type& ptr_i) const;

        //
        // CONSTRUCTORS
        //

    public:

        unmarshaller_indirect(int size_direct, int size_total,
                              const ptr_iterator_type& ptr_begin,
                              const ptr_iterator_type& ptr_end);

        //
        // ACCESSORS
        //

    public:

        void unmarshall_indirect(char * to, const char * from) const;
};

inline unmarshaller_indirect::unmarshaller_indirect(
    int size_direct,
    int size_total,
    const ptr_iterator_type& ptr_begin,
    const ptr_iterator_type& ptr_end)
    : unmarshaller_base(size_direct, size_total)
    , d_ptr_begin(ptr_begin)
    , d_ptr_end(ptr_end)
{ }

inline void unmarshaller_indirect::unmarshall_indirect(
    char * to, const char * from) const
{
    std::memcpy(to, from, d_size_direct);
    ptr_iterator_type ptr_i(d_ptr_begin), ptr_e(d_ptr_end);
    while (ptr_i != ptr_e)
    {
        int ptr_pos = *ptr_i++;
        int ptd_to_pos = *ptr_i++;
        normal_ptr(to, ptr_pos, ptd_to_pos, ptr_i);
    }
}

//==============================================================================
//                        class unmarshaller_vi_base
//==============================================================================

/**
 * \brief Generic class for marshalling variable indirect data out of a C
 * structure and into the format expected by jSuneido.
 * \author Victor Schappert
 * \since 20130812
 * \see unmarshaller_vi
 * \see unmarshaller_indirect
 * \see marshalling_roundtrip
 *
 * Variable indirect unmarshalling is not implemented in order to make this
 * class more testable.
 */
class unmarshaller_vi_base : public unmarshaller_indirect
{
        //
        // DATA
        //

        int          d_vi_count;

        //
        // INTERNALS
        //

    private:

        bool is_vi_ptr(int ptd_to_pos) const;

        void vi_ptr(char * data, int ptr_pos, int ptd_to_pos, JNIEnv * env,
                    jobjectArray vi_array, const int * vi_inst_array);

        void normal_ptr(char * data, int ptr_pos, int ptd_to_pos,
                        ptr_iterator_type& ptr_i, JNIEnv * env,
                        jobjectArray vi_array, const int * vi_inst_array);

    protected:

        virtual void vi_string_ptr(const char * str, int vi_index, JNIEnv * env,
                                   jobjectArray vi_array, int vi_inst) = 0;

        //
        // CONSTRUCTORS
        //

    public:

        unmarshaller_vi_base(int size_direct, int size_total,
                             const ptr_iterator_type& ptr_begin,
                             const ptr_iterator_type& ptr_end, int vi_count);

        virtual ~unmarshaller_vi_base();
            // NOTE: 'virtual ~unmarshaller_vi() = default;' would be preferable
            //       but MinGW/gcc 4.5.3 doesn't seem to support it.

        //
        // ACCESSORS
        //

    public:

        void unmarshall_vi(char * to, const char * from, JNIEnv * env,
                           jobjectArray vi_array,
                           const int * vi_inst_array);
};

inline bool unmarshaller_vi_base::is_vi_ptr(int ptd_to_pos) const
{ return unmarshaller_base::d_size_total <= ptd_to_pos; }

inline unmarshaller_vi_base::unmarshaller_vi_base(
    int size_direct, int size_total, const ptr_iterator_type& ptr_begin,
    const ptr_iterator_type& ptr_end, int vi_count)
    : unmarshaller_indirect(size_direct, size_total, ptr_begin, ptr_end)
    , d_vi_count(vi_count)
{ assert(0 <= vi_count); }

inline void unmarshaller_vi_base::unmarshall_vi(char * to, const char * from,
                                                JNIEnv * env,
                                                jobjectArray vi_array,
                                                const int * vi_inst_array)
{
    std::memcpy(to, from, unmarshaller_base::d_size_direct);
    ptr_iterator_type ptr_i(unmarshaller_indirect::d_ptr_begin),
                      ptr_e(unmarshaller_indirect::d_ptr_end);
    while (ptr_i != ptr_e)
    {
        int ptr_pos = *ptr_i++;
        int ptd_to_pos = *ptr_i++;
        if (is_vi_ptr(ptd_to_pos))          // vi pointer
            vi_ptr(to, ptr_pos, ptd_to_pos, env, vi_array, vi_inst_array);
        else                                // normal pointer
            normal_ptr(to, ptr_pos, ptd_to_pos, ptr_i, env, vi_array,
                       vi_inst_array);
    }
}

//==============================================================================
//                        class unmarshaller_vi_test
//==============================================================================

#ifndef __NOTEST__

/** \cond internal */

class unmarshaller_vi_test : public unmarshaller_vi_base
{
        //
        // TYPES
        //

    public:

        typedef std::shared_ptr<std::string>    vi_string;
        typedef std::vector<vi_string>          vi_vector;

        //
        // DATA
        //

    private:

        vi_vector d_vi_data;

        //
        // INTERNALS
        //

    protected:

        virtual void vi_string_ptr(const char * str, int vi_index, JNIEnv * env,
                                   jobjectArray vi_array, int vi_inst);

        //
        // CONSTRUCTORS
        //

    public:

        unmarshaller_vi_test(int size_direct, int size_total,
                        const ptr_iterator_type& ptr_begin,
                        const ptr_iterator_type& ptr_end, int vi_count);

        //
        // ACCESSORS
        //

    public:

        const vi_string& vi_at(size_t n) const;
};

inline unmarshaller_vi_test::unmarshaller_vi_test(
    int size_direct, int size_total, const ptr_iterator_type& ptr_begin,
    const ptr_iterator_type& ptr_end, int vi_count)
    : unmarshaller_vi_base(size_direct, size_total, ptr_begin, ptr_end,
                           vi_count)
    , d_vi_data(vi_count)
{ }

inline const unmarshaller_vi_test::vi_string& unmarshaller_vi_test::vi_at(
    size_t n) const
{
    assert(0 <= n && n < d_vi_data.size());
    return d_vi_data[n];
}

/** \endcond internal */

#endif // __NOTEST__

//==============================================================================
//                          class unmarshaller_vi
//==============================================================================

/**
 * Class for marshalling variable indirect data out of a C structure and into
 * the format expected by jSuneido.
 * \author Victor Schappert
 * \since 20130813
 * \see unmarshaller_vi_base
 * \see unmarshaller_indirect
 * \see marshalling_roundtrip
 *
 * Unlike \link unmarshaller_vi_base\endlink, this class fully implements
 * unmarshalling variable indirect data.
 */
class unmarshaller_vi : public unmarshaller_vi_base
{
        //
        // INTERNALS
        //

    protected:

        virtual void vi_string_ptr(const char * str, int vi_index, JNIEnv * env,
                                   jobjectArray vi_array, int vi_inst);

        //
        // CONSTRUCTORS
        //

    public:

        unmarshaller_vi(int size_direct, int size_total,
                        const ptr_iterator_type& ptr_begin,
                        const ptr_iterator_type& ptr_end, int vi_count);
};

inline unmarshaller_vi::unmarshaller_vi(int size_direct, int size_total,
                                        const ptr_iterator_type& ptr_begin,
                                        const ptr_iterator_type& ptr_end,
                                        int vi_count)
    : unmarshaller_vi_base(size_direct, size_total, ptr_begin, ptr_end,
                           vi_count)
{ }

} // namespace abi_x86
} // namespace jsdi

#endif // __INCLUDED_MARSHALLING_H___

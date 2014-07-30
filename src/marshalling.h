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

//==============================================================================
//                          typedef marshall_word_t
//==============================================================================

/**
 * \brief Type of the data word processed by the marshalling algorithms
 * \author Victor Schappert
 * \since 20140722
 */
typedef jlong marshall_word_t;

//==============================================================================
//                         struct marshalling_util
//==============================================================================

/**
 * \brief Utility types and functions used by marshalling code
 * \author Victor Schappert
 * \since 20140729
 */
struct marshalling_util
{
        /**
         * \brief Type of a pointer, itself located within a marshalled data
         *        block, to a pointer that also lives within the marshalled data
         *        block
         * \see #addr_of_ptr(marshall_word_t *, jint)
         * \see #ptr_to_anywhere_ptr_t
         * \see #ptr_to_datablock_t
         */
        typedef jbyte ** ptr_to_ptr_t;

        /**
         * \brief Type of a pointer, itself located within a marshalled data
         *        block, to a pointer at an arbitrary location in the address
         *        space
         * \see #addr_of_ptr(marshall_word_t const *, jint)
         * \see #ptr_to_ptr_t
         * \see #ptr_to_datablock_t
         */
        typedef jbyte * const * ptr_to_anywhere_ptr_t;

        /**
         * \brief Type of a pointer to an address at some offset within the
         *        marshalled data block
         * \see #addr_of_byte(marshall_word_t *, jint)
         * \see #ptr_to_ptr_t
         * \see #ptr_to_anywhere_ptr_t
         */
        typedef jbyte * ptr_to_datablock_t;

        /**
         * \brief Returns the address of a pointer within a marshalled data
         *        block that points to another location also within the
         *        marshalled data block
         * \param data Pointer to start of data block
         * \param byte_offset Amount, in bytes, the desired pointer is offset
         *        from the start of the data block
         * \return Address of the pointer at position <code>byte_offset</code>
         *         typed as a #ptr_to_ptr_t
         * \see #addr_of_ptr(marshall_word_t const *, jint)
         * \see #addr_of_byte(marshall_word_t *, jint)
         */
        static ptr_to_ptr_t addr_of_ptr(marshall_word_t * data,
                                        jint byte_offset);

        /**
         * \brief Returns the address of a pointer within a marshalled data
         *        block that points to an arbitrary (valid) address in the
         *        process address space
         * \param data Pointer to start of data block
         * \param byte_offset Amount, in bytes, the desired pointer is offset
         *        from the start of the data block
         * \return Address of the pointer at position <code>byte_offset</code>
         *         typed as a #ptr_to_anywhere_ptr_t
         * \see #addr_of_ptr(marshall_word_t *, jint)
         * \see #addr_of_byte(marshall_word_t *, jint)
         *
         * The return value is typed as <code><b>const</b></code>
         * #ptr_to_anywhere_ptr_t for the following reasons:
         * - this function is useful only for data that is being unmarshalled,
         *   where the pointer locations themselves do not need to be touched,
         *   hence the <code>const</code>;
         * - the pointer may point to any arbitrary byte location within the
         *   process address space, therefore it is a #ptr_to_anywhere_ptr_t;
         *   and
         * - such an arbitrary location may be read-only, thus the reason
         *   #ptr_to_anywhere_ptr_t is itself a pointer to a <code>const</code>
         *   byte location.
         */
        static const ptr_to_anywhere_ptr_t addr_of_ptr(
            marshall_word_t const * data, jint byte_offset);

        /**
         * \brief Returns the address of a byte within a marshalled data block
         * \param data Pointer to start of data block
         * \param byte_offset Amount, in bytes, the desired address is offset
         *        from the start of the data block
         * \return Address of the byte <code>byte_offset</code> bytes past
         *         <code>data</code>
         * \see #addr_of_ptr(marshall_word_t *, jint)
         * \see #addr_of_ptr(const marshall_word_t *, jint)
         */
        static ptr_to_datablock_t addr_of_byte(marshall_word_t * data,
                                               jint byte_offset);

        /**
         * \brief Returns number of \link marshall_word_t\endlink occupied by a
         *        given number of bytes that is an exact multiple of the size of
         *        \link marshall_word_t\endlink
         * \param bytes Non-negative exactly multiple of the size of a
         *              \link marshall_word_t\endlink
         * \return Number of words occupied by <code>bytes</code>
         */
        static jsize num_whole_words_exact(jsize bytes);
};

inline marshalling_util::ptr_to_ptr_t
marshalling_util::addr_of_ptr(marshall_word_t * data, jint byte_offset)
{ return reinterpret_cast<ptr_to_ptr_t>(addr_of_byte(data, byte_offset)); }

inline const marshalling_util::ptr_to_anywhere_ptr_t
marshalling_util::addr_of_ptr(marshall_word_t const * data, jint byte_offset)
{ return addr_of_ptr(const_cast<marshall_word_t *>(data), byte_offset); }

inline marshalling_util::ptr_to_datablock_t marshalling_util::addr_of_byte(
    marshall_word_t * data, jint byte_offset)
{ return reinterpret_cast<ptr_to_datablock_t>(data) + byte_offset; }

inline jsize marshalling_util::num_whole_words_exact(jsize bytes)
{
    assert(0 <= bytes);
    assert(0 == bytes % sizeof(marshall_word_t) ||
        !"word size must exactly divide byte size");
    return bytes / sizeof(marshall_word_t);
}

/**
 * \brief Returns the minimum number of contiguous words required to hold a
 *        given number of bytes
 * \param bytes Non-negative number of bytes
 * \tparam WordType An integral primitive type
 * \return Minimum number of contiguous <code>WordType</code> values required
 *         to hold <code>bytes</code>
 * \see size_whole_words(jsize)
 * \todo Remove the <code>\#pragma warning</code> wrappers and make this a
 *       member of \link jsdi::marshalling_util\endlink when Microsoft fixes the
 *       C++ compiler to properly support <code><b>constexpr</b></code>. As of
 *       November 2013 CTP on 20140729, <code><b>constexpr</b></code> member
 *       functions are not supported and, as a separate problem, MSVC emits a
 *       large number of pointless C4592 warnings for the subset of
 *       <code><b>constexpr</b></code> members it does support (see \em eg
 *       http://goo.gl/SvVcbg).
 */
template<typename WordType = marshall_word_t>
inline constexpr jsize min_whole_words(jsize bytes)
{
    static_assert(std::is_integral<WordType>::value, "integer type required");
    return (bytes + sizeof(WordType)-1) / sizeof(WordType);
}

/**
 * \brief Returns the size, in bytes, of the minimum number of contiguous words
 *        required to hold a given number of bytes
 * \param bytes Non-negative number of bytes
 * \tparam WordType An integral primitive type
 * \return Size in bytes of \link min_whole_words<WordType>(jsize)
 *         min_whole_words(bytes)\endlink
 * \todo Remove the <code>\#pragma warning</code> wrappers and make this a
 *       member of \link jsdi::marshalling_util\endlink when Microsoft fixes the
 *       C++ compiler to properly support <code><b>constexpr</b></code>. As of
 *       November 2013 CTP on 20140729, <code><b>constexpr</b></code> member
 *       functions are not supported and, as a separate problem, MSVC emits a
 *       large number of pointless C4592 warnings for the subset of
 *       <code><b>constexpr</b></code> members it does support (see \em eg
 *       http://goo.gl/SvVcbg).
 */
template<typename WordType = marshall_word_t>
inline constexpr jsize size_whole_words(jsize bytes)
#pragma warning(push) // TODO: remove after http://goo.gl/SvVcbg fixed
#pragma warning(disable:4592)
{ return min_whole_words<WordType>(bytes) * sizeof(WordType); }
#pragma warning(pop)

//==============================================================================
//                      class marshalling_vi_container
//==============================================================================

struct marshalling_roundtrip;

/**
 * \brief Opaque data structure to store variable indirect marshall/unmarshall
 *        state needed by \link marshalling_roundtrip\endlink
 * \author Victor Schappert
 * \since 20130812
 * \see marshalling_roundtrip#ptrs_init_vi(marshall_word_t *, jsize,
        const jint *, jsize, JNIEnv *, jobjectArray, marshalling_vi_container&)
 * \see marshalling_roundtrip#ptrs_finish_vi(jobjectArray,
 *      marshalling_vi_container&, const jni_array_region<jint>&)
 */
class marshalling_vi_container : private non_copyable
{
        //
        // FRIENDSHIPS
        //

        friend struct marshalling_roundtrip;

        //
        // INTERNAL TYPES
        //

        struct tuple
        {
                jbyte *    d_elems;     // ptr to the byte array elements, OWNED
                jbyte **   d_pp_arr;    // points to the addr in the marshalled
                                        // data array which contains the pointer
                                        // to the byte array
                jbyteArray d_global;    // non-NULL iff d_elems not NULL
                jboolean   d_is_copy;
        };

        typedef std::vector<tuple> vector_type;

        //
        // DATA
        //

        vector_type  d_arrays;
        JNIEnv     * d_env;
        jobjectArray d_object_array;    // NOT OWNED

        //
        // INTERNALS
        //

        void put_not_null(jint pos, jbyteArray array, jbyte ** pp_array);

        void put_null(jint pos, jbyte ** pp_array);

        void replace_byte_array(jint pos, jobject new_object);

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructs variable indirect roundtrip state
         * \param size Number of variable indirect pointers state must hold
         * \param env JNI environment
         * \param object_array Java <code>Object</code> array having length at
         *        least <code>size</code> where modifications to the state
         *        should be made
         *
         * \note
         * This object does not become the owner of <code>object_array</code>
         * and is not responsible for destroying it.
         */
        marshalling_vi_container(size_t size, JNIEnv * env,
                                 jobjectArray object_array);

        ~marshalling_vi_container();

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Returns the number of contained variable indirect pointers
         * \return Variable indirect pointer count
         */
        size_t size() const;

        //
        // MUTATORS
        //

    public:
    
        /**
         * \brief Stores a string pointer to be returned as part of a function
         *        return value
         * \param str Pointer to string, may be <code>null</code>
         */
        void put_return_value(jbyte * str);
};

inline marshalling_vi_container::marshalling_vi_container(
    size_t size, JNIEnv * env, jobjectArray object_array)
    : d_arrays(size, { nullptr, nullptr, nullptr, JNI_FALSE })
    , d_env(env)
    , d_object_array(object_array)
{ assert(env && object_array); }

inline size_t marshalling_vi_container::size() const
{ return d_arrays.size(); }

inline void marshalling_vi_container::put_null(jint pos, jbyte ** pp_array)
{
    assert(0 <= pos && static_cast<size_t>(pos) < d_arrays.size());
    tuple& t = d_arrays[pos];
    assert(! t.d_elems || !"duplicate variable indirect pointer");
    t.d_pp_arr = pp_array;
}

inline void marshalling_vi_container::replace_byte_array(
    jint pos, jobject new_object /* may be null */)
{
    assert(0 <= pos && static_cast<size_t>(pos) < d_arrays.size());
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

inline void marshalling_vi_container::put_return_value(jbyte * str)
{
    assert(0 < size() || !"can't put return value in empty container");
    tuple& t = d_arrays[size()-1];
    assert(! t.d_elems   || !"return value must go in unused tuple");
    assert(! t.d_global  || !"return value must go in unused tuple");
    assert(! t.d_is_copy || !"return value must go in unused tuple");
    *t.d_pp_arr = str; // t.d_pp_arr was initialized by ptrs_init_vi()
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
 *
 * The functions in this namespace are used when a roundtrip from Java to JNI
 * and back to Java is required. The "roundtrip" is composed of two phases:
 *
 * -# an "init" phase, in which pointers <code>args</code> storage block are
 *    initialized (this phase is, of course, necessary because Java has no
 *    concept of pointers and so we have to travel through all the data words in
 *    <code>args</code> that represent pointers and set their values to the
 *    address they are supposed to point to); and
 * -# a "finish" phase&mdash;only necessary where one or more pointers points to
 *    variable indirect storage&mdash;where variable indirect objects are
 *    unmarshalled into Java <code>Object</code>'s before they are sent back to
 *    the Java side (<em>this variable indirect unmarshalling is in contrast to
 *    ordinary direct and indirect storage values, which are completely
 *    unmarshalled on the Java side</em>).
 *
 * These functions are not used in non-roundtrip situations (<em>ie</em> a call
 * to copy out a <code>struct</code>, or a <code>callback</code> invocation)
 * where the "init" phase is implicitly done by the person who owns the
 * <code>struct</code>, or the callback invoker. In these cases, the appropriate
 * class derived from \link unmarshaller_base\endlink should be used.
 */
struct marshalling_roundtrip : private marshalling_util
{
        /**
         * \brief Constant value indicating a <code>null</code> pointer
         *
         * This value can only ever appear as the second value of a pointer pair
         * in a pointer array.
         *
         * A pointer array is a list of pairs <code>&lt;<i>x</i>,
         * <i>y</i>&gt;</code> where <code><i>x</i></code> is an index into the
         * <code>args</code> array giving the index of the pointer; and
         * <code><i>y</i></code> is either a positive number giving the
         * <strong>byte</strong> offset into <code>args</code> where the pointer
         * <code><i>x</i></code> must point, <em>or</em> the value
         * <code>UNKNOWN_LOCATION</code>, indicating that  <code><i>x</i></code>
         * must be set to a <code>null</code> pointer.
         *
         * \note
         * - For clarity, the value <code><i>x</i></code> in each tuple is a
         *   \link marshall_word_t\endlink offset, <strong>not a byte
         *   offset!</strong>
         * - The value <code><i>y</i></code>, assuming it is not set to
         *   <code>UNKNOWN_LOCATION</code>, is a byte offset.
         */
        static constexpr jint UNKNOWN_LOCATION = -1;

        /**
         * \brief Initializes a storage block that contains ordinary pointers to
         *        locations within the block but no variable indirect pointers
         * \param args Storage block
         * \param ptr_array List of pointer pairs
         * \param ptr_array_size Number of <em>values</em> in
         *        <code>ptr_array</code> (this is always an even number)
         * \see #ptrs_init_vi(marshall_word_t *, jsize, const jint *, jsize,
         *                    JNIEnv *, jobjectArray, marshalling_vi_container&)
         *
         * This function will not fail with the parameter
         * <code>ptr_array_size</code> set to zero, but it will do no work since
         * an empty pointer array implies only direct storage (<em>ie</em> no
         * pointers). Pure direct storage does not need an "init" phase.
         */
        static void ptrs_init(marshall_word_t * args, jint const * ptr_array,
                              jsize ptr_array_size);

        /**
         * \brief Initializes a storage block that contains variable indirect
         *        pointers&mdash;and possibly ordinary pointers as well
         * \param args Storage block
         * \param args_size Number of values in <code>args</code>
         * \param ptr_array List of pointer pairs
         * \param ptr_array_size Number of <em>values</em> in
         *        <code>ptr_array</code> (this is always an even number)
         * \param env JNI environment
         * \param vi_array_in Input variable indirect object array
         * \param vi_array_out Fresh \link marshalling_vi_container\endlink
         *        which receives the state needed to complete the roundtrip via
         *        \link #ptrs_finish_vi(jobjectArray, marshalling_vi_container&, const jni_array_region<jint>&)
         *        ptrs_finish_vi(...)\endlink
         * \see #ptrs_init(marshall_word_t *, const jint *, jsize)
         *
         * In order to unmarshall any changes to the variable indirect storage,
         * \link #ptrs_finish_vi(jobjectArray, marshalling_vi_container&, const jni_array_region<jint>&)
         * ptrs_finish_vi(...)\endlink must be called after <code>args</code>
         * has been passed to the invoked function.
         *
         * \note
         * For clarity, the parameter <code>args_size</code> is a count of
         * \link marshall_word_t\endlink, not bytes!
         */
        static void ptrs_init_vi(marshall_word_t * args, jsize args_size,
                                 jint const * ptr_array, jsize ptr_array_size,
                                 JNIEnv * env, jobjectArray vi_array_in,
                                 marshalling_vi_container& vi_array_out);

        /**
         * \brief Converts variable indirect values back into Java
         *        <code>Object</code> instances so they can be sent back to the
         *        Java side
         * \param vi_array_java Array to receive variable indirect values
         *        unmarshalled out of <code>vi_array_cpp</code>
         * \param vi_array_cpp State initialized in
         *        \link #ptrs_init_vi(marshall_word_t *, jsize, const jint *, jsize, JNIEnv *, jobjectArray, marshalling_vi_container&)
         *        ptrs_init_vi(...)\endlink, points to variable indiret values to
         *        extract
         * \param vi_inst_array Variable indirect instruction array, explains
         *        how to unmarshall out each variable indirect value
         * \see #ptrs_init_vi(marshall_word_t *, jsize, const jint *, jsize, JNIEnv *, jobjectArray, marshalling_vi_container&)
         */
        static void ptrs_finish_vi(jobjectArray vi_array_java,
                                   marshalling_vi_container& vi_array_cpp,
                                   const jni_array_region<jint>& vi_inst_array);
};

inline void marshalling_roundtrip::ptrs_init(marshall_word_t * args,
                                             jint const * ptr_array,
                                             jsize ptr_array_size)
{
    assert(0 == ptr_array_size % 2 || !"pointer array must have even size");
    jint const * i(ptr_array), * e(ptr_array + ptr_array_size);
    while (i < e)
    {
        jint ptr_byte_offset = *i++;
        jint ptd_to_byte_offset = *i++;
        // If the Java-side marshaller put UNKNOWN_LOCATION as the location to
        // point to, just skip this pointer -- we will trust that the Java side
        // put a NULL value into the data array. Otherwise, set the pointer in
        // the data array to point to the appropriate location.
        if (UNKNOWN_LOCATION != ptd_to_byte_offset)
        {
            auto ptr_addr = addr_of_ptr(args, ptr_byte_offset);
            auto ptd_to_addr = addr_of_byte(args, ptd_to_byte_offset);
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

/**
 * \brief Base for classes that unmarshall data from native &rarr; Java
 *       (<em>ie</em> not used for roundtrip Java &rarr; native &rarr; Java)
 * \author Victor Schappert
 * \since 20130813
 * 
 * Classes derived from this class are used for one-way unmarshalling, as in
 * <code>struct</code> copy-outs and <code>callback</code> invocations. Where a
 * roundtrip is involved, as in a function call, use
 * \link marshalling_roundtrip\endlink.
 */
class unmarshaller_base : private non_copyable
{
        //
        // DATA
        //

    protected:

        /** \cond internal */
        const jint d_size_direct;
        const jint d_size_total;
        /** \endcond internal */

        //
        // CONSTRUCTORS
        //

    protected:

        /**
         * \brief Constructs a base unmarshaller suitable for a data block of a
         *        given total size containing a direct block of a given size
         * \param size_direct Size, in bytes, of the direct data at the start of
         *        the marshalled data block: <code>0 &lt; size_direct &le;
         *        size_total</code>
         * \param size_total Size, in bytes, of the whole marshalled data block
         *        <em>must be a multiple of
         *        <code>sizeof(marshall_word_t)</code></em>
         */
        unmarshaller_base(jint size_direct, jint size_total);
};

inline unmarshaller_base::unmarshaller_base(jint size_direct, jint size_total)
    : d_size_direct(size_direct)
    , d_size_total(size_total)
{
    assert(0 <= size_direct && size_direct <= size_total);
    assert(0 == size_total % sizeof(marshall_word_t));
}

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

        /**
         * \brief Type of an iterator over a pointer list
         */
        typedef jint const * ptr_iterator_t;

        //
        // DATA
        //

    protected:

        /** \cond internal */
        ptr_iterator_t d_ptr_begin;  // start of pointer list
        ptr_iterator_t d_ptr_end;    // end of pointer list
        /** \endcond internal */

        //
        // INTERNALS
        //

    private:

        void normal_ptr(marshall_word_t * data, jint ptr_byte_offset,
                        jint ptd_to_byte_offset,  ptr_iterator_t& ptr_i) const;

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructs an indirect storage unmarshaller
         * \param size_direct Size, in bytes, of the direct data at the start of
         *        the marshalled data block: <code>0 &lt; size_direct &le;
         *        size_total</code>
         * \param size_total Size, in bytes, of the whole marshalled data block
         *        <em>must be a multiple of
         *        <code>sizeof(marshall_word_t)</code></em>
         * \param ptr_begin Iterator to first element in pointer list
         * \param ptr_end Iterator one-past the last element in the pointer list
         *
         * The pointer list must contain an even number of elements.
         */
        unmarshaller_indirect(jint size_direct, jint size_total,
                              const ptr_iterator_t& ptr_begin,
                              const ptr_iterator_t& ptr_end);

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Unmarshalls a data block containing indirect storage (normal
         *        pointers but no variable indirect pointers)
         * \param from Address of marshalled data block
         * \param to Address of data block to unmarshall into
         */
        void unmarshall_indirect(const void * from, marshall_word_t * to) const;
};

inline unmarshaller_indirect::unmarshaller_indirect(
    jint size_direct,
    jint size_total,
    const ptr_iterator_t& ptr_begin,
    const ptr_iterator_t& ptr_end)
    : unmarshaller_base(size_direct, size_total)
    , d_ptr_begin(ptr_begin)
    , d_ptr_end(ptr_end)
{ }

inline void unmarshaller_indirect::unmarshall_indirect(
    const void * from, marshall_word_t * to) const
{
    std::memcpy(to, from, d_size_direct);
    ptr_iterator_t ptr_i(d_ptr_begin), ptr_e(d_ptr_end);
    while (ptr_i != ptr_e)
    {
        jint ptr_byte_offset = *ptr_i++;
        jint ptd_to_byte_offset = *ptr_i++;
        normal_ptr(to, ptr_byte_offset, ptd_to_byte_offset, ptr_i);
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

        jint d_vi_count;

        //
        // INTERNALS
        //

    private:

        bool is_vi_ptr(jint ptd_to_pos) const;

        void vi_ptr(marshall_word_t * data, jint ptr_word_index,
                    jint ptd_to_pos, JNIEnv * env, jobjectArray vi_array,
                    jint const * vi_inst_array);

        void normal_ptr(marshall_word_t * data, jint ptr_word_index,
                        jint ptd_to_byte_offset, ptr_iterator_t& ptr_i,
                        JNIEnv * env, jobjectArray vi_array,
                        jint const * vi_inst_array);

    protected:

        /**
         * \brief Allows derived class to unmarshall a zero-terminated string of
         *        8-bit characters
         * \param str Non-<code>null</code> address of zero-terminated string
         * \param vi_index Index of the variable indirect pointer (a number in
         *        the range <code>[0..length(vi_array)-1]</code>)
         * \param env JNI environment, required to manipulate
         *            <code>vi_array</code>
         * \param vi_array Variable indirect output array; the unmarshalled
         *        string should be placed at index <code>vi_index</code> within
         *        this array
         * \param vi_inst Variable indirect instruction (specifies what kind of
         *        Java value <code>str</code> should be converted to)
         *
         * Called by \link #unmarshall_vi(const void *, marshall_word_t *, JNIEnv *, jobjectArray, jint const *)
         * unmarshall_vi(...)\endlink when a variable indirect pointer is
         * encountered.
         */
        virtual void vi_string_ptr(char const * str, jint vi_index,
                                   JNIEnv * env, jobjectArray vi_array,
                                   jint vi_inst) = 0;

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructs a base variable indirect storage unmarshaller
         * \param size_direct Size, in bytes, of the direct data at the start of
         *        the marshalled data block: <code>0 &lt; size_direct &le;
         *        size_total</code>
         * \param size_total Size, in bytes, of the whole marshalled data block
         *        <em>must be a multiple of
         *        <code>sizeof(marshall_word_t)</code></em>
         * \param ptr_begin Iterator to first element in pointer list
         * \param ptr_end Iterator one-past the last element in the pointer list
         * \param vi_count Number of variable indirect pointers:
         *                 <code>0 &le; vi_count</code>
         *
         * The pointer list must contain an even number of elements.
         */
        unmarshaller_vi_base(jint size_direct, jint size_total,
                             const ptr_iterator_t& ptr_begin,
                             const ptr_iterator_t& ptr_end, jint vi_count);

        virtual ~unmarshaller_vi_base() = default;

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Unmarshalls a data block containing variable indirect pointers
         * \param from Address of marshalled data block
         * \param to Address of data block to unmarshall into
         * \param env JNI environment
         * \param vi_array Variable indirect output array
         * \param vi_inst_array Variable indirect instruction array
         *
         * The elements of <code>vi_inst_array</code> are in one-to-one
         * correspondence with the elements of <code>vi_array</code> and are
         * instructions on how to unmarshall each variable indirect pointer.
         */
        void unmarshall_vi(const void * from, marshall_word_t * to,
                           JNIEnv * env, jobjectArray vi_array,
                           jint const * vi_inst_array);
};

inline bool unmarshaller_vi_base::is_vi_ptr(jint ptd_to_pos) const
{ return unmarshaller_base::d_size_total <= ptd_to_pos; }

inline unmarshaller_vi_base::unmarshaller_vi_base(
    jint size_direct, jint size_total, const ptr_iterator_t& ptr_begin,
    const ptr_iterator_t& ptr_end, jint vi_count)
    : unmarshaller_indirect(size_direct, size_total, ptr_begin, ptr_end)
    , d_vi_count(vi_count)
{ assert(0 <= vi_count); }

inline void unmarshaller_vi_base::unmarshall_vi(
    const void * from, marshall_word_t * to, JNIEnv * env,
    jobjectArray vi_array, jint const * vi_inst_array)
{
    std::memcpy(to, from, unmarshaller_base::d_size_direct);
    ptr_iterator_t ptr_i(unmarshaller_indirect::d_ptr_begin),
                   ptr_e(unmarshaller_indirect::d_ptr_end);
    while (ptr_i != ptr_e)
    {
        jint ptr_byte_offset = *ptr_i++;
        jint ptd_to_byte_offset = *ptr_i++;
        if (is_vi_ptr(ptd_to_byte_offset))  // vi pointer
            vi_ptr(to, ptr_byte_offset, ptd_to_byte_offset, env, vi_array,
                   vi_inst_array);
        else                                // normal pointer
            normal_ptr(to, ptr_byte_offset, ptd_to_byte_offset, ptr_i, env,
                       vi_array, vi_inst_array);
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

        virtual void vi_string_ptr(char const * str, jint vi_index,
                                   JNIEnv * env, jobjectArray vi_array,
                                   jint vi_inst);

        //
        // CONSTRUCTORS
        //

    public:

        unmarshaller_vi_test(jint size_direct, jint size_total,
                             const ptr_iterator_t& ptr_begin,
                             const ptr_iterator_t& ptr_end, jint vi_count);

        //
        // ACCESSORS
        //

    public:

        const vi_string& vi_at(size_t n) const;
};

inline unmarshaller_vi_test::unmarshaller_vi_test(
    jint size_direct, jint size_total, const ptr_iterator_t& ptr_begin,
    const ptr_iterator_t& ptr_end, jint vi_count)
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

        virtual void vi_string_ptr(char const * str, jint vi_index,
                                   JNIEnv * env, jobjectArray vi_array,
                                   jint vi_inst);

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructs a variable indirect storage unmarshaller
         * \param size_direct Size, in bytes, of the direct data at the start of
         *        the marshalled data block: <code>0 &lt; size_direct &le;
         *        size_total</code>
         * \param size_total Size, in bytes, of the whole marshalled data block
         *        <em>must be a multiple of
         *        <code>sizeof(marshall_word_t)</code></em>
         * \param ptr_begin Iterator to first element in pointer list
         * \param ptr_end Iterator one-past the last element in the pointer list
         * \param vi_count Number of variable indirect pointers:
         *                 <code>0 &le; vi_count</code>
         *
         * The pointer list must contain an even number of elements.
         */
        unmarshaller_vi(jint size_direct, jint size_total,
                        const ptr_iterator_t& ptr_begin,
                        const ptr_iterator_t& ptr_end, jint vi_count);
};

inline unmarshaller_vi::unmarshaller_vi(jint size_direct, jint size_total,
                                        const ptr_iterator_t& ptr_begin,
                                        const ptr_iterator_t& ptr_end,
                                        jint vi_count)
    : unmarshaller_vi_base(size_direct, size_total, ptr_begin, ptr_end,
                           vi_count)
{ }

} // namespace jsdi

#endif // __INCLUDED_MARSHALLING_H___

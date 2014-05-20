/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_JNI_UTIL_H___
#define __INCLUDED_JNI_UTIL_H___

/**
 * \file jni_util.h
 * \author Victor Schappert
 * \since 20130627
 * \brief Utility functions to simplify working with JNI.
 */

#include "util.h"
#include "utf16_util.h"
#include "jni_exception.h"

#include <jni.h>

#include <cassert>
#include <vector>

namespace jsdi {

//==============================================================================
//                             struct jni_traits
//==============================================================================

/**
 * \brief Traits class providing static information about JNI data types.
 * \author Victor Schappert
 * \since 20130627
 * \tparam JNIType The JNI data type on which to specialize the template.
 * <p>
 * The following traits are available:
 * <dl>
 * <dt><dfn>array_type</dfn></dt>
 * <dd>
 * JNI opaque array reference type &mdash; \em eg <dfn>jbyteArray</dfn>.
 * </dd>
 * </dl>
 * </p>
 * \todo Finish documenting available traits
 */
template<typename JNIType>
struct jni_traits
{
};

/** \cond internal */
template<>
struct jni_traits<jbyte>
{
    typedef jbyteArray array_type;
    typedef jbyte value_type;
    typedef jbyte const const_value_type;
    typedef value_type * pointer;
    typedef const_value_type * const_pointer;
    typedef value_type & reference;
    typedef const_value_type & const_reference;
};

template<>
struct jni_traits<jint>
{
    typedef jintArray array_type;
    typedef jint value_type;
    typedef jint const const_value_type;
    typedef value_type * pointer;
    typedef const_value_type * const_pointer;
    typedef value_type & reference;
    typedef const_value_type & const_reference;
};

template<>
struct jni_traits<jboolean>
{
    typedef jbooleanArray array_type;
    typedef jboolean value_type;
    typedef jboolean const const_value_type;
    typedef value_type * pointer;
    typedef const_value_type * const_pointer;
    typedef value_type & reference;
    typedef const_value_type & const_reference;
};
/** \endcond */

//==============================================================================
//                          class jni_array_region
//==============================================================================

/** \cond internal */
template<typename JNIType>
inline void jni_array_get_region(JNIEnv *,
                                 typename jni_traits<JNIType>::array_type,
                                 jsize, jsize,
                                 typename jni_traits<JNIType>::pointer);

template<>
inline void jni_array_get_region<jbyte>(JNIEnv * env, jbyteArray array,
                                        jsize start, jsize len, jbyte * buf)
{ env->GetByteArrayRegion(array, start, len, buf); }

template<>
inline void jni_array_get_region<jint>(JNIEnv * env, jintArray array,
                                       jsize start, jsize len, jint * buf)
{ env->GetIntArrayRegion(array, start, len, buf); }

template<>
inline void jni_array_get_region<jboolean>(JNIEnv * env, jbooleanArray array,
                                           jsize start, jsize len, jboolean * buf)
{ env->GetBooleanArrayRegion(array, start, len, buf); }
/** \endcond */

/**
 * \brief Managed array of a JNI primitive type which was retrieved from a JNI
 *        array reference \em but which cannot be copied back into the JVM.
 * \author Victor Schappert
 * \since 20130628
 * \tparam JNIType The JNI data type on which to specialize the array region
 *         &mdash; \em eg <dfn>jbyte</dfn>.
 * \see jni_array
 * \see jni_utf8_string_region
 * \see jni_utf16_string_region
 *
 * The array data is copied out of the JVM on construction using a JNI
 * <dfn>Get&lt;Type&gt;ArrayRegion(...)</dfn> function and deallocated on
 * destruction. This is a one-way data structure in the sense that it is not
 * possible to send its contents back to the JVM.
 */
template<typename JNIType>
class jni_array_region: private non_copyable
{
        //
        // TYPES
        //

    public:

        /** \brief Type of the underlying Java array. */
        typedef typename jni_traits<JNIType>::array_type array_type;
        /** \brief A signed integral type. */
        typedef jsize size_type;
        /** \brief Type of the region elements (a JNI primitive type, such as
         *         <dfn>jbyte</dfn>).
         * \see #const_value_type
         */
        typedef typename jni_traits<JNIType>::value_type value_type;
        /**
         * \brief Type of a <dfn>const</dfn> region element.
         * \see #value_type
         */
        typedef typename jni_traits<JNIType>::const_value_type const_value_type;
        /** \brief Type of a pointer a region element. */
        typedef typename jni_traits<JNIType>::pointer pointer;
        /** \brief Type of a pointer to a #const_value_type. */
        typedef typename jni_traits<JNIType>::const_pointer const_pointer;
        /** \brief Type of a reference to a #value_type. */
        typedef typename jni_traits<JNIType>::reference reference;
        /** \brief Type of a reference to a #const_value_type. */
        typedef typename jni_traits<JNIType>::const_reference const_reference;
        /** \brief Random-access iterator to a #const_value_type. */
        typedef typename jni_traits<JNIType>::const_pointer const_iterator;

        //
        // DATA
        //

    private:

        size_type d_size;
        pointer d_array;

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructor for region containing all of the elements of the
         *        Java array.
         * \param env JNI environment
         * \param array Reference to a JNI primitive array of the correct type
         *              (\em eg <dfn>jbyteArray</dfn>)
         */
        jni_array_region(JNIEnv * env, array_type array);

        /**
         * \brief Constructor for region containing all of the elements of the
         *        Java array from index 0 up to a certain size.
         * \param env JNI environment
         * \param array Reference to a JNI primitive array of the correct type
         *              (\em eg <dfn>jbyteArray</dfn>)
         * \param size Desired size of the region; this must be known in advance
         *             to be less than or equal to the array's length, because
         *             this constructor does not check the length of the array
         *
         * The constructed region contains <dfn>array[0..size-1]</dfn>.
         */
        jni_array_region(JNIEnv * env, array_type array, size_type size);

        ~jni_array_region();

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Returns the number of elements in the region.
         * \return Number of elements in the region
         */
        size_type size() const;

        /**
         * \brief Returns a pointer to the region's array.
         * \return Pointer to array
         * \see #data() const
         *
         * The pointer returned is such that [data(), data() + size()] is always
         * a valid range.
         */
        pointer data();

        /**
         * \brief Returns a pointer to the region's array.
         * \return Pointer to array
         * \see #data()
         *
         * The pointer returned is such that [data(), data() + size()] is always
         * a valid range.
         */
        const_pointer data() const;

        /**
         * \brief Subscripts an element of the region.
         * \param n Zero-based index of the region element to return
         * \return Read-only reference to the element at position <dfn>n</dfn>
         * \see #operator[](size_type)
         */
        reference operator[](size_type n);

        /**
         * \brief Subscripts an element of the region.
         * \param n Zero-based index of the region element to return
         * \return Read-only reference to the element at position <dfn>n</dfn>
         */
        const_reference operator[](size_type n) const;

        /**
         * \brief Returns a #const_iterator pointing to the first element of the
         *        array region.
         * \return A #const_iterator to the beginning of the array region
         * \see #cend() const
         * \see #begin() const
         *
         * If the array region is empty, the returned iterator shall not be
         * dereferenced.
         */
        const_iterator cbegin() const;

        /**
         * \brief Returns a #const_iterator pointing to the \em past-the-end
         *        element of the array region.
         * \return A #const_iterator to the end of the array region
         * \see #cbegin() const
         * \see #end() const
         *
         * The returned iterator shall not be dereferenced.
         */
        const_iterator cend() const;

        /**
         * \brief Returns a #const_iterator pointing to the first element of the
         *        array region.
         * \return A #const_iterator to the beginning of the array region
         * \see #end() const
         * \see #cbegin() const
         *
         * If the array region is empty, the returned iterator shall not be
         * dereferenced.
         */
        const_iterator begin() const;

        /**
         * \brief Returns a #const_iterator pointing to the \em past-the-end
         *        element of the array region.
         * \return A #const_iterator to the end of the array region
         * \see #begin() const
         * \see #cend() const
         *
         * The returned iterator shall not be dereferenced.
         */
        const_iterator end() const;
};

template <typename JNIType>
inline jni_array_region<JNIType>::jni_array_region(JNIEnv * env,
                                                   array_type array)
    : d_size(env->GetArrayLength(array))
    , d_array(new value_type[d_size])
{
    assert(env || !"JNI environment cannot be NULL");
    assert(array || !"JNI array cannot be NULL");
    jni_array_get_region<JNIType>(env, array, 0, d_size, d_array);
    JNI_EXCEPTION_CHECK(env);
}

template <typename JNIType>
inline jni_array_region<JNIType>::jni_array_region(JNIEnv * env,
                                                   array_type array,
                                                   size_type size)
    : d_size(size)
    , d_array(new value_type[d_size])
{
    assert(env || !"JNI environment cannot be NULL");
    assert(array || !"JNI array cannot be NULL");
    jni_array_get_region<JNIType>(env, array, 0, d_size, d_array);
    JNI_EXCEPTION_CHECK(env);
}

template<typename JNIType>
inline jni_array_region<JNIType>::~jni_array_region()
{
    delete[] d_array;
}

template <typename JNIType>
inline typename jni_array_region<JNIType>::size_type jni_array_region<JNIType>::size() const
{
    return d_size;
}

template <typename JNIType>
inline typename jni_array_region<JNIType>::pointer jni_array_region<JNIType>::data()
{
    return d_array;
}

template <typename JNIType>
inline typename jni_array_region<JNIType>::const_pointer jni_array_region<
    JNIType>::data() const
{
    return d_array;
}

template<typename JNIType>
inline typename jni_array_region<JNIType>::reference jni_array_region<
    JNIType>::operator[](size_type n)
{
    return d_array[n];
}

template<typename JNIType>
inline typename jni_array_region<JNIType>::const_reference jni_array_region<
    JNIType>::operator[](size_type n) const
{
    return d_array[n];
}

template<typename JNIType>
inline typename jni_array_region<JNIType>::const_iterator jni_array_region<
    JNIType>::cbegin() const
{
    return d_array;
}

template<typename JNIType>
inline typename jni_array_region<JNIType>::const_iterator jni_array_region<
    JNIType>::cend() const
{
    return d_array + d_size;
}

template<typename JNIType>
inline typename jni_array_region<JNIType>::const_iterator jni_array_region<
    JNIType>::begin() const
{
    return cbegin();
}

template<typename JNIType>
inline typename jni_array_region<JNIType>::const_iterator jni_array_region<
    JNIType>::end() const
{
    return cend();
}

//==============================================================================
//                              class jni_array
//==============================================================================

/** \cond internal */
template <typename JNIType>
inline typename jni_traits<JNIType>::pointer jni_array_get_elements(
    JNIEnv *, typename jni_traits<JNIType>::array_type, jboolean *);

template<>
inline jbyte * jni_array_get_elements<jbyte>(JNIEnv * env, jbyteArray array,
                                             jboolean * is_copy)
{
    return env->GetByteArrayElements(array, is_copy);
}

template <>
inline jint * jni_array_get_elements<jint>(JNIEnv * env, jintArray array,
                                           jboolean * is_copy)
{
    return env->GetIntArrayElements(array, is_copy);
}

template<typename JNIType>
inline void jni_array_release_elements(JNIEnv *,
                                       typename jni_traits<JNIType>::array_type,
                                       typename jni_traits<JNIType>::pointer,
                                       jint);

template<>
inline void jni_array_release_elements<jbyte>(JNIEnv * env, jbyteArray array,
                                              jbyte * elems, jint mode)
{
    env->ReleaseByteArrayElements(array, elems, mode);
}

template <>
inline void jni_array_release_elements<jint>(JNIEnv * env, jintArray array,
                                             jint * elems, jint mode)
{
    env->ReleaseIntArrayElements(array, elems, mode);
}

template <typename JNIType>
inline typename jni_traits<JNIType>::array_type jni_array_new(JNIEnv *, jsize);

template<>
inline jbyteArray jni_array_new<jbyte>(JNIEnv * env, jsize length)
{
    return env->NewByteArray(length);
}
/** \endcond */

/**
 * \brief Managed array of a JNI primitive type which was retrieved from a JNI
 *        array reference.
 * \author Victor Schappert
 * \since 20130727
 * \tparam JNIType The JNI data type on which to specialize the array region
 *         &mdash; \em eg <dfn>jbyte</dfn>.
 * \see jni_array_region
 * \see jni_critical_array
 *
 * The array data is retrieved from the JVM on construction using a JNI
 * <dfn>Get&lt;Type&gt;ArrayElements(...)</dfn> function and released on
 * destruction. This is a two-way data structure in the sense that any changes
 * made to the array are propagated back to the JVM on destruction (or earlier,
 * depending on whether the array is a copy of the JVM data, or a pointer to
 * the actual JVM data).
 *
 * <em>Since the backing data of an instance of jni_array may be a
 * <strong>copy</strong> of the underlying JVM data (see #is_copy() const),
 * changes to the array may not be visible on the Java side until the instance
 * is destroyed.</em>
 */
template<typename JNIType>
class jni_array: private non_copyable
{
        //
        // TYPES
        //

    public:

        /** \brief Type of the underlying Java array. */
        typedef typename jni_traits<JNIType>::array_type array_type;
        /** \brief An signed integral type. */
        typedef jsize size_type;
        /** \brief Type of the region elements (a JNI primitive type, such as
         *         <dfn>jbyte</dfn>).
         * \see #const_value_type
         */
        typedef typename jni_traits<JNIType>::value_type value_type;
        /**
         * \brief Type of a <dfn>const</dfn> region element.
         * \see #value_type
         */
        typedef typename jni_traits<JNIType>::const_value_type const_value_type;
        /** \brief Type of a pointer a region element. */
        typedef typename jni_traits<JNIType>::pointer pointer;
        /** \brief Type of a pointer to a #const_value_type. */
        typedef typename jni_traits<JNIType>::const_pointer const_pointer;
        /** \brief Type of a reference to a #value_type. */
        typedef typename jni_traits<JNIType>::reference reference;
        /** \brief Type of a reference to a #const_value_type. */
        typedef typename jni_traits<JNIType>::const_reference const_reference;
        /** \brief Random-access iterator to a #const_value_type. */
        typedef typename jni_traits<JNIType>::const_pointer const_iterator;

        //
        // DATA
        //

    private:

        pointer    d_array;
        size_type  d_size;
        JNIEnv *   d_env;
        array_type d_jarray;
        jboolean   d_is_copy;

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructor for an array containing all of the elements of an
         *        existing Java array where the size of the array is not known
         *        at construction time.
         * \param env JNI environment
         * \param array Reference to a JNI primitive array of the correct type
         *              (\em eg <dfn>jbyteArray</dfn>)
         * \see #jni_array(JNIEnv *, array_type, size_type)
         *
         * The only difference between this constructor and
         * #jni_array(JNIEnv *, array_type, size_type) is that the latter
         * constructor is able to avoid a JNI call to fetch the array size since
         * it is passed to the constructor.
         */
        jni_array(JNIEnv * env, array_type array);

        /**
         * \brief Constructor for an array containing all of the elements of an
         *        existing Java array where the size of the array is known at
         *        construction time.
         * \param env JNI environment
         * \param array Reference to a JNI primitive array of the correct type
         *              (\em eg <dfn>jbyteArray</dfn>)
         * \param size Number of elements in the array
         * \see #jni_array(JNIEnv *, array_type)
         *
         * The parameter <dfn>size</dfn> must correctly specify the number of
         * elements in the underlying Java array. Providing the size as a
         * parameter enables this constructor to avoid a call to the
         * JNI function <dfn>GetArrayLength</dfn>
         */
        jni_array(JNIEnv * env, array_type array, size_type size);

        ~jni_array();

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Returns the underlying JNI array reference managed by this
         *        instance.
         * \return Underlying JNI array
         * \see #is_copy() const
         */
        array_type jarray();

        /**
         * \brief Indicates whether the array data is a copy of the underlying
         * Java array.
         * \return Whether the array data is a copy of the &quot;primary&quot;
         * array held by the JVM (<dfn>true</dfn>); or whether this array
         * directly refers to the JVM data (<dfn>false</dfn>).
         * \see #jarray()
         */
        bool is_copy() const;

        /**
         * \brief Returns the number of elements in the array.
         * \return Number of elements in the array
         */
        size_type size() const;

        /**
         * \brief Returns a pointer to the array storage.
         * \return Pointer to array storage
         * \see #data() const
         *
         * The pointer returned is such that [data(), data() + size()] is always
         * a valid range.
         */
        pointer data();

        /**
         * \brief Returns a pointer to the array storage.
         * \return Pointer to array storage
         * \see #data()
         *
         * The pointer returned is such that [data(), data() + size()] is always
         * a valid range.
         */
        const_pointer data() const;

        /**
         * \brief Subscripts an element of the array.
         * \param n Zero-based index of the region element to return.
         * \return Read-only reference to the element at position <dfn>n</dfn>
         * \see #operator[](size_type)
         */
        reference operator[](size_type n);

        /**
         * \brief Subscripts an element of the array.
         * \param n Zero-based index of the array element to return
         * \return Read-only reference to the element at position <dfn>n</dfn>
         */
        const_reference operator[](size_type n) const;

        /**
         * \brief Returns a #const_iterator pointing to the first element of the
         *        array.
         * \return A #const_iterator to the beginning of the array.
         * \see #cend() const
         *
         * If the array is empty, the returned iterator shall not be
         * dereferenced.
         */
        const_iterator cbegin() const;

        /**
         * \brief Returns a #const_iterator pointing to the \em past-the-end
         *        element of the array.
         * \return A #const_iterator to the end of the array.
         * \see #cend() const
         *
         * The returned iterator shall not be dereferenced.
         */
        const_iterator cend() const;
};

template <typename JNIType>
inline jni_array<JNIType>::jni_array(JNIEnv * env, array_type array)
    : d_array(jni_array_get_elements<JNIType>(env, array, &d_is_copy))
    , d_size(env->GetArrayLength(array))
    , d_env(env)
    , d_jarray(array)
{
    assert(d_env && d_jarray);
    if (! d_array) throw jni_bad_alloc("Get*ArrayElements", __FUNCTION__);
}

template <typename JNIType>
inline jni_array<JNIType>::jni_array(JNIEnv * env, array_type array, size_type size)
    : d_array(jni_array_get_elements<JNIType>(env, array, &d_is_copy))
    , d_size(size)
    , d_env(env)
    , d_jarray(array)
{
    assert(0 <= size || !"array size cannot be negative");
    assert(env->GetArrayLength(array) == size || !"array size mismatch");
    assert(d_env && d_jarray);
    if (! d_array) throw jni_bad_alloc("Get*ArrayElements", __FUNCTION__);
}

template <typename JNIType>
inline jni_array<JNIType>::~jni_array()
{ jni_array_release_elements<JNIType>(d_env, d_jarray, d_array, 0); }

template <typename JNIType>
inline typename jni_array<JNIType>::array_type jni_array<JNIType>::jarray()
{ return d_jarray; }

template <typename JNIType>
inline bool jni_array<JNIType>::is_copy() const
{ return JNI_TRUE == d_is_copy; }

template <typename JNIType>
inline typename jni_array<JNIType>::size_type jni_array<JNIType>::size() const
{ return d_size; }

template <typename JNIType>
inline typename jni_array<JNIType>::pointer jni_array<JNIType>::data()
{ return d_array; }

template <typename JNIType>
inline typename jni_array<JNIType>::const_pointer jni_array<JNIType>::data() const
{ return d_array; }

template <typename JNIType>
inline typename jni_array<JNIType>::reference jni_array<JNIType>::operator[](
    size_type n)
{ return d_array[n]; }

template <typename JNIType>
inline typename jni_array<JNIType>::const_reference jni_array<JNIType>::operator[](
    size_type n) const
{ return d_array[n]; }

template <typename JNIType>
inline typename jni_array<JNIType>::const_iterator jni_array<JNIType>::cbegin() const
{ return d_array; }

template <typename JNIType>
inline typename jni_array<JNIType>::const_iterator jni_array<JNIType>::cend() const
{ return d_array + d_size; }

//==============================================================================
//                         class jni_critical_array
//==============================================================================

/**
 * \brief Managed \em critical array of a JNI primitive type which was retrieved
 *        from a JNI array reference. <strong>This class is subject to
 *        significant narrow usage restrictions.</strong>
 * \author Victor Schappert
 * \since 20130813
 * \tparam JNIType The JNI data type on which to specialize the array region
 *         &mdash; \em eg <dfn>jbyte</dfn>.
 * \see jni_array
 *
 * This class has the same basic behaviour as jni_array, \em including being a
 * "two-way" data structure. The major difference is that this class is based on
 * the JNI <dfn>GetPrimitiveArrayCritical(...)</dfn> function. This means it is
 * subject to the following <strong>usage restrictions</strong>:
 *
 * <ul>
 * <li>
 * After an instance of this class is instantiated, no other JNI functions may
 * be called in the same thread until the instance is destroyed. Failure to
 * follow this requirement may result in abnormal program termination.
 * </li>
 * <li>
 * Instantiation of critical arrays should be as brief as possible. No instance
 * of this class should remain alive during any lengthy operation. Failure to
 * follow this requirement may result in poor performance as other threads in
 * the VM may be blocked while a critical array is alive.
 * </li>
 * </ul>
 */
template<typename JNIType>
class jni_critical_array : private non_copyable
{
        //
        // TYPES
        //

    public:

        /** \brief Type of the underlying Java array. */
        typedef typename jni_traits<JNIType>::array_type array_type;
        /** \brief An signed integral type. */
        typedef jsize size_type;
        /** \brief Type of the region elements (a JNI primitive type, such as
         *         <dfn>jbyte</dfn>).
         * \see #const_value_type
         */
        typedef typename jni_traits<JNIType>::value_type value_type;
        /**
         * \brief Type of a <dfn>const</dfn> region element.
         * \see #value_type
         */
        typedef typename jni_traits<JNIType>::const_value_type const_value_type;
        /** \brief Type of a pointer a region element. */
        typedef typename jni_traits<JNIType>::pointer pointer;
        /** \brief Type of a pointer to a #const_value_type. */
        typedef typename jni_traits<JNIType>::const_pointer const_pointer;
        /** \brief Type of a reference to a #value_type. */
        typedef typename jni_traits<JNIType>::reference reference;
        /** \brief Type of a reference to a #const_value_type. */
        typedef typename jni_traits<JNIType>::const_reference const_reference;
        /** \brief Random-access iterator to a #const_value_type. */
        typedef typename jni_traits<JNIType>::const_pointer const_iterator;

        //
        // DATA
        //

    private:

        pointer      d_array;
        size_type    d_size;
        JNIEnv *     d_env;
        array_type   d_jarray;
        jboolean     d_is_copy;

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * Constructor for a critical array containing all of the elements of an
         * existing Java array where the size of the array is not known at
         * construction time.
         * \param env JNI environment
         * \param array Reference to a JNI primitive array of the correct type
         *              (\em eg <dfn>jintArray</dfn>)
         * \see #jni_critical_array(JNIEnv * env, array_type, size_type)
         *
         * The only difference between this constructor and
         * #jni_critical_array(JNIEnv *, array_type, size_type) is that the
         * latter constructor is able to avoid a JNI call to fetch the array
         * size since it is passed to the constructor.
         */
        jni_critical_array(JNIEnv * env, array_type array);

        /**
         * \brief Constructor for a critical array containing all of the
         *        elements of an existing Java array where the size of the array
         *        is known at construction time.
         * \param env JNI environment
         * \param array Reference to a JNI primitive array of the correct type
         *              (\em eg <dfn>jintArray</dfn>)
         * \param size Number of elements in the array
         * \see #jni_critical_array(JNIEnv *, array_type)
         *
         * The parameter <dfn>size</dfn> must correctly specify the number of
         * elements in the underlying Java array. Providing the size as a
         * parameter enables this constructor to avoid a call to the
         * JNI function <dfn>GetArrayLength</dfn>
         */
        jni_critical_array(JNIEnv * env, array_type array, size_type size);

        ~jni_critical_array();

        //
        // ACCESSORS
        //

        /**
         * \brief Returns the number of elements in the array.
         * \return Number of elements in the array
         */
        size_type size() const;

        /**
         * \brief Returns a pointer to the array storage.
         * \return Pointer to array storage
         * \see #data() const
         *
         * The pointer returned is such that [data(), data() + size()] is always
         * a valid range.
         */
        pointer data();

        /**
         * \brief Returns a pointer to the array storage.
         * \return Pointer to array storage
         * \see #data()
         *
         * The pointer returned is such that [data(), data() + size()] is always
         * a valid range.
         */
        const_pointer data() const;
};

template <typename JNIType>
inline jni_critical_array<JNIType>::jni_critical_array(JNIEnv * env,
                                                       array_type array)
    : jni_critical_array(env, array, env->GetArrayLength(array))
{ }

template <typename JNIType>
inline jni_critical_array<JNIType>::jni_critical_array(JNIEnv * env,
                                                       array_type array,
                                                       size_type size)
    : d_env(env)
    , d_jarray(array)
{
    assert(env || !"JNI environment cannot be NULL");
    assert(d_jarray || !"array cannot be NULL");
    assert(0 <= size || !"size must be non-negative");
    d_array = reinterpret_cast<pointer>(d_env->GetPrimitiveArrayCritical(
        d_jarray, &d_is_copy));
    if (!d_array)
        throw jni_bad_alloc("GetPrimitiveArrayCritical", __FUNCTION__);
    d_size = size;
}

template <typename JNIType>
inline jni_critical_array<JNIType>::~jni_critical_array()
{ d_env->ReleasePrimitiveArrayCritical(d_jarray, d_array, 0); }

template <typename JNIType>
inline typename jni_critical_array<JNIType>::size_type jni_critical_array<
    JNIType>::size() const
{ return d_size; }

template <typename JNIType>
inline typename jni_critical_array<JNIType>::pointer jni_critical_array<JNIType>::data()
{ return d_array; }

template <typename JNIType>
inline typename jni_critical_array<JNIType>::const_pointer jni_critical_array<
    JNIType>::data() const
{ return d_array; }

//==============================================================================
//                           class jni_auto_local
//==============================================================================

/**
 * \brief Automatic local reference to JNI object.
 * \author Victor Schappert
 * \since 20130628
 * \tparam JNIObjectType JNI object type on which to specialize the auto local
 *         reference.
 * \see jni_auto_local<jclass>
 * \see jni_auto_local<jobject>
 * \see jni_auto_local<jstring>
 * \see jni_auto_local<jthrowable>
 * \see jni_array
 * \see jni_array_region
 *
 * The various specializations of jni_auto_local may have type-specific
 * constructors and, perhaps, methods, but they have consistent behaviour on
 * destruction: if an auto local contains a live JNI object reference (\em ie a
 * non-<dfn>null</dfn> pointer), the reference is released via
 * <dfn>DeleteLocalRef(JNIEnv *, jobject)</dfn>.
 */
template<typename JNIObjectType>
class jni_auto_local;

/**
 * \brief Automatic local reference to a Java class.
 * \author Victor Schappert
 * \since 20130628
 * \see jni_auto_local
 * \see jni_auto_local<jobject>
 * \see jni_auto_local<jstring>
 * \see jni_auto_local<jthrowable>
 *
 * As with all specializations of jni_auto_local, this class frees its
 * managed local reference on destruction via
 * <dfn>DeleteLocalRef(JNIEnv *, jobject)</dfn>.
 */
template<>
class jni_auto_local<jclass>
{
        //
        // DATA
        //

        JNIEnv * d_env;
        jclass   d_class;

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructs an automatic local reference to the specified
         *        locally-defined Java class.
         * \param env JNI environment
         * \param class_name Fully-qualified class name
         *
         * Regarding the <dfn>class_name</dfn> parameter, the JNI documentation
         * describes it as:
         * > a fully-qualified class name (that is, a package name, delimited by
         * > “/”, followed by the class name). If the name begins with “[“ (the
         * > array signature character), it returns an array class. The string
         * > is encoded in modified UTF-8.
         *
         * \attention
         * This constructor looks up <dfn>class_name</dfn> using the JNI
         * <dfn>FindClass(JNIEnv *, const char *)</dfn> function. This function
         * may return <dfn>null</dfn> and, more importantly, <em>raise an
         * exception in the JVM</em> of which the most likely variety is
         * <dfn>NoClassDefFoundError</dfn>. In other words, although this
         * constructor will not raise a <em>C++</em> exception, both the
         * constructed class and the JVM may be in an error state post
         * construction. Any code instantiating this class should therefore test
         * for success post-construction, for example with:
         *
         *     jni_auto_local<jclass> clazz(env, "java/lang/Object");
         *     if (! clazz)
         *     {
         *         // Handle exception state
         *         // ...
         *     } 
         */
        jni_auto_local(JNIEnv * env, const char * class_name) noexcept;

        ~jni_auto_local();

        //
        // ACCESSORS
        //

    public:

        /** \brief Implicit conversion to <dfn>jclass</dfn>.
         *  \return The <dfn>jclass</dfn> managed by this auto local. */
        operator jclass();
};

inline jni_auto_local<jclass>::jni_auto_local(JNIEnv * env,
                                              const char * class_name) noexcept
    : d_env(env)
    , d_class(env->FindClass(class_name))
{ }

inline jni_auto_local<jclass>::~jni_auto_local()
{ if (d_class) d_env->DeleteLocalRef(d_class); }

inline jni_auto_local<jclass>::operator jclass()
{ return d_class; }

/**
 * \brief Automatic local reference to a Java object.
 * \author Victor Schappert
 * \since 20130628
 * \see jni_auto_local
 * \see jni_auto_local<jclass>
 * \see jni_auto_local<jstring>
 * \see jni_auto_local<jthrowable>
 *
 * As with all specializations of jni_auto_local, this class frees its
 * managed local reference on destruction via
 * <dfn>DeleteLocalRef(JNIEnv *, jobject)</dfn>.
 */
template<>
class jni_auto_local<jobject>
{
        //
        // DATA
        //

        JNIEnv * d_env;
        jobject  d_object;

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructs an automatic local reference to the specified Java
         *        object.
         * \param env JNI environment
         * \param object JNI pointer representing a Java object reference.
         * \see jni_auto_local(JNIEnv *, jclass, jfieldID)
         *
         * If <dfn>object</dfn> is <dfn>null</dfn>, this auto local will be
         * constructed "empty" and no action will be taken on destruction.
         */
        jni_auto_local(JNIEnv * env, jobject object);

        /**
         * \brief Constructs an automatic local reference to a Java object that
         *        is contained in a static field of a particular Java class.
         * \param env JNI environment
         * \param clazz Valid reference to a Java class.
         * \param static_field_id Valid reference to a static field within
         *        <dfn>clazz</dfn>.
         *
         * This constructor uses
         * <dfn>GetStaticObjectField(JNIEnv *, jclass, jfieldID)</dfn> to load
         * its local reference. The JNI documentation does not specify what
         * happens if you give an invalid <dfn>jclass</dfn> or
         * <dfn>jfieldID</dfn> or a field identifier that doesn't match the
         * class, but it suffices to assume this would be very, very bad. Don't
         * pass invalid parameters.
         *
         * Suppose you have Java code along these lines:
         *
         *     public class MyClass {
         *         public static final Object field;
         *         ...
         *     }
         *
         * If <dfn>clazz</dfn> is a <dfn>jclass</dfn> handle to
         * <dfn>MyClass.class</dfn> and <dfn>static_field_id</dfn> is a
         * <dfn>jfieldID</dfn> identifying "field" within
         * <dfn>MyClass.class</dfn>, then the C++ code
         *
         *     jni_auto_local<jobject> field(env, clazz, static_field_id);
         *
         * will construct an automatic local reference to
         * <dfn>MyClass.field</dfn>.
         */
        jni_auto_local(JNIEnv * env, jclass clazz, jfieldID static_field_id);

        ~jni_auto_local();

        //
        // ACCESSORS
        //

    public:

        /** \brief Implicit conversion to <dfn>jobject</dfn>.
         *  \return The <dfn>jobject</dfn> managed by this auto local. */
        operator jobject();
};

inline jni_auto_local<jobject>::jni_auto_local(JNIEnv * env, jobject object)
    : d_env(env)
    , d_object(object)
{ }

inline jni_auto_local<jobject>::jni_auto_local(JNIEnv * env, jclass clazz,
                                               jfieldID static_field_id)
    : d_env(env)
    , d_object(env->GetStaticObjectField(clazz, static_field_id))
{ }

inline jni_auto_local<jobject>::~jni_auto_local()
{ if (d_object) d_env->DeleteLocalRef(d_object); }

inline jni_auto_local<jobject>::operator jobject()
{ return d_object; }

/**
 * \brief Automatic local reference to a Java string.
 * \author Victor Schappert
 * \since 20130731
 * \see jni_auto_local
 * \see jni_auto_local<jclass>
 * \see jni_auto_local<jobject>
 * \see jni_auto_local<jthrowable>
 *
 * As with all specializations of jni_auto_local, this class frees its
 * managed local reference on destruction via
 * <dfn>DeleteLocalRef(JNIEnv *, jobject)</dfn>.
 *
 * \note
 * This specialization of jni_auto_local is somewhat inconsistent with the
 * others in that not only can the user deliberately initialize it "empty", it
 * can also be emptied (or have its contents changed) while alive using
 * #reset(JNIEnv *, jstring). It may be that in the future the behaviour of the
 * various specializations should be made 100% uniform.
 */
template<>
class jni_auto_local<jstring>
{
        //
        // DATA
        //

        JNIEnv * d_env;
        jstring  d_string;

        //
        // INTERNALS
        //

        void release(JNIEnv * env, jstring string);

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructs an "empty" automatic local reference (not actually
         *        managing anything).
         * \see #jni_auto_local(JNIEnv *, const jchar *, jsize)
         * \see #jni_auto_local(JNIEnv *, jstring)
         */
        jni_auto_local();

        /**
         * \brief Constructs a new <dfn>jstring</dfn> within the JNI environment
         *        and commences managing it.
         * \param env JNI environment
         * \param unicode_chars Pointer to a string of at least <dfn>size</dfn>
         *                      16-bit Unicode characters
         * \param size Length of the string <dfn>unicode_chars</dfn> points to
         * \see #jni_auto_local()
         * \see #jni_auto_local(JNIEnv *, jstring)
         *
         * Constructs a new string using
         * <dfn>NewString(JNIEnv *, const jchar *, jsize)</dfn> and manages it
         * as an automatic local reference.
         */
        jni_auto_local(JNIEnv * env, const jchar * unicode_chars, jsize size);

        /**
         * \brief Constructs an automatic local reference managing an existing
         *        string (or an "empty" automatic local reference).
         * \param env JNI environment; may be <dfn>null</dfn> but only if
         *            <dfn>string</dfn> is also <dfn>null</dfn>
         * \param string String to manage (may be <dfn>null</dfn>)
         * \see #reset(JNIEnv *, jstring)
         * \see #jni_auto_local()
         * \see #jni_auto_local(JNIEnv *, const jchar *, jsize)
         *
         * \note
         * The code
         *
         *     jni_auto_local<jstring>(nullptr, nullptr);
         * will construct an "empty" automatic local reference. However, if that
         * is the desired result, it is preferable to use the parameterless
         * constructor #jni_auto_local().
         */
        jni_auto_local(JNIEnv * env, jstring string);

        ~jni_auto_local();

        //
        // ACCESSORS
        //

    public:

        /** \brief Implicit conversion to <dfn>jstring</dfn>.
         *  \return The <dfn>jstring</dfn> managed by this auto local. */
        operator jstring();

        //
        // MUTATORS
        //

    public:

        /**
         * \brief Stop managing the currently-managed <dfn>jstring</dfn>, if
         *        any, and either begin managing a different <dfn>jstring</dfn>
         *        or set this auto local empty.
         * \param env JNI environment; may be <dfn>null</dfn> but only if
         *            <dfn>string</dfn> is also <dfn>null</dfn>
         * \param string New string to manage (may be <dfn>null</dfn>)
         *
         * Before beginning to manage <dfn>string</dfn>, this auto local deletes
         * the reference to <dfn>jstring</dfn> it is currently managing, if any.
         *
         * The following code:
         *
         *     { // deliberate block
         *         jni_auto_local<jstring> x(env, a);
         *     }
         *     jni_auto_local<jstring> x;
         *
         * is roughly equivalent to:
         *
         *     jni_auto_local<jstring> x(env, a);
         *     x.reset(nullptr, nullptr);
         */
        void reset(JNIEnv * env, jstring string);
};

inline void jni_auto_local<jstring>::release(JNIEnv * env, jstring string)
{
    assert(env || ! string);
    if (string) env->DeleteLocalRef(string);
}

inline jni_auto_local<jstring>::jni_auto_local()
    : d_env(nullptr)
    , d_string(nullptr)
{ }

inline jni_auto_local<jstring>::jni_auto_local(JNIEnv * env,
                                               const jchar * unicode_chars,
                                               jsize size)
    : d_env(env)
    , d_string(env->NewString(unicode_chars, size))
{ assert(env && unicode_chars); }

inline jni_auto_local<jstring>::jni_auto_local(JNIEnv * env, jstring string)
    : d_env(env)
    , d_string(string)
{ assert(env || ! string); }

inline jni_auto_local<jstring>::~jni_auto_local()
{ release(d_env, d_string); }

inline jni_auto_local<jstring>::operator jstring()
{ return d_string; }

inline void jni_auto_local<jstring>::reset(JNIEnv * env, jstring string)
{
    JNIEnv * old_env(d_env);
    jstring old_string(d_string);
    d_env = env;
    d_string = string;
    release(old_env, old_string);
}

/**
 * \brief Automatic local reference to a Java string.
 * \author Victor Schappert
 * \since 20131103
 * \see jni_auto_local
 * \see jni_auto_local<jclass>
 * \see jni_auto_local<jobject>
 * \see jni_auto_local<jstring>
 *
 * As with all specializations of jni_auto_local, this class frees its
 * managed local reference on destruction via
 * <dfn>DeleteLocalRef(JNIEnv *, jobject)</dfn>.
 */
template<>
class jni_auto_local<jthrowable>
{
        //
        // DATA
        //

        JNIEnv    * d_env;
        jthrowable  d_throwable;

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructs an automatic local reference to the specified Java
         *        throwable.
         * \param env JNI environment
         * \param throwable JNI value representing a Java throwable local
         *                  reference.
         *
         * If <dfn>throwable</dfn> is <dfn>null</dfn>, this auto local will be
         * constructed "empty" and no action will be taken on destruction.
         */
        jni_auto_local(JNIEnv * env, jthrowable throwable);

        ~jni_auto_local();

        //
        // ACCESSORS
        //

    public:

        /** \brief Implicit conversion to <dfn>jthrowable</dfn>.
         *  \return The <dfn>jthrowable</dfn> managed by this auto local. */
        operator jthrowable();
};

inline jni_auto_local<jthrowable>::jni_auto_local(JNIEnv * env,
                                                  jthrowable throwable)
    : d_env(env)
    , d_throwable(throwable)
{ }

inline jni_auto_local<jthrowable>::~jni_auto_local()
{ if (d_throwable) d_env->DeleteLocalRef(d_throwable); }

inline jni_auto_local<jthrowable>::operator jthrowable()
{ return d_throwable; }

//==============================================================================
//                          class jni_auto_monitor
//==============================================================================

/**
 * \brief Automatic stack object for exception-safe simulation of Java
 *        <dfn>synchronized</dfn> blocks.
 * \author Victor Schappert
 * \since 20131101
 */
class jni_auto_monitor
{
        //
        // DATA
        //

        JNIEnv * d_env;
        jobject  d_object;

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Enters the monitor for a given object (it will be exited on
         *        destruction of this automatic object)
         * \param env JNI environment
         * \param object Object whose monitor is to be entered
         * \throws jni_exception If the monitor cannot be acquired
         */
        jni_auto_monitor(JNIEnv * env, jobject object);

        ~jni_auto_monitor() noexcept(false);
};

inline jni_auto_monitor::jni_auto_monitor(JNIEnv * env, jobject object)
    : d_env(env)
    , d_object(object)
{
    assert(env && object);
    if (0 != env->MonitorEnter(object))
        throw jni_exception("can't enter monitor", false);
}

inline jni_auto_monitor::~jni_auto_monitor()
{
    if (0 != d_env->MonitorExit(d_object))
        throw jni_exception("can't exit monitor", false);
}

//==============================================================================
//                       class jni_utf8_string_region
//==============================================================================

/**
 * \brief Managed array of JNI modified UTF-8 characters that were retrieved
 *        from a JNI string reference \em but which cannot be copied back into
 *        the JVM.
 * \author Victor Schappert
 * \since 20130709
 * \see jni_utf16_string_region
 * \see jni_utf16_ostream
 * \see jni_array_region
 *
 * On construction, the string region appends a UTF-8 null character to the end
 * of the Java characters. The string region is therefore zero-terminated.
 */
class jni_utf8_string_region : private non_copyable
{
        //
        // TYPES
        //

    public:

        /** \brief An unsigned integral type. */
        typedef size_t size_type;
        /** \brief Type of the characters in this string region.
         *  \see #const_value_type */
        typedef char value_type;
        /** \brief Type of a pointer to a #value_type. */
        typedef value_type * pointer;
        /** \brief Type of <dfn>const</dfn> characters in this string region.
         *  \see #value_type */
        typedef const char const_value_type;
        /** \brief Type of a pointer to a #const_value_type. */
        typedef const_value_type * const_pointer;

        //
        // DATA
        //

    private:

        size_type d_size_bytes;
        pointer   d_str;

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Fetches modified UTF-8 characters from a Java string into a
         *        new UTF-8 string region.
         * \param env JNI environment
         * \param str String to fetch UTF-8 characters for
         */
        jni_utf8_string_region(JNIEnv * env, jstring str);

        ~jni_utf8_string_region();

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Returns the number of \em bytes in the UTF-8 string region.
         * \return Number of bytes
         *
         * The value returned is not necessarily the number of modified UTF-8
         * \em characters in the string!
         *
         * \note
         * The value returned does not include the terminating null byte.
         */
        size_type size_bytes() const;

        /**
         * \brief Returns a pointer to the characters in the string region.
         * \return Pointer to first modified UTF-8 character stored in the
         *         string region.
         */
        const_pointer str() const;
};

inline jni_utf8_string_region::jni_utf8_string_region(JNIEnv * env,
                                                      jstring str)
    : d_size_bytes(env->GetStringUTFLength(str))
    , d_str(new char[d_size_bytes + 1])
{
    env->GetStringUTFRegion(str, 0, d_size_bytes, d_str);
    d_str[d_size_bytes] = '\0';
}

inline jni_utf8_string_region::~jni_utf8_string_region()
{ delete [] d_str; }

inline jni_utf8_string_region::size_type
jni_utf8_string_region::size_bytes() const
{ return d_size_bytes; }

inline jni_utf8_string_region::const_pointer
       jni_utf8_string_region::str() const
{ return d_str; }

//==============================================================================
//                      class jni_utf16_string_region
//==============================================================================

/**
 * \brief Managed array of Java UTF-16 characters that were retrieved from a
 *        JNI string reference \em but which cannot be copied back into the JVM.
 * \author Victor Schappert
 * \since 20130709
 * \see jni_utf8_string_region
 * \see jni_utf16_ostream
 * \see jni_array_region
 *
 * On construction, the string region appends a UTF-16 null character to the end
 * of the Java characters. The string region is therefore zero-terminated.
 */
class jni_utf16_string_region : private non_copyable
{
        //
        // TYPES
        //

    public:

        /** \brief An unsigned integral type. */
        typedef size_t size_type;
        /** \brief Type of the characters in this string region.
         *  \see #const_value_type */
        typedef utf16char_t value_type;
        /** \brief Type of a pointer to a #value_type. */
        typedef value_type * pointer;
        /** \brief Type of <dfn>const</dfn> characters in this string region.
         *  \see #value_type */
        typedef const utf16char_t const_value_type;
        /** \brief Type of a pointer to a #const_value_type. */
        typedef const_value_type * const_pointer;
        /** \brief A random access iterator to #const_value_type. */
        typedef const_pointer const_iterator;

        //
        // DATA
        //

    private:

        size_type d_size;
        pointer   d_str;

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Fetches UTF-16 characters from a Java string into a new UTF-16
         *        string region.
         * \param env JNI environment
         * \param str String to fetch UTF-16 characters for
         */
        jni_utf16_string_region(JNIEnv * env, jstring str);

        ~jni_utf16_string_region();

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Returns the number of characters in the UTF-16 string region.
         * \return Number of characters
         *
         * \note
         * The value returned does not include the terminating null character.
         */
        size_type size() const;

        /**
         * \brief Returns a pointer to the characters in the string region.
         * \return Pointer to first UTF-16 character stored in the string
         *         region.
         * \see #wstr() const
         */
        const_pointer str() const;

        /**
         * \brief Returns a pointer to the characters in the string region.
         * \return Pointer to first UTF-16 character stored in the string
         *         region.
         * \see #str() const
         */
         const wchar_t * wstr() const;
        // TODO: This member function is redundant and should be deleted...

        /**
         * \brief Returns an iterator pointing to the first character of the
         *        string region.
         * \return Iterator to first character
         * \see #end() const
         */
        const_iterator begin() const;

        /**
         * \brief Returns an iterator pointing to the \em past-the-end character
         *        of the string region.
         * \return Iterator that is one position past the end of the string
         * \see #begin() const
         */
        const_iterator end() const;
};

inline jni_utf16_string_region::jni_utf16_string_region(JNIEnv * env,
                                                        jstring str)
    : d_size(env->GetStringLength(str))
    , d_str(new utf16char_t[d_size + 1])
{
    assert(env && str);
    env->GetStringRegion(str, 0, d_size, reinterpret_cast<jchar *>(d_str));
    JNI_EXCEPTION_CHECK(env);
    d_str[d_size] = UTF16('\0');
}

inline jni_utf16_string_region::~jni_utf16_string_region()
{ delete [] d_str; }

inline jni_utf16_string_region::size_type jni_utf16_string_region::size() const
{ return d_size; }


inline jni_utf16_string_region::const_pointer
       jni_utf16_string_region::str() const
{ return d_str; }

inline const wchar_t * jni_utf16_string_region::wstr() const
{
    static_assert(
        sizeof(wchar_t) == sizeof(utf16char_t),
        "can't convert utf16char_t to wchar_t"
    );
    return reinterpret_cast<wchar_t *>(d_str);
}

inline jni_utf16_string_region::const_iterator jni_utf16_string_region::begin() const
{ return d_str; }

inline jni_utf16_string_region::const_iterator jni_utf16_string_region::end() const
{ return d_str + d_size; }

//==============================================================================
//                 stream insertion operators for JNI types
//==============================================================================

utf16_ostream& operator<<(utf16_ostream&, jstring) throw(std::bad_cast);

utf16_ostream& operator<<(utf16_ostream&, const jni_utf16_string_region&);

class jni_utf16_ostream;

jni_utf16_ostream& operator<<(jni_utf16_ostream&, jstring);

//==============================================================================
//                     class jni_utf16_output_streambuf
//==============================================================================

/** \cond internal */
class jni_utf16_output_streambuf : public utf16_streambuf, private non_copyable
{
        //
        // DATA
        //

        JNIEnv *                 d_env;
        std::vector<utf16char_t> d_buf;

        //
        // CONSTRUCTORS
        //

    public:

        jni_utf16_output_streambuf(JNIEnv * env, size_t capacity);

        //
        // ACCESSORS
        //

    public:

        jstring jstr() const;

        JNIEnv * env();

        //
        // ANCESTOR CLASS: std::streambuf
        //

    private:

        std::streamsize size() const;

        void expand(size_t);

        virtual int_type overflow(int_type);

        virtual std::streamsize xsputn(const utf16char_t *, std::streamsize);
};

inline JNIEnv * jni_utf16_output_streambuf::env()
{ return d_env; }

inline std::streamsize jni_utf16_output_streambuf::size() const
{ return pptr() - pbase(); }
/** \endcond */

//==============================================================================
//                          class jni_utf16_ostream
//==============================================================================

/**
 * \brief Output stream for generating immutable Java strings.
 * \author Victor Schappert
 * \since 20130701 (Happy Canada Day!)
 *
 * The values inserted into this stream should be compatible with JNI's
 * &quot;modified UTF-8&quot; format in order to ensure that the strings sent to
 * the Java side are valid.
 *
 * \note
 * If we are sending a lot of strings back to the Java side, it may be
 * worthwhile to refactor all of the string code in jsdi to use
 * <dfn>wchar_t</dfn>, <dfn>std::wstring</dfn>, and <dfn>std::wostream</dfn> in
 * order to eliminate an unnecessary conversion between 16-bit wide characters
 * and modified UTF-8.
 */
class jni_utf16_ostream : public utf16_ostream, private non_copyable
{
        //
        // DATA
        //

        jni_utf16_output_streambuf d_buf;

        //
        // FRIENDSHIPS
        //

        friend jni_utf16_ostream& jsdi::operator<<(jni_utf16_ostream&, jstring);

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * Constructs a new stream.
         * \param env Valid pointer to the JNI environment
         * \param capacity Suggested initial capacity of the stream (the more
         * accurate this is, the less reallocation or overallocation will be
         * done).
         */
        jni_utf16_ostream(JNIEnv * env, size_t capacity = 127);

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Returns a newly allocated <dfn>jstring</dfn> containing the
         * contents of the stream.
         * \return A JNI reference to an immutable Java string containing the
         * contents of the stream.
         */
        jstring jstr() const;
};

inline jni_utf16_ostream::jni_utf16_ostream(JNIEnv * env, size_t capacity)
    : basic_ostream(nullptr)
    , d_buf(env, capacity)
{ init(&d_buf); }

inline jstring jni_utf16_ostream::jstr() const
{ return d_buf.jstr(); }

jni_utf16_ostream& operator<<(jni_utf16_ostream&, jstring);

//==============================================================================
//                         string utility functions
//==============================================================================

/**
 * \brief Converts a zero-terminated string of 8-bit characters into a vector
 *        of 16-bit Java characters.
 * \param sz Non-NULL pointer to a zero-terminated string
 * \return Vector containing the wide character equivalents of the characters
 *         in <dfn>sz</dfn> <em>but without the zero-terminator</em>
 * \author Victor Schappert
 * \see make_jstring(JNIEnv *, jbyte)
 * \since 20130801
 */
std::vector<jchar> widen(const char * sz);

/**\cond internal */
jstring make_jstring(JNIEnv * env, const char * sz);
/**\endcond internal */

/**
 * \brief Converts a zero-terminated string of 8-bit characters into a Java
 *        string.
 * \param env JNI environment
 * \param sz Pointer to zero-terminated string
 * \return Reference to a Java string
 * \author Victor Schappert
 * \since 20130812
 * \throws jni_bad_alloc If the string cannot be constructed
 * \throws jni_exception If the JVM throws an OutOfMemoryError
 * \see widen(const char * sz)
 *
 * It is caller's responsibility to free the string returned.
 */
template<typename CharType>
inline jstring make_jstring(JNIEnv * env, const CharType * sz)
{
    static_assert(1 == sizeof(CharType),
                  "make_jstring() requires 8-bit character");
    return make_jstring(env, reinterpret_cast<const char *>(sz));
}

} // namespace jsdi

#endif // __INCLUDED_JNI_UTIL_H___

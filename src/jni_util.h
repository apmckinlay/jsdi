#ifndef __INCLUDED_JNI_UTIL_H___
#define __INCLUDED_JNI_UTIL_H___

/**
 * \file jni_util.h
 * \author Victor Schappert
 * \since 20130627
 * \brief Utility functions to simplify working with JNI.
 */

#include "util.h"

#include <jni.h>

#include <cassert>

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
 * <dd><dfn>array_type</dfn></dd>
 * <dt>
 * JNI opaque array reference type &mdash; \em eg <dfn>jbyteArray</dfn>.
 * </dt>
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
{
    env->GetByteArrayRegion(array, start, len, buf);
}
/** \endcond */

/**
 * \brief Managed array of a JNI primitive type which was retrieved from a JNI
 *        array reference \em but which cannot be copied back into the JVM.
 * \author Victor Schappert
 * \since 20130628
 * \tparam The JNI data type on which to specialize the array region &mdash;
 *         \em eg <dfn>jbyte</dfn>.
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

        /** \brief An unsigned integral type. */
        typedef jsize size_type;
        /** \brief Type of the region elements (a JNI primitive type, such as
         *         <dfn>jbyte</dfn>.
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
         *              (\em eg <dfn>jbyteArray</dfn>).
         */
        jni_array_region(JNIEnv * env,
                         typename jni_traits<JNIType>::array_type array);

        ~jni_array_region();

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Returns the number of elements in the region.
         * \return Number of elements in the region.
         */
        size_type size() const;

        /**
         * \brief Subscripts an element of the region.
         * \param n Zero-based index of the region element to return.
         * \return Read-only reference to the element at position <dfn>n</dfn>.
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

template<typename JNIType>
inline jni_array_region<JNIType>::jni_array_region(
    JNIEnv * env, typename jni_traits<JNIType>::array_type array)
    : d_size(env->GetArrayLength(array)), d_array(new value_type[d_size])
{
    jni_array_get_region(env, array, 0, d_size, d_array);
}

template<typename JNIType>
inline jni_array_region<JNIType>::~jni_array_region()
{
    delete[] d_array;
}

template<typename JNIType>
inline typename jni_array_region<JNIType>::size_type jni_array_region<JNIType>::size() const
{
    return d_size;
}

template<typename JNIType>
inline typename jni_array_region<JNIType>::const_reference jni_array_region<
    JNIType>::operator[](size_type n) const
{
    assert(0 <= n && n < d_size);
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

//==============================================================================
//                         class jni_primitive_array
//==============================================================================

template<typename JNIType>
inline typename jni_traits<JNIType>::pointer jni_array_get_elements(
    JNIEnv *, typename jni_traits<JNIType>::array_type, jboolean *);

template<>
inline jbyte * jni_array_get_elements<jbyte>(JNIEnv * env, jbyteArray array,
                                             jboolean * is_copy)
{
    return env->GetByteArrayElements(array, is_copy);
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

//==============================================================================
//                           class jni_auto_local
//==============================================================================

template<typename T>
class jni_auto_local;

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

        jni_auto_local(JNIEnv * env, const char * class_name);

        ~jni_auto_local();

        //
        // ACCESSORS
        //

    public:

        operator jclass();
};

inline jni_auto_local<jclass>::jni_auto_local(JNIEnv * env,
                                              const char * class_name)
    : d_env(env)
    , d_class(env->FindClass(class_name))
{ }

inline jni_auto_local<jclass>::~jni_auto_local()
{ if (d_class) d_env->DeleteLocalRef(d_class); }

inline jni_auto_local<jclass>::operator jclass()
{ return d_class; }

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

        jni_auto_local(JNIEnv * env, jobject object);

        jni_auto_local(JNIEnv * env, jclass clazz, jfieldID static_field_id);

        ~jni_auto_local();

        //
        // ACCESSORS
        //

    public:

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

}
 // namespace jsdi

#endif // __INCLUDED_JNI_UTIL_H___

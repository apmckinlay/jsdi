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
 * \tparam The JNI data type on which to specialize the array region &mdash;
 *         \em eg <dfn>jbyte</dfn>.
 * \see jni_array
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
        jni_array_region(JNIEnv * env,
                         typename jni_traits<JNIType>::array_type array);

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
         *
         * If the array region is empty, the returned iterator shall not be
         * dereferenced.
         */
        const_iterator cbegin() const;

        /**
         * \brief Returns a #const_iterator pointing to the \em past-the-end
         *        element of the array region.
         * \return A #const_iterator to the end of the array region
         * \see #cend() const
         *
         * The returned iterator shall not be dereferenced.
         */
        const_iterator cend() const;
};

template <typename JNIType>
inline jni_array_region<JNIType>::jni_array_region(JNIEnv * env,
                                                   array_type array)
    : d_size(env->GetArrayLength(array))
    , d_array(new value_type[d_size])
{
    jni_array_get_region<JNIType>(env, array, 0, d_size, d_array);
}

template <typename JNIType>
inline jni_array_region<JNIType>::jni_array_region(JNIEnv * env,
                                                   array_type array,
                                                   size_type size)
    : d_size(size)
    , d_array(new value_type[d_size])
{
    jni_array_get_region<JNIType>(env, array, 0, d_size, d_array);
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

//==============================================================================
//                              class jni_array
//==============================================================================

/** \cond internal */
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
/** \endcond */

/**
 * \brief Managed array of a JNI primitive type which was retrieved from a JNI
 *        array reference.
 * \author Victor Schappert
 * \since 20130727
 * \tparam The JNI data type on which to specialize the array region &mdash;
 *         \em eg <dfn>jbyte</dfn>.
 * \see jni_array_region
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
 * is destroyed.
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
         * \brief Constructor for an array containing all of the elements of the
         *        Java array.
         * \param env JNI environment
         * \param array Reference to a JNI primitive array of the correct type
         *              (\em eg <dfn>jbyteArray</dfn>)
         */
        jni_array(JNIEnv * env, array_type array);

        ~jni_array();

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Indicates whether the array data is a copy of the underlying
         * Java array.
         * \return Whether the array data is a copy of the &quot;primary&quot;
         * array held by the JVM (<dfn>true</dfn>); or whether this array
         * directly refers to the JVM data (<dfn>false</dfn>).
         */
        bool is_copy() const;

        /**
         * \brief Returns the number of elements in the array.
         * \return Number of elements in the array
         */
        size_type size() const;

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
{ assert(d_array); }

template <typename JNIType>
inline jni_array<JNIType>::~jni_array()
{ jni_array_release_elements<JNIType>(d_env, d_jarray, d_array, 0); }

template <typename JNIType>
inline bool jni_array<JNIType>::is_copy() const
{ return JNI_TRUE == d_is_copy; }

template <typename JNIType>
inline typename jni_array<JNIType>::size_type jni_array<JNIType>::size() const
{ return d_size; }

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

template<>
class jni_auto_local<jstring>
{
        //
        // DATA
        //

        JNIEnv * d_env;
        jstring  d_string;

        //
        // CONSTRUCTORS
        //

    public:

        jni_auto_local(JNIEnv * env, const jchar * unicode_chars,
                       const jsize size);

        ~jni_auto_local();

        //
        // ACCESSORS
        //

    public:

        operator jstring();
};

inline jni_auto_local<jstring>::jni_auto_local(JNIEnv * env,
                                               const jchar * unicode_chars,
                                               const jsize size)
    : d_env(env)
    , d_string(env->NewString(unicode_chars, size))
{ }

inline jni_auto_local<jstring>::~jni_auto_local()
{ d_env->DeleteLocalRef(d_string); }

inline jni_auto_local<jstring>::operator jstring()
{ return d_string; }

//==============================================================================
//                     class jni_utf16_output_streambuf
//==============================================================================

/** \cond internal */
class jni_utf16_output_streambuf : public std::basic_streambuf<char16_t>,
                                   private non_copyable
{
        //
        // DATA
        //

        JNIEnv *              d_env;
        std::vector<char16_t> d_buf;

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

        //
        // ANCESTOR CLASS: std::streambuf
        //

    private:

        virtual int_type overflow(int_type ch);
};

inline jstring jni_utf16_output_streambuf::jstr() const
{
    return d_env->NewString(reinterpret_cast<jchar *>(pbase()),
                            epptr() - pbase());
}
/** \endcond */

//==============================================================================
//                          class jni_utf8_ostream
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
 * and modified UTF8-.
 */
class jni_utf16_ostream : public std::basic_ostream<char16_t, std::char_traits<char16_t>>,
                          private non_copyable
{
        //
        // DATA
        //

        jni_utf16_output_streambuf d_buf;

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
    : d_buf(env, capacity)
{ init(&d_buf); }

inline jstring jni_utf16_ostream::jstr() const
{ return d_buf.jstr(); }

//==============================================================================
//                       class jni_utf8_string_region
//==============================================================================

// todo: docs
// since: 20130709
class jni_utf8_string_region : private non_copyable
{
        //
        // TYPES
        //

    public:

        typedef size_t size_type;

        typedef char value_type;

        typedef value_type * pointer;

        typedef const value_type * const_pointer;

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

        jni_utf8_string_region(JNIEnv * env, jstring str);

        ~jni_utf8_string_region();

        //
        // ACCESSORS
        //

    public:

        size_type size_bytes() const;

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

// todo: docs
// since: 20130709
class jni_utf16_string_region : private non_copyable
{
        //
        // TYPES
        //

    public:

        typedef size_t size_type;

        typedef char16_t value_type;

        typedef value_type * pointer;

        typedef const value_type * const_pointer;

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

        jni_utf16_string_region(JNIEnv * env, jstring str);

        ~jni_utf16_string_region();

        //
        // ACCESSORS
        //

    public:

        size_type size() const;

        const_pointer str() const;

        const wchar_t * wstr() const;
};

inline jni_utf16_string_region::jni_utf16_string_region(JNIEnv * env,
                                                        jstring str)
    : d_size(env->GetStringLength(str))
    , d_str(new char16_t[d_size + 1])
{
    env->GetStringRegion(str, 0, d_size, reinterpret_cast<jchar *>(d_str));
    d_str[d_size] = u'\0';
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
        sizeof(wchar_t) == sizeof(char16_t),
        "can't convert char16_t to wchar_t"
    );
    return reinterpret_cast<wchar_t *>(d_str);
}

} // namespace jsdi

#endif // __INCLUDED_JNI_UTIL_H___

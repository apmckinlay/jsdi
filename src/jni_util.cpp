/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: jni_util.cpp
// auth: Victor Schappert
// date: 20130627
// desc: Implementation of utility functions to simplify working with JNI.
//==============================================================================

#include "jni_util.h"

#include <cstring>

namespace jsdi {

//==============================================================================
//                     class jni_utf16_output_streambuf
//==============================================================================

jni_utf16_output_streambuf::jni_utf16_output_streambuf(JNIEnv * env,
                                                       size_t capacity)
    : d_env(env)
    , d_buf(capacity)
{
    assert(0 < capacity || !"Initial capacity must be more than zero");
    char_type * base = &d_buf.front();
    setp(base, base + d_buf.size());
}

jstring jni_utf16_output_streambuf::jstr() const
{
    jstring result = d_env->NewString(reinterpret_cast<jchar *>(pbase()),
                                      static_cast<jsize>(size()));
    JNI_EXCEPTION_CHECK(d_env);
    if (! result) throw jni_bad_alloc("NewString", __FUNCTION__);
    return result;
}

void jni_utf16_output_streambuf::expand(size_t min_capacity)
{
    auto new_size = smallest_pow2(min_capacity);
    auto old_size = d_buf.size();
    if (new_size < old_size || d_buf.max_size() < new_size)
        throw std::length_error("buffer overflow");
    d_buf.resize(new_size);
    char_type * base = &d_buf.front();
    setp(base, base + new_size);
    pbump(static_cast<int>(old_size));
}

jni_utf16_output_streambuf::int_type jni_utf16_output_streambuf::overflow(
    int_type ch)
{
    if (traits_type::eof() != ch)
    {
        expand(d_buf.size() + 1);
        *pptr() = traits_type::to_char_type(ch);
        pbump(1);
    }
    return ch;
}

std::streamsize jni_utf16_output_streambuf::xsputn(const utf16char_t * s,
                                                   std::streamsize count)
{
    assert(0 < count || !"count cannot be negative");
    size_t capacity = d_buf.size();
    size_t used = static_cast<size_t>(size());
    size_t free_space = capacity - used;
    if (free_space < static_cast<size_t>(count))
        expand(used + static_cast<size_t>(count) /* minimum capacity */);
    std::copy(s, s + count, pptr());
    pbump(static_cast<int>(count));
    return count;
}

//==============================================================================
//                 stream insertion operators for JNI types
//==============================================================================

utf16_ostream& operator<<(utf16_ostream& o, jstring jstr)
{
    auto& o2(dynamic_cast<jni_utf16_ostream&>(o)); // may throw std::bad_cast
    return o2 << jstr;
}

utf16_ostream& operator<<(utf16_ostream& o, const jni_utf16_string_region& str)
{
    utf16_ostream::sentry sentry(o);
    if (sentry) o.write(str.str(), str.size());
    return o;
}

jni_utf16_ostream& operator<<(jni_utf16_ostream& o, jstring jstr)
{
    jni_utf16_string_region str(o.d_buf.env(), jstr);
    o << str;
    return o;
}

//==============================================================================
//                         string utility functions
//==============================================================================

std::vector<jchar> widen(char const * sz)
{
    assert(sz || !"sz cannot be NULL");
    size_t N = std::strlen(sz);
    std::vector<jchar> wide(N);
    const char * end(sz + N);
    std::vector<jchar>::iterator o(wide.begin());
    for (; sz != end; ++sz, ++o)
    {
        *o = static_cast<jchar>(static_cast<unsigned char>(*sz));
    }
    return wide;
}

jstring make_jstring(JNIEnv * env, char const * sz)
{
    assert(env || !"environment cannot be NULL");
    assert(sz || !"string cannot be null");
    std::vector<jchar> const jchars(widen(sz));
    jsize const jlen(static_cast<jsize>(jchars.size()));
    jstring result = env->NewString(jchars.data(), jlen);
    // According to the NewString docs, it can either:
    //   -- return NULL (if the string "cannot be constructed"); or
    //   -- throw OutOfMemoryError if JVM runs out of memory
    JNI_EXCEPTION_CHECK(env);
    if (! result) throw jni_bad_alloc("NewString", __FUNCTION__);
    return result;
}

} // namespace jsdi

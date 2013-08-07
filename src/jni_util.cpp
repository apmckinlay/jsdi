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
//                      class jni_utf8_output_streambuf
//==============================================================================

jni_utf16_output_streambuf::jni_utf16_output_streambuf(JNIEnv * env,
                                                       size_t capacity)
    : d_env(env)
    , d_buf(capacity)
{
    assert(0 < capacity || !"Initial capacity must be more than zero");
    char16_t * base = &d_buf.front();
    setp(base, base + d_buf.size());
}

jni_utf16_output_streambuf::int_type jni_utf16_output_streambuf::overflow(
    int_type ch)
{
    if (traits_type::eof() != ch)
    {
        if (epptr() <= pptr())
        {
            size_t size = d_buf.size();
            d_buf.resize(2 * size);
            char16_t * base = &d_buf.front();
            setp(base, base + size - 1);
            pbump(size);
        }
        *pptr() = traits_type::to_char_type(ch);
        pbump(1);
        return ch;
    }
    else return traits_type::eof();
}

//==============================================================================
//                         string utility functions
//==============================================================================

std::vector<jchar> widen(const char * sz)
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

} // namespace jsdi

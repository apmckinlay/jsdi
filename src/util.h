//==============================================================================
// file: util.h
// auth: Victor Schappert
// date: 20130624
// desc: Utility functions used by the JSDI project.
//==============================================================================

#ifndef __INCLUDED_UTIL_H__
#define __INCLUDED_UTIL_H__

#include <cstddef>
#include <iterator>
#include <iosfwd>
#include <typeinfo>

namespace jsdi {

/**
 * \brief Non-copyable ancestor class.
 * \author Victor Schappert
 * \since 20130624
 *
 * The copy constructor and assignment operator of this class are disabled.
 * The same is true of every derived class. This prevents accidental copying of
 * classes for which the copy operation is not well-defined.
 *
 * Note there is no virtual destructor because you would never pass around
 * a pointer to a non-copyable object. However, for correctness' sake, please
 * inherit <em>privately</em> from this class.
 */
struct non_copyable
{
    non_copyable() = default;
    non_copyable(const non_copyable&) = delete;
    non_copyable& operator=(const non_copyable&) = delete;
};

/**
 * \brief Return the length of the array parameter.
 * \author Victor Schappert
 * \since 20130626
 *
 * <p>
 * Since the result can be computed at compile-time, this function is useful for
 * compile-time assertions, <em>ie</em>:
 * </p>
 * <pre>    static_assert(array_length(ARR) == x);</pre>
 * <p>
 * where <code>x</code> is a statically-available constant.
 * </p>
 */
template<typename T, size_t N>
constexpr size_t array_length(T(&)[N])
{ return N; }

/**
 * \brief Object which can be inserted into a stream in order to trigger a C++
 *        exception.
 * \author Victor Schappert
 * \since 20130628
 */
template<typename Exception, typename Arg = void>
struct throw_cpp
{
    const Arg& arg_;
    throw_cpp(const Arg& arg) : arg_(arg) { }
    void throw_(const std::string& str) const throw(Exception)
    { throw Exception(str, arg_); }
};

/**
 * \brief Partial specialization of {@link throw_cpp} for the usual case in
 *        which the exception takes no arguments.
 * \author Victor Schappert
 * \since 20130628
 */
template <typename Exception>
struct throw_cpp<Exception, void>
{
    void throw_(const std::string& str) const throw(Exception)
    { throw Exception(str); }
};

/**
 * \brief Stream insertion operator that throws an exception.
 * \author Victor Schappert
 * \since 20130627
 * \param o Stream to "insert" into
 * \return The return value is deliberately void because it is a semantic error
 *         to attempt to insert anything after an exception is thrown!
 * \throws std::bad_cast If o is not an output string stream.
 * \throws Exception If o is an output string stream.
 */
template <typename CharT, typename Traits, typename Exception, typename Arg>
void operator<<(
    std::basic_ostream<CharT, Traits>& o, const throw_cpp<Exception, Arg> & t)
        throw (Exception, std::bad_cast)
{
    typedef std::basic_ostringstream<CharT, Traits> stream_type;
    stream_type& s(dynamic_cast<stream_type&>(o)); // may throw std::bad_cast
    t.throw_(s.str());
}
} // namespace jsdi

#endif // __INCLUDED_UTIL_H__

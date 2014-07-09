/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_UTIL_H__
#define __INCLUDED_UTIL_H__

/**
 * \file util.h
 * \author Victor Schappert
 * \since 20130624
 * \brief Utility functions used by the JSDI project
 */

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <iosfwd>
#include <typeinfo>
#include <type_traits>

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
        /** \cond internal */
        non_copyable() = default;
        non_copyable(const non_copyable&) = delete;
        non_copyable& operator=(const non_copyable&) = delete;
        /** \endcond internal */
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
 * \tparam Exception Exception type whose constructor accepts a std::string as
 *                   its first argument.
 * \tparam Arg Type of the second argument to the exception type's constructor.
 * \see throw_cpp<Exception, void>
 */
template<typename Exception, typename Arg = void>
class throw_cpp
{
        const Arg& arg_;

    public:

        /**
         * \brief Constructs the exception thrower.
         * \param arg Reference to second argument to the exception type's
         *        constructor.
         */
        throw_cpp(const Arg& arg) : arg_(arg) { }

        /**
         * \brief Throws an instance of the exception type.
         * \param str First (message string) argument to exception type's
         *            constructor.
         * \throws Exception
         */
        void throw_(const std::string& str) const throw(Exception)
        { throw Exception(str, arg_); }
};

/**
 * \brief Partial specialization of {@link throw_cpp} for the usual case in
 *        which the exception takes no arguments.
 * \author Victor Schappert
 * \since 20130628
 * \tparam Exception Exception type whose constructor accepts a std::string as
 *                   its only argument.
 * \see throw_cpp
 *
 * The following code:
 * 
 *     std::ostringstream() << "oh no!" << throw_cpp<std::runtime_error>();
 *
 * is equivalent to:
 *
 *     std::ostringstream o;
 *     o << "oh no!";
 *     std::string s(o.str());
 *     throw std::runtime_error(s);
 * 
 */
template <typename Exception>
struct throw_cpp<Exception, void>
{
        /**
         * \brief Throws an instance of the exception type.
         * \param str Message string argument to exception type's constructor.
         * \throws Exception
         */
        void throw_(const std::string& str) const throw(Exception)
        { throw Exception(str); }
};

/**
 * \brief Stream insertion operator that throws an exception.
 * \author Victor Schappert
 * \since 20130627
 * \param o Stream to "insert" into
 * \param t Exception thrower to "insert"
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

/** \cond internal */
template <typename IntType>
void or_and_shift_remainder(IntType&);

template <>
inline void or_and_shift_remainder(uint8_t&) { }

template <>
inline void or_and_shift_remainder(uint16_t& x) { x |= x >> 010; }

template <>
inline void or_and_shift_remainder(uint32_t& x)
{
    x |= x >> 010;
    x |= x >> 020;
}

template <>
inline void or_and_shift_remainder(uint64_t& x)
{
    x |= x >> 010;
    x |= x >> 020;
    x |= x >> 040;
}
/** \endcond */

/**
 * \brief Determines the smallest power of two that is greater than or equal to
 *        a given number of unsigned integer type, if it can be represented.
 * \param x A number of unsigned integer type
 * \return The smallest power of two which is greater than or equal to
 *         <code>x</code> and can be represented by the type of x. If there is
 *         no such number, the return value is 0.
 * \since 20131105
 *
 * Inspiration: http://stackoverflow.com/a/1322548/1911388
 */
template <typename IntType>
inline IntType smallest_pow2(IntType x)
{
    static_assert(std::is_integral<IntType>::value,
                  "only unsigned integers allowed");
    static_assert(std::is_unsigned<IntType>::value,
                  "only unsigned integers allowed");
    --x;
    x |= x >> 001;
    x |= x >> 002;
    x |= x >> 004;
    or_and_shift_remainder(x);
    return x + 1;
}

} // namespace jsdi

#endif // __INCLUDED_UTIL_H__

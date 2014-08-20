/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_SEH_H__
#define __INCLUDED_SEH_H__

/**
 * \file seh.h
 * \author Victor Schappert
 * \since 20140819
 * \brief Code for catching Win32 structured exception handling (SEH) exceptions
 *        and rethrowing C++ exceptions of type jsdi::seh_exception
 */

#include "jsdi_windows.h"

#include <cassert>
#include <functional>
#include <stdexcept>

namespace jsdi {

//==============================================================================
//                           class seh_exception
//==============================================================================

/**
 * \brief Exception raised when an SEH exception is caught
 * \author Victor Schappert
 * \since 20140819
 * \see seh
 */
class seh_exception : public std::runtime_error
{
        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructs an exception using the exception record returned by
         *        the <code>ExceptionRecord</code> member of the information
         *        structure return by the special Win32
         *        <code>GetExceptionInformation()</code> macro
         * \param exception_record Valid exception record
         */
        seh_exception(EXCEPTION_RECORD const& exception_record);
};

//==============================================================================
//                                  MACROS
//==============================================================================

/**
 * \brief Starts an "SEH exception conversion" block in which Win32 structured
 *        exception handling exceptions will be caught and rethrown as C++
 *        exceptions
 * \author Victor Schappert
 * \since 20140819
 * \see SEH_CONVERT_TO_CPP_END
 * \see jsdi::seh_exception
 */
#define SEH_CONVERT_TO_CPP_BEGIN                            \
    __try                                                   \
    {

/**
 * \brief Ends an "SEH exception conversion" block that converts structured
 *        exception handling exceptions to C++ exceptions
 * \author Victor Schappert
 * \since 20140819
 * \see SEH_CONVERT_TO_CPP_BEGIN
 * \see jsdi::seh_exception
 */
#define SEH_CONVERT_TO_CPP_END                              \
    }                                                       \
    __except(jsdi::seh::filter(GetExceptionInformation()))  \
    {                                                       \
        jsdi::seh::convert_last_filtered_to_cpp();          \
        assert(!"control should never pass here");          \
    }

//==============================================================================
//                                struct seh
//==============================================================================

/**
 * \brief Utility functions for working with structured exception handling
 * \author Victor Schappert
 * \since 20140819
 * \see seh_exception
 */
struct seh
{
        /**
         * \brief Exception filter function for SEH <code>__except(...)</code>
         *        clause
         * \param info Valid pointer returned by
         *        <code>GetExceptionInformation()</code>
         * \return Non-zero if the exception described in <code>info</code>
         *         should be handled, zero if it should not be
         * \see #convert_last_filtered_to_cpp()
         *
         * \remark
         * In cSuneido, the exception handling code designates certain
         * exceptions as being severe enough that the runtime environment
         * process should immediately abort (<em>eg</em>
         * <code>STACK_OVERFLOW</code>, <code>GUARD_PAGE</code>). This is
         * <em>not</em> done in JSDI because the JVM can and should handle these
         * severe exceptions. For such exceptions, this filter simply returns 0
         * without doing anything else.
         */
        static int filter(struct _EXCEPTION_POINTERS const * info);

        /**
         * \brief Raises a seh_exception corresponding to the exception
         *        information passed to
         *        #filter(struct _EXCEPTION_POINTERS const *)
         * \throws seh_exception Always thrown since throwing it is the purpose
         *         of calling this function
         *
         * \warning
         * Do not call this function outside the handler portion of an SEH
         * <code>__except</code> clause.
         */
        static void convert_last_filtered_to_cpp();

        /**
         * \brief Wraps a function call in an SEH <code>__try/__except</code>
         *        block
         * \param func_ptr Pointer to the function to call
         * \param args Parameter pack containing arguments to
         *             <code>func_ptr</code>
         * \tparam ReturnType Return type of <code>func_ptr</code>
         * \throws seh_exception If calling <code>func_ptr</code> raises an
         *         SEH exception
         */
        template<typename ReturnType, typename ... Args>
        static ReturnType convert_to_cpp(
            ReturnType (* func_ptr)(Args ... args), Args ... args);

        /**
         * \brief Wraps a <code>std::function</code> call in an SEH
         *        <code>__try/__except</code> block
         * \param std_func Function to call
         * \tparam ReturnType Return type of <code>std_func</code>
         * \throws seh_exception If calling <code>func_ptr</code> raises an
         *         SEH exception
         */
        template<typename ReturnType>
        static ReturnType convert_to_cpp(
            const std::function<ReturnType()>& std_func);
};

template<typename ReturnType, typename ... Args>
inline ReturnType seh::convert_to_cpp(
    ReturnType (* func_ptr)(Args ... args), Args ... args)
{
    SEH_CONVERT_TO_CPP_BEGIN
    return func_ptr(args...);
    SEH_CONVERT_TO_CPP_END
    return ReturnType(); // Squelch compiler warning
}

template<typename ReturnType>
inline ReturnType seh::convert_to_cpp(
    const std::function<ReturnType()>& std_func)
{
    SEH_CONVERT_TO_CPP_BEGIN
    return std_func();
    SEH_CONVERT_TO_CPP_END
    return ReturnType(); // Squelch compiler warning
}

} // namespace jsdi

#endif // __INCLUDED_SEH_H__

/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_LOG_H___
#define __INCLUDED_LOG_H___

/**
 * \file log.h
 * \author Victor Schappert
 * \since 20140517
 * \brief Simple system for logging that can be switched on/off at compile time
 *        (static logging) and runtime (dynamic logging).
 */

#include "util.h"

#include <memory>
#include <mutex>

//==============================================================================
//                        #defines for static logging
//==============================================================================

/** \brief Value of #STATIC_LOG_THRESHOLD where all log messages are stripped
 *         at compile time. */
#define LOG_LEVEL_NONE 0
/**
 * \brief Value of #STATIC_LOG_THRESHOLD where all non-fatal log messages are
 *        stripped at compile time
 * \see LOG_FATAL(expr)
 */
#define LOG_LEVEL_FATAL 1
/**
 * \brief Value of #STATIC_LOG_THRESHOLD where all log messages whose priority
 *        level is not at least error are stripped at compile time
 * \see LOG_ERROR(expr)
 */
#define LOG_LEVEL_ERROR 2
/**
 * \brief Value of #STATIC_LOG_THRESHOLD where all log messages whose priority
 *        level is not at least warning are stripped at compile time
 * \see LOG_WARN(expr)
 */
#define LOG_LEVEL_WARN  3
/**
 * \brief Value of #STATIC_LOG_THRESHOLD where all log messages whose priority
 *        level is not at least information are stripped at compile time
 * \see LOG_INFO(expr)
 */
#define LOG_LEVEL_INFO  4
/**
 * \brief Value of #STATIC_LOG_THRESHOLD where all log messages whose priority
 *        level is not at least debug are stripped at compile time (i.e. only
 *        trace messages are stripped)
 * \see LOG_DEBUG(expr)
 */
#define LOG_LEVEL_DEBUG 5
/**
 * \brief Value of #STATIC_LOG_THRESHOLD where all log messages are included at
 *        compile time
 * \see LOG_TRACE(expr)
 */
#define LOG_LEVEL_TRACE 6

#ifndef STATIC_LOG_THRESHOLD
/**
 * \brief Minimum log level of messages that are not stripped at compile time
 * \author Victor Schappert
 * \since 20140517
 * \see LOG_LEVEL_FATAL
 * \see LOG_LEVEL_ERROR
 * \see LOG_LEVEL_WARN
 * \see LOG_LEVEL_INFO
 * \see LOG_LEVEL_DEBUG
 * \see LOG_LEVEL_TRACE
 * \see jsdi::log_level
 * \see jsdi::log_manager::threshold() const
 *
 * To override the static log threshold during the compile phase, simply use the
 * preprocessor or compiler's command-line interface to set the value of
 * <dfn>STATIC_LOG_THRESHOLD</dfn>. For example, if you wanted to avoid
 * compile-time stripping of all log messages of severity "DEBUG" or higher, you
 * might do something like:
 *
 *     $ cl.exe ... /DSTATIC_LOG_THRESHOLD=LOG_LEVEL_TRACE
 */
#define STATIC_LOG_THRESHOLD LOG_LEVEL_WARN
#endif

/**
 * \brief Logs a fatal error message at the current location.
 * \param expr Any expression that can appear on the left side of a stream
 *             insertion operator (e.g. <dfn>"str literal"</dfn> or
 *             <dfn>"strlit1" "strlit2" << 14 << std::endl</dfn>)
 *
 * See the documentation for LOG_INFO(expr) for an explanation of how and when
 * this macro will generate log output.
 */
#define LOG_FATAL(expr) (nullptr)
/**
 * \brief Logs a non-fatal error message at the current location.
 * \param expr Any expression that can appear on the left side of a stream
 *             insertion operator (e.g. <dfn>"str literal"</dfn> or
 *             <dfn>"strlit1" "strlit2" << 14 << std::endl</dfn>)
 *
 * See the documentation for LOG_INFO(expr) for an explanation of how and when
 * this macro will generate log output.
 */
#define LOG_ERROR(expr) (nullptr)
/**
 * \brief Logs a warning message at the current location.
 * \param expr Any expression that can appear on the left side of a stream
 *             insertion operator (e.g. <dfn>"str literal"</dfn> or
 *             <dfn>"strlit1" "strlit2" << 14 << std::endl</dfn>)
 *
 * See the documentation for LOG_INFO(expr) for an explanation of how and when
 * this macro will generate log output.
 */
#define LOG_WARN(expr)  (nullptr)
/**
 * \brief Logs an informational message at the current location.
 * \param expr Any expression that can appear on the left side of a stream
 *             insertion operator (e.g. <dfn>"str literal"</dfn> or
 *             <dfn>"strlit1" "strlit2" << 14 << std::endl</dfn>)
 * \see LOG_FATAL(expr)
 * \see LOG_ERROR(expr)
 * \see LOG_WARN(expr)
 * \see LOG_DEBUG(expr)
 * \see LOG_TRACE(expr)
 *
 * If STATIC_LOG_THRESHOLD is set to a value less than LOG_LEVEL_INFO, this macro
 * expands to a null expression. There will thus only be log output if two
 * conditions are met: (1) the STATIC_LOG_THRESHOLD is at least LOG_LEVEL_INFO;
 * and (2) the \link jsdi::log_manager::threshold() const dynamic log
 * threshold\endlink is at least jsdi::log_level::INFO. These conditions apply,
 * <i>mutatis mutandi</i> to the other <dfn>LOG_*</dfn> macros.
 */
#define LOG_INFO(expr)  (nullptr)
/**
 * \brief Logs a debug message at the current location.
 * \param expr Any expression that can appear on the left side of a stream
 *             insertion operator (e.g. <dfn>"str literal"</dfn> or
 *             <dfn>"strlit1" "strlit2" << 14 << std::endl</dfn>)
 *
 * See the documentation for LOG_INFO(expr) for an explanation of how and when
 * this macro will generate log output.
 */
#define LOG_DEBUG(expr) (nullptr)
/**
 * \brief Logs a trace  message at the current location.
 * \param expr Any expression that can appear on the left side of a stream
 *             insertion operator (e.g. <dfn>"str literal"</dfn> or
 *             <dfn>"strlit1" "strlit2" << 14 << std::endl</dfn>)
 *
 * See the documentation for LOG_INFO(expr) for an explanation of how and when
 * this macro will generate log output.
 */
#define LOG_TRACE(expr) (nullptr)

/** \cond internal */
#if LOG_LEVEL_FATAL <= STATIC_LOG_THRESHOLD
#define LOG_IF_LEVEL(level, expr)                                           \
{                                                                           \
    jsdi::log_manager& manager(jsdi::log_manager::instance());              \
    if (level <= manager.threshold())                                       \
    {                                                                       \
        std::lock_guard<jsdi::log_manager> l0ck(manager);                   \
        manager.stream(level, __FILE__, __LINE__, __func__) << expr         \
                                                            << std::endl;   \
    }                                                                       \
}
#undef LOG_FATAL
#define LOG_FATAL(expr) LOG_IF_LEVEL(jsdi::log_level::FATAL, expr)
#if LOG_LEVEL_ERROR <= STATIC_LOG_THRESHOLD
#undef LOG_ERROR
#define LOG_ERROR(expr) LOG_IF_LEVEL(jsdi::log_level::ERROR, expr)
#if LOG_LEVEL_WARN <= STATIC_LOG_THRESHOLD
#undef LOG_WARN
#define LOG_WARN(expr) LOG_IF_LEVEL(jsdi::log_level::WARN, expr)
#if LOG_LEVEL_INFO <= STATIC_LOG_THRESHOLD
#undef LOG_INFO
#define LOG_INFO(expr) LOG_IF_LEVEL(jsdi::log_level::INFO, expr)
#if LOG_LEVEL_DEBUG <= STATIC_LOG_THRESHOLD
#undef LOG_DEBUG
#define LOG_DEBUG(expr) LOG_IF_LEVEL(jsdi::log_level::DEBUG, expr)
#if LOG_LEVEL_TRACE <= STATIC_LOG_THRESHOLD
#undef LOG_TRACE
#define LOG_TRACE(expr) LOG_IF_LEVEL(jsdi::log_level::TRACE, expr)
#endif // LOG_LEVEL_TRACE <= STATIC_LOG_THRESHOLD
#endif // LOG_LEVEL_DEBUG <= STATIC_LOG_THRESHOLD
#endif // LOG_LEVEL_INFO  <= STATIC_LOG_THRESHOLD
#endif // LOG_LEVEL_WARN  <= STATIC_LOG_THRESHOLD
#endif // LOG_LEVEL_ERROR <= STATIC_LOG_THRESHOLD
#endif // LOG_LEVEL_FATAL <= STATIC_LOG_THRESHOLD
/** \endcond */

namespace jsdi {

//==============================================================================
//                              enum log_level
//==============================================================================

/**
 * \brief Enumerates possible dynamic log levels.
 * \author Victor Schappert
 * \since 20140517
 * \see log_manager::threshold() const
 * \see log_manager::set_threshold(log_level)
 * \see log_manager::stream(log_level, const char *, int, const char *)
 */
enum log_level
{
    /** \brief Does not correspond to an actual log level&mdash;when the
     *         dynamic log threshold is set here, nothing at all is logged
     *         (corresponds to the static log level #LOG_LEVEL_NONE) */
    NONE = LOG_LEVEL_NONE,
    /** \brief Fatal log level (corresponds to the static log level
     *  #LOG_LEVEL_FATAL) */
    FATAL = LOG_LEVEL_FATAL,
    /** \brief Error log level (corresponds to the static log level
     *  #LOG_LEVEL_ERROR) */
    ERROR = LOG_LEVEL_ERROR,
    /** \brief Warning log level (corresponds to the static log level
     *  #LOG_LEVEL_WARN) */
    WARN  = LOG_LEVEL_WARN,
    /** \brief Information message log level (corresponds to the static log
     *  level #LOG_LEVEL_INFO) */
    INFO  = LOG_LEVEL_INFO,
    /** \brief Debug message log level (corresponds to the static log level
     *  #LOG_LEVEL_DEBUG) */
    DEBUG = LOG_LEVEL_DEBUG,
    /** \brief Trace log level (corresponds to the static log level
     *  #LOG_LEVEL_TRACE) */
    TRACE = LOG_LEVEL_TRACE
};

/** \brief Stream insertion operator for log_level */
std::ostream& operator<<(std::ostream&, log_level);

//==============================================================================
//                             class log_manager
//==============================================================================

struct log_manager_impl;

/**
 * \brief Singleton class to manage logging
 * \author Victor Schappert
 * \since 20140517
 */
class log_manager : public non_copyable
{
        //
        // FRIENDSHIPS
        //

        /** \cond internal */
        friend class std::lock_guard<log_manager>;
        /** \endcond */

        //
        // DATA
        //

        std::unique_ptr<log_manager_impl>   d_impl;
        std::mutex                          d_mutex;
        log_level                           d_threshold;

        //
        // CONSTRUCTORS
        //

        log_manager();

        //
        // INTERNALS
        //

        void lock();

        void unlock();

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Queries the log file path
         * \return Current log file path
         * \see #set_path(const std::string&)
         */
        std::string path() const;

        /**
         * \brief Queries the dynamic log threshold
         * \return Minimum level of log messages that should be sent to the
         *         log stream
         * \see set_threshold(log_level)
         * \see STATIC_LOG_THRESHOLD
         * \note The static log threshold is set with STATIC_LOG_THRESHOLD
         */
        log_level threshold() const;

        /** \cond internal */
        std::ostream& stream(log_level level, const char * file_name,
                             int line_no, const char * func_name);
        /** \endcond */

        //
        // MUTATORS
        //

    public:

        /**
         * \brief Changes the log file path
         * \param log_file_path New log file path
         * \see #path() const
         * \see #set_threshold(log_level)
         *
         * This function closes the old log file, if it was open, but does not
         * delete or rename it. The new log file is opened on a lazy basis, so
         * it will not be opened until a log message is sent to it.
         */
        void set_path(const std::string& log_file_path);

        /**
         * \brief Sets the dynamic log level threshold
         * \param threshold Lowest priority log level that should actually be
         *        logged at runtime
         * \see #threshold() const
         * \see #set_path(const std::string&)
         *
         * If <dfn>threshold</dfn> is log_level::NONE, no messages at all will
         * be logged at runtime. If <dfn>threshold</dfn> is log_level::FATAL,
         * only fatal messages will be logged. If <dfn>threshold</dfn> is
         * log_level::TRACE, all messages will be logged.
         */
        void set_threshold(log_level threshold);

        //
        // STATICS
        //

    public:

        /**
         * \brief Returns the singleton instance
         * \return Singleton log manager
         */
        static log_manager& instance();
};

inline void log_manager::lock()
{ d_mutex.lock(); }

inline void log_manager::unlock()
{ d_mutex.unlock(); }

inline log_level log_manager::threshold() const
{ return d_threshold; }

} // jsdi

#endif // __INCLUDED_LOG_H___
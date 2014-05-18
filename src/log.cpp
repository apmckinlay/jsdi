/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#include "log.h"

#include "util.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <cassert>
#include <ctime>

namespace jsdi {

//==============================================================================
//                                  INTERNALS
//==============================================================================

namespace {

#define CHECK_LEVEL(level) \
    assert(log_level::NONE <= level && level <= log_level::TRACE);

#define DEFAULT_LOG_FILE_PATH "log"

log_level static_to_dynamic()
{
    static_assert(log_level::NONE <= STATIC_LOG_THRESHOLD &&
                  STATIC_LOG_THRESHOLD <= log_level::TRACE,
                  "invalid static log threshold");
    return static_cast<log_level>(STATIC_LOG_THRESHOLD);
}

} // anonymous namespace

//==============================================================================
//                              enum log_level
//==============================================================================

std::ostream& operator<<(std::ostream& o, log_level level)
{
    static const char * STR[] =
    { "NONE", "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE" };
    assert(array_length(STR) == 1 + log_level::TRACE || !"array size mismatch");
    // TODO: Make the above a static_assert as soon as MSVC supports constexpr
    //       properly.
    assert(NONE <= level && level <= TRACE);
    o << STR[level];
    return o;
}

//==============================================================================
//                          struct log_manager_impl
//==============================================================================

struct log_manager_impl
{
    std::string                     d_log_file_path;
    std::unique_ptr<std::ofstream>  d_stream;
    log_manager_impl()
        : d_log_file_path(DEFAULT_LOG_FILE_PATH)
    { }
};

//==============================================================================
//                             class log_manager
//==============================================================================

log_manager::log_manager()
    : d_impl(new log_manager_impl)
    , d_threshold(static_to_dynamic())
{ }

std::string log_manager::path() const
{
    std::lock_guard<log_manager> lock(const_cast<log_manager&>(*this));
    std::string log_file_path(d_impl->d_log_file_path);
    return log_file_path;
}

std::ostream& log_manager::stream(log_level level, const char * file_name,
                                  int line_no, const char * func_name)
{
    // NOTE: This function should only be called using the appropriate LOG_*
    //       macros. The manager must be locked before the function is called
    //       and the stream reference returned is only valid until the next call
    //       to ANY function of the log manager.
    CHECK_LEVEL(level);
    assert(file_name && func_name);
    // Ensure there's an open stream.
    if (! d_impl->d_stream)
    {
        std::unique_ptr<std::ofstream> stream(new std::ofstream(
            d_impl->d_log_file_path.c_str(), std::ios_base::app));
        if (stream->good())
            d_impl->d_stream = std::move(stream);
        else
            std::ostringstream() << "Failed to open log file path '"
                                 << d_impl->d_log_file_path << '\''
                                 << throw_cpp<std::runtime_error>();
    }
    std::ostream& o(*d_impl->d_stream);
    // Put the boilerplate into the stream. Borrowed this solution from
    // here: http://stackoverflow.com/a/17817079/1911388
    auto now(std::chrono::system_clock::now());
    time_t now_c(std::chrono::system_clock::to_time_t(now));
    o << std::put_time(std::localtime(&now_c), "%c") << '\t' << level
      << '\t' << file_name << ':' << line_no << '\t' << func_name << '\t';
    // Return the stream.
    return o;
}

void log_manager::set_path(const std::string& log_file_path)
{
    std::lock_guard<log_manager> lock(*this);
    if (log_file_path != d_impl->d_log_file_path)
    {
        d_impl->d_log_file_path = log_file_path;
        d_impl->d_stream.reset();
    }
}

void log_manager::set_threshold(log_level threshold)
{
    CHECK_LEVEL(threshold);
    std::lock_guard<log_manager> lock(*this);
    d_threshold = threshold;
}

log_manager& log_manager::instance()
{
    // NOTE: This should be thread-safe if compiled in Microsoft Visual C++
    //       November 2013 CTP or higher. This is based on thread-safe
    //       initialization of statics which is §6.7 of the C++11 standard (see
    //       here http://stackoverflow.com/a/11711991/1911388). Microsoft seems
    //       to refer to that part of the standard, and a number of others, as
    //       "Magic statics", which it lists as "YES" in the November 2013 CTP
    //       compiler release on this December 12, 2013 blog posting
    //       http://goo.gl/eyRS4i entitled "C++11/14 Core Language Features in
    //       VS 2013 and the Nov 2013 CTP".
    static log_manager manager;
    return manager;
}

} // jsdi


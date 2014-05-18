/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: main_exe.cpp
// auth: Victor Schappert
// date: 20130726
// desc: Executable entry-point for JSuneido DLL interface
//==============================================================================

#include "test.h"

#include <iostream>
#include <cstring>

namespace {

void exception(const char * suite_name, const char * test_name,
               const char * what)
{
    std::cerr << "main() caught exception running ";
    if (suite_name)
    {
        std::cerr << suite_name;
        if (test_name) std::cerr << '@' << test_name;
    }
    else std::cerr << "all tests";
    std::cerr << ": " << what << std::endl;
}

void run(const char * suite_name, const char * test_name, int& exit_code)
{
    try
    {
        if (nullptr == suite_name)
            jsdi::test_manager::instance().run_all();
        else if (nullptr == test_name)
            jsdi::test_manager::instance().run_suite(suite_name);
        else
            jsdi::test_manager::instance().run_test(suite_name, test_name);
        return;
    }
    catch (const std::exception& e)
    { exception(suite_name, test_name, e.what()); }
    catch (const char * e)
    { exception(suite_name, test_name, e); }
    catch (...)
    { exception(suite_name, test_name, "..."); }
    exit_code = -1; // Return -1 if any exception is thrown.
}

} // anonymous namespace

//  The command line should have the format
//      <exe> [test|suite]* (/jvm [jvm-arg]*)?
int main(int argc, char * argv[])
{
    int return_value(0);
    // Collect the JVM arguments, if any.
    for (int j = 1; j < argc; ++j)
    { 
        if ('/' == argv[j][0] && ! std::strcmp(argv[j] + 1, "jvm"))
        {
            jsdi::test_manager::instance().set_jvm_args(argv + j + 1,
                                                        argc - j - 1);
            argc = j;
            break;
        }
    }
    // Run the tests.
    if (argc < 2)
        run(nullptr, nullptr, return_value);
    else for (int k = 1; k < argc; ++k)
    {
        char * at_sign = std::strchr(argv[k], '@');
        if (at_sign)
        {
            *at_sign = '\0';
            run(argv[k], at_sign + 1, return_value);
        }
        else run(argv[k], nullptr, return_value);
    }
    jsdi::test_manager::instance().dump_report(std::cout);
    return jsdi::test_manager::instance().num_tests_failed() ? 1 : return_value;
}

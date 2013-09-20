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

int main(int argc, char * argv[])
{
    int return_value(0);
    if (argc < 2)
        jsdi::test_manager::instance().run_all();
    else for (int k = 1; k < argc; ++k)
    {
        char * at_sign = std::strchr(argv[k], '@');
        if (at_sign)
        {
            *at_sign = '\0';
            try
            { jsdi::test_manager::instance().run_test(argv[k], at_sign + 1); }
            catch (const std::exception& e)
            {
                std::cerr << e.what() << std::endl;
                return_value = -1;
            }
        }
        else try
        { jsdi::test_manager::instance().run_suite(argv[k]); }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            return_value = -1;
        }
    }
    jsdi::test_manager::instance().dump_report(std::cout);
    return jsdi::test_manager::instance().num_tests_failed() ? 1 : return_value;
}


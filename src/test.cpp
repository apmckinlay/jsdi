/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: test.cpp
// auth: Victor Schappert
// date: 20130725
// desc: Implementation of simple test framework
//==============================================================================

#include "test.h"

#ifndef __NOTEST__

#include <map>
#include <cstring>
#include <cstdlib>
#include <stdexcept>

namespace jsdi {

namespace {

struct cstr_less
{
    bool operator()(char const * const a, char const * const b) const
    { return std::strcmp(a, b) < 0; }
};

typedef std::map<const char *, std::shared_ptr<test>, cstr_less> test_map;
typedef std::map<const char *, test_map, cstr_less> suite_map;


} // anonymous namespace

struct test_manager_impl
{
    suite_map d_map;
    std::vector<test_failure> d_failures;
    std::shared_ptr<test> d_running_test;
    int d_num_tests;
    int d_num_tests_run;
    int d_num_tests_failed;
    test_manager_impl()
        : d_num_tests(0)
        , d_num_tests_run(0)
        , d_num_tests_failed(0)
    { }
    void init_run()
    {
        d_failures.clear();
        d_num_tests_run = 0;
        d_num_tests_failed = 0;
    }
    void run_test(const std::shared_ptr<test>& _test)
    {
        size_t failed_before = d_failures.size();
        d_running_test = _test;
        ++d_num_tests_run;
        try
        { _test->run(); }
        catch (...)
        {
            d_running_test.reset();
            ++d_num_tests_failed;
            throw;
        }
        d_running_test.reset();
        if (failed_before < d_failures.size()) ++d_num_tests_failed;
    }
    void add_failure(const std::string& output)
    { d_failures.push_back(test_failure(d_running_test, output)); }
    void add_failure(const char * which, const char * expr, const char * line)
    {
        std::string output("assert_");
        output.append(which);
        output.push_back('(');
        output.append(expr);
        output.append(") at line ");
        output.append(line);
        add_failure(output);
    }
};

//==============================================================================
//                                class test
//==============================================================================

test::~test()
{ }


void test::fail_assert(const char * which, const char * expr, const char * line)
{
    test_manager::instance().d_impl->add_failure(which, expr, line);
}

void test::fail_assert_equals(const char * a_expr, const std::string& a_str,
                              const char * b_expr, const std::string& b_str,
                              const char * line)
{
    std::string output("assert_equals(");
    output.append(a_expr);
    output.append(" => ");
    output.append(a_str);
    output.append(", ");
    output.append(b_expr);
    output.append(" => ");
    output.append(b_str);
    output.append(") at line ");
    output.append(line);
    test_manager::instance().d_impl->add_failure(output);
}

//==============================================================================
//                            class test_manager
//==============================================================================

test_manager::test_manager() : d_impl(new test_manager_impl) { }

int test_manager::num_tests_failed() const
{ return d_impl->d_num_tests_failed; }

void test_manager::dump_report(std::ostream& o) const
{
    std::vector<test_failure>::const_iterator i = d_impl->d_failures.begin(),
                                              e = d_impl->d_failures.end();
    std::string last_full_name;
    for (; i != e; ++i)
    {
        const std::string& full_name(i->get_test().full_name());
        if (full_name != last_full_name)
        {
            last_full_name = full_name;
            o << full_name << '\n';
        }
        o << '\t' << i->output() << '\n';
    }
    if (0 < d_impl->d_num_tests_failed)
        o << "FAILED " << d_impl->d_num_tests_failed;
    else
        o << "SUCCEEDED " << d_impl->d_num_tests_run;
    o << " OF " << d_impl->d_num_tests_run << std::endl;
}

void test_manager::register_test(std::shared_ptr<test>& _test)
{
    suite_map::iterator f = d_impl->d_map.lower_bound(_test->suite_name()),
                        e = d_impl->d_map.end();
    if (f == e || 0 != std::strcmp(_test->suite_name(), f->first))
        f = d_impl->d_map.insert(
            f, suite_map::value_type(_test->suite_name(), test_map()));
    test_map::iterator g = f->second.lower_bound(_test->test_name()),
                       h = f->second.end();
    if (g == h || 0 != std::strcmp(_test->test_name(), g->first))
    {
        f->second.insert(g, test_map::value_type(_test->test_name(), _test));
        ++d_impl->d_num_tests;
    }
    else throw std::logic_error("already registered: " + _test->full_name());
}

void test_manager::run_test(const char * suite_name, const char * test_name)
{
    suite_map::const_iterator f = d_impl->d_map.find(suite_name);
    if (d_impl->d_map.end() != f)
    {
        test_map::const_iterator g = f->second.find(test_name);
        if (f->second.end() != g)
        {
            d_impl->init_run();
            d_impl->run_test(g->second);
        }
        else throw std::logic_error(
            std::string("no such test: ") + suite_name + '@' + test_name);
    }
    else throw std::logic_error(std::string("no such suite: ") + suite_name);
}

void test_manager::run_suite(const char * suite_name)
{
    suite_map::const_iterator f = d_impl->d_map.find(suite_name);
    if (d_impl->d_map.end() != f)
    {
        d_impl->init_run();
        test_map::const_iterator i = f->second.begin(), e = f->second.end();
        for (; i != e; ++i) d_impl->run_test(i->second);
    }
    else throw std::logic_error(std::string("no such suite: ") + suite_name);
}

void test_manager::run_all()
{
    d_impl->init_run();
    suite_map::const_iterator i = d_impl->d_map.begin(),
                              e = d_impl->d_map.end();
    for (; i != e; ++i)
    {
        test_map::const_iterator j = i->second.begin(),
                                 f = i->second.end();
        for (; j != f; ++j) d_impl->run_test(j->second);
    }
}

test_manager& test_manager::instance()
{
    // Not thread-safe!
    static test_manager instance;
    return instance;
}

} // namespace jsdi

#endif // __NOTEST__

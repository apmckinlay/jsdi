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

#include "util.h"

#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
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
    std::vector<test_failure> d_failures; // May be more than one fail per test
    std::vector<test_failure> d_cancels;  // Max 1 cancel per test
    std::shared_ptr<test> d_running_test;
    std::vector<std::string> d_jvm_args;
    int d_num_tests;
    int d_num_tests_run;
    int d_num_tests_failed;
    bool d_has_jvm_args;
    test_manager_impl()
        : d_num_tests(0)
        , d_num_tests_run(0)
        , d_num_tests_failed(0)
        , d_has_jvm_args(false)
    { }
    void init_run()
    {
        d_failures.clear();
        d_cancels.clear();
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
        catch (const test_java_vm_create_error& e)
        {
            if (d_has_jvm_args)
                add_failure(e.what());
            else
                d_cancels.push_back(test_failure(d_running_test, e.what()));
        }
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
    if (i != e)
    {
        o << "-- failure messages --" << std::endl;
        for (; i != e; ++i)
        {
            const std::string& full_name(i->get_test().full_name());
            if (full_name != last_full_name)
            {
                last_full_name = full_name;
                o << '\t' << full_name << std::endl;
            }
            o << "\t\t" << i->output() << std::endl;
        }
    }
    i = d_impl->d_cancels.begin();
    e = d_impl->d_cancels.end();
    if (i != e)
    {
        o << "-- cancellation messages --" << std::endl;
        for (; i != e; ++i)
        {
            o << '\t' << i->get_test().full_name() << std::endl << "\t\t"
              << i->output() << std::endl;
        }
    }
    if (0 < d_impl->d_num_tests_failed)
        o << "FAILED " << d_impl->d_num_tests_failed;
    else if (0 < d_impl->d_cancels.size())
        o << "CANCELLED " << d_impl->d_cancels.size();
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

void test_manager::set_jvm_args(char * const argv[], int argc)
{
    d_impl->d_jvm_args.clear();
    d_impl->d_jvm_args.reserve(argc);
    for (int k = 0; k < argc; ++k)
        d_impl->d_jvm_args.push_back(std::string(argv[k]));
    d_impl->d_has_jvm_args = true;
}

test_manager& test_manager::instance()
{
    // Not thread-safe!
    static test_manager instance;
    return instance;
}

//==============================================================================
//                            class test_java_vm
//==============================================================================

test_java_vm::test_java_vm()
    : d_env(nullptr)
{
    // Any exceptions thrown by this constructor during a test::run() will be
    // caught by test_manager_impl::run_test(...) an processed appropriately.
    test_manager_impl * const impl = test_manager::instance().d_impl.get();
    // If user didn't specify to instantiate a JVM on the command-line, throw
    // a create exception.
    if (! impl->d_has_jvm_args)
    {
        std::ostringstream() << "No /jvm switch specified"
                             << throw_cpp<test_java_vm_create_error>();
    }
    // Otherwise, try to create the JVM.
    else
    {
        const size_t nopt(impl->d_jvm_args.size());
        JavaVMInitArgs vm_args;
        std::unique_ptr<JavaVMOption[]> options(new JavaVMOption[nopt]);
        vm_args.version             = JNI_VERSION_1_2;
        vm_args.options             = options.get();
        vm_args.nOptions            = nopt;
        vm_args.ignoreUnrecognized  = true;
        for (size_t k = 0; k < nopt; ++k)
        {
            char * str(const_cast<char *>(impl->d_jvm_args[k].c_str()));
            options[k].optionString = str;
            options[k].extraInfo = nullptr;
        }
        JavaVM * vm(nullptr);
        int result = JNI_CreateJavaVM(&vm, reinterpret_cast<void **>(&d_env),
                                      &vm_args);
        if (JNI_OK == result)
            d_java_vm.reset(vm, std::mem_fn(&JavaVM::DestroyJavaVM));
        else
        {
            std::ostringstream() << "Failed to create JVM: got error code "
                                 << result
                                 << throw_cpp<test_java_vm_create_error>();
        }
    }
    // If we get to the end of the constructor without an exception having been
    // thrown, both the virtual machine pointer and the environment pointer must
    // be valid.
    assert((d_java_vm && d_env) || !"failed to create JVM and JNI environment");
}

} // namespace jsdi

#endif // __NOTEST__

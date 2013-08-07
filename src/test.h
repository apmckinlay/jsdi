#ifndef __INCLUDED_TEST_H___
#define __INCLUDED_TEST_H___

/**
 * \file test.h
 * \author Victor Schappert
 * \since 20130725
 * \brief Simple framework for unit testing
 */

#ifndef __NOTEST__

#include <cassert>
#include <sstream>
#include <vector>
#include <string>
#include <memory>

namespace jsdi {

//==============================================================================
//                            class test_failure
//==============================================================================

class test;

class test_failure
{
        //
        // DATA
        //

    std::shared_ptr<test> d_test;
    std::string           d_output;

        //
        // CONSTRUCTORS
        //

    public:

        test_failure(std::shared_ptr<test>& _test, const std::string& output);

        //
        // ACCESSORS
        //

    public:

        const test& get_test() const;

        const std::string& output() const;
};

inline test_failure::test_failure(std::shared_ptr<test>& _test,
                                  const std::string& output)
    : d_test(_test)
    , d_output(output)
{ assert(_test); }

inline const test& test_failure::get_test() const
{ return *d_test; }

inline const std::string& test_failure::output() const
{ return d_output; }

//==============================================================================
//                                class test
//==============================================================================

class test_manager;

class test
{
        //
        // DATA
        //

        std::string    d_full_name;
        const char   * d_suite_name;
        const char   * d_test_name;

        //
        // FRIENDSHIPS
        //

        friend class test_manager;

        //
        // CONSTRUCTORS
        //

    public:

        test(const char * suite_name, const char * test_name);

        virtual ~test();

        //
        // ACCESSORS
        //

    public:

        const std::string& full_name() const;

        const char * suite_name() const;

        const char * test_name() const;

        virtual void run() = 0;

    protected:

        void fail_assert(const char * which, const char * expr,
                         const char * line);

        void fail_assert_equals(const char * a_expr, const std::string& a_str,
                                const char * b_expr, const std::string& b_str,
                                const char * line);
};

inline test::test(const char * suite_name, const char * test_name)
    : d_suite_name(suite_name)
    , d_test_name(test_name)
{
    assert(suite_name && test_name);
    d_full_name.append(d_suite_name);
    d_full_name.push_back('@');
    d_full_name.append(d_test_name);
}

inline const std::string& test::full_name() const
{ return d_full_name; }

inline const char * test::suite_name() const
{ return d_suite_name; }

inline const char * test::test_name() const
{ return d_test_name; }

//==============================================================================
//                            class test_manager
//==============================================================================

struct test_manager_impl;

class test_registrar;

class test_manager
{
        //
        // DATA
        //

        std::shared_ptr<test_manager_impl> d_impl;

        //
        // FRIENDSHIPS
        //

        friend class test;

        //
        // CONSTRUCTORS
        //

    public:

        test_manager();

        //
        // ACCESSORS
        //

    public:

        int num_tests_failed() const;

        void dump_report(std::ostream& o) const;

        //
        // MUTATORS
        //

    public:

        void register_test(std::shared_ptr<test>& _test);

        void run_test(const char * suite_name, const char * test_name);

        void run_suite(const char * suite_name);

        void run_all();

        //
        // SINGLETON INSTANCE
        //

    public:

        static test_manager& instance();
};

//==============================================================================
//                             class test_holder
//==============================================================================

class test_registrar
{
        //
        // DATA
        //

        std::shared_ptr<test> d_test;

        //
        // CONSTRUCTORS
        //

    public:

        test_registrar(test * _test);
};

inline test_registrar::test_registrar(test * _test) : d_test(_test)
{ test_manager::instance().register_test(d_test); }

} // namespace jsdi

//==============================================================================
//                                  MACROS
//==============================================================================

namespace { // anonymous namespace

template<typename T>
inline std::string runtime_stringify(const T& t)
{
    std::ostringstream o;
    o << t;
    return o.str();
}

} // anonymous namespace

#define stringify_int_(x) #x

#define stringify_int(x) stringify_int_(x)

#define assert_true(expr) \
    { if (! expr) fail_assert("true", #expr, stringify_int(__LINE__)); }

#define assert_false(expr) \
    { if (expr) fail_assert("false", #expr, stringify_int(__LINE__)); }

#define assert_equals(a, b)           \
    {                                 \
    if (a != b)                       \
        fail_assert_equals(           \
            #a, runtime_stringify(a), \
            #b, runtime_stringify(b), \
            stringify_int(__LINE__)   \
        );                            \
    }

#define TEST(name, ...)                                          \
namespace tests {                                                \
                                                                 \
struct test_ ## name : public jsdi::test                         \
{                                                                \
    test_ ## name() : test(__FILE__, #name) { }                  \
    void run()                                                   \
    {                                                            \
        __VA_ARGS__                                              \
    }                                                            \
};                                                               \
jsdi::test_registrar test_ ## name ## __(new test_ ## name);     \
                                                                 \
} /* namespace tests */

#endif // #ifndef __NOTEST__

#endif // __INCLUDED_TEST_H__

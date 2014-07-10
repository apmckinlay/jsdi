/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

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

#include <jni.h>

namespace jsdi {

//==============================================================================
//                            class test_failure
//==============================================================================

class test;

/**
 * \brief Encapsulates a failed \link test\endlink.
 * \author Victor Schappert
 * \since 20130726
 */
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

        /**
         * \brief Constructs a test failure object
         * \param _test Pointer to failed test
         * \param output Message describing the failure
         */
        test_failure(const std::shared_ptr<test>& _test,
                     const std::string& output);

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Returns a reference to the failed test.
         * \return Failed test
         */
        const test& get_test() const;

        /**
         * \brief Returns a reference to the failure description
         * \return Test output string
         */
        const std::string& output() const;
};

inline test_failure::test_failure(const std::shared_ptr<test>& _test,
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

/**
 * \brief Abstract base class for tests
 * \author Victor Schappert
 * \since 20130726
 *
 * Concrete testing classes are typically derived from this class via the
 * \link TEST(name, ...)\endlink macro.
 */
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

        /**
         * \brief Constructs a test
         * \param suite_name Non-<code>null</code> pointer to zero-terminated
         *        string containing the name of the suite the test belongs to
         * \param test_name Non-<code>null</code> pointer to zero-terminated
         *        string containing the name of the test
         *
         * The preferable way to create tests is with the TEST(name, ...) macro.
         * Using that system, the <code>suite_name</code> is the name of the
         * file containing the test, and the <code>test_name</code> is the
         * <code>name</code> argument to the macro.
         */
        test(const char * suite_name, const char * test_name);

        virtual ~test();

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Returns a string containing #suite_name() const +
         *        <code>\@</code> + #test_name() const&mdash<em>ie</em>
         *        "<code>file.cpp@test3</code>"
         * \return Full name of the test
         * \see #suite_name() const
         * \see #test_name() const
         */
        const std::string& full_name() const;

        /**
         * \brief Returns the suite name
         * \return Suite name
         * \see #test_name() const
         * \see #full_name() const
         */
        const char * suite_name() const;

        /**
         * \brief Returns the test name
         * \return Test name
         * \see #suite_name() const
         * \see #full_name() const
         */
        const char * test_name() const;

        /**
         * \brief Runs the test
         *
         * \attention
         * Do not override this function directly. Use the TEST(name, ...)
         * macro.
         *
         * \attention
         * Do not call this function directly. The test should be registered
         * via TEST(name, ...) and invoked using the appropriate
         * <code>run_*()</code> function of the test_manager::instance().
         */
        virtual void run() = 0;

    protected:

        /** \cond internal */
        void fail_assert(const char * which, const char * expr,
                         const char * line);

        void fail_assert_equals(const char * a_expr, const std::string& a_str,
                                const char * b_expr, const std::string& b_str,
                                const char * line);
        /** \endcond internal */
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

class test_java_vm;

/**
 * \brief Stores test parameters, runs tests, and stores results
 * \author Victor Schappert
 * \since 20130726
 *
 * This class is a singleton: see instance()
 */
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
        friend class test_java_vm;

        //
        // CONSTRUCTORS
        //

    public:

        test_manager();

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Gives number of tests that failed in the last call to one of
         *        the <code>run_*()</code> functions
         * \return Number of tests having at least one assertion failure
         *
         * This function returns the number of <em>tests</em> that had at least
         * one failure, not the aggregate number of failures (a test run may
         * produce more than one failure).
         */
        int num_tests_failed() const;

        /**
         * \brief Prints a simple textual summary of the results of the last
         *        <code>run_*()</code> call
         * \param o Reference to an output stream to receive the results
         */
        void dump_report(std::ostream& o) const;

        //
        // MUTATORS
        //

    public:

        /**
         * \brief Places a test under management so it can be run in all future
         *        calls to a <code>run_*()</code> function
         * \param _test Pointer to the test to manage
         */
        void register_test(std::shared_ptr<test>& _test);

        /**
         * \brief Runs an individual test
         * \param suite_name String indicating the test::suite_name() of the
         *        test to run
         * \param test_name String indicatnig the test::test_name() of the test
         *        to run
         * \throws std::logic_error If no test with the given suite and test
         *         name has been registered
         * \throws Any exception thrown by test code
         * \see run_suite(const char *)
         * \see run_all()
         */
        void run_test(const char * suite_name, const char * test_name);

        /**
         * \brief Runs all tests belonging to a particular suite
         * \param suite_name String indicating the test::suite_name() of the
         *        tests to run
         * \throws std::logic_error If no tests with the given suite name have
         *         been registered
         * \throws Any exception thrown by test code
         * \see run_test(const char *, const char *)
         * \see run_all()
         */
        void run_suite(const char * suite_name);

        /**
         * \brief Runs all registered tests
         * \throws Any exception thrown by test code
         * \see run_test(const char *, const char *)
         * \see run_suite(const char *)
         */
        void run_all();

        /**
         * \brief Sets the command-line arguments to be passed to the JVM on
         *        creation
         * \param argc Non-negative count of strings in <code>argv</code>
         * \param argv Non-<code>null</code> pointer to an array of
         *        <code>argc</code> zero-terminated strings
         *
         * This function <em>copies</em> the arguments. They are then passed to
         * the JVM creation routine every time a jsdi::test_java_vm is
         * constructed.
         */
        void set_jvm_args(int argc, char * const argv[]);

        //
        // SINGLETON INSTANCE
        //

    public:

        /**
         * \brief Returns singleton instance
         * \return Reference to test manager singleton instance
         */ 
        static test_manager& instance();
};

//==============================================================================
//                           class test_registrar
//==============================================================================

/** \cond internal */
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
/** \endcond internal */

//==============================================================================
//                     class test_java_vm_create_error
//==============================================================================

/**
 * \brief Exception thrown when a jsdi::test_java_vm cannot be constructed
 * \author Victor Schappert
 * \since 20140510
 */
struct test_java_vm_create_error : public std::runtime_error
{
        //
        // CONSTRUCTORS
        //

        /** \cond internal */
        test_java_vm_create_error(const std::string& what);
            // TODO: When MSVC finally gets support for inheriting constructors,
            //       just replace this with
            //           "using std::runtime_error::runtime_error"
        /** \endcond internal */
};

/** \cond internal */
inline test_java_vm_create_error::test_java_vm_create_error(const std::string& what)
    : runtime_error(what)
{ } // TODO: Can delete when MSVC++ adds support for inheriting constructors
/** \endcond internal */

//==============================================================================
//                            class test_java_vm
//==============================================================================

/**
 * \brief Automatic managed object for obtaining a JVM for testing purposes
 * \author Victor Schappert
 * \since 20140510
 */
class test_java_vm
{
        //
        // DATA
        //

        std::shared_ptr<JavaVM> d_java_vm;
        JNIEnv *                d_env;

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructs a JVM
         * \throws test_java_vm_create_error If JVM creation fails
         *
         * A JVM may fail to be created in any of the following scenarios:
         * - There are no JVM arguments because
         *   test_manager::set_jvm_args(int, char * const []) has not been
         *   called yet
         * - The JVM shared library cannot be loaded
         * - The JVM creation function cannot be located within the shared
         *   library
         * - The JVM creation function itself fails
         */
        test_java_vm();

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Returns a pointer to the managed JVM
         * \return Pointer to managed JVM
         * \see env_of_creating_thread()
         *
         * This function is guaranteed to return a valid pointer to an existing
         * JVM.
         *
         * \warning
         * Do not attempt to destroy the JVM. It will be destroyed when this
         * object is destroyed.
         */
        JavaVM * java_vm();

        /**
         * \brief Returns a pointer to the JNI environment of the thread that
         *        instantiated <code>this</code>
         * \return Pointer  to JNI environment
         * \see #java_vm()
         */
        JNIEnv * env_of_creating_thread();
};

inline JavaVM * test_java_vm::java_vm()
{ return d_java_vm.get(); }

inline JNIEnv * test_java_vm::env_of_creating_thread()
{ return d_env; }

} // namespace jsdi

//==============================================================================
//                                  MACROS
//==============================================================================

/** \cond internal */
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
/** \endcond internal */

/**
 * \brief Asserts <code>expr</code> is logically true within the context of
 *        \link TEST(name, ...)\endlink
 * \author Victor Schappert
 * \since 20130726
 * \see TEST(name, ...)
 * \see assert_false(expr)
 * \see assert_equals(a, b)
 *
 * If <code>(! expr)</code> then a jsdi::test_failure is recorded for the 
 * current test.
 */
#define assert_true(expr) \
    { if (! (expr)) fail_assert("true", #expr, stringify_int(__LINE__)); }

/**
 * \brief Asserts <code>expr</code> is logically false within the context of
 *        \link TEST(name, ...)\endlink
 * \author Victor Schappert
 * \since 20130726
 * \see TEST(name, ...)
 * \see assert_true(expr)
 * \see assert_equals(a, b)
 *
 * If <code>(expr)</code> then a jsdi::test_failure is recorded for the current
 * test.
 */
#define assert_false(expr) \
    { if (expr) fail_assert("false", #expr, stringify_int(__LINE__)); }

/**
 * \brief Asserts <code>(a == b)</code> within the context of
 *        \link TEST(name, ...)\endlink
 * \author Victor Schappert
 * \since 20130726
 * \see TEST(name, ...)
 * \see assert_true(expr)
 * \see assert_false(expr)
 *
 * If <code>(a != b)</code> then a jsdi::test_failure is recorded for the
 * current test.
 */
#define assert_equals(a, b)           \
    {                                 \
    if ((a) != (b))                   \
        fail_assert_equals(           \
            #a, runtime_stringify(a), \
            #b, runtime_stringify(b), \
            stringify_int(__LINE__)   \
        );                            \
    }

/**
 * \brief Declares a test with the given name
 * \author Victor Schappert
 * \since 20130728
 * \see assert_equals(a, b)
 * \see assert_true(expr)
 * \see assert_false(expr)
 *
 * Example usage:
 *
 *     TEST(simple_test,
 *         int x = 5;
 *         int * p(&x);
 *         assert_equals(*p, 5);
 *     );
 */
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

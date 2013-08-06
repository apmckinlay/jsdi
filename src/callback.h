#ifndef __INCLUDED_CALLBACK_H___
#define __INCLUDED_CALLBACK_H___

/**
 * \file callback.h
 * \author Victor Schappert
 * \since 20130804
 * \brief Generic interface for a callback function
 */

#include <vector>
#include <cassert>

namespace jsdi {

//==============================================================================
//                            class callback_args
//==============================================================================

/**
 * \brief Interface for argument state required by
 *        \link callback::call(const char *) \endlink
 * \author Victor Schappert
 * \since 20130804
 * \see callback
 * \see test_callback_args
 * \see jsdi_callback_args_basic
 * \see jsdi_callback_args_vi
 *
 * Implementors of the \link callback \endlink protocol should implement an
 * appropriate version of this class. For example, the
 * \link jsdi_callback_args_basic \endlink class maintains argument state for
 * \link jsdi_callback_basic \endlink in a manner suitable for calling back into
 * the JSDI Java classes via JNI.
 */
class callback_args
{
        //
        // DATA
        //

        char * d_data;

        //
        // CONSTRUCTORS
        //

    protected:

        callback_args();

    public:

        virtual ~callback_args();

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Returns a pointer to the argument data array (direct and
         * indirect storage).
         * \return Argument data array
         * \see #set_data(char *)
         *
         * Implementing subclasses must ensure that this function always returns
         * a pointer to their internal data array by making appropriate calls to
         * #set_data(char *).
         */
        char * data();

        /**
         * \brief Stores a string in the arguments' variable indirect storage.
         * \param str Zero-terminated string to store (may not be NULL)
         * \param vi_index Zero-based index of the variable indirect storage
         *        slot in which to place the string
         *
         * Implementing subclasses must implement this function in order to
         * store <dfn>str</dfn> in the manner appropriate for their
         * \link callback \endlink implementation.
         *
         * The parameter <dfn>str</dfn> will always be a non-NULL pointer to a
         * zero-terminated string. Implementing subclasses should assume that if
         * <dfn>vi_string_ptr(...)</dfn> is not called for a given
         * <dfn>vi_index</dfn> that that index in the variable indirect storage
         * should contain a NULL pointer.
         */
        virtual void vi_string_ptr(const char * str, int vi_index) = 0;

        //
        // MUTATORS
        //

    protected:

        /**
         * \brief Sets the internal data pointer.
         * \param data Data pointer
         * \see #data()
         */
        void set_data(char * data);
};

inline callback_args::callback_args() : d_data(0) { }

inline char * callback_args::data()
{ return d_data; }

inline void callback_args::set_data(char * data)
{ d_data = data; }

//==============================================================================
//                              class callback
//==============================================================================

/**
 * \brief Interface for a callback function
 * \author Victor Schappert
 * \since 20130804
 * \see test_callback
 * \see jsdi_callback_basic
 * \see jsdi_callback_vi
 *
 * When #call(const char *) is invoked, this class unmarshalls the arguments
 * (by which I mean it copies all direct and indirect arguments into a single
 * contiguous data block and copies variable indirect arguments into an
 * appropriate location) the arguments and stores them in an appropriate
 * implementation of \link callback_args \endlink returned by #alloc_args()
 * const. Once the arguments are unmarshalled, #call(const char *) invokes
 * #call(const callback_args&), which implementing subclasses must implement
 * in order to cause the desired function to be invoked.
 */
class callback
{
        //
        // DATA
        //

    protected:

        std::vector<int> d_ptr_array;
        int              d_size_direct;
        int              d_size_indirect;
        int              d_size_total;
        int              d_vi_count;

        //
        // INTERNALS
        //

    private:

        bool is_vi_ptr(int ptd_to_pos) const;

        void vi_ptr(callback_args& args, int ptr_pos, int ptd_to_pos);

        void normal_ptr(callback_args& args, int ptr_pos, int ptd_to_pos,
                        std::vector<int>::const_iterator& i,
                        std::vector<int>::const_iterator& e);

        //
        // CONSTRUCTORS
        //

    public:

        /**
         * \brief Constructs a callback with a given set of unmarshalling
         * parameters.
         * \param size_direct Size of the on-stack arguments
         * \param size_indirect Size of data indirectly accessible from
         * pointers passed on the stack
         * \param ptr_array Array of tuples indicating which positions in the
         * direct and indirect storage are pointers, and which positions they
         * point to
         * \param ptr_array_size Size of <dfn>ptr_array</dfn> (must be even)
         * \param vi_count Number of variable indirect pointers that must be
         * unmarshalled
         * \see #set_unmarshall_info(int, int, const int *, int , int)
         */
        callback(int size_direct, int size_indirect, const int * ptr_array,
                 int ptr_array_size, int vi_count);

        virtual ~callback();

        //
        // ACCESSORS
        //

    public:

        /**
         * \brief Returns the size of the callback's on-stack arguments in
         *  bytes.
         * \return Direct argument size in bytes
         * \see #set_unmarshall_info(int, int, const int *, int, int)
         */
        int size_direct() const;

    protected:

        /**
         * \brief Allocates an instance of \link callback_args \endlink suitable
         * for storing the unmarshalled arguments.
         * \return Container for argument state
         */
        virtual callback_args * alloc_args() const = 0;

        //
        // MUTATORS
        //

    public:

        /**
         * \brief Modifies the unmarshalling parameters.
         * \see #callback(int, int, const int *, int, int)
         * \see stdcall_thunk#set_unmarshall_params(int,int,const int *,int,int)
         */
        void set_unmarshall_params(int size_direct, int size_indirect,
                                   const int * ptr_array, int ptr_array_size,
                                   int vi_count);

        /**
         * \brief Unmarshalls the parameters and invokes
         * #call(const callback_args&).
         * \param args On-stack arguments to the callback function
         * \see #call(const callback_args&)
         */
        long call(const char * args);

    protected:

        /**
         * \brief Passes the unmarshalled arguments to the final call routine.
         * \param args Reference to the \link callback_args\endlink returned by
         * #alloc_args() const during the current invocation of
         * #call(const char *)
         */
        virtual long call(callback_args& args) = 0;
};

inline int callback::size_direct() const { return d_size_direct; }

} // namespace jsdi

#ifndef __NOTEST__

#include "util.h"

#include <string>
#include <memory>

namespace jsdi {

//==============================================================================
//                         class test_callback_args
//==============================================================================

struct test_callback_args_impl;

/**
 * \brief Implementation of \link callback_args \endlink used by
 * \link test_callback\endlink for testing purposes.
 * \author Victor Schappert
 * \since 20130805
 * \see test_callback
 */
class test_callback_args : public callback_args, private non_copyable
{
        //
        // TYPES
        //

    public:

        typedef std::vector<std::shared_ptr<std::string>> vi_vector;

    private:

        //
        // DATA
        //

        std::vector<char>   d_data;
        vi_vector           d_vi_array;

        //
        // CONSTRUCTORS
        //

    public:

        test_callback_args(int data_size, int vi_count);

        ~test_callback_args();

        //
        // ACCESSORS
        //

    public:

        const vi_vector& vi_array() const;

        //
        // MUTATORS
        //

    protected:

        virtual void vi_string_ptr(const char * str, int vi_index);

        //
        // STATICS
        //

    public:

        static const test_callback_args& last_value();
};

inline test_callback_args::test_callback_args(int data_size, int vi_count)
    : d_data(data_size, 0xff)
    , d_vi_array(vi_count)
{ set_data(d_data.data()); }

inline const test_callback_args::vi_vector& test_callback_args::vi_array() const
{ return d_vi_array; }

//==============================================================================
//                             class test_callback
//==============================================================================

/**
 * \brief Implementation of \link callback\endlink for testing purposes.
 * \author Victor Schappert
 * \since 20130805
 */
class test_callback : public callback, private non_copyable
{
    public:

        //
        // CONSTRUCTORS
        //

        test_callback(int size_direct, int size_indirect, const int * ptr_array,
                      int ptr_array_size, int vi_count);

        //
        // ACCESSORS
        //

    protected:

        virtual callback_args * alloc_args() const;

        //
        // MUTATORS
        //

    protected:

        virtual long call(callback_args& args);
};

inline test_callback::test_callback(int size_direct, int size_indirect,
                                    const int * ptr_array, int ptr_array_size,
                                    int vi_count)
    : callback(size_direct, size_indirect, ptr_array, ptr_array_size, vi_count)
{ }

#endif // __NOTEST__

} // namespace jsdi

#endif // __INCLUDED_CALLBACK_H___

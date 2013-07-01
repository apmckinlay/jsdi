//==============================================================================
// file: basic_type.h
// auth: Victor Schappert
// date: 20130625
// desc: Interface contract for the 'basic' (non-struct, non-callback) part of
//       the type hierarchy.
//==============================================================================

#ifndef __INCLUDED_BASIC_TYPE_H__
#define __INCLUDED_BASIC_TYPE_H__

#include "type_descriptor.h"
#include "java_enum.h"

#include <cstdint>

namespace jsdi {

//==============================================================================
//                           class basic_pointer
//==============================================================================

class basic_value;

class basic_pointer : public type_descriptor
{
        //
        // DATA
        //

        const basic_value * d_underlying;
            // CONTAINS THIS. THEREFORE NEVER DESTROYED.

        //
        // CONSTRUCTORS
        //

    private:

        basic_pointer(const basic_value * underlying);
        friend class basic_value;

        //
        // ACCESSORS
        //

    public:

        virtual void marshall_in(char *& stack_ptr, char *& heap_ptr,
                                 jobject value) const;

        virtual jobject marshall_out(char *& stack_ptr) const;

        virtual jobject return_value(long lo, long hi) const;
};

//==============================================================================
//                            class basic_value
//==============================================================================

class basic_value : public type_descriptor
{
        //
        // DATA
        //

        basic_pointer d_pointer_type;

        //
        // CONSTRUCTORS
        //

    protected:

        basic_value(size_t type_size, type_id type, const std::string& name);

        //
        // ACCESSORS
        //

    public:

        const basic_pointer * pointer_type() const noexcept;

        //
        // STATICS
        //

    public:

        static const basic_value& instance(
            suneido_language_jsdi_type_BasicType);
};

inline basic_value::basic_value(size_t type_size, type_id type,
                              const std::string& name)
    : type_descriptor(type_size, type, name)
    , d_pointer_type(this) // by now, type_descriptor is fully constructed
{ }

inline const basic_pointer * basic_value::pointer_type() const noexcept
{ return &d_pointer_type; }

//==============================================================================
//                             class basic_array
//==============================================================================

class basic_array : public type_descriptor
{
        //
        // DATA
        //

        const basic_value * d_underlying;    // NEVER DESTROYED
        size_t               d_num_elems;

        //
        // CONSTRUCTORS
        //

    public:

        basic_array(const basic_value * underlying, size_t num_elems);

        //
        // ACCESSORS
        //

    public:

        virtual void marshall_in(char *& stack_ptr, char *& heap_ptr,
                                 jobject value) const;

        virtual jobject marshall_out(char *& stack_ptr) const;

        virtual jobject return_value(long lo, long hi) const;
};

//==============================================================================
//                             class bool_type
//==============================================================================

class bool_type : public basic_value
{
        //
        // CONSTRUCTORS
        //

    public:

        bool_type();

        //
        // ACCESSORS
        //

    public:

        virtual void marshall_in(char *&, char *&, jobject) const;

        virtual jobject marshall_out(char *&) const;

        virtual jobject return_value(long lo, long hi) const;
};

inline bool_type::bool_type()
    : basic_value(sizeof(int), TYPE_BOOL, std::string("bool"))
{ }

//==============================================================================
//                           class integral_type
//==============================================================================

template<typename T>
class integral_type : public basic_value
{
        //
        // STATIC
        //

        typedef T value_type;

        //
        // CONSTRUCTORS
        //

    protected:

        integral_type(type_id type, const std::string& name);

        //
        // ACCESSORS
        //

    public:

        virtual void marshall_in(char *&, char *&, jobject) const;

        virtual jobject marshall_out(char *&) const;

        virtual jobject return_value(long lo, long hi) const;
};

template<typename T>
inline integral_type<T>::integral_type(type_id type, const std::string& name)
    : basic_value(sizeof(value_type), type, name)
{ }

//==============================================================================
//                              class char_type
//==============================================================================

class char_type : public integral_type<char>
{
    public:
        char_type();
};

inline char_type::char_type() : integral_type(TYPE_CHAR, std::string("char"))
{ }

//==============================================================================
//                             class short_type
//==============================================================================

class short_type : public integral_type<short>
{
    public:
        short_type();
};

inline short_type::short_type()
    : integral_type(TYPE_SHORT, std::string("short"))
{ }

//==============================================================================
//                             class long_type
//==============================================================================

class long_type : public integral_type<long>
{
    public:
        long_type();
};

inline long_type::long_type()
    : integral_type(TYPE_LONG, std::string("long"))
{ }

//==============================================================================
//                             class int64_type
//==============================================================================

class int64_type : public integral_type<int64_t>
{
        //
        // CONSTRUCTORS
        //

    public:

        int64_type();

        //
        // ACCESSORS
        //

    public:

        virtual void marshall_in(char *&, char *&, jobject) const;

        virtual jobject marshall_out(char *&) const;

        virtual jobject return_value(long lo, long hi) const;
};

inline int64_type::int64_type()
    : integral_type(TYPE_INT64, std::string("int64"))
{ }

} // namespace jsdi

#endif // __INCLUDED_BASIC_TYPE_H__

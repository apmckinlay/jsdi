//==============================================================================
// file: type_descriptor.h
// auth: Victor Schappert
// date: 20130624
// desc: Interface contract for type hierarchy
//==============================================================================

#ifndef __INCLUDED_TYPE_DESCRIPTOR_H__
#define __INCLUDED_TYPE_DESCRIPTOR_H__

#include "util.h"

#include <jni.h>

#include <string>
#include <memory>

namespace jsdi {

enum
{
    FLAG_CLOSED   = 0x01,
    FLAG_BASIC    = 0x02,
    FLAG_COMPLEX  = 0x04,
    FLAG_INT      = 0x08,
};

enum storage_type
{
    STORAGE_TYPE_SIMPLE   = 0x10,
    STORAGE_TYPE_POINTER  = 0x20,
    STORAGE_TYPE_ARRAY    = 0x40,
};

enum type_id
{
    TYPE_INVALID       = 0,
    TYPE_BOOL          = (1 << 3) | FLAG_BASIC,
    TYPE_CHAR          = (2 << 3) | FLAG_INT,
    TYPE_SHORT         = (3 << 3) | FLAG_INT,
    TYPE_LONG          = (4 << 3) | FLAG_INT,
    TYPE_INT64         = (5 << 3) | FLAG_INT,
    TYPE_FLOAT         = (6 << 3) | FLAG_BASIC,
    TYPE_DOUBLE        = (7 << 3) | FLAG_BASIC,
    TYPE_BASIC_POINTER = (8 << 3) | STORAGE_TYPE_POINTER,
    TYPE_BASIC_ARRAY   = (9 << 3) | STORAGE_TYPE_ARRAY,
    // TODO: pointer...
    TYPE_PROXY_SIMPLE_UNBOUND,
    TYPE_PROXY_SIMPLE_BOUND,
    TYPE_PROXY_POINTER_UNBOUND,
    TYPE_PROXY_POINTER_BOUND,
    TYPE_PROXY_ARRAY_UNBOUND,
    TYPE_PROXY_ARRAY_BOUND
};

//==============================================================================
//                           class type_descriptor
//==============================================================================

class type_descriptor : private non_copyable
{
        //
        // DATA
        //

#ifndef NDEBUG
        int         d_magic;    // only when asserts are enabled
#endif
        size_t      d_type_size;
        type_id     d_type_id;
        std::string d_name;

        //
        // STATIC
        //

#ifndef NDEBUG
        enum { MAGIC = 0x19820207 };
#endif

    public:

        static const size_t INVALID_TYPE_SIZE = 0;

        //
        // CONSTRUCTORS
        //

    public:

        type_descriptor(size_t type_size, type_id type,
                        const std::string& name);

        virtual ~type_descriptor();

        //
        // ACCESSORS
        //

    public:

        size_t type_size() const;

        type_id type() const;

        const std::string& name() const;

        bool is_closed() const;

        virtual void marshall_in(char *& stack_ptr, char *& heap_ptr,
                                 jobject value) const = 0;

        virtual jobject marshall_out(char *& stack_ptr) const = 0;

        virtual jobject return_value(long lo, long hi) const;
};

inline size_t type_descriptor::type_size() const
{ return d_type_size; }

inline type_id type_descriptor::type() const
{ return d_type_id; }

inline const std::string& type_descriptor::name() const
{ return d_name; }

inline bool type_descriptor::is_closed() const
{ return FLAG_CLOSED == (d_type_id & FLAG_CLOSED); }

//==============================================================================
//                             class type_list
//==============================================================================

class type_list : private non_copyable
{
        //
        // DATA
        //

        type_descriptor ** d_types;
            // std::dynarray would be preferable, but not yet available.
        size_t             d_size;
        size_t             d_num_open;

        //
        // TYPES
        //

        typedef size_t                  size_type;
        typedef type_descriptor *       value_type;
        typedef const value_type *      const_iterator;

        //
        // CONSTRUCTORS
        //

    public:

        type_list(size_type size);

        ~type_list() noexcept;

        //
        // ACCESSORS
        //

    public:

        bool is_closed() const noexcept;

        size_type size() const noexcept;

        const_iterator cbegin() const noexcept;

        const_iterator cend() const noexcept;

        //
        // MUTATORS
        //

    public:

        void put(std::unique_ptr<type_descriptor>& type, size_t pos) noexcept;
};

inline bool type_list::is_closed() const noexcept
{ return 0 == d_num_open; }

inline type_list::size_type type_list::size() const noexcept
{ return d_size; }

inline type_list::const_iterator type_list::cbegin() const noexcept
{ return d_types; }

inline type_list::const_iterator type_list::cend() const noexcept
{ return d_types + d_size; }

} // namespace jsdi

#endif // __INCLUDED_TYPE_DESCRIPTOR_H__

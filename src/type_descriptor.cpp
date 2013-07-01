//==============================================================================
// file: type_descriptor.cpp
// auth: Victor Schappert
// date: 20130624
// desc: Implementation file for type hierarchy
//==============================================================================

#include "type_descriptor.h"

#include <cassert>

namespace jsdi {

//==============================================================================
//                           class type_descriptor
//==============================================================================

type_descriptor::type_descriptor(size_t type_size, type_id type,
                                 const std::string& name)
    :
#ifndef NDEBUG
      d_magic(MAGIC)
    ,
#endif
      d_type_size(type_size)
    , d_type_id(type)
    , d_name(name)
{ }

type_descriptor::~type_descriptor()
{ } // anchor for virtual destructor

jobject type_descriptor::return_value(long lo, long hi) const
{
    // TODO: throw java exception
    throw "not implemented";
}

//==============================================================================
//                             class type_list
//==============================================================================

type_list::type_list(size_type size)
    : d_types(new type_descriptor *[size])
    , d_size(0)
    , d_num_open(0)
{ }

type_list::~type_list() noexcept
{
    for (size_type k = 0; k < d_size; ++k)
    {
        delete d_types[k];
    }
    delete [] d_types;
    d_types = 0;
}

void type_list::put(std::unique_ptr<type_descriptor>& type, size_t pos) noexcept
{
    assert((0 <= pos && pos < d_size) || !"Invalid position");
    assert(type.get() || !"Can't put a null pointer into type_list");
    if (d_types[pos])
    {
        if (! d_types[pos]->is_closed())
        {
            assert(0 <d_num_open);
            --d_num_open;
        }
        delete d_types[pos];
    }
    d_types[pos] = type.release();
    if (! d_types[pos]->is_closed())
    {
        ++d_num_open;
    }
}

} // namespace jsdi

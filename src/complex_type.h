//==============================================================================
// file: complex_type.h
// auth: Victor Schappert
// date: 20130626
// desc: Interface contract for the 'complex' part of the type hierarchy.
//==============================================================================

#ifndef __INCLUDED_COMPLEX_TYPE_H__
#define __INCLUDED_COMPLEX_TYPE_H__

#include "type_descriptor.h"

namespace jsdi {

//==============================================================================
//                            class complex_type
//==============================================================================

class complex_type : public type_descriptor
{
        //
        // DATA
        //

        std::unique_ptr<type_list> d_items; // struct members/callback params

        //
        // CONSTRUCTORS
        //

    public:

        complex_type(size_t type_size, type_id type, const std::string& name);

        //
        // ACCESSORS
        //


};

inline complex_type::complex_type(size_t type_size, type_id type,
                                  const std::string& name)
    : type_descriptor(type_size, type, name)
{ }

} // namespace jsdi

#endif // __INCLUDED_COMPLEX_TYPE_H__

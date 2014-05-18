/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_COM_UTIL_H___
#define __INCLUDED_COM_UTIL_H___

/**
 * \file com_util.h
 * \author Victor Schappert
 * \since 20131022
 * \brief Utility functions to simplify common COM tasks
 */

#include <memory>
#include <cassert>

#include "jsdi_ole2.h"

namespace jsdi {

/**
 * \brief Deleter which can be used with <dfn>std::unique_ptr</dfn> that just
 *        calls <dfn>Release()</dfn> on an underlying COM interface pointer
 *        derived from IUnknown.
 * \author Victor Schappert
 * \since 20131022
 * \see com_managed_interface
 */
template <typename COMInterface>
struct com_interface_deleter
{
        /** \brief "Deletes" <dfn>ptr</dfn> by calling
         *         <dfn>ptr->Release()</dfn>. */
        void operator()(COMInterface * ptr) const;
};

template <typename COMInterface>
inline void com_interface_deleter<COMInterface>::operator()(
    COMInterface * ptr) const
{
    assert(ptr);
    ptr->Release();
}

/**
 * \brief A <dfn>std::unique_ptr</dfn> whose deleter is a
 *        \link com_interface_deleter \endlink.
 * \author Victor Schappert
 * \since 20131022
 */
template <typename COMInterface>
using com_managed_interface =
    std::unique_ptr<COMInterface, com_interface_deleter<COMInterface>>;

/**
 * \brief Deleter which can be used with <dfn>std::unique_ptr</dfn> that just
 *        calls <dfn>SysFreeString()</dfn> on an underlying <dfn>BSTR</dfn>.
 * \author Victor Schappert
 * \since 20131022
 * \see com_managed_bstr
 */
struct com_bstr_deleter
{
        /** \brief Deletes <dfn>ptr</dfn> by calling
         *         <dfn>SysFreeString(ptr)</dfn>. */
        void operator()(BSTR ptr) const;
};

inline void com_bstr_deleter::operator()(BSTR ptr) const
{
    assert(ptr);
    SysFreeString(ptr);
}

/**
 * \brief A <dfn>std::unique_ptr</dfn> whose deleter is a
 *        \link com_bstr_deleter \endlink.
 * \author Victor Schappert
 * \since 20131022
 */
typedef std::unique_ptr<OLECHAR, com_bstr_deleter> com_managed_bstr;

/**
 * \brief Deleter which can be used with <dfn>std::unique_ptr</dfn> that just
 *        calls <dfn>VariantClear()</dfn> on an underlying <dfn>VARIANT</dfn>
 *        pointer.
 * \author Victor Schappert
 * \since 20131102
 */
struct com_variant_deleter
{
        /** \brief "Deletes" <dfn>ptr</dfn> by calling
         *         <dfn>VariantClear(ptr)</dfn>. */
        void operator()(VARIANT * ptr) const;
};

inline void com_variant_deleter::operator()(VARIANT * ptr) const
{
    assert(ptr);
    VariantClear(ptr);
}

/**
 * \brief A <dfn>std::unique_ptr</dfn> whose deleter is a
 *        \link com_variant_deleter \endlink.
 * \author Victor Schappert
 * \since 20131102
 *
 * 'Deleting' a variant pointer by calling <dfn>VariantClear()</dfn> on it will
 * ensure that resources allocated for the <dfn>VARIANT</dfn> structure, such as
 * <dfn>BSTR</dfn> memory and reference increments on COM <dfn>IUnknown</dfn> or
 * <dfn>IDispatch</dfn> pointers are correctly released.
 */
typedef std::unique_ptr<VARIANT, com_variant_deleter> com_managed_variant;

} // namespace jsdi

#endif // __INCLUDED_COM_UTIL_H___

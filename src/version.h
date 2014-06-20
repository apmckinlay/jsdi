/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

#ifndef __INCLUDED_VERSION_H__
#define __INCLUDED_VERSION_H__

/**
 * \file version.h
 * \author Victor Schappert
 * \since 20140520
 * \brief Library version information
 * \remark For this translation unit to function appropriately, it must begin
 *         rebuilt <em>on every build</em> of the library.
 */

namespace jsdi {
 
//==============================================================================
//                              struct version
//==============================================================================

/**
 * \brief Library version information
 * \author Victor Schappert
 * \since 20140520
 */
struct version
{
    /**
     * \brief Constant string containing the last date and time the library was
     *        built.
     */
    static char const * const BUILD_DATE;
};

} // namespace jsdi

#endif // __INCLUDED_VERSION_H__

/* Copyright 2013 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: util.cpp
// auth: Victor Schappert
// date: 20130628
// desc: Utility functions used by the JSDI project.
//==============================================================================

#include "util.h"

namespace jsdi {

} // namespace jsdi

//==============================================================================
//                                  TESTS
//==============================================================================

#ifndef __NOTEST__

#include "test.h"

#include <limits>

using namespace jsdi;

TEST(smallest_pow2,
    assert_equals(  0, smallest_pow2(static_cast<uint8_t> (  0)));
    assert_equals(  1, smallest_pow2(static_cast<uint8_t> (  1)));
    assert_equals(  2, smallest_pow2(static_cast<uint8_t> (  2)));
    assert_equals(  4, smallest_pow2(static_cast<uint8_t> (  3)));
    assert_equals(  4, smallest_pow2(static_cast<uint8_t> (  4)));
    assert_equals(  8, smallest_pow2(static_cast<uint8_t> (  5)));
    assert_equals(  8, smallest_pow2(static_cast<uint8_t> (  6)));
    assert_equals(  8, smallest_pow2(static_cast<uint8_t> (  7)));
    assert_equals(  8, smallest_pow2(static_cast<uint8_t> (  8)));
    assert_equals( 16, smallest_pow2(static_cast<uint8_t> (  9)));
    assert_equals(128, smallest_pow2(static_cast<uint8_t> (127)));
    assert_equals(128, smallest_pow2(static_cast<uint8_t> (128)));
    assert_equals(  0, smallest_pow2(static_cast<uint8_t> (129)));
    assert_equals(  0, smallest_pow2(static_cast<uint16_t>(  0)));
    assert_equals(  1, smallest_pow2(static_cast<uint16_t>(  1)));
    assert_equals(  2, smallest_pow2(static_cast<uint16_t>(  2)));
    assert_equals(  4, smallest_pow2(static_cast<uint16_t>(  3)));
    assert_equals(  4, smallest_pow2(static_cast<uint16_t>(  4)));
    assert_equals(  8, smallest_pow2(static_cast<uint16_t>(  5)));
    assert_equals(  8, smallest_pow2(static_cast<uint16_t>(  6)));
    assert_equals(  8, smallest_pow2(static_cast<uint16_t>(  7)));
    assert_equals(  8, smallest_pow2(static_cast<uint16_t>(  8)));
    assert_equals( 16, smallest_pow2(static_cast<uint16_t>(  9)));
    assert_equals(  0, smallest_pow2(std::numeric_limits<uint16_t>::max()));
    assert_equals(  0, smallest_pow2(std::numeric_limits<uint32_t>::max()));
    uint16_t x16(10), y16(16);
    uint32_t x32(10), y32(16);
    for (;
         x32 <= static_cast<uint32_t>(std::numeric_limits<int16_t>::max());
         ++x16, ++x32)
    {
        if (x16 < y16)
        {
            assert_equals(y16, smallest_pow2(x16));
            assert_equals(y32, smallest_pow2(x32));
        }
        else
        {
            assert_equals(x16, y16);
            assert_equals(x32, y32);
            assert_equals(y16, smallest_pow2(y16));
            assert_equals(y32, smallest_pow2(y32));
            assert_equals(y16 * 2,
                          smallest_pow2(static_cast<uint16_t>(y16 + 1)));
            assert_equals(y32 * 2,
                          smallest_pow2(static_cast<uint32_t>(y32 + 1)));
            y16 *= 2;
            y32 *= 2;
        }
    }
    assert_equals(0x40000, smallest_pow2(static_cast<uint32_t>(0x3010a)));
);

#endif // __NOTEST__


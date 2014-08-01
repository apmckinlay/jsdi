/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: register64.cpp
// auth: Victor Schappert
// date: 20140714
// desc: Information about Windows x64 ABI register usage
//==============================================================================

#include "register64.h"

#include "util.h"

#include <sstream>
#include <iomanip>

namespace jsdi {
namespace abi_amd64 {

namespace {

void validate(uint32_t value, uint32_t encoding, size_t param_num)
{
    if (! (static_cast<uint32_t>(UINT64) <= value &&
            value <= static_cast<uint32_t>(FLOAT)))
    {
        std::ostringstream() << "invalid register encoding at param "
                             << param_num << " (" << value << " in "
                             << std::hex << ')'
                             << throw_cpp<std::invalid_argument>();
    }
}

} // anonymous namespace

param_register_types::param_register_types(uint32_t encoding)
    : d_data(encoding)
{
    uint32_t param0 = encoding >> 030;
    uint32_t param1 = (encoding >> 020) & 0xffu;
    uint32_t param2 = (encoding >> 010) & 0xffu;
    uint32_t param3 = (encoding >> 000) & 0xffu;
    validate(param0, encoding, 0);
    validate(param1, encoding, 1);
    validate(param2, encoding, 2);
    validate(param3, encoding, 3);
}

} // namespace abi_amd64
} // namespace jsdi

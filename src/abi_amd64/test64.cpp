/* Copyright 2014 (c) Suneido Software Corp. All rights reserved.
 * Licensed under GPLv2.
 */

//==============================================================================
// file: test64.cpp
// auth: Victor Schappert
// date: 20140716
// desc: Test functions used only in the x64 test builds
//==============================================================================

#ifndef __NOTEST__

#include "test64.h"

#include "util.h"

#include <algorithm>
#include <numeric>
#include <sstream>
#include <string>
#include <type_traits>

namespace jsdi {
namespace abi_amd64 {
namespace test64 {

namespace {

template<typename T>
inline T sum(const std::initializer_list<T>& list)
{ return std::accumulate(list.begin(), list.end(), T(0)); }

template<typename T>
struct param_register_typeof
{
    static const param_register_type value = param_register_type::UINT64;
    static_assert(std::is_integral<T>::value,
                  "template parameter must be an integer type");
};

template<>
struct param_register_typeof<double>
{
    static const param_register_type value = param_register_type::DOUBLE;
};

template<>
struct param_register_typeof<float>
{
    static const param_register_type value = param_register_type::FLOAT;
};

template<typename ReturnType, typename ... ArgTypes>
struct f : public function
{
    f() : function(reinterpret_cast<void *>(&actual_function),
                   param_register_typeof<ReturnType>::value,
                   { param_register_typeof<ArgTypes>::value... })
    { }
    static ReturnType actual_function(ArgTypes ... args)
    { return sum({ static_cast<ReturnType>(args)... }); }
};

template<typename ... ArgTypes>
using u = f<uint64_t, ArgTypes...>;

const function * FP_FUNCTIONS_[] =
{
    // One parameter (100% of possibilities)
    new u<uint64_t>, new u<double>, new u<float>,
    // Two parameters (100% of possibilities)
    new u<uint64_t, uint64_t>, new u<uint64_t, double>, new u<uint64_t, float>,
    new u<double, uint64_t>, new u<double, double>, new u<double, float>,
    new u<float, uint64_t>, new u<float, double>, new u<float, float>,
    // Three parameters (100% of possibilities)
    new u<uint64_t, uint64_t, uint64_t>, new u<uint64_t, uint64_t, double>,
    new u<uint64_t, uint64_t, float>,
    new u<uint64_t, double, uint64_t>, new u<uint64_t, double, double>,
    new u<uint64_t, double, float>,
    new u<uint64_t, float, uint64_t>, new u<uint64_t, float, double>,
    new u<uint64_t, float, float>,
    new u<double, uint64_t, uint64_t>, new u<double, uint64_t, double>,
    new u<double, uint64_t, float>,
    new u<double, double, uint64_t>, new u<double, double, double>,
    new u<double, double, float>,
    new u<double, float, uint64_t>, new u<double, float, double>,
    new u<double, float, float>,
    new u<float, uint64_t, uint64_t>, new u<float, uint64_t, double>,
    new u<float, uint64_t, float>,
    new u<float, double, uint64_t>, new u<float, double, double>,
    new u<float, double, float>,
    new u<float, float, uint64_t>, new u<float, float, double>,
    new u<float, float, float>,
    // Four parameters (spot coverage)
    new u<uint64_t, uint64_t, uint64_t, uint64_t>,
    // Five parameters (spot coverage)
    new u<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>,
    new u<double, double, double, double, double>,
    new u<float, float, float, float, float>,
    new u<uint64_t, double, double, float, uint64_t>,
    
    // Six parameters (spot coverage)
    new u<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>,
    new u<double, double, double, double, double, double>,
    new u<float, float, float, float, float, float>,
    new u<uint64_t, double, float, uint64_t, double, float>,

    // Seven parameters (spot coverage)
    new u<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>,
    new u<double, double, double, double, double, double, double>,
    new u<float, float, float, float, float, float, float>,
    new u<double, float, uint64_t, uint64_t, double, float, uint64_t>,
    new u<double, int8_t, int16_t, float, int32_t, int64_t, double>,

    // Eight parameters (spot coverage)
    new u<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t>,
    new u<double, double, double, double, double, double, double, double>,
    new u<float, float, float, float, float, float, float, float>,
    new u<float, float, float, uint64_t, double, double, double, uint64_t>,
    
    // Nine parameters (spot coverage)
    new u<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t>,
    new u<double, double, double, double, double, double, double, double,
          double>,
    new u<float, float, float, float, float, float, float, float, float>,
    new u<double, double, double, float, float, double, float, float, double>,
    
    // 10 parameters (spot coverage)
    new u<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t>,
    new u<double, double, double, double, double, double, double, double,
          double, double>,
    new u<float, float, float, float, float, float, float, float, float, float>,
    new u<uint64_t, uint64_t, uint64_t, double, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t>,

    // 11 parameters (spot coverage)
    new u<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t>,
    new u<double, double, double, double, double, double, double, double,
          double, double, double>,
    new u<float, float, float, float, float, float, float, float, float, float,
          float>,
    new u<uint64_t, uint64_t, uint64_t, float, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t>,
    
    // 12 parameters (spot coverage)
    new u<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>,
    new u<double, double, double, double, double, double, double, double,
          double, double, double, double>,
    new u<float, float, float, float, float, float, float, float, float, float,
          float, float>,
    new u<uint64_t, uint64_t, double, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>,
    
    // 13 parameters (spot coverage)
    new u<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>,
    new u<double, double, double, double, double, double, double, double,
          double, double, double, double, double>,
    new u<float, float, float, float, float, float, float, float, float, float,
          float, float, float>,
    new u<uint64_t, uint64_t, float, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>,

    // 14 parameters (spot coverage)
    new u<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>,
    new u<double, double, double, double, double, double, double, double,
          double, double, double, double, double, double>,
    new u<float, float, float, float, float, float, float, float, float, float,
          float, float, float, float>,
    new u<uint64_t, double, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>,

    // 15 parameters (spot coverage)
    new u<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t>,
    new u<double, double, double, double, double, double, double, double,
          double, double, double, double, double, double, double>,
    new u<float, float, float, float, float, float, float, float, float, float,
          float, float, float, float, float>,
    new u<uint64_t, float, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t>,

    // 16 parameters (spot coverage)
    new u<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t>,
    new u<double, double, double, double, double, double, double, double,
          double, double, double, double, double, double, double, double>,
    new u<float, float, float, float, float, float, float, float, float, float,
          float, float, float, float, float, float>,
    new u<double, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t>,

    // 17 parameters (spot coverage)
    new u<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t>,
    new u<double, double, double, double, double, double, double, double,
          double, double, double, double, double, double, double, double,
          double>,
    new u<float, float, float, float, float, float, float, float, float, float,
          float, float, float, float, float, float, float>,
    new u<float, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t>,

    // 18 parameters (spot coverage)
    new u<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t>,
    new u<double, double, double, double, double, double, double, double,
          double, double, double, double, double, double, double, double,
          double, double>,
    new u<float, float, float, float, float, float, float, float, float, float,
          float, float, float, float, float, float, float, float>,
    new u<float, double, float, uint64_t, float, double, float, uint64_t, float,
          double, float, uint64_t, float, double, float, uint64_t, float,
          double>,

    // 19 parameters (spot coverage)
    new u<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>,
    new u<double, double, double, double, double, double, double,
          double, double, double, double, double, double, double,
          double, double, double, double, double>,
    new u<float, float, float, float, float, float, float,
          float, float, float, float, float, float, float,
          float, float, float, float, float>,
    new u<uint64_t, float, uint64_t, double, uint64_t, float, uint64_t,
          uint64_t, float, uint64_t, double, uint64_t, float, uint64_t,
          uint64_t, float, uint64_t, double, uint64_t>,

    // 20 parameters (spot coverage)
    new u<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>,
    new u<double, double, double, double, double, double, double,
          double, double, double, double, double, double, double,
          double, double, double, double, double, double>,
    new u<float, float, float, float, float, float, float,
          float, float, float, float, float, float, float,
          float, float, float, float, float, float>,
    new u<uint64_t, float, double, float, uint64_t, uint64_t, uint64_t,
          uint64_t, uint64_t, uint64_t, double, uint64_t, uint64_t, uint64_t,
          uint64_t, float, uint64_t, uint64_t, uint64_t, uint64_t>,

};

std::ostream& operator<<(std::ostream& o, param_register_type t)
{
    const char * str = nullptr;
    switch (t)
    {
        case param_register_type::UINT64: str = "uint64_t"; break;
        case param_register_type::DOUBLE: str = "double"; break;
        case param_register_type::FLOAT:  str = "float"; break;
        default: assert(false || !"control should never pass here");
    }
    return o << str;
}

void make_signature(function& f)
{
    std::ostringstream o;
    o << f.return_type << "(*)(";
    auto i = f.arg_types.begin(), e = f.arg_types.end();
    if (i != e)
    {
        o << *i;
        for (++i; i != e; ++i) o << ',' << *i;
    }
    o << ')';
    auto s(o.str());
    auto p = std::copy_n(
        s.begin(), std::min(jsdi::array_length(f.signature) - 1, s.size()),
        f.signature);
    *p = '\0';
}

param_register_types make_register_types(
    const std::vector<param_register_type>& arg_types)
{
    return param_register_types(
        0 < arg_types.size() ? arg_types[0] : param_register_type::UINT64,
        1 < arg_types.size() ? arg_types[1] : param_register_type::UINT64,
        2 < arg_types.size() ? arg_types[2] : param_register_type::UINT64,
        3 < arg_types.size() ? arg_types[3] : param_register_type::UINT64
    );
}



} // anonymous namespace

const function_list FP_FUNCTIONS =
{
    FP_FUNCTIONS_,
    FP_FUNCTIONS_ + jsdi::array_length(FP_FUNCTIONS_)
};

function::function(void * func_ptr_, param_register_type return_type_,
                   const std::initializer_list<param_register_type>& arg_types_)
    : func_ptr(func_ptr_)
    , nargs(arg_types_.size())
    , return_type(return_type_)
    , arg_types(arg_types_)
    , register_types(make_register_types(arg_types))
{ make_signature(*this); }

} // namespace test64
} // namespace abi_amd64
} // namespace jsdi

#endif // __NOTEST__

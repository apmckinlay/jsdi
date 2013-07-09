//==============================================================================
// file: test.cpp
// auth: Victor Schappert
// date: 20130709
// desc: Exports functions which can be used to test dll functionality
//==============================================================================

#include <cstdint>
#include <cstring>

extern "C" {

#define EXPORT_STDCALL __declspec(dllexport) __stdcall

EXPORT_STDCALL void TestVoid()
{ }

EXPORT_STDCALL char TestChar(char a)
{ return a; }

EXPORT_STDCALL short TestShort(short a)
{ return a; }

EXPORT_STDCALL long TestLong(long a)
{ return a; }

EXPORT_STDCALL int64_t TestInt64(int64_t a)
{ return a; }

EXPORT_STDCALL long TestSumFourLongs(long a, long b, long c, long d)
{ return a + b + c + d; }

EXPORT_STDCALL long TestStrLen(char * str)
{ return str ? std::strlen(str) : 0; }

} // extern "C"


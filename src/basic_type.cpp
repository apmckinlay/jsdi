#include "basic_type.h"

#include "jni_exception.h"
#include "global_refs.h"

#include <cassert>
#include <sstream>

namespace jsdi {

//==============================================================================
//                           class basic_pointer
//==============================================================================

basic_pointer::basic_pointer(const basic_value * underlying)
    : type_descriptor(sizeof(void *), TYPE_BASIC_POINTER, underlying->name())
    , d_underlying(underlying)
{ }

void basic_pointer::marshall_in(char *& stack_ptr, char *& heap_ptr,
                                jobject value) const
{
    // todo
}

jobject basic_pointer::marshall_out(char *& stack_ptr) const
{
    // todo
}

jobject basic_pointer::return_value(long lo, long hi) const
{
    char * ptr = (char *)lo;
    return d_underlying->marshall_out(ptr); // todo...
}

//==============================================================================
//                            class basic_value
//==============================================================================

static bool_type  bool_type_instance;
static char_type  char_type_instance;
static short_type short_type_instance;
static long_type  long_type_instance;
static int64_type int64_type_instance;

const basic_value& basic_value::instance(
    suneido_language_jsdi_type_BasicType basic_type)
{
    switch (basic_type)
    {
        case suneido_language_jsdi_type_BasicType::BOOL:
            return bool_type_instance;
        case suneido_language_jsdi_type_BasicType::CHAR:
            return char_type_instance;
        case suneido_language_jsdi_type_BasicType::SHORT:
            return short_type_instance;
        case suneido_language_jsdi_type_BasicType::LONG:
            return long_type_instance;
        case suneido_language_jsdi_type_BasicType::INT64:
            return int64_type_instance;
        default:
            std::ostringstream o;
            o << __FUNCTION__ << ": No switch case for basic type "
                              << basic_type;
            throw jni_exception(o.str(), false);
    }
}

//==============================================================================
//                             class basic_array
//==============================================================================

basic_array::basic_array(const basic_value * underlying, size_t num_elems)
    : type_descriptor(
          num_elems * underlying->type_size(),
          TYPE_BASIC_ARRAY,
          underlying->name()
      )
    , d_underlying(underlying)
    , d_num_elems(num_elems)
{ }

void basic_array::marshall_in(char *& stack_ptr, char *& heap_ptr,
                              jobject value) const
{
    // todo
}

jobject basic_array::marshall_out(char *& stack_ptr) const
{
    // todo
}

jobject basic_array::return_value(long lo, long hi) const
{
    // todo
}

//==============================================================================
//                             class bool_type
//==============================================================================

void bool_type::marshall_in(char *&, char *&, jobject) const
{

}

jobject bool_type::marshall_out(char *&) const
{

}

jobject bool_type::return_value(long lo, long) const
{
    return lo
        ? global_refs::ptr->TRUE_object()
        : global_refs::ptr->FALSE_object()
        ;
}

//==============================================================================
//                            class integral_type
//==============================================================================

template<typename T>
void integral_type<T>::marshall_in(char *& stack_ptr, char *& heap_ptr,
                                   jobject value) const
{

}

template<typename T>
jobject integral_type<T>::marshall_out(char *&) const
{

}

template<typename T>
jobject integral_type<T>::return_value(long lo, long hi) const
{

}

//==============================================================================
//                             class int64_type
//==============================================================================

void int64_type::marshall_in(char *& stack_ptr, char *& heap_ptr,
                             jobject value) const
{

}

jobject int64_type::marshall_out(char *& stack_ptr) const
{

}

jobject int64_type::return_value(long lo, long hi) const
{

}

} // namespace jsdi

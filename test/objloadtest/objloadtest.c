#include "../../src/support/dtypes.h"
extern void jl_the_callback() __attribute__ ((weak));
extern void _jl_the_callback() __attribute__ ((weak));

// We have both with/without underscore to match platform mangling conventions.
JL_DLLEXPORT void call_the_callback_back()
{
    return jl_the_callback();
}
JL_DLLEXPORT void _call_the_callback_back()
{
    return _jl_the_callback();
}

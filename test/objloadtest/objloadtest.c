extern void _jl_the_callback() __attribute__ ((weak));

// We have both with/without underscore to match platform mangling conventions.
__attribute__ ((visibility("default"))) void call_the_callback_back()
{
    return _jl_the_callback();
}
__attribute__ ((visibility("default"))) void _call_the_callback_back()
{
    return _jl_the_callback();
}

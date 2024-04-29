# memguard

Simple macros for valgrindish memory protection

This is just a very quick-and-dirty example of how to put
clobber-detection bytes around your variables so that
you can detect certain kinds of memory errors.

Tools like valgrind or compiling with `-fsanitize=address`
give you the same thing, but better, but sometimes on an
embedded system you don't have access to them. This lets
you instrument a few suspect variables.

There are two basic modes of use. For automatic variables,
you use the macros for declaring the variable as well as
for checking it.

For variables that are allocated permanently file level or
static, you can use the `memguard_register()` function to build
up a list of such variables and then just call `memguard_check()`
periodically to check on all of them.


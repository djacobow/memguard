#pragma once

#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*

This module implements a "poor man's" memory guarding. It works by over
allocating memory so that there is extra space at the beginning and end
of an array that you ask for. Then it "colors" in that extrace space
with known bytes, and later, it checks to see if those bytes have been
disturbed. If they have, someone has been writing where they should not
have, anad an error is flagged.

So, lets say you want a buffer of 5 uint32_t's, like:

    uint32_t foo[5];

We'd do this instead:

    uint8_t _real_foo[sizeof(uint32_t) * 5 + (2 * 16]
    __attribute__((__aligned(alignof(uint32_t))));
    uint32_t *foo = (uint32_t *)(_real_foo + 16);

    GGGGGGGGGGGGGGGG....................GGGGGGGGGGGGGGGG 
    ^               ^
    |               | 
    _real_foo       foo 
    
Note that it is important that foo be aligned on the natural alignment
of whatever the type in question (here, uint32_t) expects. Aligning
_real_foo the same way and padding with a numbe of bytes that is a
multiple of 8 guarantees that foo will be properly aligned.

Your code will use foo just as it would have normally, but this module
will hold onto the _rel_foo. If _real_foo is written do outside of the
range [foo, foo + sizeof(uint32_t) * 5), we will complain and log.

NB:

This module is a big, fat no-op if compiled for simulator. The reason? The
sim has much better support for more sophisticated memory checking,
including clang's -fsanitize=memory. Adding in our own guards would just
be confusing and inferior.

*/

// initialize the module
void memguard_init();

// run the a check on all registered pointers, and return the number
// of failures observed
uint32_t memguard_check();

// add a pointer to be tracked. The target is the "real_ptr", but the size
// is the size of the type, and the count is the size of the user's buffer
// without the extra guard bytes. The name is there for errors.
//
// NB: We can only track "static" pointers -- pointers that won't go away
// like this.
bool memguard_object_register(void *target, size_t target_size, uint32_t count, const char *name);

// paints (or repaints) the guard area
void memguard_object_prep(void *target, size_t size, uint32_t count);

// checks a single guarded pointer. Again, the target is to the real_ptr.
// This function can be used to check on local / stack objects, and it is
// also use internally
bool memguard_local_object_check(const void *target, size_t target_size, uint32_t count, const char *name);

// the buffer is padded by GUARD_LEN on both sides. NB:
// if GUARD_LEN is at least 8, then it can't interfere with
// any ordinary alignment needs.
#define GUARD_LEN (16)

// macro sugar to declare a an array of type, name, size,
// and create a shadow variable that is the "real_ptr"
// as well as a second ptr that points to the user area.
#define STATIC_GUARDED_ARRAY_ALIGNED(type, name, size, algn) \
    static char _real_##name[(size * sizeof(type)) + (2*GUARD_LEN)] \
        __attribute__((__aligned__(algn))) = {}; \
    static type *name = (type *)(_real_##name + GUARD_LEN)

#define STATIC_GUARDED_ARRAY(type, name, size) \
    STATIC_GUARDED_ARRAY_ALIGNED(type, name, size, alignof(type))

// sugar to register such a buffer with this module
#define REGISTER_STATIC_GUARDED_ARRAY(name, type, count) \
    memguard_object_register(&_real_##name, sizeof(type), count, #name)

// for creating struct members that are similarly padded
#define GUARDED_CHAR_STRUCT_MEMBER(name, size) \
    char name[size + 2*GUARD_LEN]

// for accessing the user area of the padded buffer
#define UNGUARD_CHAR_STRUCT_MEMBER(name) \
    (name + GUARD_LEN)

// for making local buffers that we can send to be checked
#define LOCAL_GUARDED_ARRAY(type, name, count) \
    char _real_##name[(count * sizeof(type)) + (2*GUARD_LEN)] \
        __attribute__((__aligned__(alignof(type)))) = {}; \
    type *name = (type *)(_real_##name + GUARD_LEN); \
    memguard_object_prep(_real_##name, sizeof(type), count)

#define LOCAL_GUARDED_OBJECT(type, name) \
    char _real_##name[sizeof(type)  + (2*GUARD_LEN)] \
        __attribute__((__aligned__(alignof(type)))) = {}; \
    type *name = (type *)(_real_##name + GUARD_LEN); \
    memguard_object_prep(_real_##name, sizeof(type), 1)
    
// ... and checking local buffers
#define LOCAL_GUARDED_ARRAY_CHECK(type, name, count) \
    memguard_local_object_check(_real_##name, sizeof(type), count, #name)

#define LOCAL_GUARDED_OBJECT_CHECK(type, name) \
    memguard_local_object_check(_real_##name, sizeof(type), 1, #name)


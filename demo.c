#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "memguard.h"

STATIC_GUARDED_ARRAY(int16_t, bob, 64);  // same as static int16_t bob[64];
STATIC_GUARDED_ARRAY(double,  sue, 512); // same as static double sue[512];

void static_example(void) {
    memset(bob, 0x99, sizeof(int16_t) * 64); // ok
    sue[513] = 6.022e23; // not ok
    memguard_check();
}

void local_example(void) {
    // equivalent of uint32_t foo[1024];
    LOCAL_GUARDED_ARRAY(uint32_t, foo, 1024);

    for (uint32_t i=0; i< 1024; i++) {
        foo[i] = i;
    } 

    // check foo: it should be fine
    LOCAL_GUARDED_ARRAY_CHECK(uint32_t, foo, 1024);

    for (uint32_t i=0; i<= 1024; i++) {
        foo[i] = i;
    } 

    // check foo again now. It will _not_ be fine.
    LOCAL_GUARDED_ARRAY_CHECK(uint32_t, foo, 1024);
};

int main(int argc, const char *argv[]) {
    memguard_init();
    REGISTER_STATIC_GUARDED_ARRAY(bob, int16_t, 64);
    REGISTER_STATIC_GUARDED_ARRAY(sue, double, 512);

    static_example();


    local_example();
    return 0;
}


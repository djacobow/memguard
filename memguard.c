#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "memguard.h"

#define MAX_GUARDED_OBJECTS (30)

static const uint8_t GUARD_SENTINEL = 0x77;

typedef struct guarded_record_t {
    void *target;
    size_t size;
    uint32_t count;
    const char *name;
} guarded_record_t;

typedef struct memguard_context_t {
    guarded_record_t records[MAX_GUARDED_OBJECTS];
    uint32_t count;
} memguard_context_t;

static memguard_context_t memguard_context;


static void bin_to_hex(char *dst, const void *src, size_t srclen) {
    const char digits[] = "0123456789abcdef";
    uint8_t *s = (uint8_t *)src;
    char *d = dst;
    for (uint32_t i=0; i<srclen; i++) {
        uint8_t b = s[i];
        *(d++) = digits[(b >> 4) & 0xf];
        *(d++) = digits[(b     ) & 0xf];
    }
    *d = 0;
}

bool memguard_object_register(void *target, size_t size, uint32_t count, const char *name) {
    if (memguard_context.count < MAX_GUARDED_OBJECTS) {
        printf("-Info- Registering: %s, %p, size %"PRIu32", count: %"PRIu32"\n", name, target, (uint32_t)size, count);
        guarded_record_t *gr = &memguard_context.records[memguard_context.count++];
        *gr = (guarded_record_t){ target, size, count, name };
        memguard_object_prep(target, size, count);
        return true;
    }
    printf("-Warning- Out of guard slots\n");
    return false;
}


void memguard_object_prep(void *target, size_t size, uint32_t count) {
    char *front = (char *)target;
    char *back = front + GUARD_LEN + (count * size);
    memset(front, GUARD_SENTINEL, GUARD_LEN);
    memset(back,  GUARD_SENTINEL, GUARD_LEN);
}

static bool memguard_object_check(const void *target, size_t size, uint32_t count) {
    const char *front = (const char*)target;
    const char *back = (const char *)front + GUARD_LEN + (count * size);
    uint32_t miss_count = 0;
    for (uint32_t i=0; i<GUARD_LEN; i++) {
        if (front[i] != GUARD_SENTINEL) {
            miss_count++;
        }
        if (back[i] != GUARD_SENTINEL) {
            miss_count++;
        }
    }
    return (miss_count);
}

static void memguard_report_clobber(const void *target, size_t size, uint32_t count, const char *name) {
    const char *front = (const char*)target;
    const char *back = (const char *)front + GUARD_LEN + (count * size);
        #define SL (GUARD_LEN*2 + 16)
        char s_front[SL] = {};
        char s_back [SL] = {};
        bin_to_hex(s_front, front, GUARD_LEN);
        bin_to_hex(s_back,  back,  GUARD_LEN);
        printf("-Error- !!clobber!! %p, %s: [%s] ... [%s]\n", target, name, s_front, s_back);
}

bool memguard_local_object_check(const void *target, size_t size, uint32_t count, const char *name) {
    if (memguard_object_check(target, size, count)) {
        memguard_report_clobber(target, size, count, name);
        return true;
    }
    return false;
}

uint32_t memguard_check() {
    uint32_t fails = 0;
    for (uint32_t i=0; i< memguard_context.count; i++) {
       guarded_record_t *gr = &memguard_context.records[i];
       if (memguard_object_check(gr->target, gr->size, gr->count)) {
            fails++;
            memguard_report_clobber(gr->target, gr->size, gr->count, gr->name);
            // reset the guard area so we can detect _new_ clobberage next time
            memguard_object_prep(gr->target, gr->size, gr->count);
       }
    }
    return fails;
}

void memguard_init() {
    memset(&memguard_context, 0, sizeof(memguard_context));
};


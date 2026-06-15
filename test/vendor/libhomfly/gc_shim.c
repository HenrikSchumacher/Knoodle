/*
 * gc_shim.c — tracked allocator backing gc_shim.h. See that header for the
 * rationale and the safety argument. Added by the Knoodle test suite; not part
 * of upstream libhomfly.
 */
#include "gc_shim.h"

#include <stdlib.h>

/* Every block carries this header so realloc can update its registry slot in
   O(1). The union forces the payload (which follows the header) to be aligned
   for any type, exactly as malloc would return. */
typedef union Header
{
    size_t      index;   /* slot of this block in g_blocks */
    max_align_t _align;
} Header;

/* Registry of raw (header) pointers for all live blocks. Reset by free_all;
   the registry array itself is kept and reused (bounded by the peak number of
   simultaneous allocations within a single homfly() call). */
static void  **g_blocks = NULL;
static size_t  g_count  = 0;
static size_t  g_cap    = 0;

void *knoodle_gc_malloc(size_t n)
{
    Header *raw = (Header *)malloc(sizeof(Header) + n);
    if (raw == NULL) { return NULL; }

    if (g_count == g_cap)
    {
        size_t ncap = (g_cap != 0) ? (g_cap * 2) : (size_t)1024;
        void **nb = (void **)realloc(g_blocks, ncap * sizeof(void *));
        if (nb == NULL) { free(raw); return NULL; }
        g_blocks = nb;
        g_cap    = ncap;
    }

    raw->index          = g_count;
    g_blocks[g_count++] = raw;
    return (void *)(raw + 1);
}

void *knoodle_gc_realloc(void *p, size_t n)
{
    if (p == NULL) { return knoodle_gc_malloc(n); }

    Header *raw  = ((Header *)p) - 1;
    size_t  idx  = raw->index;

    Header *nraw = (Header *)realloc(raw, sizeof(Header) + n);
    if (nraw == NULL) { return NULL; }

    nraw->index    = idx;   /* preserved by realloc, but be explicit */
    g_blocks[idx]  = nraw;
    return (void *)(nraw + 1);
}

void knoodle_gc_free_all(void)
{
    for (size_t i = 0; i < g_count; ++i) { free(g_blocks[i]); }
    g_count = 0;
}

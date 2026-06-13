/*
 * gc_shim.h — local replacement for the Boehm GC (<gc.h>) that libhomfly
 * upstream depends on, so the vendored copy builds against libc only.
 *
 * Upstream relied on the collector to free libhomfly's working memory. Rather
 * than leak it, this shim is a tracked allocator: every block is registered,
 * and knoodle_gc_free_all() reclaims everything allocated since the last call.
 * The Knoodle test driver calls knoodle_gc_free_all() after copying out each
 * homfly() result, keeping memory bounded across arbitrarily many calls.
 *
 * This is safe because libhomfly keeps no allocated state across homfly()
 * calls: its file-scope global polynomials (llplus, lplusm, lminusm, llminus,
 * mll in control.c) are re-created on every call by c_init() via p_term ->
 * p_add, which overwrites .term with a fresh allocation without reading the
 * old (now-freed) pointer. Single-threaded use only (the registry is global).
 *
 * Added by the Knoodle test suite; not part of upstream libhomfly.
 */
#ifndef KNOODLE_GC_SHIM_H
#define KNOODLE_GC_SHIM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *knoodle_gc_malloc(size_t n);
void *knoodle_gc_realloc(void *p, size_t n);

/* Free every block allocated since the last call. Invalidates all pointers
   previously returned by GC_MALLOC/GC_REALLOC — call only once the caller has
   copied out anything it still needs from the last homfly() result. */
void  knoodle_gc_free_all(void);

#ifdef __cplusplus
}
#endif

#define GC_INIT()        ((void)0)
#define GC_MALLOC(n)     knoodle_gc_malloc((size_t)(n))
#define GC_REALLOC(p, n) knoodle_gc_realloc((p), (size_t)(n))

#endif /* KNOODLE_GC_SHIM_H */

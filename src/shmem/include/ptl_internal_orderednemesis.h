#ifndef PTL_INTERNAL_NEMESIS_H
#define PTL_INTERNAL_NEMESIS_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* System headers */
#ifdef HAVE_PTHREAD_SHMEM_LOCKS
# include <pthread.h>                  /* for pthread_*_t */
#endif
#include <stdint.h>                    /* for uint32_t */

/* Internal headers */
#include "ptl_internal_alignment.h"
#include "ptl_internal_assert.h"
#include "ptl_internal_atomic.h"
#include "ptl_internal_commpad.h"

typedef struct ordered_NEMESIS_entry_s ordered_NEMESIS_entry;

typedef struct {
    volatile ordered_NEMESIS_entry *volatile ptr;
    volatile uint64_t                        val;
} ordered_NEMESIS_ptr;

struct ordered_NEMESIS_entry_s {
    ordered_NEMESIS_ptr volatile next;
    char                         data[];
};

typedef struct {
    /* The First Cacheline */
    ordered_NEMESIS_ptr head;
    ordered_NEMESIS_ptr tail;
    char                pad1[CACHELINE_WIDTH -
                             (2 * sizeof(ordered_NEMESIS_ptr))];
    /* The Second Cacheline */
    ordered_NEMESIS_ptr shadow_head;
    char                pad2[CACHELINE_WIDTH - sizeof(ordered_NEMESIS_ptr)];
} ordered_NEMESIS_queue ALIGNED (CACHELINE_WIDTH);

/***********************************************/
static inline ordered_NEMESIS_ptr PtlInternalAtomicCas128(volatile
                                                          ordered_NEMESIS_ptr
                                                          * addr,
                                                          const
                                                          ordered_NEMESIS_ptr
                                                          oldval,
                                                          const
                                                          ordered_NEMESIS_ptr
                                                          newval)
{                                      /*{{{ */
#ifdef HAVE_CMPXCHG16B
    ordered_NEMESIS_ptr ret;
    assert(((uintptr_t)addr & 0xf) == 0);
    __asm__ __volatile__ ("lock cmpxchg16b %0\n\t"
                          "mov %%rax, %1\n\t" // put rax into ret->ptr
                          "mov %%rdx, %2\n\t" // put rdx into ret->val
                          : "+m" (*addr),
                          "=q" (ret.ptr),
                          "=q" (ret.val)
                          : "a"  (oldval.ptr),
                          "d"  (oldval.val),
                          "b"  (newval.ptr),
                          "c"  (newval.val)
                          : "cc",
                          "memory");
    return ret;

#else  /* ifdef HAVE_CMPXCHG16B */
# error No known 128-bit atomic CAS operations are available
#endif  /* ifdef HAVE_CMPXCHG16B */
}                                      /*}}} */

static inline ordered_NEMESIS_ptr PtlInternalAtomicSwap128(
                                                           volatile
                                                           ordered_NEMESIS_ptr
                                                           *addr,
                                                           const
                                                           ordered_NEMESIS_ptr
                                                           newval)
{   /*{{{*/
    ordered_NEMESIS_ptr oldval = *addr;
    ordered_NEMESIS_ptr tmp;

    tmp = PtlInternalAtomicCas128(addr, oldval, newval);
    while (tmp.ptr != oldval.ptr || tmp.val != oldval.val) {
        oldval = tmp;
        tmp = PtlInternalAtomicCas128(addr, oldval, newval);
    }
    return oldval;
} /*}}}*/

static inline void PtlInternalOrderedNEMESISInit(ordered_NEMESIS_queue * q)
{
    assert(sizeof(ordered_NEMESIS_ptr) == 16);
    q->head.ptr = q->tail.ptr = NULL;
    q->head.val = q->tail.val = 0;
    q->shadow_head = q->head;
}

static inline void PtlInternalOrderedNEMESISEnqueue(ordered_NEMESIS_queue *
                                                    restrict q,
                                                    ordered_NEMESIS_entry *
                                                    e,
                                                    ptl_size_t v)
{
    ordered_NEMESIS_ptr f = { .ptr = e, .val = v };
    ordered_NEMESIS_ptr prev =
        PtlInternalAtomicSwap128(&(q->tail), f);

    if (prev.ptr == NULL) {
        q->head = f;
    } else {
        prev.ptr->next = f;
    }
}

static inline void *PtlInternalOrderedNEMESISDequeue(ordered_NEMESIS_queue
                                                     * q,
                                                     ptl_size_t upper_bound)
{
    ordered_NEMESIS_ptr retval = q->head;
    const ordered_NEMESIS_ptr nil = {.ptr = NULL, .val = 0};

    if (retval.ptr != NULL) {
        if (retval.ptr->next.ptr != NULL) {
            q->head = retval.ptr->next;
            retval.ptr->next.ptr = NULL;
        } else {
            ordered_NEMESIS_ptr old;
            q->head.ptr = NULL;
            old = PtlInternalAtomicCas128(&(q->tail), retval, nil);
            if ((old.ptr != retval.ptr) || (old.val != retval.val)) {
                while (retval.ptr->next.ptr == NULL) ;
                q->head = retval.ptr->next;
                retval.ptr->next.ptr = NULL;
            }
        }
        return (void*)(retval.ptr->data);
    } else {
        return NULL;
    }
}

#endif /* ifndef PTL_INTERNAL_NEMESIS_H */
/* vim:set expandtab: */
/*
 * Copyright (C) 1998-2000 Netscape Communications Corporation.
 * Copyright (C) 2003-6 Apple Computer
 *
 * Other contributors:
 *   Nick Blievers <nickb@adacel.com.au>
 *   Jeff Hostetler <jeff@nerdone.com>
 *   Tom Rini <trini@kernel.crashing.org>
 *   Raffaele Sena <raff@netwinder.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Alternatively, the contents of this file may be used under the terms
 * of either the Mozilla Public License Version 1.1, found at
 * http://www.mozilla.org/MPL/ (the "MPL") or the GNU General Public
 * License Version 2.0, found at http://www.fsf.org/copyleft/gpl.html
 * (the "GPL"), in which case the provisions of the MPL or the GPL are
 * applicable instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of one of those two
 * licenses (the MPL or the GPL) and not to allow others to use your
 * version of this file under the LGPL, indicate your decision by
 * deletingthe provisions above and replace them with the notice and
 * other provisions required by the MPL or the GPL, as the case may be.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under any of the LGPL, the MPL or the GPL.
 */

/*
 * Lifetime-based fast allocation, inspired by much prior art, including
 * "Fast Allocation and Deallocation of Memory Based on Object Lifetimes"
 * David R. Hanson, Software -- Practice and Experience, Vol. 20(1).
 */

#include "config.h"
#include "Arena.h"

#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <wtf/Assertions.h>
#include <wtf/FastMalloc.h>

using namespace std;

namespace WebCore {

//#define DEBUG_ARENA_MALLOC
#ifdef DEBUG_ARENA_MALLOC
static int i = 0;
#endif

#define FREELIST_MAX 30
static Arena *arena_freelist;
static int freelist_count = 0;

#define ARENA_DEFAULT_ALIGN  sizeof(double)
#define BIT(n)                          ((unsigned int)1 << (n))
#define BITMASK(n)                      (BIT(n) - 1)
#define CEILING_LOG2(_log2,_n)   \
      unsigned int j_ = (unsigned int)(_n);   \
      (_log2) = 0;                    \
      if ((j_) & ((j_)-1))            \
      (_log2) += 1;               \
      if ((j_) >> 16)                 \
      (_log2) += 16, (j_) >>= 16; \
      if ((j_) >> 8)                  \
      (_log2) += 8, (j_) >>= 8;   \
      if ((j_) >> 4)                  \
      (_log2) += 4, (j_) >>= 4;   \
      if ((j_) >> 2)                  \
      (_log2) += 2, (j_) >>= 2;   \
      if ((j_) >> 1)                  \
      (_log2) += 1;

static int CeilingLog2(unsigned int i) {
    int log2;
    CEILING_LOG2(log2,i);
    return log2;
}

void InitArenaPool(ArenaPool* pool, const char*, unsigned size, unsigned align)
{
     if (align == 0)
         align = ARENA_DEFAULT_ALIGN;
     pool->mask = BITMASK(CeilingLog2(align));
     pool->first.next = NULL;
     pool->first.base = pool->first.avail = pool->first.limit =
         (uword)ARENA_ALIGN(&pool->first + 1);
     pool->current = &pool->first;
     pool->arenasize = size;                                  
}


/*
 ** ArenaAllocate() -- allocate space from an arena pool
 ** 
 ** Description: ArenaAllocate() allocates space from an arena
 ** pool. 
 **
 ** First try to satisfy the request from arenas starting at
 ** pool->current.
 **
 ** If there is not enough space in the arena pool->current, try
 ** to claim an arena, on a first fit basis, from the global
 ** freelist (arena_freelist).
 ** 
 ** If no arena in arena_freelist is suitable, then try to
 ** allocate a new arena from the heap.
 **
 ** Returns: pointer to allocated space or NULL
 ** 
 */
void* ArenaAllocate(ArenaPool *pool, unsigned int nb)
{
    Arena *a;   
    char *rp;     /* returned pointer */

    ASSERT((nb & pool->mask) == 0);
    
    nb = (uword)ARENA_ALIGN(nb); /* force alignment */

    /* attempt to allocate from arenas at pool->current */
    {
        a = pool->current;
        do {
            if ( a->avail +nb <= a->limit )  {
                pool->current = a;
                rp = (char *)a->avail;
                a->avail += nb;
                return rp;
            }
        } while( NULL != (a = a->next) );
    }

    /* attempt to allocate from arena_freelist */
    {
        Arena *p = NULL; /* previous pointer, for unlinking from freelist */

        for ( a = arena_freelist; a != NULL ; p = a, a = a->next ) {
            if ( a->base +nb <= a->limit )  {
                if ( p == NULL )
                    arena_freelist = a->next;
                else
                    p->next = a->next;
                a->avail = a->base;
                rp = (char *)a->avail;
                a->avail += nb;
                /* the newly allocated arena is linked after pool->current 
                 *  and becomes pool->current */
                a->next = pool->current->next;
                pool->current->next = a;
                pool->current = a;
                if ( 0 == pool->first.next )
                    pool->first.next = a;
                freelist_count--;
                return(rp);
            }
        }
    }

    /* attempt to allocate from the heap */ 
    {  
        unsigned int sz = max(pool->arenasize, nb);
        sz += sizeof *a + pool->mask;  /* header and alignment slop */
#ifdef DEBUG_ARENA_MALLOC
        i++;
        printf("Malloc: %d\n", i);
#endif
        a = (Arena*)fastMalloc(sz);
        // fastMalloc will abort() if it fails, so we are guaranteed that a is not 0.
        a->limit = (uword)a + sz;
        a->base = a->avail = (uword)ARENA_ALIGN(a + 1);
        rp = (char *)a->avail;
        a->avail += nb;
        /* the newly allocated arena is linked after pool->current 
        *  and becomes pool->current */
        a->next = pool->current->next;
        pool->current->next = a;
        pool->current = a;
        if ( !pool->first.next )
            pool->first.next = a;
        return(rp);
    }
} /* --- end ArenaAllocate() --- */

/*
 * Free tail arenas linked after head, which may not be the true list head.
 * Reset pool->current to point to head in case it pointed at a tail arena.
 */
static void FreeArenaList(ArenaPool *pool, Arena *head, bool reallyFree)
{
    Arena **ap, *a;

    ap = &head->next;
    a = *ap;
    if (!a)
        return;

#ifdef DEBUG
    do {
        ASSERT(a->base <= a->avail && a->avail <= a->limit);
        a->avail = a->base;
        CLEAR_UNUSED(a);
    } while ((a = a->next) != 0);
    a = *ap;
#endif

    if (freelist_count >= FREELIST_MAX)
        reallyFree = true;
        
    if (reallyFree) {
        do {
            *ap = a->next;
            CLEAR_ARENA(a);
#ifdef DEBUG_ARENA_MALLOC
            if (a) {
                i--;
                printf("Free: %d\n", i);
            }
#endif
            fastFree(a); a = 0;
        } while ((a = *ap) != 0);
    } else {
        /* Insert the whole arena chain at the front of the freelist. */
        do {
            ap = &(*ap)->next;
            freelist_count++;
        } while (*ap);
        *ap = arena_freelist;
        arena_freelist = a;
        head->next = 0;
    }
    pool->current = head;
}

void FreeArenaPool(ArenaPool *pool)
{
    FreeArenaList(pool, &pool->first, false);
}

void FinishArenaPool(ArenaPool *pool)
{
    FreeArenaList(pool, &pool->first, true);
}

}

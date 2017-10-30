/*
 * Copyright © 2017  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_SORT_R_HH
#define HB_SORT_R_HH

#include "hb-private.hh"


static inline void *
hb_bsearch_r(const void *key, const void *base,
	     size_t nmemb, size_t size,
	     int (*compar)(const void *_key, const void *_item, void *_arg),
	     void *arg)
{
  int min = 0, max = (int) nmemb - 1;
  while (min <= max)
  {
    int mid = (min + max) / 2;
    const void *p = (const void *) (((const char *) base) + (mid * size));
    int c = compar (key, p, arg);
    if (c < 0)
      max = mid - 1;
    else if (c > 0)
      min = mid + 1;
    else
      return (void *) p;
  }
  return NULL;
}



/* From https://github.com/noporpoise/sort_r */

/* Isaac Turner 29 April 2014 Public Domain */

/*

hb_sort_r function to be exported.

Parameters:
  base is the array to be sorted
  nel is the number of elements in the array
  width is the size in bytes of each element of the array
  compar is the comparison function
  arg is a pointer to be passed to the comparison function

void hb_sort_r(void *base, size_t nel, size_t width,
               int (*compar)(const void *_a, const void *_b, void *_arg),
               void *arg);

*/

#define _SORT_R_INLINE inline

#if (defined __APPLE__ || defined __MACH__ || defined __DARWIN__ || \
     defined __FreeBSD__ || defined __DragonFly__)
#  define _SORT_R_BSD
#elif (defined _GNU_SOURCE || defined __gnu_hurd__ || defined __GNU__ || \
       defined __linux__ || defined __MINGW32__ || defined __GLIBC__)
#  define _SORT_R_LINUX
#elif (defined _WIN32 || defined _WIN64 || defined __WINDOWS__)
#  define _SORT_R_WINDOWS
#  undef _SORT_R_INLINE
#  define _SORT_R_INLINE __inline
#else
  /* Using our own recursive quicksort sort_r_simple() */
#endif

#if (defined NESTED_QSORT && NESTED_QSORT == 0)
#  undef NESTED_QSORT
#endif

/* swap a, b iff a>b */
/* __restrict is same as restrict but better support on old machines */
static _SORT_R_INLINE int sort_r_cmpswap(char *__restrict a, char *__restrict b, size_t w,
                                         int (*compar)(const void *_a, const void *_b,
                                                       void *_arg),
                                         void *arg)
{
  char tmp, *end = a+w;
  if(compar(a, b, arg) > 0) {
    for(; a < end; a++, b++) { tmp = *a; *a = *b; *b = tmp; }
    return 1;
  }
  return 0;
}

/* Implement recursive quicksort ourselves */
/* Note: quicksort is not stable, equivalent values may be swapped */
static _SORT_R_INLINE void sort_r_simple(void *base, size_t nel, size_t w,
                                         int (*compar)(const void *_a, const void *_b,
                                                       void *_arg),
                                         void *arg)
{
  char *b = (char *)base, *end = b + nel*w;
  if(nel < 7) {
    /* Insertion sort for arbitrarily small inputs */
    char *pi, *pj;
    for(pi = b+w; pi < end; pi += w) {
      for(pj = pi; pj > b && sort_r_cmpswap(pj-w,pj,w,compar,arg); pj -= w) {}
    }
  }
  else
  {
    /* nel > 6; Quicksort */

    /* Use median of first, middle and last items as pivot */
    char *x, *y, *xend, ch;
    char *pl, *pr;
    char *last = b+w*(nel-1), *tmp;
    char *l[3];
    l[0] = b;
    l[1] = b+w*(nel/2);
    l[2] = last;

    if(compar(l[0],l[1],arg) > 0) { tmp=l[0]; l[0]=l[1]; l[1]=tmp; }
    if(compar(l[1],l[2],arg) > 0) {
      tmp=l[1]; l[1]=l[2]; l[2]=tmp; /* swap(l[1],l[2]) */
      if(compar(l[0],l[1],arg) > 0) { tmp=l[0]; l[0]=l[1]; l[1]=tmp; }
    }

    /* swap l[id], l[2] to put pivot as last element */
    for(x = l[1], y = last, xend = x+w; x<xend; x++, y++) {
      ch = *x; *x = *y; *y = ch;
    }

    pl = b;
    pr = last;

    while(pl < pr) {
      for(; pl < pr; pl += w) {
        if(sort_r_cmpswap(pl, pr, w, compar, arg)) {
          pr -= w; /* pivot now at pl */
          break;
        }
      }
      for(; pl < pr; pr -= w) {
        if(sort_r_cmpswap(pl, pr, w, compar, arg)) {
          pl += w; /* pivot now at pr */
          break;
        }
      }
    }

    sort_r_simple(b, (pl-b)/w, w, compar, arg);
    sort_r_simple(pl+w, (end-(pl+w))/w, w, compar, arg);
  }
}


#if defined NESTED_QSORT

  static _SORT_R_INLINE void hb_sort_r(void *base, size_t nel, size_t width,
                                       int (*compar)(const void *_a, const void *_b,
                                                     void *aarg),
                                       void *arg)
  {
    int nested_cmp(const void *a, const void *b)
    {
      return compar(a, b, arg);
    }

    qsort(base, nel, width, nested_cmp);
  }

#else /* !NESTED_QSORT */

  /* Declare structs and functions */

  #if defined _SORT_R_BSD

    /* Ensure qsort_r is defined */
    extern void qsort_r(void *base, size_t nel, size_t width, void *thunk,
                        int (*compar)(void *_thunk, const void *_a, const void *_b));

  #endif

  #if defined _SORT_R_BSD || defined _SORT_R_WINDOWS

    /* BSD (qsort_r), Windows (qsort_s) require argument swap */

    struct sort_r_data
    {
      void *arg;
      int (*compar)(const void *_a, const void *_b, void *_arg);
    };

    static _SORT_R_INLINE int sort_r_arg_swap(void *s, const void *a, const void *b)
    {
      struct sort_r_data *ss = (struct sort_r_data*)s;
      return (ss->compar)(a, b, ss->arg);
    }

  #endif

  #if defined _SORT_R_LINUX

#if 0 /* BE: To avoid redeclaration warning. */
    typedef int(* __compar_d_fn_t)(const void *, const void *, void *);
    extern void qsort_r(void *base, size_t nel, size_t width,
                        __compar_d_fn_t __compar, void *arg)
      __attribute__((nonnull (1, 4)));
#endif

  #endif

  /* implementation */

  static _SORT_R_INLINE void hb_sort_r(void *base, size_t nel, size_t width,
                                       int (*compar)(const void *_a, const void *_b, void *_arg),
                                       void *arg)
  {
    #if defined _SORT_R_LINUX

      #if defined __GLIBC__ && ((__GLIBC__ < 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 8))

        /* no qsort_r in glibc before 2.8, need to use nested qsort */
        sort_r_simple(base, nel, width, compar, arg);

      #else

        qsort_r(base, nel, width, compar, arg);

      #endif

    #elif defined _SORT_R_BSD

      struct sort_r_data tmp;
      tmp.arg = arg;
      tmp.compar = compar;
      qsort_r(base, nel, width, &tmp, sort_r_arg_swap);

    #elif defined _SORT_R_WINDOWS

      struct sort_r_data tmp;
      tmp.arg = arg;
      tmp.compar = compar;
      qsort_s(base, nel, width, sort_r_arg_swap, &tmp);

    #else

      /* Fall back to our own quicksort implementation */
      sort_r_simple(base, nel, width, compar, arg);

    #endif
  }

#endif /* !NESTED_QSORT */

#undef _SORT_R_INLINE
#undef _SORT_R_WINDOWS
#undef _SORT_R_LINUX
#undef _SORT_R_BSD

#endif /* HB_SORT_R_HH */

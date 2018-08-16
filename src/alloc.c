/* Storage allocation and gc for GNU Emacs Lisp interpreter.

Copyright (C) 1985-1986, 1988, 1993-1995, 1997-2018 Free Software
Foundation, Inc.

This file is part of GNU Emacs.

GNU Emacs is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

GNU Emacs is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Emacs.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>		/* For CHAR_BIT.  */
#include <signal.h>		/* For SIGABRT, SIGDANGER.  */

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

#include "lisp.h"
#include "dispextern.h"
#include "intervals.h"
#include "ptr-bounds.h"
#include "puresize.h"
#include "sheap.h"
#include "systime.h"
#include "character.h"
#include "buffer.h"
#include "window.h"
#include "keyboard.h"
#include "frame.h"
#include "blockinput.h"
#include "termhooks.h"		/* For struct terminal.  */
#ifdef HAVE_WINDOW_SYSTEM
#include TERM_HEADER
#endif /* HAVE_WINDOW_SYSTEM */

#include <flexmember.h>
#include <verify.h>
#include <execinfo.h>           /* For backtrace.  */

#ifdef HAVE_LINUX_SYSINFO
#include <sys/sysinfo.h>
#endif

#ifdef MSDOS
#include "dosfns.h"		/* For dos_memory_info.  */
#endif

#ifdef HAVE_MALLOC_H
# include <malloc.h>
#endif

#if (defined ENABLE_CHECKING			\
     && defined HAVE_VALGRIND_VALGRIND_H	\
     && !defined USE_VALGRIND)
# define USE_VALGRIND 1
#endif

#if USE_VALGRIND
#include <valgrind/valgrind.h>
#include <valgrind/memcheck.h>
static bool valgrind_p;
#endif

/* GC_CHECK_MARKED_OBJECTS means do sanity checks on allocated objects.
   We turn that on by default when ENABLE_CHECKING is defined;
   define GC_CHECK_MARKED_OBJECTS to zero to disable.  */

#if defined ENABLE_CHECKING && !defined GC_CHECK_MARKED_OBJECTS
# define GC_CHECK_MARKED_OBJECTS 1
#endif

/* GC_MALLOC_CHECK defined means perform validity checks of malloc'd
   memory.  Can do this only if using gmalloc.c and if not checking
   marked objects.  */

#if (defined SYSTEM_MALLOC || defined DOUG_LEA_MALLOC \
     || defined HYBRID_MALLOC || GC_CHECK_MARKED_OBJECTS)
#undef GC_MALLOC_CHECK
#endif

#include <unistd.h>
#include <fcntl.h>

#ifdef USE_GTK
# include "gtkutil.h"
#endif
#ifdef WINDOWSNT
#include "w32.h"
#include "w32heap.h"	/* for sbrk */
#endif

#if defined GNU_LINUX && !defined CANNOT_DUMP
/* The address where the heap starts.  */
void *
my_heap_start (void)
{
  static void *start;
  if (! start)
    start = sbrk (0);
  return start;
}
#endif

#ifdef DOUG_LEA_MALLOC

/* Specify maximum number of areas to mmap.  It would be nice to use a
   value that explicitly means "no limit".  */

#define MMAP_MAX_AREAS 100000000

/* A pointer to the memory allocated that copies that static data
   inside glibc's malloc.  */
static void *malloc_state_ptr;

/* Restore the dumped malloc state.  Because malloc can be invoked
   even before main (e.g. by the dynamic linker), the dumped malloc
   state must be restored as early as possible using this special hook.  */
static void
malloc_initialize_hook (void)
{
  static bool malloc_using_checking;

  if (! initialized)
    {
#ifdef GNU_LINUX
      my_heap_start ();
#endif
      malloc_using_checking = getenv ("MALLOC_CHECK_") != NULL;
    }
  else
    {
      if (!malloc_using_checking)
	{
	  /* Work around a bug in glibc's malloc.  MALLOC_CHECK_ must be
	     ignored if the heap to be restored was constructed without
	     malloc checking.  Can't use unsetenv, since that calls malloc.  */
	  char **p = environ;
	  if (p)
	    for (; *p; p++)
	      if (strncmp (*p, "MALLOC_CHECK_=", 14) == 0)
		{
		  do
		    *p = p[1];
		  while (*++p);

		  break;
		}
	}

      if (malloc_set_state (malloc_state_ptr) != 0)
	emacs_abort ();
# ifndef XMALLOC_OVERRUN_CHECK
      alloc_unexec_post ();
# endif
    }
}

/* Declare the malloc initialization hook, which runs before 'main' starts.
   EXTERNALLY_VISIBLE works around Bug#22522.  */
typedef void (*voidfuncptr) (void);
# ifndef __MALLOC_HOOK_VOLATILE
#  define __MALLOC_HOOK_VOLATILE
# endif
voidfuncptr __MALLOC_HOOK_VOLATILE __malloc_initialize_hook EXTERNALLY_VISIBLE
  = malloc_initialize_hook;

#endif

#if defined DOUG_LEA_MALLOC || !defined CANNOT_DUMP

/* Allocator-related actions to do just before and after unexec.  */

void
alloc_unexec_pre (void)
{
# ifdef DOUG_LEA_MALLOC
  malloc_state_ptr = malloc_get_state ();
  if (!malloc_state_ptr)
    fatal ("malloc_get_state: %s", strerror (errno));
# endif
# ifdef HYBRID_MALLOC
  bss_sbrk_did_unexec = true;
# endif
}

void
alloc_unexec_post (void)
{
# ifdef DOUG_LEA_MALLOC
  free (malloc_state_ptr);
# endif
# ifdef HYBRID_MALLOC
  bss_sbrk_did_unexec = false;
# endif
}
#endif

#define VECTOR_MARK(V)		((V)->header.size |= ARRAY_MARK_FLAG)
#define VECTOR_MARKED_P(V)	(((V)->header.size & ARRAY_MARK_FLAG) != 0)

/* Default value of gc_cons_threshold (see below).  */

#define GC_DEFAULT_THRESHOLD (100000 * word_size)

/* Global variables.  */
struct emacs_globals globals;

/* Number of bytes of consing done since the last gc.  */

EMACS_INT consing_since_gc;

/* Similar minimum, computed from Vgc_cons_percentage.  */

EMACS_INT gc_relative_threshold;

/* Minimum number of bytes of consing since GC before next GC,
   when memory is full.  */

EMACS_INT memory_full_cons_threshold;

/* True during GC.  */

bool gc_in_progress;

/* Number of live and free conses etc.  */

static EMACS_INT total_conses, total_markers, total_symbols;
static EMACS_INT total_free_conses;

/* Points to memory space allocated as "spare", to be freed if we run
   out of memory.  We keep one large block, four cons-blocks, and
   two string blocks.  */

static char *spare_memory[7];

/* Amount of spare memory to keep in large reserve block, or to see
   whether this much is available when malloc fails on a larger request.  */

#define SPARE_MEMORY (1 << 14)

/* Initialize it to a nonzero value to force it into data space
   (rather than bss space).  That way unexec will remap it into text
   space (pure), on some systems.  We have not implemented the
   remapping on more recent systems because this is less important
   nowadays than in the days of small memories and timesharing.  */

EMACS_INT pure[(PURESIZE + sizeof (EMACS_INT) - 1) / sizeof (EMACS_INT)] = {1,};
#define PUREBEG (char *) pure

/* Pointer to the pure area, and its size.  */

static char *purebeg;
static ptrdiff_t pure_size;

/* Number of bytes of pure storage used before pure storage overflowed.
   If this is non-zero, this implies that an overflow occurred.  */

static ptrdiff_t pure_bytes_used_before_overflow;

/* Index in pure at which next pure Lisp object will be allocated..  */

static ptrdiff_t pure_bytes_used_lisp;

/* Number of bytes allocated for non-Lisp objects in pure storage.  */

static ptrdiff_t pure_bytes_used_non_lisp;

/* If nonzero, this is a warning delivered by malloc and not yet
   displayed.  */

const char *pending_malloc_warning;

#if 0 /* Normally, pointer sanity only on request... */
#ifdef ENABLE_CHECKING
#define SUSPICIOUS_OBJECT_CHECKING 1
#endif
#endif

/* ... but unconditionally use SUSPICIOUS_OBJECT_CHECKING while the GC
   bug is unresolved.  */
#define SUSPICIOUS_OBJECT_CHECKING 1

#ifdef SUSPICIOUS_OBJECT_CHECKING
struct suspicious_free_record
{
  void *suspicious_object;
  void *backtrace[128];
};
static void *suspicious_objects[32];
static int suspicious_object_index;
struct suspicious_free_record suspicious_free_history[64] EXTERNALLY_VISIBLE;
static int suspicious_free_history_index;
/* Find the first currently-monitored suspicious pointer in range
   [begin,end) or NULL if no such pointer exists.  */
static void *find_suspicious_object_in_range (void *begin, void *end);
static void detect_suspicious_free (void *ptr);
#else
# define find_suspicious_object_in_range(begin, end) NULL
# define detect_suspicious_free(ptr) (void)
#endif

/* Maximum amount of C stack to save when a GC happens.  */

#ifndef MAX_SAVE_STACK
#define MAX_SAVE_STACK 16000
#endif

/* Buffer in which we save a copy of the C stack at each GC.  */

static void mark_terminals (void);
static Lisp_Object make_pure_vector (ptrdiff_t);
static void mark_buffer (struct buffer *);

#if !defined REL_ALLOC || defined SYSTEM_MALLOC || defined HYBRID_MALLOC
static void refill_memory_reserve (void);
#endif
static void compact_small_strings (void);
extern Lisp_Object which_symbols (Lisp_Object, EMACS_INT) EXTERNALLY_VISIBLE;

/* When scanning the C stack for live Lisp objects, Emacs keeps track of
   what memory allocated via lisp_malloc and lisp_align_malloc is intended
   for what purpose.  This enumeration specifies the type of memory.  */

enum mem_type
{
  MEM_TYPE_NON_LISP,
  MEM_TYPE_BUFFER,
  MEM_TYPE_CONS,
  MEM_TYPE_STRING,
  MEM_TYPE_SYMBOL,
  MEM_TYPE_FLOAT,
  /* Since all non-bool pseudovectors are small enough to be
     allocated from vector blocks, this memory type denotes
     large regular vectors and large bool pseudovectors.  */
  MEM_TYPE_VECTORLIKE,
  /* Special type to denote vector blocks.  */
  MEM_TYPE_VECTOR_BLOCK,
  /* Special type to denote reserved memory.  */
  MEM_TYPE_SPARE
};

/* A unique object in pure space used to make some Lisp objects
   on free lists recognizable in O(1).  */

static Lisp_Object Vdead;
#define DEADP(x) EQ (x, Vdead)

#ifdef GC_MALLOC_CHECK

enum mem_type allocated_mem_type;

#endif /* GC_MALLOC_CHECK */

/* A node in the red-black tree describing allocated memory containing
   Lisp data.  Each such block is recorded with its start and end
   address when it is allocated, and removed from the tree when it
   is freed.

   A red-black tree is a balanced binary tree with the following
   properties:

   1. Every node is either red or black.
   2. Every leaf is black.
   3. If a node is red, then both of its children are black.
   4. Every simple path from a node to a descendant leaf contains
   the same number of black nodes.
   5. The root is always black.

   When nodes are inserted into the tree, or deleted from the tree,
   the tree is "fixed" so that these properties are always true.

   A red-black tree with N internal nodes has height at most 2
   log(N+1).  Searches, insertions and deletions are done in O(log N).
   Please see a text book about data structures for a detailed
   description of red-black trees.  Any book worth its salt should
   describe them.  */

enum mem_node_color {MEM_BLACK, MEM_RED};

struct mem_node
{
  /* Children of this node.  These pointers are never NULL.  When there
     is no child, the value is MEM_NIL, which points to a dummy node.  */
  struct mem_node *left, *right;

  /* The parent of this node.  In the root node, this is NULL.  */
  struct mem_node *parent;

  /* Start and end of allocated region.  */
  void *start, *end;

  /* Node color.  */
  enum mem_node_color color;

  /* Memory type.  */
  enum mem_type type;
};

/* Root of the tree describing allocated Lisp memory.  */

static struct mem_node *mem_root;

/* Lowest and highest known address in the heap.  */

static void *min_heap_address, *max_heap_address;

/* Sentinel node of the tree.  */

static struct mem_node mem_z;
#define MEM_NIL &mem_z

static struct mem_node *mem_insert (void *, void *, enum mem_type);
static void mem_insert_fixup (struct mem_node *);
static void mem_rotate_left (struct mem_node *);
static void mem_rotate_right (struct mem_node *);
static void mem_delete (struct mem_node *);
static void mem_delete_fixup (struct mem_node *);
static struct mem_node *mem_find (void *);

#ifndef DEADP
# define DEADP(x) 0
#endif

/* Addresses of staticpro'd variables.  Initialize it to a nonzero
   value; otherwise some compilers put it into BSS.  */

enum { NSTATICS = 2048 };
ELisp_Struct_Value *staticvec[NSTATICS] = {};

/* Index of next unused slot in staticvec.  */

static int staticidx;

static void *pure_alloc (size_t, int);

/* True if N is a power of 2.  N should be positive.  */

#define POWER_OF_2(n) (((n) & ((n) - 1)) == 0)

/* Return X rounded to the next multiple of Y.  Y should be positive,
   and Y - 1 + X should not overflow.  Arguments should not have side
   effects, as they are evaluated more than once.  Tune for Y being a
   power of 2.  */

#define ROUNDUP(x, y) (POWER_OF_2 (y)					\
		       ? ((y) - 1 + (x)) & ~ ((y) - 1)			\
		       : ((y) - 1 + (x)) - ((y) - 1 + (x)) % (y))

/* Return PTR rounded up to the next multiple of ALIGNMENT.  */

static void *
pointer_align (void *ptr, int alignment)
{
  return (void *) ROUNDUP ((uintptr_t) ptr, alignment);
}

static void
XFLOAT_INIT (Lisp_Object *f, double n)
{
  f.u.stack[0].v.v.setDouble(n);
}

#ifdef DOUG_LEA_MALLOC
static bool
pointers_fit_in_lispobj_p (void)
{
  return (UINTPTR_MAX <= VAL_MAX) || USE_LSB_TAG;
}

static bool
mmap_lisp_allowed_p (void)
{
  /* If we can't store all memory addresses in our lisp objects, it's
     risky to let the heap use mmap and give us addresses from all
     over our address space.  We also can't use mmap for lisp objects
     if we might dump: unexec doesn't preserve the contents of mmapped
     regions.  */
  return pointers_fit_in_lispobj_p () && !might_dump;
}
#endif

/* Head of a circularly-linked list of extant finalizers. */
static struct Lisp_Finalizer finalizers;

/* Head of a circularly-linked list of finalizers that must be invoked
   because we deemed them unreachable.  This list must be global, and
   not a local inside garbage_collect_1, in case we GC again while
   running finalizers.  */
static struct Lisp_Finalizer doomed_finalizers;


/************************************************************************
				Malloc
 ************************************************************************/

#if defined SIGDANGER || (!defined SYSTEM_MALLOC && !defined HYBRID_MALLOC)

/* Function malloc calls this if it finds we are near exhausting storage.  */

void
malloc_warning (const char *str)
{
  pending_malloc_warning = str;
}

#endif

/* Display an already-pending malloc warning.  */

void
display_malloc_warning (void)
{
  call3 (intern ("display-warning"),
	 intern ("alloc"),
	 build_string (pending_malloc_warning),
	 intern ("emergency"));
  pending_malloc_warning = 0;
}

/* Called if we can't allocate relocatable space for a buffer.  */

void
buffer_memory_full (ptrdiff_t nbytes)
{
  /* If buffers use the relocating allocator, no need to free
     spare_memory, because we may have plenty of malloc space left
     that we could get, and if we don't, the malloc that fails will
     itself cause spare_memory to be freed.  If buffers don't use the
     relocating allocator, treat this like any other failing
     malloc.  */

#ifndef REL_ALLOC
  memory_full (nbytes);
#else
  /* This used to call error, but if we've run out of memory, we could
     get infinite recursion trying to build the string.  */
  xsignal (Qnil, Vmemory_signal_data);
#endif
}

/* A common multiple of the positive integers A and B.  Ideally this
   would be the least common multiple, but there's no way to do that
   as a constant expression in C, so do the best that we can easily do.  */
#define COMMON_MULTIPLE(a, b) \
  ((a) % (b) == 0 ? (a) : (b) % (a) == 0 ? (b) : (a) * (b))

/* LISP_ALIGNMENT is the alignment of Lisp objects.  It must be at
   least GCALIGNMENT so that pointers can be tagged.  It also must be
   at least as strict as the alignment of all the C types used to
   implement Lisp objects; since pseudovectors can contain any C type,
   this is max_align_t.  On recent GNU/Linux x86 and x86-64 this can
   often waste up to 8 bytes, since alignof (max_align_t) is 16 but
   typical vectors need only an alignment of 8.  However, it is not
   worth the hassle to avoid this waste.  */
enum { LISP_ALIGNMENT = 8 };
verify (LISP_ALIGNMENT % GCALIGNMENT == 0);

/* True if malloc (N) is known to return storage suitably aligned for
   Lisp objects whenever N is a multiple of LISP_ALIGNMENT.  In
   practice this is true whenever alignof (max_align_t) is also a
   multiple of LISP_ALIGNMENT.  This works even for x86, where some
   platform combinations (e.g., GCC 7 and later, glibc 2.25 and
   earlier) have bugs where alignof (max_align_t) is 16 even though
   the malloc alignment is only 8, and where Emacs still works because
   it never does anything that requires an alignment of 16.  */
enum { MALLOC_IS_LISP_ALIGNED = alignof (max_align_t) % LISP_ALIGNMENT == 0 };

#ifndef XMALLOC_OVERRUN_CHECK
#define XMALLOC_OVERRUN_CHECK_OVERHEAD 0
#else

/* Check for overrun in malloc'ed buffers by wrapping a header and trailer
   around each block.

   The header consists of XMALLOC_OVERRUN_CHECK_SIZE fixed bytes
   followed by XMALLOC_OVERRUN_SIZE_SIZE bytes containing the original
   block size in little-endian order.  The trailer consists of
   XMALLOC_OVERRUN_CHECK_SIZE fixed bytes.

   The header is used to detect whether this block has been allocated
   through these functions, as some low-level libc functions may
   bypass the malloc hooks.  */

#define XMALLOC_OVERRUN_CHECK_SIZE 16
#define XMALLOC_OVERRUN_CHECK_OVERHEAD \
  (2 * XMALLOC_OVERRUN_CHECK_SIZE + XMALLOC_OVERRUN_SIZE_SIZE)

/* Define XMALLOC_OVERRUN_SIZE_SIZE so that (1) it's large enough to
   hold a size_t value and (2) the header size is a multiple of the
   alignment that Emacs needs for C types and for USE_LSB_TAG.  */
#define XMALLOC_OVERRUN_SIZE_SIZE				\
   (((XMALLOC_OVERRUN_CHECK_SIZE + sizeof (size_t)		\
      + LISP_ALIGNMENT - 1)					\
     / LISP_ALIGNMENT * LISP_ALIGNMENT)				\
    - XMALLOC_OVERRUN_CHECK_SIZE)

static char const xmalloc_overrun_check_header[XMALLOC_OVERRUN_CHECK_SIZE] =
  { '\x9a', '\x9b', '\xae', '\xaf',
    '\xbf', '\xbe', '\xce', '\xcf',
    '\xea', '\xeb', '\xec', '\xed',
    '\xdf', '\xde', '\x9c', '\x9d' };

static char const xmalloc_overrun_check_trailer[XMALLOC_OVERRUN_CHECK_SIZE] =
  { '\xaa', '\xab', '\xac', '\xad',
    '\xba', '\xbb', '\xbc', '\xbd',
    '\xca', '\xcb', '\xcc', '\xcd',
    '\xda', '\xdb', '\xdc', '\xdd' };

/* Insert and extract the block size in the header.  */

static void
xmalloc_put_size (unsigned char *ptr, size_t size)
{
  int i;
  for (i = 0; i < XMALLOC_OVERRUN_SIZE_SIZE; i++)
    {
      *--ptr = size & ((1 << CHAR_BIT) - 1);
      size >>= CHAR_BIT;
    }
}

static size_t
xmalloc_get_size (unsigned char *ptr)
{
  size_t size = 0;
  int i;
  ptr -= XMALLOC_OVERRUN_SIZE_SIZE;
  for (i = 0; i < XMALLOC_OVERRUN_SIZE_SIZE; i++)
    {
      size <<= CHAR_BIT;
      size += *ptr++;
    }
  return size;
}


/* Like malloc, but wraps allocated block with header and trailer.  */

static void *
overrun_check_malloc (size_t size)
{
  register unsigned char *val;
  if (SIZE_MAX - XMALLOC_OVERRUN_CHECK_OVERHEAD < size)
    emacs_abort ();

  val = malloc (size + XMALLOC_OVERRUN_CHECK_OVERHEAD);
  if (val)
    {
      memcpy (val, xmalloc_overrun_check_header, XMALLOC_OVERRUN_CHECK_SIZE);
      val += XMALLOC_OVERRUN_CHECK_SIZE + XMALLOC_OVERRUN_SIZE_SIZE;
      xmalloc_put_size (val, size);
      memcpy (val + size, xmalloc_overrun_check_trailer,
	      XMALLOC_OVERRUN_CHECK_SIZE);
    }
  return val;
}


/* Like realloc, but checks old block for overrun, and wraps new block
   with header and trailer.  */

static void *
overrun_check_realloc (void *block, size_t size)
{
  register unsigned char *val = (unsigned char *) block;
  if (SIZE_MAX - XMALLOC_OVERRUN_CHECK_OVERHEAD < size)
    emacs_abort ();

  if (val
      && memcmp (xmalloc_overrun_check_header,
		 val - XMALLOC_OVERRUN_CHECK_SIZE - XMALLOC_OVERRUN_SIZE_SIZE,
		 XMALLOC_OVERRUN_CHECK_SIZE) == 0)
    {
      size_t osize = xmalloc_get_size (val);
      if (memcmp (xmalloc_overrun_check_trailer, val + osize,
		  XMALLOC_OVERRUN_CHECK_SIZE))
	emacs_abort ();
      memset (val + osize, 0, XMALLOC_OVERRUN_CHECK_SIZE);
      val -= XMALLOC_OVERRUN_CHECK_SIZE + XMALLOC_OVERRUN_SIZE_SIZE;
      memset (val, 0, XMALLOC_OVERRUN_CHECK_SIZE + XMALLOC_OVERRUN_SIZE_SIZE);
    }

  val = realloc (val, size + XMALLOC_OVERRUN_CHECK_OVERHEAD);

  if (val)
    {
      memcpy (val, xmalloc_overrun_check_header, XMALLOC_OVERRUN_CHECK_SIZE);
      val += XMALLOC_OVERRUN_CHECK_SIZE + XMALLOC_OVERRUN_SIZE_SIZE;
      xmalloc_put_size (val, size);
      memcpy (val + size, xmalloc_overrun_check_trailer,
	      XMALLOC_OVERRUN_CHECK_SIZE);
    }
  return val;
}

/* Like free, but checks block for overrun.  */

static void
overrun_check_free (void *block)
{
  unsigned char *val = (unsigned char *) block;

  if (val
      && memcmp (xmalloc_overrun_check_header,
		 val - XMALLOC_OVERRUN_CHECK_SIZE - XMALLOC_OVERRUN_SIZE_SIZE,
		 XMALLOC_OVERRUN_CHECK_SIZE) == 0)
    {
      size_t osize = xmalloc_get_size (val);
      if (memcmp (xmalloc_overrun_check_trailer, val + osize,
		  XMALLOC_OVERRUN_CHECK_SIZE))
	emacs_abort ();
#ifdef XMALLOC_CLEAR_FREE_MEMORY
      val -= XMALLOC_OVERRUN_CHECK_SIZE + XMALLOC_OVERRUN_SIZE_SIZE;
      memset (val, 0xff, osize + XMALLOC_OVERRUN_CHECK_OVERHEAD);
#else
      memset (val + osize, 0, XMALLOC_OVERRUN_CHECK_SIZE);
      val -= XMALLOC_OVERRUN_CHECK_SIZE + XMALLOC_OVERRUN_SIZE_SIZE;
      memset (val, 0, XMALLOC_OVERRUN_CHECK_SIZE + XMALLOC_OVERRUN_SIZE_SIZE);
#endif
    }

  free (val);
}

#undef malloc
#undef realloc
#undef free
#define malloc overrun_check_malloc
#define realloc overrun_check_realloc
#define free overrun_check_free
#endif

/* If compiled with XMALLOC_BLOCK_INPUT_CHECK, define a symbol
   BLOCK_INPUT_IN_MEMORY_ALLOCATORS that is visible to the debugger.
   If that variable is set, block input while in one of Emacs's memory
   allocation functions.  There should be no need for this debugging
   option, since signal handlers do not allocate memory, but Emacs
   formerly allocated memory in signal handlers and this compile-time
   option remains as a way to help debug the issue should it rear its
   ugly head again.  */
#ifdef XMALLOC_BLOCK_INPUT_CHECK
bool block_input_in_memory_allocators EXTERNALLY_VISIBLE;
static void
malloc_block_input (void)
{
  if (block_input_in_memory_allocators)
    block_input ();
}
static void
malloc_unblock_input (void)
{
  if (block_input_in_memory_allocators)
    unblock_input ();
}
# define MALLOC_BLOCK_INPUT malloc_block_input ()
# define MALLOC_UNBLOCK_INPUT malloc_unblock_input ()
#else
# define MALLOC_BLOCK_INPUT ((void) 0)
# define MALLOC_UNBLOCK_INPUT ((void) 0)
#endif

#define MALLOC_PROBE(size)			\
  do {						\
    if (profiler_memory_running)		\
      malloc_probe (size);			\
  } while (0)

static void *lmalloc (size_t) ATTRIBUTE_MALLOC_SIZE ((1));
static void *lrealloc (void *, size_t);

/* Like malloc but check for no memory and block interrupt input.  */

void *
xmalloc (size_t size)
{
  void *val;

  MALLOC_BLOCK_INPUT;
  val = lmalloc (size);
  MALLOC_UNBLOCK_INPUT;

  if (!val && size)
    memory_full (size);
  MALLOC_PROBE (size);
  return val;
}

/* Like the above, but zeroes out the memory just allocated.  */

void *
xzalloc (size_t size)
{
  void *val;

  MALLOC_BLOCK_INPUT;
  val = lmalloc (size);
  MALLOC_UNBLOCK_INPUT;

  if (!val && size)
    memory_full (size);
  memset (val, 0, size);
  MALLOC_PROBE (size);
  return val;
}

/* Like realloc but check for no memory and block interrupt input..  */

void *
xrealloc (void *block, size_t size)
{
  void *val;

  MALLOC_BLOCK_INPUT;
  /* We must call malloc explicitly when BLOCK is 0, since some
     reallocs don't do this.  */
  if (! block)
    val = lmalloc (size);
  else
    val = lrealloc (block, size);
  MALLOC_UNBLOCK_INPUT;

  if (!val && size)
    memory_full (size);
  MALLOC_PROBE (size);
  return val;
}


/* Like free but block interrupt input.  */

void
xfree (void *block)
{
  if (!block)
    return;
  MALLOC_BLOCK_INPUT;
  free (block);
  MALLOC_UNBLOCK_INPUT;
  /* We don't call refill_memory_reserve here
     because in practice the call in r_alloc_free seems to suffice.  */
}


/* Other parts of Emacs pass large int values to allocator functions
   expecting ptrdiff_t.  This is portable in practice, but check it to
   be safe.  */
verify (INT_MAX <= PTRDIFF_MAX);


/* Allocate an array of NITEMS items, each of size ITEM_SIZE.
   Signal an error on memory exhaustion, and block interrupt input.  */

void *
xnmalloc (ptrdiff_t nitems, ptrdiff_t item_size)
{
  eassert (0 <= nitems && 0 < item_size);
  ptrdiff_t nbytes;
  if (INT_MULTIPLY_WRAPV (nitems, item_size, &nbytes) || SIZE_MAX < nbytes)
    memory_full (SIZE_MAX);
  return xmalloc (nbytes);
}


/* Reallocate an array PA to make it of NITEMS items, each of size ITEM_SIZE.
   Signal an error on memory exhaustion, and block interrupt input.  */

void *
xnrealloc (void *pa, ptrdiff_t nitems, ptrdiff_t item_size)
{
  eassert (0 <= nitems && 0 < item_size);
  ptrdiff_t nbytes;
  if (INT_MULTIPLY_WRAPV (nitems, item_size, &nbytes) || SIZE_MAX < nbytes)
    memory_full (SIZE_MAX);
  return xrealloc (pa, nbytes);
}


/* Grow PA, which points to an array of *NITEMS items, and return the
   location of the reallocated array, updating *NITEMS to reflect its
   new size.  The new array will contain at least NITEMS_INCR_MIN more
   items, but will not contain more than NITEMS_MAX items total.
   ITEM_SIZE is the size of each item, in bytes.

   ITEM_SIZE and NITEMS_INCR_MIN must be positive.  *NITEMS must be
   nonnegative.  If NITEMS_MAX is -1, it is treated as if it were
   infinity.

   If PA is null, then allocate a new array instead of reallocating
   the old one.

   Block interrupt input as needed.  If memory exhaustion occurs, set
   *NITEMS to zero if PA is null, and signal an error (i.e., do not
   return).

   Thus, to grow an array A without saving its old contents, do
   { xfree (A); A = NULL; A = xpalloc (NULL, &AITEMS, ...); }.
   The A = NULL avoids a dangling pointer if xpalloc exhausts memory
   and signals an error, and later this code is reexecuted and
   attempts to free A.  */

void *
xpalloc (void *pa, ptrdiff_t *nitems, ptrdiff_t nitems_incr_min,
	 ptrdiff_t nitems_max, ptrdiff_t item_size)
{
  ptrdiff_t n0 = *nitems;
  eassume (0 < item_size && 0 < nitems_incr_min && 0 <= n0 && -1 <= nitems_max);

  /* The approximate size to use for initial small allocation
     requests.  This is the largest "small" request for the GNU C
     library malloc.  */
  enum { DEFAULT_MXFAST = 64 * sizeof (size_t) / 4 };

  /* If the array is tiny, grow it to about (but no greater than)
     DEFAULT_MXFAST bytes.  Otherwise, grow it by about 50%.
     Adjust the growth according to three constraints: NITEMS_INCR_MIN,
     NITEMS_MAX, and what the C language can represent safely.  */

  ptrdiff_t n, nbytes;
  if (INT_ADD_WRAPV (n0, n0 >> 1, &n))
    n = PTRDIFF_MAX;
  if (0 <= nitems_max && nitems_max < n)
    n = nitems_max;

  ptrdiff_t adjusted_nbytes
    = ((INT_MULTIPLY_WRAPV (n, item_size, &nbytes) || SIZE_MAX < nbytes)
       ? min (PTRDIFF_MAX, SIZE_MAX)
       : nbytes < DEFAULT_MXFAST ? DEFAULT_MXFAST : 0);
  if (adjusted_nbytes)
    {
      n = adjusted_nbytes / item_size;
      nbytes = adjusted_nbytes - adjusted_nbytes % item_size;
    }

  if (! pa)
    *nitems = 0;
  if (n - n0 < nitems_incr_min
      && (INT_ADD_WRAPV (n0, nitems_incr_min, &n)
	  || (0 <= nitems_max && nitems_max < n)
	  || INT_MULTIPLY_WRAPV (n, item_size, &nbytes)))
    memory_full (SIZE_MAX);
  pa = xrealloc (pa, nbytes);
  *nitems = n;
  return pa;
}


/* Like strdup, but uses xmalloc.  */

char *
xstrdup (const char *s)
{
  ptrdiff_t size;
  eassert (s);
  size = strlen (s) + 1;
  return (char *) memcpy (xmalloc (size), s, size);
}

/* Like above, but duplicates Lisp string to C string.  */

char *
xlispstrdup (Lisp_Object string)
{
  ptrdiff_t size = SBYTES (string) + 1;
  return (char *) memcpy (xmalloc (size), SSDATA (string), size);
}

/* Assign to *PTR a copy of STRING, freeing any storage *PTR formerly
   pointed to.  If STRING is null, assign it without copying anything.
   Allocate before freeing, to avoid a dangling pointer if allocation
   fails.  */

void
dupstring (char **ptr, char const *string)
{
  char *old = *ptr;
  *ptr = string ? xstrdup (string) : 0;
  xfree (old);
}


/* Like putenv, but (1) use the equivalent of xmalloc and (2) the
   argument is a const pointer.  */

void
xputenv (char const *string)
{
  if (putenv ((char *) string) != 0)
    memory_full (0);
}

/* Return a newly allocated memory block of SIZE bytes, remembering
   to free it when unwinding.  */
void *
record_xmalloc (size_t size)
{
  void *p = (void *)xmalloc (size);
  record_unwind_protect_ptr (xfree, p);
  return p;
}


/* Like malloc but used for allocating Lisp data.  NBYTES is the
   number of bytes to allocate, TYPE describes the intended use of the
   allocated memory block (for strings, for conses, ...).  */

#if ! USE_LSB_TAG
void *lisp_malloc_loser EXTERNALLY_VISIBLE;
#endif

static void *
lisp_malloc (size_t nbytes, enum mem_type type)
{
  register void *val;

  MALLOC_BLOCK_INPUT;

#ifdef GC_MALLOC_CHECK
  allocated_mem_type = type;
#endif

  val = lmalloc (nbytes);

#ifndef GC_MALLOC_CHECK
  if (val && type != MEM_TYPE_NON_LISP)
    mem_insert (val, (char *) val + nbytes, type);
#endif

  MALLOC_UNBLOCK_INPUT;
  if (!val && nbytes)
    memory_full (nbytes);
  MALLOC_PROBE (nbytes);
  return val;
}

/* Free BLOCK.  This must be called to free memory allocated with a
   call to lisp_malloc.  */

static void
lisp_free (void *block)
{
  MALLOC_BLOCK_INPUT;
  free (block);
#ifndef GC_MALLOC_CHECK
  mem_delete (mem_find (block));
#endif
  MALLOC_UNBLOCK_INPUT;
}

/*****  Allocation of aligned blocks of memory to store Lisp data.  *****/

/* The entry point is lisp_align_malloc which returns blocks of at most
   BLOCK_BYTES and guarantees they are aligned on a BLOCK_ALIGN boundary.  */

/* Byte alignment of storage blocks.  */
#define BLOCK_ALIGN (1 << 10)
verify (POWER_OF_2 (BLOCK_ALIGN));

/* Use aligned_alloc if it or a simple substitute is available.
   Aligned allocation is incompatible with unexmacosx.c, so don't use
   it on Darwin unless CANNOT_DUMP.  */

#if !defined DARWIN_OS || defined CANNOT_DUMP
# if (defined HAVE_ALIGNED_ALLOC					\
      || (defined HYBRID_MALLOC						\
	  ? defined HAVE_POSIX_MEMALIGN					\
	  : !defined SYSTEM_MALLOC && !defined DOUG_LEA_MALLOC))
#  define USE_ALIGNED_ALLOC 1
# elif !defined HYBRID_MALLOC && defined HAVE_POSIX_MEMALIGN
#  define USE_ALIGNED_ALLOC 1
#  define aligned_alloc my_aligned_alloc /* Avoid collision with lisp.h.  */
static void *
aligned_alloc (size_t alignment, size_t size)
{
  /* POSIX says the alignment must be a power-of-2 multiple of sizeof (void *).
     Verify this for all arguments this function is given.  */
  verify (BLOCK_ALIGN % sizeof (void *) == 0
	  && POWER_OF_2 (BLOCK_ALIGN / sizeof (void *)));
  verify (MALLOC_IS_LISP_ALIGNED
	  || (LISP_ALIGNMENT % sizeof (void *) == 0
	      && POWER_OF_2 (LISP_ALIGNMENT / sizeof (void *))));
  eassert (alignment == BLOCK_ALIGN
	   || (!MALLOC_IS_LISP_ALIGNED && alignment == LISP_ALIGNMENT));

  void *p;
  return posix_memalign (&p, alignment, size) == 0 ? p : 0;
}
# endif
#endif

/* Padding to leave at the end of a malloc'd block.  This is to give
   malloc a chance to minimize the amount of memory wasted to alignment.
   It should be tuned to the particular malloc library used.
   On glibc-2.3.2, malloc never tries to align, so a padding of 0 is best.
   aligned_alloc on the other hand would ideally prefer a value of 4
   because otherwise, there's 1020 bytes wasted between each ablocks.
   In Emacs, testing shows that those 1020 can most of the time be
   efficiently used by malloc to place other objects, so a value of 0 can
   still preferable unless you have a lot of aligned blocks and virtually
   nothing else.  */
#define BLOCK_PADDING 0
#define BLOCK_BYTES \
  (BLOCK_ALIGN - sizeof (struct ablocks *) - BLOCK_PADDING)

/* Internal data structures and constants.  */

#define ABLOCKS_SIZE 16

/* An aligned block of memory.  */
struct ablock
{
  union
  {
    char payload[BLOCK_BYTES];
    struct ablock *next_free;
  } x;

  /* ABASE is the aligned base of the ablocks.  It is overloaded to
     hold a virtual "busy" field that counts twice the number of used
     ablock values in the parent ablocks, plus one if the real base of
     the parent ablocks is ABASE (if the "busy" field is even, the
     word before the first ablock holds a pointer to the real base).
     The first ablock has a "busy" ABASE, and the others have an
     ordinary pointer ABASE.  To tell the difference, the code assumes
     that pointers, when cast to uintptr_t, are at least 2 *
     ABLOCKS_SIZE + 1.  */
  struct ablocks *abase;

  /* The padding of all but the last ablock is unused.  The padding of
     the last ablock in an ablocks is not allocated.  */
#if BLOCK_PADDING
  char padding[BLOCK_PADDING];
#endif
};

/* A bunch of consecutive aligned blocks.  */
struct ablocks
{
  struct ablock blocks[ABLOCKS_SIZE];
};

/* Size of the block requested from malloc or aligned_alloc.  */
#define ABLOCKS_BYTES (sizeof (struct ablocks) - BLOCK_PADDING)

#define ABLOCK_ABASE(block) \
  (((uintptr_t) (block)->abase) <= (1 + 2 * ABLOCKS_SIZE)	\
   ? (struct ablocks *) (block)					\
   : (block)->abase)

/* Virtual `busy' field.  */
#define ABLOCKS_BUSY(a_base) ((a_base)->blocks[0].abase)

/* Pointer to the (not necessarily aligned) malloc block.  */
#ifdef USE_ALIGNED_ALLOC
#define ABLOCKS_BASE(abase) (abase)
#else
#define ABLOCKS_BASE(abase) \
  (1 & (intptr_t) ABLOCKS_BUSY (abase) ? abase : ((void **) (abase))[-1])
#endif

/* The list of free ablock.   */
static struct ablock *free_ablock;

/* Allocate an aligned block of nbytes.
   Alignment is on a multiple of BLOCK_ALIGN and `nbytes' has to be
   smaller or equal to BLOCK_BYTES.  */
static void *
lisp_align_malloc (size_t nbytes, enum mem_type type)
{
  void *base, *val;
  struct ablocks *abase;

  eassert (nbytes <= BLOCK_BYTES);

  MALLOC_BLOCK_INPUT;

#ifdef GC_MALLOC_CHECK
  allocated_mem_type = type;
#endif

  if (!free_ablock)
    {
      int i;
      bool aligned;

#ifdef DOUG_LEA_MALLOC
      if (!mmap_lisp_allowed_p ())
        mallopt (M_MMAP_MAX, 0);
#endif

#ifdef USE_ALIGNED_ALLOC
      verify (ABLOCKS_BYTES % BLOCK_ALIGN == 0);
      abase = base = aligned_alloc (BLOCK_ALIGN, ABLOCKS_BYTES);
#else
      base = malloc (ABLOCKS_BYTES);
      abase = pointer_align (base, BLOCK_ALIGN);
#endif

      if (base == 0)
	{
	  MALLOC_UNBLOCK_INPUT;
	  memory_full (ABLOCKS_BYTES);
	}

      aligned = (base == abase);
      if (!aligned)
	((void **) abase)[-1] = base;

#ifdef DOUG_LEA_MALLOC
      if (!mmap_lisp_allowed_p ())
          mallopt (M_MMAP_MAX, MMAP_MAX_AREAS);
#endif

      /* Initialize the blocks and put them on the free list.
	 If `base' was not properly aligned, we can't use the last block.  */
      for (i = 0; i < (aligned ? ABLOCKS_SIZE : ABLOCKS_SIZE - 1); i++)
	{
	  abase->blocks[i].abase = abase;
	  abase->blocks[i].x.next_free = free_ablock;
	  free_ablock = &abase->blocks[i];
	}
      intptr_t ialigned = aligned;
      ABLOCKS_BUSY (abase) = (struct ablocks *) ialigned;

      eassert ((uintptr_t) abase % BLOCK_ALIGN == 0);
      eassert (ABLOCK_ABASE (&abase->blocks[3]) == abase); /* 3 is arbitrary */
      eassert (ABLOCK_ABASE (&abase->blocks[0]) == abase);
      eassert (ABLOCKS_BASE (abase) == base);
      eassert ((intptr_t) ABLOCKS_BUSY (abase) == aligned);
    }

  abase = ABLOCK_ABASE (free_ablock);
  ABLOCKS_BUSY (abase)
    = (struct ablocks *) (2 + (intptr_t) ABLOCKS_BUSY (abase));
  val = free_ablock;
  free_ablock = free_ablock->x.next_free;

#ifndef GC_MALLOC_CHECK
  if (type != MEM_TYPE_NON_LISP)
    mem_insert (val, (char *) val + nbytes, type);
#endif

  MALLOC_UNBLOCK_INPUT;

  MALLOC_PROBE (nbytes);

  eassert (0 == ((uintptr_t) val) % BLOCK_ALIGN);
  return val;
}

static void
lisp_align_free (void *block)
{
  struct ablock *ablock = block;
  struct ablocks *abase = ABLOCK_ABASE (ablock);

  MALLOC_BLOCK_INPUT;
#ifndef GC_MALLOC_CHECK
  mem_delete (mem_find (block));
#endif
  /* Put on free list.  */
  ablock->x.next_free = free_ablock;
  free_ablock = ablock;
  /* Update busy count.  */
  intptr_t busy = (intptr_t) ABLOCKS_BUSY (abase) - 2;
  eassume (0 <= busy && busy <= 2 * ABLOCKS_SIZE - 1);
  ABLOCKS_BUSY (abase) = (struct ablocks *) busy;

  if (busy < 2)
    { /* All the blocks are free.  */
      int i = 0;
      bool aligned = busy;
      struct ablock **tem = &free_ablock;
      struct ablock *atop = &abase->blocks[aligned ? ABLOCKS_SIZE : ABLOCKS_SIZE - 1];

      while (*tem)
	{
	  if (*tem >= (struct ablock *) abase && *tem < atop)
	    {
	      i++;
	      *tem = (*tem)->x.next_free;
	    }
	  else
	    tem = &(*tem)->x.next_free;
	}
      eassert ((aligned & 1) == aligned);
      eassert (i == (aligned ? ABLOCKS_SIZE : ABLOCKS_SIZE - 1));
#ifdef USE_POSIX_MEMALIGN
      eassert ((uintptr_t) ABLOCKS_BASE (abase) % BLOCK_ALIGN == 0);
#endif
      free (ABLOCKS_BASE (abase));
    }
  MALLOC_UNBLOCK_INPUT;
}

/* True if a malloc-returned pointer P is suitably aligned for SIZE,
   where Lisp object alignment may be needed if SIZE is a multiple of
   LISP_ALIGNMENT.  */

static bool
laligned (void *p, size_t size)
{
  return (MALLOC_IS_LISP_ALIGNED || (intptr_t) p % LISP_ALIGNMENT == 0
	  || size % LISP_ALIGNMENT != 0);
}

/* Like malloc and realloc except that if SIZE is Lisp-aligned, make
   sure the result is too, if necessary by reallocating (typically
   with larger and larger sizes) until the allocator returns a
   Lisp-aligned pointer.  Code that needs to allocate C heap memory
   for a Lisp object should use one of these functions to obtain a
   pointer P; that way, if T is an enum Lisp_Type value and L ==
   make_lisp_ptr (P, T), then XPNTR (L) == P and XTYPE (L) == T.

   On typical modern platforms these functions' loops do not iterate.
   On now-rare (and perhaps nonexistent) platforms, the loops in
   theory could repeat forever.  If an infinite loop is possible on a
   platform, a build would surely loop and the builder can then send
   us a bug report.  Adding a counter to try to detect any such loop
   would complicate the code (and possibly introduce bugs, in code
   that's never really exercised) for little benefit.  */

static void *
lmalloc (size_t size)
{
#ifdef USE_ALIGNED_ALLOC
  if (! MALLOC_IS_LISP_ALIGNED && size % LISP_ALIGNMENT == 0)
    return aligned_alloc (LISP_ALIGNMENT, size);
#endif

  while (true)
    {
      void *p = malloc (size);
      if (laligned (p, size))
	return p;
      free (p);
      size_t bigger = size + LISP_ALIGNMENT;
      if (size < bigger)
	size = bigger;
    }
}

static void *
lrealloc (void *p, size_t size)
{
  while (true)
    {
      p = realloc (p, size);
      if (laligned (p, size))
	return p;
      size_t bigger = size + LISP_ALIGNMENT;
      if (size < bigger)
	size = bigger;
    }
}


/***********************************************************************
			 Interval Allocation
 ***********************************************************************/

/* Number of intervals allocated in an interval_block structure.
   The 1020 is 1024 minus malloc overhead.  */

#define INTERVAL_BLOCK_SIZE \
  ((1020 - sizeof (struct interval_block *)) / sizeof (struct interval))

/* Intervals are allocated in chunks in the form of an interval_block
   structure.  */

struct interval_block
{
  /* Place `intervals' first, to preserve alignment.  */
  struct interval intervals[INTERVAL_BLOCK_SIZE];
  struct interval_block *next;
};

/* Current interval block.  Its `next' pointer points to older
   blocks.  */

static struct interval_block *interval_block;

/* Index in interval_block above of the next unused interval
   structure.  */

static int interval_block_index = INTERVAL_BLOCK_SIZE;

/* Number of free and live intervals.  */

static EMACS_INT total_free_intervals, total_intervals;

/* List of free intervals.  */

static INTERVAL interval_free_list;

/* Return a new interval.  */

INTERVAL
make_interval (void)
{
  INTERVAL val = xzalloc(sizeof *val);

  val->plist = Qnil;

  return val;
}

static void
init_strings (void)
{
  empty_unibyte_string = make_pure_string ("", 0, 0, 0);
  empty_multibyte_string = make_pure_string ("", 0, 0, 1);
}

DEFUN ("make-string", Fmake_string, Smake_string, 2, 3, 0,
       doc: /* Return a newly created string of length LENGTH, with INIT in each element.
LENGTH must be an integer.
INIT must be an integer that represents a character.
If optional argument MULTIBYTE is non-nil, the result will be
a multibyte string even if INIT is an ASCII character.  */)
  (Lisp_Object length, Lisp_Object init, Lisp_Object multibyte)
{
  register Lisp_Object val;
  int c;
  EMACS_INT nbytes;

  CHECK_FIXNAT (length);
  CHECK_CHARACTER (init);

  c = XFIXNUM (init);
  if (ASCII_CHAR_P (c) && NILP (multibyte))
    {
      nbytes = XFIXNUM (length);
      val = make_uninit_string (nbytes);
      if (nbytes)
	{
	  memset (SDATA (val), c, nbytes);
	  SDATA (val)[nbytes] = 0;
	}
    }
  else
    {
      unsigned char str[MAX_MULTIBYTE_LENGTH];
      ptrdiff_t len = CHAR_STRING (c, str);
      EMACS_INT string_len = XFIXNUM (length);
      unsigned char *p, *beg, *end;

      if (INT_MULTIPLY_WRAPV (len, string_len, &nbytes))
	string_overflow ();
      val = make_uninit_multibyte_string (string_len, nbytes);
      for (beg = SDATA (val), p = beg, end = beg + nbytes; p < end; p += len)
	{
	  /* First time we just copy `str' to the data of `val'.  */
	  if (p == beg)
	    memcpy (p, str, len);
	  else
	    {
	      /* Next time we copy largest possible chunk from
		 initialized to uninitialized part of `val'.  */
	      len = min (p - beg, end - p);
	      memcpy (p, beg, len);
	    }
	}
      if (nbytes)
	*p = 0;
    }

  return val;
}

/* Fill A with 1 bits if INIT is non-nil, and with 0 bits otherwise.
   Return A.  */

Lisp_Object
bool_vector_fill (Lisp_Object a, Lisp_Object init)
{
  EMACS_INT nbits = bool_vector_size (a);
  if (0 < nbits)
    {
      unsigned char *data = bool_vector_uchar_data (a);
      int pattern = NILP (init) ? 0 : (1 << BOOL_VECTOR_BITS_PER_CHAR) - 1;
      ptrdiff_t nbytes = bool_vector_bytes (nbits);
      int last_mask = ~ (~0u << ((nbits - 1) % BOOL_VECTOR_BITS_PER_CHAR + 1));
      memset (data, pattern, nbytes - 1);
      data[nbytes - 1] = pattern & last_mask;
    }
  return a;
}

/* Return a newly allocated, uninitialized bool vector of size NBITS.  */

Lisp_Object
make_uninit_bool_vector (EMACS_INT nbits)
{
  Lisp_Object val;
  EMACS_INT words = bool_vector_words (nbits);
  EMACS_INT word_bytes = words * sizeof (bits_word);
  EMACS_INT needed_elements = ((bool_header_size - header_size + word_bytes
				+ word_size - 1)
			       / word_size);
  struct Lisp_Bool_Vector *p
    = (struct Lisp_Bool_Vector *) allocate_vector (needed_elements);
  XSETBOOL_VECTOR (val, p);
  XSETPVECTYPESIZE (XVECTOR (val), PVEC_BOOL_VECTOR, 0, 0);
  p->size = nbits;

  /* Clear padding at the end.  */
  if (words)
    p->data[words - 1] = 0;

  return val;
}

DEFUN ("make-bool-vector", Fmake_bool_vector, Smake_bool_vector, 2, 2, 0,
       doc: /* Return a new bool-vector of length LENGTH, using INIT for each element.
LENGTH must be a number.  INIT matters only in whether it is t or nil.  */)
  (Lisp_Object length, Lisp_Object init)
{
  Lisp_Object val;

  CHECK_FIXNAT (length);
  val = make_uninit_bool_vector (XFIXNUM (length));
  return bool_vector_fill (val, init);
}

DEFUN ("bool-vector", Fbool_vector, Sbool_vector, 0, MANY, 0,
       doc: /* Return a new bool-vector with specified arguments as elements.
Any number of arguments, even zero arguments, are allowed.
usage: (bool-vector &rest OBJECTS)  */)
  (ptrdiff_t nargs, Lisp_Object *args)
{
  ptrdiff_t i;
  Lisp_Object vector;

  vector = make_uninit_bool_vector (nargs);
  for (i = 0; i < nargs; i++)
    bool_vector_set (vector, i, !NILP (args[i]));

  return vector;
}

/* Make a string from NBYTES bytes at CONTENTS, and compute the number
   of characters from the contents.  This string may be unibyte or
   multibyte, depending on the contents.  */

Lisp_Object
make_string (const char *contents, ptrdiff_t nbytes)
{
  register Lisp_Object val;
  ptrdiff_t nchars, multibyte_nbytes;

  parse_str_as_multibyte ((const unsigned char *) contents, nbytes,
			  &nchars, &multibyte_nbytes);
  if (nbytes == nchars || nbytes != multibyte_nbytes)
    /* CONTENTS contains no multibyte sequences or contains an invalid
       multibyte sequence.  We must make unibyte string.  */
    val = make_unibyte_string (contents, nbytes);
  else
    val = make_multibyte_string (contents, nchars, nbytes);
  return val;
}

/* Make a unibyte string from LENGTH bytes at CONTENTS.  */

Lisp_Object
make_unibyte_string (const char *contents, ptrdiff_t length)
{
  register Lisp_Object val;
  val = make_uninit_string (length);
  memcpy (SDATA (val), contents, length);
  return val;
}


/* Make a multibyte string from NCHARS characters occupying NBYTES
   bytes at CONTENTS.  */

Lisp_Object
make_multibyte_string (const char *contents,
		       ptrdiff_t nchars, ptrdiff_t nbytes)
{
  register Lisp_Object val;
  val = make_uninit_multibyte_string (nchars, nbytes);
  memcpy (SDATA (val), contents, nbytes);
  return val;
}


/* Make a string from NCHARS characters occupying NBYTES bytes at
   CONTENTS.  It is a multibyte string if NBYTES != NCHARS.  */

Lisp_Object
make_string_from_bytes (const char *contents,
			ptrdiff_t nchars, ptrdiff_t nbytes)
{
  register Lisp_Object val;
  val = make_uninit_multibyte_string (nchars, nbytes);
  memcpy (SDATA (val), contents, nbytes);
  if (SBYTES (val) == SCHARS (val))
    STRING_SET_UNIBYTE (val);
  return val;
}


/* Make a string from NCHARS characters occupying NBYTES bytes at
   CONTENTS.  The argument MULTIBYTE controls whether to label the
   string as multibyte.  If NCHARS is negative, it counts the number of
   characters by itself.  */

Lisp_Object
make_specified_string (const char *contents,
		       ptrdiff_t nchars, ptrdiff_t nbytes, bool multibyte)
{
  Lisp_Object val;

  if (nchars < 0)
    {
      if (multibyte)
	nchars = multibyte_chars_in_text ((const unsigned char *) contents,
					  nbytes);
      else
	nchars = nbytes;
    }
  val = make_uninit_multibyte_string (nchars, nbytes);
  memcpy (SDATA (val), contents, nbytes);
  if (!multibyte)
    STRING_SET_UNIBYTE (val);
  return val;
}


/* Return a unibyte Lisp_String set up to hold LENGTH characters
   occupying LENGTH bytes.  */

Lisp_Object
make_uninit_string (EMACS_INT length)
{
  Lisp_Object val;

  if (!length)
    return empty_unibyte_string;
  val = make_uninit_multibyte_string (length, length);
  STRING_SET_UNIBYTE (val);
  return val;
}


/* Return a multibyte Lisp_String set up to hold NCHARS characters
   which occupy NBYTES bytes.  */

Lisp_Object
make_uninit_multibyte_string (EMACS_INT nchars, EMACS_INT nbytes)
{
  void *buf = xmalloc(nbytes + 1);
  memset(buf, 0, nbytes + 1);
  Lisp_Object fixbuf = elisp_fixbuf(buf, nbytes + 1);
  Lisp_Object mbstring = elisp_mbstring(fixbuf, nbytes, nchars);
  Lisp_Object string = elisp_string(mbstring, NULL);

  return string;
}

/* Print arguments to BUF according to a FORMAT, then return
   a Lisp_String initialized with the data from BUF.  */

Lisp_Object
make_formatted_string (char *buf, const char *format, ...)
{
  va_list ap;
  int length;

  va_start (ap, format);
  length = vsprintf (buf, format, ap);
  va_end (ap);
  return make_string (buf, length);
}


/***********************************************************************
			   Float Allocation
 ***********************************************************************/

/* We store float cells inside of float_blocks, allocating a new
   float_block with malloc whenever necessary.  Float cells reclaimed
   by GC are put on a free list to be reallocated before allocating
   any new float cells from the latest float_block.  */

#define GETMARKBIT(block,n)				\
  (((block)->gcmarkbits[(n) / BITS_PER_BITS_WORD]	\
    >> ((n) % BITS_PER_BITS_WORD))			\
   & 1)

#define SETMARKBIT(block,n)				\
  ((block)->gcmarkbits[(n) / BITS_PER_BITS_WORD]	\
   |= (bits_word) 1 << ((n) % BITS_PER_BITS_WORD))

#define UNSETMARKBIT(block,n)				\
  ((block)->gcmarkbits[(n) / BITS_PER_BITS_WORD]	\
   &= ~((bits_word) 1 << ((n) % BITS_PER_BITS_WORD)))

/* Return a new float object with value FLOAT_VALUE.  */

Lisp_Object
make_float (double float_value)
{
  register Lisp_Object val;

  XFLOAT_INIT(&val, float_value);

  return val;
}


DEFUN ("cons", Fcons, Scons, 2, 2, 0,
       doc: /* Create a new cons, give it CAR and CDR as components, and return it.  */)
  (Lisp_Object car, Lisp_Object cdr)
{
  Lisp_Object val;

  val.xsetcons();

  XSETCAR (val, car);
  XSETCDR (val, cdr);

  return val;
}

/* Make a list of 1, 2, 3, 4 or 5 specified objects.  */

Lisp_Object
list1 (Lisp_Object arg1)
{
  return Fcons (arg1, Qnil);
}

Lisp_Object
list2 (Lisp_Object arg1, Lisp_Object arg2)
{
  return Fcons (arg1, Fcons (arg2, Qnil));
}


Lisp_Object
list3 (Lisp_Object arg1, Lisp_Object arg2, Lisp_Object arg3)
{
  return Fcons (arg1, Fcons (arg2, Fcons (arg3, Qnil)));
}


Lisp_Object
list4 (Lisp_Object arg1, Lisp_Object arg2, Lisp_Object arg3, Lisp_Object arg4)
{
  return Fcons (arg1, Fcons (arg2, Fcons (arg3, Fcons (arg4, Qnil))));
}


Lisp_Object
list5 (Lisp_Object arg1, Lisp_Object arg2, Lisp_Object arg3, Lisp_Object arg4, Lisp_Object arg5)
{
  return Fcons (arg1, Fcons (arg2, Fcons (arg3, Fcons (arg4,
						       Fcons (arg5, Qnil)))));
}

DEFUN ("list", Flist, Slist, 0, MANY, 0,
       doc: /* Return a newly created list with specified arguments as elements.
Any number of arguments, even zero arguments, are allowed.
usage: (list &rest OBJECTS)  */)
  (ptrdiff_t nargs, Lisp_Object *args)
{
  register Lisp_Object val;
  val = Qnil;

  while (nargs > 0)
    {
      nargs--;
      val = Fcons (args[nargs], val);
    }
  return val;
}


DEFUN ("make-list", Fmake_list, Smake_list, 2, 2, 0,
       doc: /* Return a newly created list of length LENGTH, with each element being INIT.  */)
  (Lisp_Object length, Lisp_Object init)
{
  Lisp_Object val = Qnil;
  CHECK_FIXNAT (length);

  for (EMACS_INT size = XFIXNUM (length); 0 < size; size--)
    {
      val = Fcons (init, val);
      rarely_quit (size);
    }

  return val;
}



/* This value is balanced well enough to avoid too much internal overhead
   for the most common cases; it's not required to be a power of two, but
   it's expected to be a mult-of-ROUNDUP_SIZE (see below).  */

enum { VECTOR_BLOCK_SIZE = 4096 };

#define roundup_size 8

/* Verify assumptions described above.  */
verify (VECTOR_BLOCK_SIZE % roundup_size == 0);
verify (VECTOR_BLOCK_SIZE <= (1 << PSEUDOVECTOR_SIZE_BITS));

/* Round up X to nearest mult-of-ROUNDUP_SIZE --- use at compile time.  */
#define vroundup_ct(x) ROUNDUP (x, roundup_size)
/* Round up X to nearest mult-of-ROUNDUP_SIZE --- use at runtime.  */
#define vroundup(x) (eassume ((x) >= 0), vroundup_ct (x))

/* Rounding helps to maintain alignment constraints if USE_LSB_TAG.  */

enum {VECTOR_BLOCK_BYTES = VECTOR_BLOCK_SIZE - vroundup_ct (sizeof (void *))};

/* Size of the minimal vector allocated from block.  */

#define VBLOCK_BYTES_MIN vroundup_ct (header_size + sizeof (ELisp_Return_Value))
enum { VBLOCK_BYTES_MAX = vroundup_ct ((VECTOR_BLOCK_BYTES / 2) - word_size) };

/* We maintain one free list for each possible block-allocated
   vector size, and this is the number of free lists we have.  */

enum { VECTOR_MAX_FREE_LIST_INDEX =
       (VECTOR_BLOCK_BYTES - VBLOCK_BYTES_MIN) / roundup_size + 1 };

/* Common shortcut to advance vector pointer over a block data.  */

static struct Lisp_Vector *
ADVANCE (struct Lisp_Vector *v, ptrdiff_t nbytes)
{
  void *vv = v;
  char *cv = vv;
  void *p = cv + nbytes;
  return p;
}

/* Common shortcut to calculate NBYTES-vector index in VECTOR_FREE_LISTS.  */

static ptrdiff_t
VINDEX (ptrdiff_t nbytes)
{
  eassume (VBLOCK_BYTES_MIN <= nbytes);
  return (nbytes - VBLOCK_BYTES_MIN) / roundup_size;
}

/* This internal type is used to maintain the list of large vectors
   which are allocated at their own, e.g. outside of vector blocks.

   struct large_vector itself cannot contain a struct Lisp_Vector, as
   the latter contains a flexible array member and C99 does not allow
   such structs to be nested.  Instead, each struct large_vector
   object LV is followed by a struct Lisp_Vector, which is at offset
   large_vector_offset from LV, and whose address is therefore
   large_vector_vec (&LV).  */

struct large_vector
{
  struct large_vector *next;
};

enum
{
  large_vector_offset = ROUNDUP (sizeof (struct large_vector), LISP_ALIGNMENT)
};

static struct Lisp_Vector *
large_vector_vec (struct large_vector *p)
{
  return (struct Lisp_Vector *) ((char *) p + large_vector_offset);
}

/* This internal type is used to maintain an underlying storage
   for small vectors.  */

struct vector_block
{
  char data[VECTOR_BLOCK_BYTES];
  struct vector_block *next;
};

/* Chain of vector blocks.  */

static struct vector_block *vector_blocks;

/* Vector free lists, where NTH item points to a chain of free
   vectors of the same NBYTES size, so NTH == VINDEX (NBYTES).  */

static struct Lisp_Vector *vector_free_lists[VECTOR_MAX_FREE_LIST_INDEX];

/* Singly-linked list of large vectors.  */

static struct large_vector *large_vectors;

/* The only vector with 0 slots, allocated from pure space.  */

Lisp_Object zero_vector;

/* Number of live vectors.  */

static EMACS_INT total_vectors;

/* Total size of live and free vectors, in Lisp_Object units.  */

static EMACS_INT total_vector_slots, total_free_vector_slots;

/* Called once to initialize vector allocation.  */

static void
init_vectors (void)
{
  zero_vector = make_pure_vector (0);
}

static ptrdiff_t
vector_nbytes (struct Lisp_Vector *v)
{
  ptrdiff_t size = v->header.size & ~ARRAY_MARK_FLAG;
  ptrdiff_t nwords;

  if (size & PSEUDOVECTOR_FLAG)
    {
      if (PSEUDOVECTOR_TYPEP (&v->header, PVEC_BOOL_VECTOR))
        {
          struct Lisp_Bool_Vector *bv = (struct Lisp_Bool_Vector *) v;
	  ptrdiff_t word_bytes = (bool_vector_words (bv->size)
				  * sizeof (bits_word));
	  ptrdiff_t boolvec_bytes = bool_header_size + word_bytes;
	  verify (header_size <= bool_header_size);
	  nwords = (boolvec_bytes - header_size + word_size - 1) / word_size;
        }
      else
	nwords = ((size & PSEUDOVECTOR_SIZE_MASK)
		  + ((size & PSEUDOVECTOR_REST_MASK)
		     >> PSEUDOVECTOR_SIZE_BITS));
    }
  else
    nwords = size;
  return vroundup (header_size + word_size * nwords);
}

/* Convert a pseudovector pointer P to its underlying struct T pointer.
   Verify that the struct is small, since cleanup_vector is called
   only on small vector-like objects.  */

#define PSEUDOVEC_STRUCT(p, t) \
  verify_expr ((header_size + VECSIZE (struct t) * word_size \
		<= VBLOCK_BYTES_MAX), \
	       (struct t *) (p))

/* Release extra resources still in use by VECTOR, which may be any
   small vector-like object.  */

static void
cleanup_vector (struct Lisp_Vector *vector)
{
  detect_suspicious_free (vector);
  if (PSEUDOVECTOR_TYPEP (&vector->header, PVEC_FONT))
    {
      if ((vector->header.size & PSEUDOVECTOR_SIZE_MASK) == FONT_OBJECT_MAX)
	{
	  struct font *font = PSEUDOVEC_STRUCT (vector, font);
	  struct font_driver const *drv = font->driver;

	  /* The font driver might sometimes be NULL, e.g. if Emacs was
	     interrupted before it had time to set it up.  */
	  if (drv)
	    {
	      /* Attempt to catch subtle bugs like Bug#16140.  */
	      eassert (valid_font_driver (drv));
	      drv->close (font);
	    }
	}
    }
  else if (PSEUDOVECTOR_TYPEP (&vector->header, PVEC_THREAD))
    finalize_one_thread (PSEUDOVEC_STRUCT (vector, thread_state));
  else if (PSEUDOVECTOR_TYPEP (&vector->header, PVEC_MUTEX))
    finalize_one_mutex (PSEUDOVEC_STRUCT (vector, Lisp_Mutex));
  else if (PSEUDOVECTOR_TYPEP (&vector->header, PVEC_CONDVAR))
    finalize_one_condvar (PSEUDOVEC_STRUCT (vector, Lisp_CondVar));
}

/* Value is a pointer to a newly allocated Lisp_Vector structure
   with room for LEN Lisp_Objects.  */

static struct Lisp_Vector *
allocate_vectorlike (ptrdiff_t len)
{
  if (len == 0)
    return XVECTOR (zero_vector);
  else
    return (struct Lisp_Vector *)xzalloc(header_size + len * word_size);
}


/* Allocate a vector with LEN slots.  */

struct Lisp_Vector *
allocate_vector (EMACS_INT len)
{
  struct Lisp_Vector *v;
  ptrdiff_t nbytes_max = min (PTRDIFF_MAX, SIZE_MAX);

  if (min ((nbytes_max - header_size) / word_size, MOST_POSITIVE_FIXNUM) < len)
    memory_full (SIZE_MAX);
  v = allocate_vectorlike (len);
  v->header.size = len;
  return v;
}


/* Allocate other vector-like structures.  */

struct Lisp_Vector *
allocate_pseudovector (int memlen, int lisplen,
		       int zerolen, enum pvec_type tag)
{
  struct Lisp_Vector *v = allocate_vectorlike (memlen);

  v->header.jsval.setDouble(0);
  /* Catch bogus values.  */
  eassert (0 <= tag && tag <= PVEC_FONT);
  eassert (0 <= lisplen && lisplen <= zerolen && zerolen <= memlen);
  eassert (memlen - lisplen <= (1 << PSEUDOVECTOR_REST_BITS) - 1);
  eassert (lisplen <= PSEUDOVECTOR_SIZE_MASK);

  /* Only the first LISPLEN slots will be traced normally by the GC.  */
  memclear (v->contents, zerolen * word_size);
  for (ptrdiff_t i = 0; i < lisplen; i++)
    v->contents[i] = Qnil;
  XSETPVECTYPESIZE (v, tag, lisplen, memlen - lisplen);
  return v;
}

struct buffer *
allocate_buffer (void)
{
  struct buffer *b = lisp_malloc (sizeof *b, MEM_TYPE_BUFFER);
  memset(b, 0, sizeof *b);
  BUFFER_PVEC_INIT (b);
  for (ptrdiff_t i = 0; i < BUFFER_LISP_SIZE; i++)
    ((struct Lisp_Vector *)b)->contents[i] = Qnil;
  /* Put B on the chain of all buffers including killed ones.  */
  b->next = all_buffers;
  all_buffers = b;
  /* Note that the rest fields of B are not initialized.  */
  return b;
}


/* Allocate a record with COUNT slots.  COUNT must be positive, and
   includes the type slot.  */

static struct Lisp_Vector *
allocate_record (EMACS_INT count)
{
  if (count > PSEUDOVECTOR_SIZE_MASK)
    error ("Attempt to allocate a record of %"pI"d slots; max is %d",
	   count, PSEUDOVECTOR_SIZE_MASK);
  struct Lisp_Vector *p = allocate_vectorlike (count);
  p->header.size = count;
  XSETPVECTYPE (p, PVEC_RECORD);
  return p;
}


DEFUN ("make-record", Fmake_record, Smake_record, 3, 3, 0,
       doc: /* Create a new record.
TYPE is its type as returned by `type-of'; it should be either a
symbol or a type descriptor.  SLOTS is the number of non-type slots,
each initialized to INIT.  */)
  (Lisp_Object type, Lisp_Object slots, Lisp_Object init)
{
  CHECK_FIXNAT (slots);
  EMACS_INT size = XFIXNUM (slots) + 1;
  struct Lisp_Vector *p = allocate_record (size);
  p->contents[0] = type;
  for (ptrdiff_t i = 1; i < size; i++)
    p->contents[i] = init;
  ELisp_Value ret;
  ret.xsetvector(p);
  return ret;
}


DEFUN ("record", Frecord, Srecord, 1, MANY, 0,
       doc: /* Create a new record.
TYPE is its type as returned by `type-of'; it should be either a
symbol or a type descriptor.  SLOTS is used to initialize the record
slots with shallow copies of the arguments.
usage: (record TYPE &rest SLOTS) */)
  (ptrdiff_t nargs, Lisp_Object *args)
{
  struct Lisp_Vector *p = allocate_record (nargs);
  for (size_t i = 0; i < nargs; i++)
    p->contents[i] = args[i];
  ELisp_Value ret;
  ret.xsetvector(p);
  return ret;
}


DEFUN ("make-vector", Fmake_vector, Smake_vector, 2, 2, 0,
       doc: /* Return a newly created vector of length LENGTH, with each element being INIT.
See also the function `vector'.  */)
  (Lisp_Object length, Lisp_Object init)
{
  CHECK_FIXNAT (length);
  struct Lisp_Vector *p = allocate_vector (XFIXNUM (length));
  for (ptrdiff_t i = 0; i < XFIXNUM (length); i++)
    p->contents[i] = init;
  ELisp_Value ret;
  ret.xsetvector(p);
  return ret;
}

DEFUN ("vector", Fvector, Svector, 0, MANY, 0,
       doc: /* Return a newly created vector with specified arguments as elements.
Any number of arguments, even zero arguments, are allowed.
usage: (vector &rest OBJECTS)  */)
  (ptrdiff_t nargs, Lisp_Object *args)
{
  Lisp_Object val = make_uninit_vector (nargs);
  struct Lisp_Vector *p = XVECTOR (val);
  for (size_t i = 0; i < nargs; i++)
    p->contents[i] = args[i];
  return val;
}

void
make_byte_code (struct Lisp_Vector *v)
{
  /* Don't allow the global zero_vector to become a byte code object.  */
  eassert (0 < v->header.size);

  if (v->header.size > 1 && STRINGP (v->contents[1])
      && STRING_MULTIBYTE (v->contents[1]))
    /* BYTECODE-STRING must have been produced by Emacs 20.2 or the
       earlier because they produced a raw 8-bit string for byte-code
       and now such a byte-code string is loaded as multibyte while
       raw 8-bit characters converted to multibyte form.  Thus, now we
       must convert them back to the original unibyte form.  */
    v->contents[1] = Fstring_as_unibyte (v->contents[1]);
  XSETPVECTYPE (v, PVEC_COMPILED);
}

DEFUN ("make-byte-code", Fmake_byte_code, Smake_byte_code, 4, MANY, 0,
       doc: /* Create a byte-code object with specified arguments as elements.
The arguments should be the ARGLIST, bytecode-string BYTE-CODE, constant
vector CONSTANTS, maximum stack size DEPTH, (optional) DOCSTRING,
and (optional) INTERACTIVE-SPEC.
The first four arguments are required; at most six have any
significance.
The ARGLIST can be either like the one of `lambda', in which case the arguments
will be dynamically bound before executing the byte code, or it can be an
integer of the form NNNNNNNRMMMMMMM where the 7bit MMMMMMM specifies the
minimum number of arguments, the 7-bit NNNNNNN specifies the maximum number
of arguments (ignoring &rest) and the R bit specifies whether there is a &rest
argument to catch the left-over arguments.  If such an integer is used, the
arguments will not be dynamically bound but will be instead pushed on the
stack before executing the byte-code.
usage: (make-byte-code ARGLIST BYTE-CODE CONSTANTS DEPTH &optional DOCSTRING INTERACTIVE-SPEC &rest ELEMENTS)  */)
  (ptrdiff_t nargs, Lisp_Object *args)
{
  Lisp_Object val = make_uninit_vector (nargs);
  struct Lisp_Vector *p = XVECTOR (val);

  /* We used to purecopy everything here, if purify-flag was set.  This worked
     OK for Emacs-23, but with Emacs-24's lexical binding code, it can be
     dangerous, since make-byte-code is used during execution to build
     closures, so any closure built during the preload phase would end up
     copied into pure space, including its free variables, which is sometimes
     just wasteful and other times plainly wrong (e.g. those free vars may want
     to be setcar'd).  */

  for (size_t i = 0; i < nargs; i++)
    p->contents[i] = args[i];
  make_byte_code (p);
  XSETCOMPILED (val, p);
  return val;
}



void
init_symbol (Lisp_Object val, Lisp_Object name)
{
  set_symbol_name (val, name);
  set_symbol_plist (val, Qnil);
  SET_SYMBOL_REDIRECT (val, SYMBOL_PLAINVAL);
  elisp_symbol_set_value (val, LRH (Qunbound));
  set_symbol_function (val, Qnil);
  SET_SYMBOL_INTERNED (val, SYMBOL_UNINTERNED);
  SET_SYMBOL_TRAPPED_WRITE (val, SYMBOL_UNTRAPPED_WRITE);
  SET_SYMBOL_DECLARED_SPECIAL (val, false);
  SET_SYMBOL_PINNED (val, false);
}

DEFUN ("make-symbol", Fmake_symbol, Smake_symbol, 1, 1, 0,
       doc: /* Return a newly allocated uninterned symbol whose name is NAME.
Its value is void, and its function definition and property list are nil.  */)
  (Lisp_Object name)
{
  Lisp_Object val;

  CHECK_STRING (name);

  val = elisp_symbol();
  init_symbol (val, name);

  return val;
}

ELisp_Return_Value
make_misc_ptr (void *a)
{
  Lisp_Object val;
  struct Lisp_Misc_Ptr *p = ALLOCATE_PSEUDOVECTOR (struct Lisp_Misc_Ptr, pointer, PVEC_MISC_PTR);
  p->pointer = a;
  XSETVECTOR (val, (struct Lisp_Vector *)p);

  return val;
}

Lisp_Object
make_save_ptr_ptr (void *a, void *b)
{
}

Lisp_Object
make_save_funcptr_ptr_obj (void (*a) (void), void *b, Lisp_Object c)
{
}

Lisp_Object
make_save_memory (Lisp_Object *a, ptrdiff_t n)
{
}

/* Free a Lisp_Save_Value object.  Do not use this function
   if SAVE contains pointer other than returned by xmalloc.  */

void
free_save_value (Lisp_Object save)
{
}

/* Return a Lisp_Misc_Overlay object with specified START, END and PLIST.  */

Lisp_Object
build_overlay (Lisp_Object start, Lisp_Object end, Lisp_Object plist)
{
  struct Lisp_Overlay *p = ALLOCATE_PSEUDOVECTOR (struct Lisp_Overlay, next,
						  PVEC_OVERLAY);
  Lisp_Object overlay;
  XSETOVERLAY (overlay, p);
  OVERLAY_START (overlay) = start;
  OVERLAY_END (overlay) = end;
  set_overlay_plist (overlay, plist);
  XOVERLAY (overlay)->next = NULL;
  return overlay;
}

DEFUN ("make-marker", Fmake_marker, Smake_marker, 0, 0, 0,
       doc: /* Return a newly allocated marker which does not point at any place.  */)
  (void)
{
  register Lisp_Object val;
  register struct Lisp_Marker *p = ALLOCATE_PSEUDOVECTOR (struct Lisp_Marker, buffer, PVEC_MARKER);

  XSETMARKER (val, p);
  p->buffer = 0;
  p->bytepos = 0;
  p->charpos = 0;
  p->next = NULL;
  p->insertion_type = 0;
  p->need_adjustment = 0;
  return val;
}

/* Return a newly allocated marker which points into BUF
   at character position CHARPOS and byte position BYTEPOS.  */

Lisp_Object
build_marker (struct buffer *buf, ptrdiff_t charpos, ptrdiff_t bytepos)
{
  Lisp_Object obj;

  /* No dead buffers here.  */
  eassert (BUFFER_LIVE_P (buf));

  /* Every character is at least one byte.  */
  eassert (charpos <= bytepos);

  struct Lisp_Marker *m = ALLOCATE_PSEUDOVECTOR (struct Lisp_Marker, buffer,
                                                 PVEC_MARKER);;
  XSETMARKER (obj, m);
  m->buffer = buf;
  m->charpos = charpos;
  m->bytepos = bytepos;
  m->insertion_type = 0;
  m->need_adjustment = 0;
  m->next = BUF_MARKERS (buf);
  BUF_MARKERS (buf) = m;
  return obj;
}


/* Return a newly created vector or string with specified arguments as
   elements.  If all the arguments are characters that can fit
   in a string of events, make a string; otherwise, make a vector.

   Any number of arguments, even zero arguments, are allowed.  */

#ifdef HAVE_MODULES
/* Create a new module user ptr object.  */
Lisp_Object
make_user_ptr (void (*finalizer) (void *), void *p)
{
  Lisp_Object obj;
  struct Lisp_User_Ptr *uptr;

  return obj;
}
#endif

static void
init_finalizer_list (struct Lisp_Finalizer *head)
{
}

/* Insert FINALIZER before ELEMENT.  */

static void
finalizer_insert (struct Lisp_Finalizer *element,
                  struct Lisp_Finalizer *finalizer)
{
}

static void
unchain_finalizer (struct Lisp_Finalizer *finalizer)
{
}

static void
mark_finalizer_list (struct Lisp_Finalizer *head)
{
}

/* Move doomed finalizers to list DEST from list SRC.  A doomed
   finalizer is one that is not GC-reachable and whose
   finalizer->function is non-nil.  */

static void
queue_doomed_finalizers (struct Lisp_Finalizer *dest,
                         struct Lisp_Finalizer *src)
{
}

static Lisp_Object
run_finalizer_handler (Lisp_Object args)
{
  add_to_log ("finalizer failed: %S", args);
  return Qnil;
}

static void
run_finalizer_function (Lisp_Object function)
{
}

static void
run_finalizers (struct Lisp_Finalizer *finalizers)
{
}

DEFUN ("make-finalizer", Fmake_finalizer, Smake_finalizer, 1, 1, 0,
       doc: /* Make a finalizer that will run FUNCTION.
FUNCTION will be called after garbage collection when the returned
finalizer object becomes unreachable.  If the finalizer object is
reachable only through references from finalizer objects, it does not
count as reachable for the purpose of deciding whether to run
FUNCTION.  FUNCTION will be run once per finalizer object.  */)
  (Lisp_Object function)
{
  return Qnil;
}


/************************************************************************
			   Memory Full Handling
 ************************************************************************/


/* Called if malloc (NBYTES) returns zero.  If NBYTES == SIZE_MAX,
   there may have been size_t overflow so that malloc was never
   called, or perhaps malloc was invoked successfully but the
   resulting pointer had problems fitting into a tagged EMACS_INT.  In
   either case this counts as memory being full even though malloc did
   not fail.  */

void
memory_full (size_t nbytes)
{
  /* Do not go into hysterics merely because a large request failed.  */
  bool enough_free_memory = 0;
  if (SPARE_MEMORY < nbytes)
    {
      void *p;

      MALLOC_BLOCK_INPUT;
      p = malloc (SPARE_MEMORY);
      if (p)
	{
	  free (p);
	  enough_free_memory = 1;
	}
      MALLOC_UNBLOCK_INPUT;
    }

  if (! enough_free_memory)
    {
      int i;

      Vmemory_full = Qt;

      /* The first time we get here, free the spare memory.  */
      for (i = 0; i < ARRAYELTS (spare_memory); i++)
	if (spare_memory[i])
	  {
	    if (i == 0)
	      free (spare_memory[i]);
	    else if (i >= 1 && i <= 4)
	      lisp_align_free (spare_memory[i]);
	    else
	      lisp_free (spare_memory[i]);
	    spare_memory[i] = 0;
	  }
    }

  /* This used to call error, but if we've run out of memory, we could
     get infinite recursion trying to build the string.  */
  xsignal (Qnil, Vmemory_signal_data);
}

/* If we released our reserve (due to running out of memory),
   and we have a fair amount free once again,
   try to set aside another reserve in case we run out once more.

   This is called when a relocatable block is freed in ralloc.c,
   and also directly from this file, in case we're not using ralloc.c.  */

void
refill_memory_reserve (void)
{
}

/************************************************************************
			   C Stack Marking
 ************************************************************************/

/* Conservative C stack marking requires a method to identify possibly
   live Lisp objects given a pointer value.  We do this by keeping
   track of blocks of Lisp data that are allocated in a red-black tree
   (see also the comment of mem_node which is the type of nodes in
   that tree).  Function lisp_malloc adds information for an allocated
   block to the red-black tree with calls to mem_insert, and function
   lisp_free removes it with mem_delete.  Functions live_string_p etc
   call mem_find to lookup information about a given pointer in the
   tree, and use that to determine if the pointer points into a Lisp
   object or not.  */

/* Initialize this part of alloc.c.  */

static void
mem_init (void)
{
  mem_z.left = mem_z.right = MEM_NIL;
  mem_z.parent = NULL;
  mem_z.color = MEM_BLACK;
  mem_z.start = mem_z.end = NULL;
  mem_root = MEM_NIL;
}


/* Value is a pointer to the mem_node containing START.  Value is
   MEM_NIL if there is no node in the tree containing START.  */


/* Insert a new node into the tree for a block of memory with start
   address START, end address END, and type TYPE.  Value is a
   pointer to the node that was inserted.  */

/* Re-establish the red-black properties of the tree, and thereby
   balance the tree, after node X has been inserted; X is always red.  */

/*   (x)                   (y)
     / \                   / \
    a   (y)      ===>    (x)  c
        / \              / \
       b   c            a   b  */

static void
mem_rotate_left (struct mem_node *x)
{
  struct mem_node *y;

  /* Turn y's left sub-tree into x's right sub-tree.  */
  y = x->right;
  x->right = y->left;
  if (y->left != MEM_NIL)
    y->left->parent = x;

  /* Y's parent was x's parent.  */
  if (y != MEM_NIL)
    y->parent = x->parent;

  /* Get the parent to point to y instead of x.  */
  if (x->parent)
    {
      if (x == x->parent->left)
	x->parent->left = y;
      else
	x->parent->right = y;
    }
  else
    mem_root = y;

  /* Put x on y's left.  */
  y->left = x;
  if (x != MEM_NIL)
    x->parent = y;
}


/*     (x)                (Y)
       / \                / \
     (y)  c      ===>    a  (x)
     / \                    / \
    a   b                  b   c  */

static void
mem_rotate_right (struct mem_node *x)
{
  struct mem_node *y = x->left;

  x->left = y->right;
  if (y->right != MEM_NIL)
    y->right->parent = x;

  if (y != MEM_NIL)
    y->parent = x->parent;
  if (x->parent)
    {
      if (x == x->parent->right)
	x->parent->right = y;
      else
	x->parent->left = y;
    }
  else
    mem_root = y;

  y->right = x;
  if (x != MEM_NIL)
    x->parent = y;
}


/* Delete node Z from the tree.  If Z is null or MEM_NIL, do nothing.  */

static void
mem_delete (struct mem_node *z)
{
}


/* Re-establish the red-black properties of the tree, after a
   deletion.  */

static void
mem_delete_fixup (struct mem_node *x)
{
  while (x != mem_root && x->color == MEM_BLACK)
    {
      if (x == x->parent->left)
	{
	  struct mem_node *w = x->parent->right;

	  if (w->color == MEM_RED)
	    {
	      w->color = MEM_BLACK;
	      x->parent->color = MEM_RED;
	      mem_rotate_left (x->parent);
	      w = x->parent->right;
            }

	  if (w->left->color == MEM_BLACK && w->right->color == MEM_BLACK)
	    {
	      w->color = MEM_RED;
	      x = x->parent;
            }
	  else
	    {
	      if (w->right->color == MEM_BLACK)
		{
		  w->left->color = MEM_BLACK;
		  w->color = MEM_RED;
		  mem_rotate_right (w);
		  w = x->parent->right;
                }
	      w->color = x->parent->color;
	      x->parent->color = MEM_BLACK;
	      w->right->color = MEM_BLACK;
	      mem_rotate_left (x->parent);
	      x = mem_root;
            }
        }
      else
	{
	  struct mem_node *w = x->parent->left;

	  if (w->color == MEM_RED)
	    {
	      w->color = MEM_BLACK;
	      x->parent->color = MEM_RED;
	      mem_rotate_right (x->parent);
	      w = x->parent->left;
            }

	  if (w->right->color == MEM_BLACK && w->left->color == MEM_BLACK)
	    {
	      w->color = MEM_RED;
	      x = x->parent;
            }
	  else
	    {
	      if (w->left->color == MEM_BLACK)
		{
		  w->right->color = MEM_BLACK;
		  w->color = MEM_RED;
		  mem_rotate_left (w);
		  w = x->parent->left;
                }

	      w->color = x->parent->color;
	      x->parent->color = MEM_BLACK;
	      w->left->color = MEM_BLACK;
	      mem_rotate_right (x->parent);
	      x = mem_root;
            }
        }
    }

  x->color = MEM_BLACK;
}


/* If P is a pointer into a live Lisp string object on the heap,
   return the object.  Otherwise, return nil.  M is a pointer to the
   mem_block for P.

   This and other *_holding functions look for a pointer anywhere into
   the object, not merely for a pointer to the start of the object,
   because some compilers sometimes optimize away the latter.  See
   Bug#28213.  */

/* If P is a pointer into a live Lisp cons object on the heap, return
   the object.  Otherwise, return nil.  M is a pointer to the
   mem_block for P.  */

/* If P is a pointer into a live Lisp symbol object on the heap,
   return the object.  Otherwise, return nil.  M is a pointer to the
   mem_block for P.  */

/* If P is a pointer to a live Lisp Misc on the heap, return the object.
   Otherwise, return nil.  M is a pointer to the mem_block for P.  */

static Lisp_Object
live_misc_holding (struct mem_node *m, void *p)
{
}

static bool
live_misc_p (struct mem_node *m, void *p)
{
}

/* Return true if P might point to Lisp data that can be garbage
   collected, and false otherwise (i.e., false if it is easy to see
   that P cannot point to Lisp data that can be garbage collected).
   Symbols are implemented via offsets not pointers, but the offsets
   are also multiples of LISP_ALIGNMENT.  */

/* If P points to Lisp data, mark that as live if it isn't already
   marked.  */

static void
mark_maybe_pointer (void *p)
{
}

/* Mark Lisp objects referenced from the address range START+OFFSET..END
   or END+OFFSET..START.  */

static void ATTRIBUTE_NO_SANITIZE_ADDRESS
mark_memory (void *start, void *end)
{
}



/* Return a new overlay with specified START, END and PLIST.  */



Lisp_Object
make_bignum_str (const char *num, int base)
{
  struct Lisp_Bignum *b = ALLOCATE_PSEUDOVECTOR (struct Lisp_Bignum, value,
						 PVEC_BIGNUM);
  mpz_init (b->value);
  int check = mpz_set_str (b->value, num, base);
  eassert (check == 0);
  Lisp_Object bignum;
  XSETBIGNUM (bignum, b);
  return bignum;
}

/* Given an mpz_t, make a number.  This may return a bignum or a
   fixnum depending on VALUE.  */

Lisp_Object
make_number (mpz_t value)
{
  if (mpz_fits_slong_p (value))
    {
      long l = mpz_get_si (value);
      if (!FIXNUM_OVERFLOW_P (l))
	return make_fixnum (l);
    }

  /* Check if fixnum can be larger than long.  */
  if (sizeof (EMACS_INT) > sizeof (long))
    {
      size_t bits = mpz_sizeinbase (value, 2);
      int sign = mpz_sgn (value);

      if (bits < FIXNUM_BITS + (sign < 0))
        {
          EMACS_INT v = 0;
          size_t limbs = mpz_size (value);
          mp_size_t i;

          for (i = 0; i < limbs; i++)
            {
              mp_limb_t limb = mpz_getlimbn (value, i);
              v |= (EMACS_INT) ((EMACS_UINT) limb << (i * mp_bits_per_limb));
            }
          if (sign < 0)
            v = -v;

          if (!FIXNUM_OVERFLOW_P (v))
	    return make_fixnum (v);
        }
    }

  struct Lisp_Bignum *b = ALLOCATE_PSEUDOVECTOR (struct Lisp_Bignum, value,
						 PVEC_BIGNUM);
  /* We could mpz_init + mpz_swap here, to avoid a copy, but the
     resulting API seemed possibly confusing.  */
  mpz_init_set (b->value, value);

  Lisp_Object bignum;
  XSETBIGNUM (bignum, b);
  return bignum;
}

void
mpz_set_intmax_slow (mpz_t result, intmax_t v)
{
  /* If long is larger then a faster path is taken.  */
  eassert (sizeof (intmax_t) > sizeof (long));

  bool negate = false;
  if (v < 0)
    {
      v = -v;
      negate = true;
    }
  mpz_set_uintmax_slow (result, (uintmax_t) v);
  if (negate)
    mpz_neg (result, result);
}

void
mpz_set_uintmax_slow (mpz_t result, uintmax_t v)
{
  /* If long is larger then a faster path is taken.  */
  eassert (sizeof (uintmax_t) > sizeof (unsigned long));

  /* COUNT = 1 means just a single word of the given size.  ORDER = -1
     is arbitrary since there's only a single word.  ENDIAN = 0 means
     use the native endian-ness.  NAILS = 0 means use the whole
     word.  */
  mpz_import (result, 1, -1, sizeof (uintmax_t), 0, 0, &v);
}


/* Return a newly created vector or string with specified arguments as
   elements.  If all the arguments are characters that can fit
   in a string of events, make a string; otherwise, make a vector.

   Any number of arguments, even zero arguments, are allowed.  */

Lisp_Object
make_event_array (ptrdiff_t nargs, Lisp_Object *args)
{
  ptrdiff_t i;

  for (i = 0; i < nargs; i++)
    /* The things that fit in a string
       are characters that are in 0...127,
       after discarding the meta bit and all the bits above it.  */
    if (!FIXNUMP (args[i])
	|| (XFIXNUM (args[i]) & ~(-CHAR_META)) >= 0200)
      return Fvector (nargs, args);

  /* Since the loop exited, we know that all the things in it are
     characters, so we can make a string.  */
  {
    Lisp_Object result;

    result = Fmake_string (make_fixnum (nargs), make_fixnum (0), Qnil);
    for (i = 0; i < nargs; i++)
      {
	SSET (result, i, XFIXNUM (args[i]));
	/* Move the meta bit to the right place for a string char.  */
	if (XFIXNUM (args[i]) & CHAR_META)
	  SSET (result, i, SREF (result, i) | 0x80);
      }

    return result;
  }
}

#ifdef HAVE_MODULES
/* Create a new module user ptr object.  */
Lisp_Object
make_user_ptr (void (*finalizer) (void *), void *p)
{
  struct Lisp_User_Ptr *uptr = ALLOCATE_PSEUDOVECTOR (struct Lisp_User_Ptr,
						      finalizer, PVEC_USER_PTR);
  uptr->finalizer = finalizer;
  uptr->p = p;
  return make_lisp_ptr (uptr, Lisp_Vectorlike);
}
#endif

/* Insert FINALIZER before ELEMENT.  */


/* Move doomed finalizers to list DEST from list SRC.  A doomed
   finalizer is one that is not GC-reachable and whose
   finalizer->function is non-nil.  */


/************************************************************************
			   Memory Full Handling
 ************************************************************************/


/* Called if malloc (NBYTES) returns zero.  If NBYTES == SIZE_MAX,
   there may have been size_t overflow so that malloc was never
   called, or perhaps malloc was invoked successfully but the
   resulting pointer had problems fitting into a tagged EMACS_INT.  In
   either case this counts as memory being full even though malloc did
   not fail.  */

/* If we released our reserve (due to running out of memory),
   and we have a fair amount free once again,
   try to set aside another reserve in case we run out once more.

   This is called when a relocatable block is freed in ralloc.c,
   and also directly from this file, in case we're not using ralloc.c.  */


/************************************************************************
			   C Stack Marking
 ************************************************************************/

/* Conservative C stack marking requires a method to identify possibly
   live Lisp objects given a pointer value.  We do this by keeping
   track of blocks of Lisp data that are allocated in a red-black tree
   (see also the comment of mem_node which is the type of nodes in
   that tree).  Function lisp_malloc adds information for an allocated
   block to the red-black tree with calls to mem_insert, and function
   lisp_free removes it with mem_delete.  Functions live_string_p etc
   call mem_find to lookup information about a given pointer in the
   tree, and use that to determine if the pointer points into a Lisp
   object or not.  */

/* Initialize this part of alloc.c.  */

/* Value is a pointer to the mem_node containing START.  Value is
   MEM_NIL if there is no node in the tree containing START.  */

static struct mem_node *
mem_find (void *start)
{
  struct mem_node *p;

  if (start < min_heap_address || start > max_heap_address)
    return MEM_NIL;

  /* Make the search always successful to speed up the loop below.  */
  mem_z.start = start;
  mem_z.end = (char *) start + 1;

  p = mem_root;
  while (start < p->start || start >= p->end)
    p = start < p->start ? p->left : p->right;
  return p;
}


/* Insert a new node into the tree for a block of memory with start
   address START, end address END, and type TYPE.  Value is a
   pointer to the node that was inserted.  */

static struct mem_node *
mem_insert (void *start, void *end, enum mem_type type)
{
  struct mem_node *c, *parent, *x;

  if (min_heap_address == NULL || start < min_heap_address)
    min_heap_address = start;
  if (max_heap_address == NULL || end > max_heap_address)
    max_heap_address = end;

  /* See where in the tree a node for START belongs.  In this
     particular application, it shouldn't happen that a node is already
     present.  For debugging purposes, let's check that.  */
  c = mem_root;
  parent = NULL;

  while (c != MEM_NIL)
    {
      parent = c;
      c = start < c->start ? c->left : c->right;
    }

  /* Create a new node.  */
#ifdef GC_MALLOC_CHECK
  x = malloc (sizeof *x);
  if (x == NULL)
    emacs_abort ();
#else
  x = xmalloc (sizeof *x);
#endif
  x->start = start;
  x->end = end;
  x->type = type;
  x->parent = parent;
  x->left = x->right = MEM_NIL;
  x->color = MEM_RED;

  /* Insert it as child of PARENT or install it as root.  */
  if (parent)
    {
      if (start < parent->start)
	parent->left = x;
      else
	parent->right = x;
    }
  else
    mem_root = x;

  /* Re-establish red-black tree properties.  */
  mem_insert_fixup (x);

  return x;
}


/* Re-establish the red-black properties of the tree, and thereby
   balance the tree, after node X has been inserted; X is always red.  */

static void
mem_insert_fixup (struct mem_node *x)
{
  while (x != mem_root && x->parent->color == MEM_RED)
    {
      /* X is red and its parent is red.  This is a violation of
	 red-black tree property #3.  */

      if (x->parent == x->parent->parent->left)
	{
	  /* We're on the left side of our grandparent, and Y is our
	     "uncle".  */
	  struct mem_node *y = x->parent->parent->right;

	  if (y->color == MEM_RED)
	    {
	      /* Uncle and parent are red but should be black because
		 X is red.  Change the colors accordingly and proceed
		 with the grandparent.  */
	      x->parent->color = MEM_BLACK;
	      y->color = MEM_BLACK;
	      x->parent->parent->color = MEM_RED;
	      x = x->parent->parent;
            }
	  else
	    {
	      /* Parent and uncle have different colors; parent is
		 red, uncle is black.  */
	      if (x == x->parent->right)
		{
		  x = x->parent;
		  mem_rotate_left (x);
                }

	      x->parent->color = MEM_BLACK;
	      x->parent->parent->color = MEM_RED;
	      mem_rotate_right (x->parent->parent);
            }
        }
      else
	{
	  /* This is the symmetrical case of above.  */
	  struct mem_node *y = x->parent->parent->left;

	  if (y->color == MEM_RED)
	    {
	      x->parent->color = MEM_BLACK;
	      y->color = MEM_BLACK;
	      x->parent->parent->color = MEM_RED;
	      x = x->parent->parent;
            }
	  else
	    {
	      if (x == x->parent->left)
		{
		  x = x->parent;
		  mem_rotate_right (x);
		}

	      x->parent->color = MEM_BLACK;
	      x->parent->parent->color = MEM_RED;
	      mem_rotate_left (x->parent->parent);
            }
        }
    }

  /* The root may have been changed to red due to the algorithm.  Set
     it to black so that property #5 is satisfied.  */
  mem_root->color = MEM_BLACK;
}


/* If P is a pointer into a live Lisp cons object on the heap, return
   the object.  Otherwise, return nil.  M is a pointer to the
   mem_block for P.  */

static Lisp_Object
live_cons_holding (struct mem_node *m, void *p)
{
  return Qnil;
}

static bool
live_cons_p (struct mem_node *m, void *p)
{
  return !NILP (live_cons_holding (m, p));
}


/* If P is a pointer into a live Lisp symbol object on the heap,
   return the object.  Otherwise, return nil.  M is a pointer to the
   mem_block for P.  */

static Lisp_Object
live_symbol_holding (struct mem_node *m, void *p)
{
  return Qnil;
}

static bool
live_symbol_p (struct mem_node *m, void *p)
{
  return !NILP (live_symbol_holding (m, p));
}


/* Return true if P is a pointer to a live Lisp float on
   the heap.  M is a pointer to the mem_block for P.  */

static bool
live_float_p (struct mem_node *m, void *p)
{
    return 0;
}

/* If P is a pointer to a live vector-like object, return the object.
   Otherwise, return nil.
   M is a pointer to the mem_block for P.  */

static Lisp_Object
live_vector_holding (struct mem_node *m, void *p)
{
  return Qnil;
}

static bool
live_vector_p (struct mem_node *m, void *p)
{
  return !NILP (live_vector_holding (m, p));
}

/* If P is a pointer into a live buffer, return the buffer.
   Otherwise, return nil.  M is a pointer to the mem_block for P.  */

static Lisp_Object
live_buffer_holding (struct mem_node *m, void *p)
{
  /* P must point into the block, and the buffer
     must not have been killed.  */
  if (m->type == MEM_TYPE_BUFFER)
    {
      struct buffer *b = m->start;
      char *cb = m->start;
      char *cp = p;
      ptrdiff_t offset = cp - cb;
      if (0 <= offset && offset < sizeof *b && !NILP (b->name_))
	{
	  Lisp_Object obj;
	  XSETBUFFER (obj, b);
	  return obj;
	}
    }
  return Qnil;
}

static bool
live_buffer_p (struct mem_node *m, void *p)
{
  return !NILP (live_buffer_holding (m, p));
}

#ifndef HAVE_MODULES
enum { HAVE_MODULES = false };
#endif

/* Force callee-saved registers and register windows onto the stack.
   Use the platform-defined __builtin_unwind_init if available,
   obviating the need for machine dependent methods.  */
#ifndef HAVE___BUILTIN_UNWIND_INIT
# ifdef __sparc__
   /* This trick flushes the register windows so that all the state of
      the process is contained in the stack.
      FreeBSD does not have a ta 3 handler, so handle it specially.
      FIXME: Code in the Boehm GC suggests flushing (with 'flushrs') is
      needed on ia64 too.  See mach_dep.c, where it also says inline
      assembler doesn't work with relevant proprietary compilers.  */
#  if defined __sparc64__ && defined __FreeBSD__
#   define __builtin_unwind_init() asm ("flushw")
#  else
#   define __builtin_unwind_init() asm ("ta 3")
#  endif
# else
#  define __builtin_unwind_init() ((void) 0)
# endif
#endif

/* Yield an address close enough to the top of the stack that the
   garbage collector need not scan above it.  Callers should be
   declared NO_INLINE.  */
#ifdef HAVE___BUILTIN_FRAME_ADDRESS
# define NEAR_STACK_TOP(addr) ((void) (addr), __builtin_frame_address (0))
#else
# define NEAR_STACK_TOP(addr) (addr)
#endif

/* Mark live Lisp objects on the C stack.

   There are several system-dependent problems to consider when
   porting this to new architectures:

   Processor Registers

   We have to mark Lisp objects in CPU registers that can hold local
   variables or are used to pass parameters.

   This code assumes that calling setjmp saves registers we need
   to see in a jmp_buf which itself lies on the stack.  This doesn't
   have to be true!  It must be verified for each system, possibly
   by taking a look at the source code of setjmp.

   If __builtin_unwind_init is available (defined by GCC >= 2.8) we
   can use it as a machine independent method to store all registers
   to the stack.  In this case the macros described in the previous
   two paragraphs are not used.

   Stack Layout

   Architectures differ in the way their processor stack is organized.
   For example, the stack might look like this

     +----------------+
     |  Lisp_Object   |  size = 4
     +----------------+
     | something else |  size = 2
     +----------------+
     |  Lisp_Object   |  size = 4
     +----------------+
     |	...	      |
*/

/***********************************************************************
		       Pure Storage Management
 ***********************************************************************/

/* Allocate room for SIZE bytes from pure Lisp storage and return a
   pointer to it.  TYPE is the Lisp type for which the memory is
   allocated.  TYPE < 0 means it's not used for a Lisp object.  */

static void *
pure_alloc (size_t size, int type)
{
  return xmalloc (size);
}


#ifndef CANNOT_DUMP

/* Print a warning if PURESIZE is too small.  */

void
check_pure_size (void)
{
  if (pure_bytes_used_before_overflow)
    message (("emacs:0:Pure Lisp storage overflow (approx. %"pI"d"
	      " bytes needed)"),
	     pure_bytes_used + pure_bytes_used_before_overflow);
}
#endif


/* Return a string allocated in pure space.  Do not
   allocate the string data, just point to DATA.  */

static Lisp_Object purecopy (Lisp_Object obj);

/* Return a cons allocated from pure space.  Give it pure copies
   of CAR as car and CDR as cdr.  */

Lisp_Object
pure_cons (Lisp_Object car, Lisp_Object cdr)
{
  Lisp_Object new = elisp_cons();
  XSETCAR (new, purecopy (car));
  XSETCDR (new, purecopy (cdr));
  return new;
}


/* Value is a float object with value NUM allocated from pure space.  */

static Lisp_Object
make_pure_float (double num)
{
  Lisp_Object new;
  XSETFLOAT (new, num);
  return new;
}

/* Value is a bignum object with value VALUE allocated from pure
   space.  */

static Lisp_Object
make_pure_bignum (struct Lisp_Bignum *value)
{

  size_t i, nlimbs = mpz_size (value->value);
  size_t nbytes = nlimbs * sizeof (mp_limb_t);
  mp_limb_t *pure_limbs;
  mp_size_t new_size;

  struct Lisp_Bignum *b = pure_alloc (sizeof *b, Lisp_Vectorlike);
  XSETPVECTYPESIZE (b, PVEC_BIGNUM, 0, VECSIZE (struct Lisp_Bignum));

  pure_limbs = pure_alloc (nbytes, -1);
  for (i = 0; i < nlimbs; ++i)
    pure_limbs[i] = mpz_getlimbn (value->value, i);

  new_size = nlimbs;
  if (mpz_sgn (value->value) < 0)
    new_size = -new_size;

  mpz_roinit_n (b->value, pure_limbs, new_size);

  Lisp_Object bignum;
  XSETBIGNUM (bignum, b);
  return bignum;
}

/* Return a vector with room for LEN Lisp_Objects allocated from
   pure space.  */

static Lisp_Object
make_pure_vector (ptrdiff_t len)
{
  Lisp_Object new;
  size_t size = header_size + len * word_size;
  struct Lisp_Vector *p = pure_alloc (size, Lisp_Vectorlike);
  XSETVECTOR (new, p);
  XVECTOR (new)->header.size = len;
  return new;
}

/* Copy all contents and parameters of TABLE to a new table allocated
   from pure space, return the purified table.  */
static struct Lisp_Hash_Table *
purecopy_hash_table (struct Lisp_Hash_Table *table)
{
  eassert (NILP (table->weak));
  eassert (table->pure);

  struct Lisp_Hash_Table *pure = pure_alloc (sizeof *pure, Lisp_Vectorlike);
  struct hash_table_test pure_test = table->test;

  /* Purecopy the hash table test.  */
  pure_test.name = purecopy (table->test.name);
  pure_test.user_hash_function = purecopy (table->test.user_hash_function);
  pure_test.user_cmp_function = purecopy (table->test.user_cmp_function);

  pure->header = table->header;
  pure->weak = purecopy (Qnil);
  pure->hash = purecopy (table->hash);
  pure->next = purecopy (table->next);
  pure->index = purecopy (table->index);
  pure->count = table->count;
  pure->next_free = table->next_free;
  pure->pure = table->pure;
  pure->rehash_threshold = table->rehash_threshold;
  pure->rehash_size = table->rehash_size;
  pure->key_and_value = purecopy (table->key_and_value);
  pure->test = pure_test;

  return pure;
}

DEFUN ("purecopy", Fpurecopy, Spurecopy, 1, 1, 0,
       doc: /* Make a copy of object OBJ in pure storage.
Recursively copies contents of vectors and cons cells.
Does not copy symbols.  Copies strings without text properties.  */)
  (register Lisp_Object obj)
{
  if (NILP (Vpurify_flag))
    return obj;
  else if (MARKERP (obj) || OVERLAYP (obj) || SYMBOLP (obj))
    /* Can't purify those.  */
    return obj;
  else
    return purecopy (obj);
}

/* Pinned objects are marked before every GC cycle.  */
static struct pinned_object
{
  Lisp_Object object;
  struct pinned_object *next;
} *pinned_objects;

static Lisp_Object
purecopy (Lisp_Object obj)
{
  return obj;    /* Already pure.  */
}



/***********************************************************************
			  Protection from GC

   In such a case, not every Lisp_Object will be aligned equally.  To
   find all Lisp_Object on the stack it won't be sufficient to walk
   the stack in steps of 4 bytes.  Instead, two passes will be
   necessary, one starting at the start of the stack, and a second
   pass starting at the start of the stack + 2.  Likewise, if the
   minimal alignment of Lisp_Objects on the stack is 1, four passes
   would be necessary, each one starting with one byte more offset
   from the stack start.  */

void
mark_stack (char *bottom, char *end)
{
  /* This assumes that the stack is a contiguous region in memory.  If
     that's not the case, something has to be done here to iterate
     over the stack segments.  */
  mark_memory (bottom, end);

  /* Allow for marking a secondary stack, like the register stack on the
     ia64.  */
#ifdef GC_MARK_SECONDARY_STACK
  GC_MARK_SECONDARY_STACK ();
#endif
}

/* This is a trampoline function that flushes registers to the stack,
   and then calls FUNC.  ARG is passed through to FUNC verbatim.

   This function must be called whenever Emacs is about to release the
   global interpreter lock.  This lets the garbage collector easily
   find roots in registers on threads that are not actively running
   Lisp.

   It is invalid to run any Lisp code or to allocate any GC memory
   from FUNC.  */

NO_INLINE void
flush_stack_call_func (void (*func) (void *arg), void *arg)
{
  void *end;
  struct thread_state *self = current_thread;
  self->stack_top = end;
  func (arg);
  eassert (current_thread == self);
}

static bool
c_symbol_p (struct Lisp_Symbol *sym)
{
  char *lispsym_ptr = (char *) lispsym;
  char *sym_ptr = (char *) sym;
  ptrdiff_t lispsym_offset = sym_ptr - lispsym_ptr;
  return 0 <= lispsym_offset && lispsym_offset < sizeof lispsym;
}

/* Determine whether it is safe to access memory at address P.  */
static int
valid_pointer_p (void *p)
{
#ifdef WINDOWSNT
  return w32_valid_pointer_p (p, 16);
#else

  if (ADDRESS_SANITIZER)
    return p ? -1 : 0;

  int fd[2];

  /* Obviously, we cannot just access it (we would SEGV trying), so we
     trick the o/s to tell us whether p is a valid pointer.
     Unfortunately, we cannot use NULL_DEVICE here, as emacs_write may
     not validate p in that case.  */

  if (emacs_pipe (fd) == 0)
    {
      bool valid = emacs_write (fd[1], p, 16) == 16;
      emacs_close (fd[1]);
      emacs_close (fd[0]);
      return valid;
    }

  return -1;
#endif
}

/* Return 2 if OBJ is a killed or special buffer object, 1 if OBJ is a
   valid lisp object, 0 if OBJ is NOT a valid lisp object, or -1 if we
   cannot validate OBJ.  This function can be quite slow, and is used
   only in debugging.  */

int
valid_lisp_object_p (Lisp_Object obj)
{
  return 0;
}

/***********************************************************************
		       Pure Storage Management
 ***********************************************************************/

#ifndef CANNOT_DUMP

/* Print a warning if PURESIZE is too small.  */

void
check_pure_size (void)
{
  if (pure_bytes_used_before_overflow)
    message (("emacs:0:Pure Lisp storage overflow (approx. %"pI"d"
	      " bytes needed)"),
	     pure_bytes_used + pure_bytes_used_before_overflow);
}
#endif


/* Return a string allocated in pure space.  DATA is a buffer holding
   NCHARS characters, and NBYTES bytes of string data.  MULTIBYTE
   means make the result string multibyte.

   Must get an error if pure storage is full, since if it cannot hold
   a large string it may be able to hold conses that point to that
   string; then the string is not protected from gc.  */

Lisp_Object
make_pure_string (const char *data,
		  ptrdiff_t nchars, ptrdiff_t nbytes, bool multibyte)
{
  char *newbuf = xmalloc(nbytes + 1);
  memcpy (newbuf, data, nbytes);
  newbuf[nbytes] = '\0';

  Lisp_Object fixbuf = elisp_fixbuf(newbuf, nbytes + 1);
  Lisp_Object mbstring = elisp_mbstring(fixbuf, multibyte ? nbytes : -1, nchars);
  Lisp_Object string = elisp_string(mbstring, NULL);

  return string;
}

/* Return a string allocated in pure space.  Do not
   allocate the string data, just point to DATA.  */

Lisp_Object
make_pure_c_string (const char *data, ptrdiff_t nchars)
{
  Lisp_Object fixbuf = elisp_fixbuf((char *)data, nchars + 1);
  Lisp_Object mbstring = elisp_mbstring(fixbuf, -1, nchars);
  Lisp_Object string = elisp_string(mbstring, NULL);

  return string;
}


/***********************************************************************
			  Protection from GC
 ***********************************************************************/

/* Put an entry in staticvec, pointing at the variable with address
   VARADDRESS.  */

void
staticpro_1 (Lisp_Object *varaddress)
{
  if (staticidx >= NSTATICS)
    fatal ("NSTATICS too small; try increasing and recompiling Emacs.");
  staticvec[staticidx++] = varaddress;
}


/***********************************************************************
			  Protection from GC
 ***********************************************************************/

/* Temporarily prevent garbage collection.  */

ptrdiff_t
inhibit_garbage_collection (void)
{
  ptrdiff_t count = SPECPDL_INDEX ();

  specbind (Qgc_cons_threshold, make_fixnum (MOST_POSITIVE_FIXNUM));
  return count;
}

/* Used to avoid possible overflows when
   converting from C to Lisp integers.  */

static Lisp_Object
bounded_number (EMACS_INT number)
{
  return make_fixnum (min (MOST_POSITIVE_FIXNUM, number));
}

/* Calculate total bytes of live objects.  */

static size_t
total_bytes_of_live_objects (void)
{
  size_t tot = 0;
  tot += total_vector_slots * word_size;
  tot += total_intervals * sizeof (struct interval);
  return tot;
}

#ifdef HAVE_WINDOW_SYSTEM

/* Compact font caches on all terminals and mark
   everything which is still here after compaction.  */

#else /* not HAVE_WINDOW_SYSTEM */

#define compact_font_caches() (void)(0)

#endif /* HAVE_WINDOW_SYSTEM */

DEFUN ("garbage-collect", Fgarbage_collect, Sgarbage_collect, 0, 0, "",
       doc: /* Reclaim storage for Lisp objects no longer needed.
Garbage collection happens automatically if you cons more than
`gc-cons-threshold' bytes of Lisp data since previous garbage collection.
`garbage-collect' normally returns a list with info on amount of space in use,
where each entry has the form (NAME SIZE USED FREE), where:
- NAME is a symbol describing the kind of objects this entry represents,
- SIZE is the number of bytes used by each one,
- USED is the number of those objects that were found live in the heap,
- FREE is the number of those objects that are not live but that Emacs
  keeps around for future allocations (maybe because it does not know how
  to return them to the OS).
However, if there was overflow in pure space, `garbage-collect'
returns nil, because real GC can't be done.
See Info node `(elisp)Garbage Collection'.  */
       attributes: noinline)
  (void)
{
  return Qnil;
}

enum { LAST_MARKED_SIZE = 1 << 9 }; /* Must be a power of 2.  */
Lisp_Object last_marked[LAST_MARKED_SIZE] EXTERNALLY_VISIBLE;
static int last_marked_index;

/* For debugging--call abort when we cdr down this many
   links of a list, in mark_object.  In debugging,
   the call to abort will hit a breakpoint.
   Normally this is zero and the check never goes off.  */
ptrdiff_t mark_object_loop_halt EXTERNALLY_VISIBLE;

DEFUN ("memory-info", Fmemory_info, Smemory_info, 0, 0, 0,
       doc: /* Return a list of (TOTAL-RAM FREE-RAM TOTAL-SWAP FREE-SWAP).
All values are in Kbytes.  If there is no swap space,
last two values are zero.  If the system is not supported
or memory information can't be obtained, return nil.  */)
  (void)
{
#if defined HAVE_LINUX_SYSINFO
  struct sysinfo si;
  uintmax_t units;

  if (sysinfo (&si))
    return Qnil;
#ifdef LINUX_SYSINFO_UNIT
  units = si.mem_unit;
#else
  units = 1;
#endif
  return list4i ((uintmax_t) si.totalram * units / 1024,
		 (uintmax_t) si.freeram * units / 1024,
		 (uintmax_t) si.totalswap * units / 1024,
		 (uintmax_t) si.freeswap * units / 1024);
#elif defined WINDOWSNT
  unsigned long long totalram, freeram, totalswap, freeswap;

  if (w32_memory_info (&totalram, &freeram, &totalswap, &freeswap) == 0)
    return list4i ((uintmax_t) totalram / 1024,
		   (uintmax_t) freeram / 1024,
		   (uintmax_t) totalswap / 1024,
		   (uintmax_t) freeswap / 1024);
  else
    return Qnil;
#elif defined MSDOS
  unsigned long totalram, freeram, totalswap, freeswap;

  if (dos_memory_info (&totalram, &freeram, &totalswap, &freeswap) == 0)
    return list4i ((uintmax_t) totalram / 1024,
		   (uintmax_t) freeram / 1024,
		   (uintmax_t) totalswap / 1024,
		   (uintmax_t) freeswap / 1024);
  else
    return Qnil;
#else /* not HAVE_LINUX_SYSINFO, not WINDOWSNT, not MSDOS */
  /* FIXME: add more systems.  */
  return Qnil;
#endif /* HAVE_LINUX_SYSINFO, not WINDOWSNT, not MSDOS */
}

/* Debugging aids.  */

DEFUN ("memory-use-counts", Fmemory_use_counts, Smemory_use_counts, 0, 0, 0,
       doc: /* Return a list of counters that measure how much consing there has been.
Each of these counters increments for a certain kind of object.
The counters wrap around from the largest positive integer to zero.
Garbage collection does not decrease them.
The elements of the value are as follows:
  (CONSES FLOATS VECTOR-CELLS SYMBOLS STRING-CHARS INTERVALS STRINGS)
All are in units of 1 = one object consed
except for VECTOR-CELLS and STRING-CHARS, which count the total length of
objects consed.
Frames, windows, buffers, and subprocesses count as vectors
  (but the contents of a buffer's text do not count here).  */)
  (void)
{
  return listn (CONSTYPE_HEAP, 8,
		bounded_number (cons_cells_consed),
		bounded_number (floats_consed),
		bounded_number (vector_cells_consed),
		bounded_number (symbols_consed),
		bounded_number (string_chars_consed),
		bounded_number (intervals_consed),
		bounded_number (strings_consed));
}

static bool
symbol_uses_obj (Lisp_Object symbol, Lisp_Object obj)
{
  Lisp_Object val = find_symbol_value (symbol);
  return (EQ (val, obj)
	  || EQ (elisp_symbol_function (symbol), obj)
	  || (!NILP (elisp_symbol_function (symbol))
	      && COMPILEDP (elisp_symbol_function (symbol))
	      && EQ (AREF (elisp_symbol_function (symbol), COMPILED_BYTECODE), obj))
	  || (!NILP (val)
	      && COMPILEDP (val)
	      && EQ (AREF (val, COMPILED_BYTECODE), obj)));
}

/* Find at most FIND_MAX symbols which have OBJ as their value or
   function.  This is used in gdbinit's `xwhichsymbols' command.  */

Lisp_Object
which_symbols (Lisp_Object obj, EMACS_INT find_max)
{
   Lisp_Object found = Qnil;
   return found;
}


DEFUN ("suspicious-object", Fsuspicious_object, Ssuspicious_object, 1, 1, 0,
       doc: /* Return OBJ, maybe marking it for extra scrutiny.
If Emacs is compiled with suspicious object checking, capture
a stack trace when OBJ is freed in order to help track down
garbage collection bugs.  Otherwise, do nothing and return OBJ.   */)
   (Lisp_Object obj)
{
  return obj;
}

#ifdef ENABLE_CHECKING

bool suppress_checking;

void
die (const char *msg, const char *file, int line)
{
  fprintf (stderr, "\r\n%s:%d: Emacs fatal error: assertion failed: %s\r\n",
	   file, line, msg);
  terminate_due_to_signal (SIGABRT, INT_MAX);
}

#endif /* ENABLE_CHECKING */

/* Initialization.  */

void
init_alloc_once (void)
{
  for (ptrdiff_t i = 0; i < GLOBAL_OBJECTS; i++) {
    ((ELisp_Struct_Value *)&globals)[i] = Qnil;
  }
  /* Even though Qt's contents are not set up, its address is known.  */
  Vpurify_flag = Qt;

  purebeg = PUREBEG;
  pure_size = PURESIZE;

  init_finalizer_list (&finalizers);
  init_finalizer_list (&doomed_finalizers);

  mem_init ();
  Vdead = make_pure_string ("DEAD", 4, 4, 0);

#ifdef DOUG_LEA_MALLOC
  mallopt (M_TRIM_THRESHOLD, 128 * 1024); /* Trim threshold.  */
  mallopt (M_MMAP_THRESHOLD, 64 * 1024);  /* Mmap threshold.  */
  mallopt (M_MMAP_MAX, MMAP_MAX_AREAS);   /* Max. number of mmap'ed areas.  */
#endif
  init_strings ();
  init_vectors ();

  refill_memory_reserve ();
  gc_cons_threshold = GC_DEFAULT_THRESHOLD;
}

void
init_alloc (void)
{
  Vgc_elapsed = make_float (0.0);
  gcs_done = 0;

#if USE_VALGRIND
  valgrind_p = RUNNING_ON_VALGRIND != 0;
#endif
}

void
syms_of_alloc (void)
{
  staticpro_1 (&zero_vector);

  DEFVAR_INT ("gc-cons-threshold", gc_cons_threshold,
	      doc: /* Number of bytes of consing between garbage collections.
Garbage collection can happen automatically once this many bytes have been
allocated since the last garbage collection.  All data types count.

Garbage collection happens automatically only when `eval' is called.

By binding this temporarily to a large number, you can effectively
prevent garbage collection during a part of the program.
See also `gc-cons-percentage'.  */);

  DEFVAR_LISP ("gc-cons-percentage", Vgc_cons_percentage,
	       doc: /* Portion of the heap used for allocation.
Garbage collection can happen automatically once this portion of the heap
has been allocated since the last garbage collection.
If this portion is smaller than `gc-cons-threshold', this is ignored.  */);
  Vgc_cons_percentage = make_float (0.1);

  DEFVAR_INT ("pure-bytes-used", pure_bytes_used,
	      doc: /* Number of bytes of shareable Lisp data allocated so far.  */);

  DEFVAR_INT ("cons-cells-consed", cons_cells_consed,
	      doc: /* Number of cons cells that have been consed so far.  */);

  DEFVAR_INT ("floats-consed", floats_consed,
	      doc: /* Number of floats that have been consed so far.  */);

  DEFVAR_INT ("vector-cells-consed", vector_cells_consed,
	      doc: /* Number of vector cells that have been consed so far.  */);

  DEFVAR_INT ("symbols-consed", symbols_consed,
	      doc: /* Number of symbols that have been consed so far.  */);
  symbols_consed += ARRAYELTS (lispsym);

  DEFVAR_INT ("string-chars-consed", string_chars_consed,
	      doc: /* Number of string characters that have been consed so far.  */);

  DEFVAR_INT ("intervals-consed", intervals_consed,
	      doc: /* Number of intervals that have been consed so far.  */);

  DEFVAR_INT ("strings-consed", strings_consed,
	      doc: /* Number of strings that have been consed so far.  */);

  DEFVAR_LISP ("purify-flag", Vpurify_flag,
	       doc: /* Non-nil means loading Lisp code in order to dump an executable.
This means that certain objects should be allocated in shared (pure) space.
It can also be set to a hash-table, in which case this table is used to
do hash-consing of the objects allocated to pure space.  */);

  DEFVAR_BOOL ("garbage-collection-messages", garbage_collection_messages,
	       doc: /* Non-nil means display messages at start and end of garbage collection.  */);
  garbage_collection_messages = 0;

  DEFVAR_LISP ("post-gc-hook", Vpost_gc_hook,
	       doc: /* Hook run after garbage collection has finished.  */);
  Vpost_gc_hook = Qnil;
  DEFSYM (Qpost_gc_hook, "post-gc-hook");

  DEFVAR_LISP ("memory-signal-data", Vmemory_signal_data,
	       doc: /* Precomputed `signal' argument for memory-full error.  */);
  /* We build this in advance because if we wait until we need it, we might
     not be able to allocate the memory to hold it.  */
  Vmemory_signal_data
    = listn (CONSTYPE_PURE, 2, Qerror,
	     build_pure_c_string ("Memory exhausted--use M-x save-some-buffers then exit and restart Emacs"));

  DEFVAR_LISP ("memory-full", Vmemory_full,
	       doc: /* Non-nil means Emacs cannot get much more Lisp memory.  */);
  Vmemory_full = Qnil;

  DEFSYM (Qconses, "conses");
  DEFSYM (Qsymbols, "symbols");
  DEFSYM (Qstrings, "strings");
  DEFSYM (Qvectors, "vectors");
  DEFSYM (Qfloats, "floats");
  DEFSYM (Qintervals, "intervals");
  DEFSYM (Qbuffers, "buffers");
  DEFSYM (Qstring_bytes, "string-bytes");
  DEFSYM (Qvector_slots, "vector-slots");
  DEFSYM (Qheap, "heap");
  DEFSYM (QAutomatic_GC, "Automatic GC");

  DEFSYM (Qgc_cons_threshold, "gc-cons-threshold");
  DEFSYM (Qchar_table_extra_slots, "char-table-extra-slots");

  DEFVAR_LISP ("gc-elapsed", Vgc_elapsed,
	       doc: /* Accumulated time elapsed in garbage collections.
The time is in seconds as a floating point value.  */);
  DEFVAR_INT ("gcs-done", gcs_done,
              doc: /* Accumulated number of garbage collections done.  */);

  defsubr (&Scons);
  defsubr (&Slist);
  defsubr (&Svector);
  defsubr (&Srecord);
  defsubr (&Sbool_vector);
  defsubr (&Smake_byte_code);
  defsubr (&Smake_list);
  defsubr (&Smake_vector);
  defsubr (&Smake_record);
  defsubr (&Smake_string);
  defsubr (&Smake_bool_vector);
  defsubr (&Smake_symbol);
  defsubr (&Smake_marker);
  defsubr (&Smake_finalizer);
  defsubr (&Spurecopy);
  defsubr (&Sgarbage_collect);
  defsubr (&Smemory_info);
  defsubr (&Smemory_use_counts);
  defsubr (&Ssuspicious_object);
}

/* When compiled with GCC, GDB might say "No enum type named
   pvec_type" if we don't have at least one symbol with that type, and
   then xbacktrace could fail.  Similarly for the other enums and
   their values.  Some non-GCC compilers don't like these constructs.  */
#ifdef __GNUC__
union
{
  enum CHARTAB_SIZE_BITS CHARTAB_SIZE_BITS;
  enum char_table_specials char_table_specials;
  enum char_bits char_bits;
  //enum CHECK_LISP_OBJECT_TYPE CHECK_LISP_OBJECT_TYPE;
  enum DEFAULT_HASH_SIZE DEFAULT_HASH_SIZE;
  enum Lisp_Bits Lisp_Bits;
  enum Lisp_Compiled Lisp_Compiled;
  enum maxargs maxargs;
  enum MAX_ALLOCA MAX_ALLOCA;
  enum More_Lisp_Bits More_Lisp_Bits;
  enum pvec_type pvec_type;
} const EXTERNALLY_VISIBLE gdb_make_enums_visible = {0};
#endif	/* __GNUC__ */

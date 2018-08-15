#if defined(JSLISP_HH_SECTION_1)
#undef JSLISP_HH_SECTION_1
#include "jslisp-headers.hh"
#elif defined(JSLISP_HH_SECTION_1B)
#undef JSLISP_HH_SECTION_1B
#include "jslisp-jsg.hh"
#elif defined(JSLISP_HH_SECTION_1C)
#undef JSLISP_HH_SECTION_1C

#include <alloca.h>
#include <setjmp.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>

#include <intprops.h>
#include <verify.h>

INLINE_HEADER_BEGIN

/* Define a TYPE constant ID as an externally visible name.  Use like this:

      DEFINE_GDB_SYMBOL_BEGIN (TYPE, ID)
      # define ID (some integer preprocessor expression of type TYPE)
      DEFINE_GDB_SYMBOL_END (ID)

   This hack is for the benefit of compilers that do not make macro
   definitions or enums visible to the debugger.  It's used for symbols
   that .gdbinit needs.  */

#define DECLARE_GDB_SYM(type, id) type const id EXTERNALLY_VISIBLE
#ifdef MAIN_PROGRAM
# define DEFINE_GDB_SYMBOL_BEGIN(type, id) DECLARE_GDB_SYM (type, id)
# define DEFINE_GDB_SYMBOL_END(id) = id;
#else
# define DEFINE_GDB_SYMBOL_BEGIN(type, id) extern DECLARE_GDB_SYM (type, id)
# define DEFINE_GDB_SYMBOL_END(val) ;
#endif

/* The ubiquitous max and min macros.  */
#undef c_min
#undef c_max
#define c_max(a, b) ((a) > (b) ? (a) : (b))
#define c_min(a, b) ((a) < (b) ? (a) : (b))

/* Number of bits in a Lisp_Object tag.  */
DEFINE_GDB_SYMBOL_BEGIN (int, GCTYPEBITS)
#define GCTYPEBITS 3
DEFINE_GDB_SYMBOL_END (GCTYPEBITS)

/* EMACS_INT - signed integer wide enough to hold an Emacs value
   EMACS_INT_WIDTH - width in bits of EMACS_INT
   EMACS_INT_MAX - maximum value of EMACS_INT; can be used in #if
   pI - printf length modifier for EMACS_INT
   EMACS_UINT - unsigned variant of EMACS_INT */
#ifndef EMACS_INT_MAX
# if INTPTR_MAX <= 0
#  error "INTPTR_MAX misconfigured"
# elif INTPTR_MAX <= INT_MAX && !defined WIDE_EMACS_INT
typedef int EMACS_INT;
typedef unsigned int EMACS_UINT;
enum { EMACS_INT_WIDTH = INT_WIDTH, EMACS_UINT_WIDTH = UINT_WIDTH };
#  define EMACS_INT_MAX INT_MAX
#  define pI ""
# elif INTPTR_MAX <= LONG_MAX && !defined WIDE_EMACS_INT
typedef long int EMACS_INT;
typedef unsigned long EMACS_UINT;
enum { EMACS_INT_WIDTH = LONG_WIDTH, EMACS_UINT_WIDTH = ULONG_WIDTH };
#  define EMACS_INT_MAX LONG_MAX
#  define pI "l"
# elif INTPTR_MAX <= LLONG_MAX
typedef long long int EMACS_INT;
typedef unsigned long long int EMACS_UINT;
enum { EMACS_INT_WIDTH = LLONG_WIDTH, EMACS_UINT_WIDTH = ULLONG_WIDTH };
#  define EMACS_INT_MAX LLONG_MAX
/* MinGW supports %lld only if __USE_MINGW_ANSI_STDIO is non-zero,
   which is arranged by config.h, and (for mingw.org) if GCC is 6.0 or
   later and the runtime version is 5.0.0 or later.  Otherwise,
   printf-like functions are declared with __ms_printf__ attribute,
   which will cause a warning for %lld etc.  */
#  if defined __MINGW32__						\
  && (!defined __USE_MINGW_ANSI_STDIO					\
      || (!defined MINGW_W64						\
          && !(GNUC_PREREQ (6, 0, 0) && __MINGW32_MAJOR_VERSION >= 5)))
#   define pI "I64"
#  else	 /* ! MinGW */
#   define pI "ll"
#  endif
# else
#  error "INTPTR_MAX too large"
# endif
#endif

/* Number of bits to put in each character in the internal representation
   of bool vectors.  This should not vary across implementations.  */
enum {  BOOL_VECTOR_BITS_PER_CHAR =
#define BOOL_VECTOR_BITS_PER_CHAR 8
        BOOL_VECTOR_BITS_PER_CHAR
};

/* An unsigned integer type representing a fixed-length bit sequence,
   suitable for bool vector words, GC mark bits, etc.  Normally it is size_t
   for speed, but on weird platforms it is unsigned char and not all
   its bits are used.  */
#if BOOL_VECTOR_BITS_PER_CHAR == CHAR_BIT
typedef size_t bits_word;
# define BITS_WORD_MAX SIZE_MAX
enum { BITS_PER_BITS_WORD = SIZE_WIDTH };
#else
typedef unsigned char bits_word;
# define BITS_WORD_MAX ((1u << BOOL_VECTOR_BITS_PER_CHAR) - 1)
enum { BITS_PER_BITS_WORD = BOOL_VECTOR_BITS_PER_CHAR };
#endif
verify (BITS_WORD_MAX >> (BITS_PER_BITS_WORD - 1) == 1);

/* printmax_t and uprintmax_t are types for printing large integers.
   These are the widest integers that are supported for printing.
   pMd etc. are conversions for printing them.
   On C99 hosts, there's no problem, as even the widest integers work.
   Fall back on EMACS_INT on pre-C99 hosts.  */
#ifdef PRIdMAX
typedef intmax_t printmax_t;
typedef uintmax_t uprintmax_t;
# define pMd PRIdMAX
# define pMu PRIuMAX
#else
typedef EMACS_INT printmax_t;
typedef EMACS_UINT uprintmax_t;
# define pMd pI"d"
# define pMu pI"u"
#endif

/* Use pD to format ptrdiff_t values, which suffice for indexes into
   buffers and strings.  Emacs never allocates objects larger than
   PTRDIFF_MAX bytes, as they cause problems with pointer subtraction.
   In C99, pD can always be "t"; configure it here for the sake of
   pre-C99 libraries such as glibc 2.0 and Solaris 8.  */
#if PTRDIFF_MAX == INT_MAX
# define pD ""
#elif PTRDIFF_MAX == LONG_MAX
# define pD "l"
#elif PTRDIFF_MAX == LLONG_MAX
# define pD "ll"
#else
# define pD "t"
#endif

/* Extra internal type checking?  */

/* Define Emacs versions of <assert.h>'s 'assert (COND)' and <verify.h>'s
   'assume (COND)'.  COND should be free of side effects, as it may or
   may not be evaluated.

   'eassert (COND)' checks COND at runtime if ENABLE_CHECKING is
   defined and suppress_checking is false, and does nothing otherwise.
   Emacs dies if COND is checked and is false.  The suppress_checking
   variable is initialized to 0 in alloc.c.  Set it to 1 using a
   debugger to temporarily disable aborting on detected internal
   inconsistencies or error conditions.

   In some cases, a good compiler may be able to optimize away the
   eassert macro even if ENABLE_CHECKING is true, e.g., if XSTRING (x)
   uses eassert to test STRINGP (x), but a particular use of XSTRING
   is invoked only after testing that STRINGP (x) is true, making the
   test redundant.

   eassume is like eassert except that it also causes the compiler to
   assume that COND is true afterwards, regardless of whether runtime
   checking is enabled.  This can improve performance in some cases,
   though it can degrade performance in others.  It's often suboptimal
   for COND to call external functions or access volatile storage.  */

#ifndef ENABLE_CHECKING
# define eassert(cond) ((void) (false && (cond))) /* Check COND compiles.  */
# define eassume(cond) assume (cond)
#else /* ENABLE_CHECKING */

extern _Noreturn void die (const char *, const char *, int);

extern bool suppress_checking EXTERNALLY_VISIBLE;

# define eassert(cond)						\
   (suppress_checking || (cond)                                 \
    ? (void) 0							\
    : die (# cond, __FILE__, __LINE__))
# define eassume(cond)						\
   (suppress_checking						\
    ? assume (cond)						\
    : (cond)							\
    ? (void) 0							\
    : die (# cond, __FILE__, __LINE__))
#endif /* ENABLE_CHECKING */


/* Use the configure flag --enable-check-lisp-object-type to make
   Lisp_Object use a struct type instead of the default int.  The flag
   causes CHECK_LISP_OBJECT_TYPE to be defined.  */

/***** Select the tagging scheme.  *****/
/* The following option controls the tagging scheme:
   - USE_LSB_TAG means that we can assume the least 3 bits of pointers are
     always 0, and we can thus use them to hold tag bits, without
     restricting our addressing space.

   If ! USE_LSB_TAG, then use the top 3 bits for tagging, thus
   restricting our possible address range.

   USE_LSB_TAG not only requires the least 3 bits of pointers returned by
   malloc to be 0 but also needs to be able to impose a mult-of-8 alignment
   on the few static Lisp_Objects used: lispsym, all the defsubr, and
   the two special buffers buffer_defaults and buffer_local_symbols.  */

enum Lisp_Bits
  {
    /* 2**GCTYPEBITS.  This must be a macro that expands to a literal
       integer constant, for MSVC.  */
#define GCALIGNMENT 8

    /* Number of bits in a Lisp_Object value, not counting the tag.  */
    VALBITS = EMACS_INT_WIDTH - GCTYPEBITS,

    /* Number of bits in a Lisp fixnum tag.  */
    INTTYPEBITS = GCTYPEBITS - 1,

    /* Number of bits in a Lisp fixnum value, not counting the tag.  */
   FIXNUM_BITS = VALBITS + 1
  };

#if GCALIGNMENT != 1 << GCTYPEBITS
# error "GCALIGNMENT and GCTYPEBITS are inconsistent"
#endif

/* The maximum value that can be stored in a EMACS_INT, assuming all
   bits other than the type bits contribute to a nonnegative signed value.
   This can be used in #if, e.g., '#if USE_LSB_TAG' below expands to an
   expression involving VAL_MAX.  */
#define VAL_MAX (EMACS_INT_MAX >> (GCTYPEBITS - 1))

/* Whether the least-significant bits of an EMACS_INT contain the tag.
   On hosts where pointers-as-ints do not exceed VAL_MAX / 2, USE_LSB_TAG is:
    a. unnecessary, because the top bits of an EMACS_INT are unused, and
    b. slower, because it typically requires extra masking.
   So, USE_LSB_TAG is true only on hosts where it might be useful.  */
DEFINE_GDB_SYMBOL_BEGIN (bool, USE_LSB_TAG)
#define USE_LSB_TAG (VAL_MAX / 2 < INTPTR_MAX)
DEFINE_GDB_SYMBOL_END (USE_LSB_TAG)

/* Mask for the value (as opposed to the type bits) of a Lisp object.  */
DEFINE_GDB_SYMBOL_BEGIN (EMACS_INT, VALMASK)
# define VALMASK (USE_LSB_TAG ? - (1 << GCTYPEBITS) : VAL_MAX)
DEFINE_GDB_SYMBOL_END (VALMASK)

#if !USE_LSB_TAG && !defined WIDE_EMACS_INT
# error "USE_LSB_TAG not supported on this platform; please report this." \
        "Try 'configure --with-wide-int' to work around the problem."
#endif

#ifdef HAVE_STRUCT_ATTRIBUTE_ALIGNED
# define GCALIGNED __attribute__ ((aligned (GCALIGNMENT)))
#else
# define GCALIGNED /* empty */
#endif

/* Some operations are so commonly executed that they are implemented
   as macros, not functions, because otherwise runtime performance would
   suffer too much when compiling with GCC without optimization.
   There's no need to inline everything, just the operations that
   would otherwise cause a serious performance problem.

   For each such operation OP, define a macro lisp_h_OP that contains
   the operation's implementation.  That way, OP can be implemented
   via a macro definition like this:

     #define OP(x) lisp_h_OP (x)

   and/or via a function definition like this:

     Lisp_Object (OP) (Lisp_Object x) { return lisp_h_OP (x); }

   without worrying about the implementations diverging, since
   lisp_h_OP defines the actual implementation.  The lisp_h_OP macros
   are intended to be private to this include file, and should not be
   used elsewhere.

   FIXME: Remove the lisp_h_OP macros, and define just the inline OP
   functions, once "gcc -Og" (new to GCC 4.8) works well enough for
   Emacs developers.  Maybe in the year 2020.  See Bug#11935.

   Commentary for these macros can be found near their corresponding
   functions, below.  */

#define lisp_h_CHECK_NUMBER(x) CHECK_TYPE (NUMBERP (x), LSH (Qintegerp), x)
#define lisp_h_CHECK_FIXNUM(x) CHECK_TYPE (FIXNUMP (x), LSH (Qfixnump), x)
#define lisp_h_CHECK_SYMBOL(x) CHECK_TYPE (SYMBOLP (x), LSH (Qsymbolp), x)
#define lisp_h_CHECK_TYPE(ok, predicate, x) \
   ((ok) ? (void) 0 : wrong_type_argument (predicate, x))
#define lisp_h_CONSP(x) (XTYPE (x) == Lisp_Cons)
#define lisp_h_EQ(x, y) (x).eq(y)
#define lisp_h_FLOATP(x) (XTYPE (x) == Lisp_Float)
#define lisp_h_FIXNUMP(x) ((XTYPE (x) & (Lisp_Int0 | ~Lisp_Int1)) == Lisp_Int0)
#define lisp_h_MARKERP(x) (PSEUDOVECTORP (x, PVEC_MARKER))
#define lisp_h_NILP(x) EQ (x, Qnil)
#define lisp_h_SET_SYMBOL_VAL(sym, v) \
  (elisp_symbol_set_value (sym, v))
#define lisp_h_SYMBOL_CONSTANT_P(sym) (elisp_symbol_trapped_write_value(sym) == SYMBOL_NOWRITE)
#define lisp_h_SYMBOL_TRAPPED_WRITE_P(sym) (elisp_symbol_trapped_write_value (sym))
#define lisp_h_SYMBOL_VAL(sym) \
   (elisp_symbol_value (sym))
#define lisp_h_SYMBOLP(x) (XTYPE (x) == Lisp_Symbol)
#ifndef GC_CHECK_CONS_LIST
# define lisp_h_check_cons_list() ((void) 0)
#endif
#if USE_LSB_TAG
#endif

/* When compiling via gcc -O0, define the key operations as macros, as
   Emacs is too slow otherwise.  To disable this optimization, compile
   with -DINLINING=false.  */
# define DEFINE_KEY_OPS_AS_MACROS true

#define CHECK_TYPE(ok, predicate, x) lisp_h_CHECK_TYPE (ok, predicate, x)
#if DEFINE_KEY_OPS_AS_MACROS
# define FLOATP(x) ((x).floatp())
# define FIXNUMP(x) ((x).integerp())
# define SET_SYMBOL_VAL(sym, v) lisp_h_SET_SYMBOL_VAL (sym, v)
# define SYMBOL_CONSTANT_P(sym) lisp_h_SYMBOL_CONSTANT_P (sym)
# define SYMBOL_TRAPPED_WRITE_P(sym) lisp_h_SYMBOL_TRAPPED_WRITE_P (sym)
# define SYMBOL_VAL(sym) lisp_h_SYMBOL_VAL (sym)
# ifndef GC_CHECK_CONS_LIST
#  define check_cons_list() lisp_h_check_cons_list ()
# endif
# if USE_LSB_TAG
# endif
#endif


/* Define the fundamental Lisp data structures.  */

/* This is the set of Lisp data types.  If you want to define a new
   data type, read the comments after Lisp_Fwd_Type definition
   below.  */

/* Lisp integers use 2 tags, to give them one extra bit, thus
   extending their range from, e.g., -2^28..2^28-1 to -2^29..2^29-1.  */
#define INTMASK 0xffffffff
#define case_Lisp_Int case Lisp_Int0: case Lisp_Int1

/* Idea stolen from GDB.  Pedantic GCC complains about enum bitfields,
   MSVC doesn't support them, and xlc and Oracle Studio c99 complain
   vociferously about them.  */
#if (defined __STRICT_ANSI__ || defined _MSC_VER || defined __IBMC__ \
     || (defined __SUNPRO_C && __STDC__))
#define ENUM_BF(TYPE) unsigned int
#else
#define ENUM_BF(TYPE) enum TYPE
#endif


enum Lisp_Type
  {
   Lisp_Symbol = 0,

    /* Integer.  XFIXNUM (obj) is the integer value.  */
    Lisp_Int0 = 2,
    Lisp_Int1 = USE_LSB_TAG ? 6 : 3,

    /* String.  XSTRING (object) points to a struct Lisp_String.
       The length of the string, and its contents, are stored therein.  */
    Lisp_String = 4,

    /* Vector of Lisp objects, or something resembling it.
       XVECTOR (object) points to a struct Lisp_Vector, which contains
       the size and contents.  The size field also contains the type
       information, if it's not a real vector object.  */
    Lisp_Vectorlike = 5,

    /* Cons.  XCONS (object) points to a struct Lisp_Cons.  */
    Lisp_Cons = USE_LSB_TAG ? 3 : 6,

   Lisp_Float = 7,
   Lisp_JSValue = 8,
  };

/* These are the types of forwarding objects used in the value slot
   of symbols for special built-in variables whose value is stored in
   C variables.  */
enum Lisp_Fwd_Type
  {
    Lisp_Fwd_Int,		/* Fwd to a C `int' variable.  */
    Lisp_Fwd_Bool,		/* Fwd to a C boolean var.  */
    Lisp_Fwd_Obj,		/* Fwd to a C Lisp_Object variable.  */
    Lisp_Fwd_Buffer_Obj,	/* Fwd to a Lisp_Object field of buffers.  */
    Lisp_Fwd_Kboard_Obj		/* Fwd to a Lisp_Object field of kboards.  */
  };

/* If you want to define a new Lisp data type, here are some
   instructions.  See the thread at
   https://lists.gnu.org/archive/html/emacs-devel/2012-10/msg00561.html
   for more info.

   First, there are already a couple of Lisp types that can be used if
   your new type does not need to be exposed to Lisp programs nor
   displayed to users.  These are Lisp_Save_Value, a Lisp_Misc
   subtype; and PVEC_OTHER, a kind of vectorlike object.  The former
   is suitable for temporarily stashing away pointers and integers in
   a Lisp object.  The latter is useful for vector-like Lisp objects
   that need to be used as part of other objects, but which are never
   shown to users or Lisp code (search for PVEC_OTHER in xterm.c for
   an example).

   These two types don't look pretty when printed, so they are
   unsuitable for Lisp objects that can be exposed to users.

   To define a new data type, add one more Lisp_Misc subtype or one
   more pseudovector subtype.  Pseudovectors are more suitable for
   objects with several slots that need to support fast random access,
   while Lisp_Misc types are for everything else.  A pseudovector object
   provides one or more slots for Lisp objects, followed by struct
   members that are accessible only from C.  A Lisp_Misc object is a
   wrapper for a C struct that can contain anything you like.

   Explicit freeing is discouraged for Lisp objects in general.  But if
   you really need to exploit this, use Lisp_Misc (check free_misc in
   alloc.c to see why).  There is no way to free a vectorlike object.

   To add a new pseudovector type, extend the pvec_type enumeration;
   to add a new Lisp_Misc, extend the Lisp_Misc_Type enumeration.

   For a Lisp_Misc, you will also need to add your entry to union
   Lisp_Misc, but make sure the first word has the same structure as
   the others, starting with a 16-bit member of the Lisp_Misc_Type
   enumeration and a 1-bit GC markbit.  Also make sure the overall
   size of the union is not increased by your addition.  The latter
   requirement is to keep Lisp_Misc objects small enough, so they
   are handled faster: since all Lisp_Misc types use the same space,
   enlarging any of them will affect all the rest.  If you really
   need a larger object, it is best to use Lisp_Vectorlike instead.

   For a new pseudovector, it's highly desirable to limit the size
   of your data type by VBLOCK_BYTES_MAX bytes (defined in alloc.c).
   Otherwise you will need to change sweep_vectors (also in alloc.c).

   Then you will need to add switch branches in print.c (in
   print_object, to print your object, and possibly also in
   print_preprocess) and to alloc.c, to mark your object (in
   mark_object) and to free it (in gc_sweep).  The latter is also the
   right place to call any code specific to your data type that needs
   to run when the object is recycled -- e.g., free any additional
   resources allocated for it that are not Lisp objects.  You can even
   make a pointer to the function that frees the resources a slot in
   your object -- this way, the same object could be used to represent
   several disparate C structures.  */

#if defined (USE_TOOLKIT_SCROLL_BARS) && !defined (USE_GTK)
  /* Last value of whole for horizontal scrollbars.  */
  int whole;
#endif

#elif defined(JSLISP_HH_SECTION_1D)
#undef JSLISP_HH_SECTION_1D

#include "jslisp-objects.hh"

#elif defined (JSLISP_HH_SECTION_2)
#undef JSLISP_HH_SECTION_2
#include "jslisp-vectors.hh"
#elif defined (JSLISP_HH_SECTION_3)
#undef JSLISP_HH_SECTION_3
/* Forward declarations.  */

/* Defined in this file.  */
INLINE void set_sub_char_table_contents (ELisp_Handle, ptrdiff_t,
                                              ELisp_Handle);

/* Defined in chartab.c.  */
extern ELisp_Return_Value char_table_ref (ELisp_Handle, int);
extern void char_table_set (ELisp_Handle, int, ELisp_Handle);

#ifdef CANNOT_DUMP
enum { smight_dump = false };
#elif defined DOUG_LEA_MALLOC
/* Defined in emacs.c.  */
extern bool might_dump;
#endif
/* True means Emacs has already been initialized.
   Used during startup to detect startup of dumped Emacs.  */
extern bool initialized;

/* Defined in floatfns.c.  */
extern double extract_float (ELisp_Handle);


#elif defined (JSLISP_HH_SECTION_4)
#undef JSLISP_HH_SECTION_4
/* Interned state of a symbol.  */

enum symbol_interned
{
  SYMBOL_UNINTERNED = 0,
  SYMBOL_INTERNED = 1,
  SYMBOL_INTERNED_IN_INITIAL_OBARRAY = 2
};

enum symbol_redirect
{
  SYMBOL_PLAINVAL  = 4,
  SYMBOL_VARALIAS  = 1,
  SYMBOL_LOCALIZED = 2,
  SYMBOL_FORWARDED = 3
};

enum symbol_trapped_write
{
  SYMBOL_UNTRAPPED_WRITE = 0,
  SYMBOL_NOWRITE = 1,
  SYMBOL_TRAPPED_WRITE = 2
};

union Lisp_Symbol_Flags
{
  struct
  {
    /* Indicates where the value can be found:
       0 : it's a plain var, the value is in the `value' field.
       1 : it's a varalias, the value is really in the `alias' symbol.
       2 : it's a localized var, the value is in the `blv' object.
       3 : it's a forwarding variable, the value is in `forward'.  */
    ENUM_BF (symbol_redirect) redirect : 3;

    /* 0 : normal case, just set the value
       1 : constant, cannot set, e.g. nil, t, :keywords.
       2 : trap the write, call watcher functions.  */
    ENUM_BF (symbol_trapped_write) trapped_write : 2;

    /* Interned state of the symbol.  This is an enumerator from
       enum symbol_interned.  */
    unsigned interned : 2;

    /* True means that this variable has been explicitly declared
       special (with `defvar' etc), and shouldn't be lexically bound.  */
    bool_bf declared_special : 1;

    /* True if pointed to from purespace and hence can't be GC'd.  */
    bool_bf pinned : 1;
  } s;
  int32_t i;
};

/* Declare a Lisp-callable function.  The MAXARGS parameter has the same
   meaning as in the DEFUN macro, and is used to construct a prototype.  */
/* We can use the same trick as in the DEFUN macro to generate the
   appropriate prototype.  */
#define EXFUN(fnname, maxargs) \
  extern ELisp_Return_Value fnname DEFUN_ARGS_ ## maxargs

/* Note that the weird token-substitution semantics of ANSI C makes
   this work for MANY and UNEVALLED.  */
#define DEFUN_ARGS_MANY		(ELisp_Vector_Handle)
#define DEFUN_ARGS_UNEVALLED	(ELisp_Handle)
#define DEFUN_ARGS_0	(void)
#define DEFUN_ARGS_1	(ELisp_Handle)
#define DEFUN_ARGS_2	(ELisp_Handle, ELisp_Handle)
#define DEFUN_ARGS_3	(ELisp_Handle, ELisp_Handle, ELisp_Handle)
#define DEFUN_ARGS_4	(ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle)
#define DEFUN_ARGS_5	(ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle, \
                         ELisp_Handle)
#define DEFUN_ARGS_6	(ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle, \
                         ELisp_Handle, ELisp_Handle)
#define DEFUN_ARGS_7	(ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle, \
                         ELisp_Handle, ELisp_Handle, ELisp_Handle)
#define DEFUN_ARGS_8	(ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle, \
                         ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle)

/* Yield a signed integer that contains TAG along with PTR.

   Sign-extend pointers when USE_LSB_TAG (this simplifies emacs-module.c),
   and zero-extend otherwise (that’s a bit faster here).
   Sign extension matters only when EMACS_INT is wider than a pointer.  */
#define TAG_PTR(tag, ptr) \
  (USE_LSB_TAG \
   ? (intptr_t) (ptr) + (tag) \
   : (EMACS_INT) (((EMACS_UINT) (tag) << VALBITS) + (uintptr_t) (ptr)))

/* Declare extern constants for Lisp symbols.  These can be helpful
   when using a debugger like GDB, on older platforms where the debug
   format does not represent C macros.  */
#define DEFINE_LISP_SYMBOL(name) \
  DEFINE_GDB_SYMBOL_BEGIN (ELisp_Struct_Value, name) \
  DEFINE_GDB_SYMBOL_END (LISPSYM_INITIALLY (name))

/* The index of the C-defined Lisp symbol SYM.
   This can be used in a static initializer.  */
#define SYMBOL_INDEX(sym) i##sym

/* Yield an integer that contains a symbol tag along with OFFSET.
   OFFSET should be the offset in bytes from 'lispsym' to the symbol.  */
#define TAG_SYMOFFSET(offset) TAG_PTR (Lisp_Symbol, offset)

/* XLI_BUILTIN_LISPSYM (iQwhatever) is equivalent to
   XLI (builtin_lisp_symbol (Qwhatever)),
   except the former expands to an integer constant expression.  */
#define XLI_BUILTIN_LISPSYM(iname) TAG_SYMOFFSET ((iname) * sizeof *lispsym)

/* By default, define macros for Qt, etc., as this leads to a bit
   better performance in the core Emacs interpreter.  A plugin can
   define DEFINE_NON_NIL_Q_SYMBOL_MACROS to be false, to be portable to
   other Emacs instances that assign different values to Qt, etc.  */
#ifndef DEFINE_NON_NIL_Q_SYMBOL_MACROS
# define DEFINE_NON_NIL_Q_SYMBOL_MACROS true
#endif

INLINE enum Lisp_Type
XTYPE (ELisp_Handle a)
{
  return a.xtype();
}

INLINE bool
SYMBOLP (ELisp_Handle x)
{
  return x.symbolp();
}

extern ELisp_Return_Value elisp_symbol_value(ELisp_Handle a);
extern void elisp_symbol_set_value(ELisp_Handle a, ELisp_Handle b);

extern ELisp_Return_Value elisp_symbol_function(ELisp_Handle a);
extern void elisp_symbol_set_function(ELisp_Handle a, ELisp_Handle b);

extern ELisp_Return_Value elisp_symbol_plist(ELisp_Handle a);
extern void elisp_symbol_set_plist(ELisp_Handle a, ELisp_Handle b);

extern ELisp_Return_Value elisp_symbol_name(ELisp_Handle a);
extern void elisp_symbol_set_name(ELisp_Handle a, ELisp_Handle b);

extern ELisp_Return_Value elisp_symbol_next(ELisp_Handle a);
extern void elisp_symbol_set_next(ELisp_Handle a, ELisp_Handle b);

INLINE ELisp_Return_Value
XSYMBOL_PLIST (ELisp_Handle a)
{
  return elisp_symbol_plist (a);
}

INLINE void
XSYMBOL_PLIST_SET (ELisp_Handle a, ELisp_Handle b)
{
  elisp_symbol_set_plist(a, b);
}

INLINE ELisp_Return_Value
XSYMBOL_NAME (ELisp_Handle a)
{
  return elisp_symbol_name (a);
}

INLINE void
XSYMBOL_NAME_SET (ELisp_Handle a, ELisp_Handle b)
{
  elisp_symbol_set_name(a, b);
}

INLINE ELisp_Return_Value
XSYMBOL_NEXT (ELisp_Handle a)
{
  return elisp_symbol_next (a);
}

INLINE void
XSYMBOL_NEXT_SET (ELisp_Handle a, ELisp_Handle b)
{
  elisp_symbol_set_next(a, b);
}

INLINE void
CHECK_SYMBOL (ELisp_Handle x)
{
  lisp_h_CHECK_SYMBOL (x);
}

/* In the size word of a vector, this bit means the vector has been marked.  */

DEFINE_GDB_SYMBOL_BEGIN (ptrdiff_t, ARRAY_MARK_FLAG)
# define ARRAY_MARK_FLAG PTRDIFF_MIN
DEFINE_GDB_SYMBOL_END (ARRAY_MARK_FLAG)

#define XSETMARKER(a, b) ((a).xsetvector ((struct Lisp_Vector *)(b)))
#define XSETOVERLAY(a, b) ((a).xsetvector ((struct Lisp_Vector *)(b)))
#define XSETSCROLL_BAR(a,b) (a).xsetvector((struct Lisp_Vector *)b)

/* The cast to struct vectorlike_header * avoids aliasing issues.  */
#define XSETPSEUDOVECTOR(a, b, code) \
  XSETTYPED_PSEUDOVECTOR (a, b,					\
                          (((struct vectorlike_header *)	\
                            (a).xvector())                      \
                           ->size),				\
                          code)
#define XSETTYPED_PSEUDOVECTOR(a, b, size, code)			\
  (XSETVECTOR (a, b),							\
   eassert ((size & (PSEUDOVECTOR_FLAG | PVEC_TYPE_MASK))		\
            == (PSEUDOVECTOR_FLAG | (code << PSEUDOVECTOR_AREA_BITS))))

#define XSETWINDOW_CONFIGURATION(a, b) \
  (XSETPSEUDOVECTOR (a, b, PVEC_WINDOW_CONFIGURATION))
#define XSETPROCESS(a, b) (XSETPSEUDOVECTOR (a, b, PVEC_PROCESS))
#define XSETWINDOW(a, b) (XSETPSEUDOVECTOR (a, b, PVEC_WINDOW))
#define XSETTERMINAL(a, b) (XSETPSEUDOVECTOR (a, b, PVEC_TERMINAL))
#define XSETSUBR(a, b) (XSETPSEUDOVECTOR (a, b, PVEC_SUBR))
#define XSETCOMPILED(a, b) (XSETPSEUDOVECTOR (a, b, PVEC_COMPILED))
#define XSETBUFFER(a, b) (XSETPSEUDOVECTOR (a, b, PVEC_BUFFER))
#define XSETCHAR_TABLE(a, b) (XSETPSEUDOVECTOR (a, b, PVEC_CHAR_TABLE))
#define XSETBOOL_VECTOR(a, b) (XSETPSEUDOVECTOR (a, b, PVEC_BOOL_VECTOR))
#define XSETSUB_CHAR_TABLE(a, b) (XSETPSEUDOVECTOR (a, b, PVEC_SUB_CHAR_TABLE))
#define XSETTHREAD(a, b) (XSETPSEUDOVECTOR (a, b, PVEC_THREAD))
#define XSETMUTEX(a, b) (XSETPSEUDOVECTOR (a, b, PVEC_MUTEX))
#define XSETCONDVAR(a, b) (XSETPSEUDOVECTOR (a, b, PVEC_CONDVAR))
#define XSETBIGNUM(a, b) (XSETPSEUDOVECTOR (a, b, PVEC_BIGNUM))

/* Compute A OP B, using the unsigned comparison operator OP.  A and B
   should be integer expressions.  This is not the same as
   mathematical comparison; for example, UNSIGNED_CMP (0, <, -1)
   returns true.  For efficiency, prefer plain unsigned comparison if A
   and B's sizes both fit (after integer promotion).  */
#define UNSIGNED_CMP(a, op, b)						\
  (c_max (sizeof ((a) + 0), sizeof ((b) + 0)) <= sizeof (unsigned)	\
   ? ((a) + (unsigned) 0) op ((b) + (unsigned) 0)			\
   : ((a) + (uintmax_t) 0) op ((b) + (uintmax_t) 0))

/* True iff C is an ASCII character.  */
#define ASCII_CHAR_P(c) UNSIGNED_CMP (c, <, 0x80)

extern const int chartab_size[4];

INLINE ELisp_Return_Value
CHAR_TABLE_REF_ASCII (ELisp_Handle ct, ptrdiff_t idx)
{
  struct Lisp_Char_Table *tbl = NULL;
  ELisp_Value val;
  do
    {
      tbl = tbl ? XCHAR_TABLE (LSH (tbl->parent)) : XCHAR_TABLE (ct);
      val = (! SUB_CHAR_TABLE_P (LSH (tbl->ascii)) ? tbl->ascii
             : XSUB_CHAR_TABLE (LSH (tbl->ascii))->contents[idx]);
      if (NILP (val))
        val = tbl->defalt;
    }
  while (NILP (val) && ! NILP (LSH (tbl->parent)));

  return val;
}

/* Almost equivalent to Faref (CT, IDX) with optimization for ASCII
   characters.  Do not check validity of CT.  */
INLINE ELisp_Return_Value
CHAR_TABLE_REF (ELisp_Handle ct, int idx)
{
  return (ASCII_CHAR_P (idx)
          ? CHAR_TABLE_REF_ASCII (ct, idx)
          : char_table_ref (ct, idx));
}

/* Equivalent to Faset (CT, IDX, VAL) with optimization for ASCII and
   8-bit European characters.  Do not check validity of CT.  */
INLINE void
CHAR_TABLE_SET (ELisp_Handle ct, int idx, ELisp_Handle val)
{
  if (ASCII_CHAR_P (idx) && SUB_CHAR_TABLE_P (LSH (XCHAR_TABLE (ct)->ascii)))
    set_sub_char_table_contents (LSH (XCHAR_TABLE (ct)->ascii), idx, val);
  else
    char_table_set (ct, idx, val);
}

/* This structure describes a built-in function.
   It is generated by the DEFUN macro only.
   defsubr makes it into a Lisp object.  */

union subr_function {
  ELisp_Return_Value (*a0) (void);
  ELisp_Return_Value (*a1) (ELisp_Handle);
  ELisp_Return_Value (*a2) (ELisp_Handle, ELisp_Handle);
  ELisp_Return_Value (*a3) (ELisp_Handle, ELisp_Handle, ELisp_Handle);
  ELisp_Return_Value (*a4) (ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle);
  ELisp_Return_Value (*a5) (ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle);
  ELisp_Return_Value (*a6) (ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle);
  ELisp_Return_Value (*a7) (ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle);
  ELisp_Return_Value (*a8) (ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle, ELisp_Handle);
  ELisp_Return_Value (*aUNEVALLED) (ELisp_Handle args);
  ELisp_Return_Value (*aMANY) (ELisp_Vector);
};
struct Lisp_Subr
  {
    struct vectorlike_header header;
    union subr_function function;
    short min_args; short max_args;
    const char *symbol_name;
    const char *intspec;
    EMACS_INT doc;
  };

/* Save and restore the instruction and environment pointers,
   without affecting the signal mask.  */


#include "thread.h.hh"

#include "jslisp-symbol-accessors.hh"

/* Placeholder for make-docfile to process.  The actual symbol
   definition is done by lread.c's defsym.  */
#define DEFSYM(sym, name) /* empty */

INLINE ELisp_Return_Value
make_mint_ptr (void *a)
{
  return make_misc_ptr ((void *)(intptr_t) a);
}

INLINE bool
mint_ptrp (ELisp_Handle x)
{
  return FIXNUMP (x) || (PSEUDOVECTORP (x, PVEC_MISC_PTR));
}

INLINE void *
xmint_pointer (ELisp_Handle a)
{
  return ((struct Lisp_Misc_Ptr *)XVECTOR (a))->pointer;
}


INLINE bool
(FLOATP) (ELisp_Handle x)
{
  return lisp_h_FLOATP (x);
}

INLINE double
XFLOAT (ELisp_Handle a)
{
  return a.xfloat();
}

INLINE double
XFLOAT_DATA (ELisp_Handle f)
{
  return XFLOAT (f);
}

/* Most hosts nowadays use IEEE floating point, so they use IEC 60559
   representations, have infinities and NaNs, and do not trap on
   exceptions.  Define IEEE_FLOATING_POINT if this host is one of the
   typical ones.  The C11 macro __STDC_IEC_559__ is close to what is
   wanted here, but is not quite right because Emacs does not require
   all the features of C11 Annex F (and does not require C11 at all,
   for that matter).  */
enum
  {
    IEEE_FLOATING_POINT
      = (FLT_RADIX == 2 && FLT_MANT_DIG == 24
         && FLT_MIN_EXP == -125 && FLT_MAX_EXP == 128)
  };

/* Data type checking.  */

INLINE bool
NUMBERP (ELisp_Handle x)
{
  return INTEGERP (x) || FLOATP (x);
}
INLINE bool
FIXED_OR_FLOATP (ELisp_Handle x)
{
  return FIXNUMP (x) || FLOATP (x);
}
INLINE bool
FIXNATP (ELisp_Handle x)
{
  return FIXNUMP (x) && 0 <= XFIXNUM (x);
}

INLINE bool
NATNUMP (ELisp_Handle x)
{
  return FIXNUMP (x) && 0 <= XFIXNUM (x);
}

INLINE bool
RANGED_FIXNUMP (intmax_t lo, ELisp_Handle x, intmax_t hi)
{
  return FIXNUMP (x) && lo <= XFIXNUM (x) && XFIXNUM (x) <= hi;
}

#define TYPE_RANGED_FIXNUMP(type, x) \
  (FIXNUMP (LRH (ELisp_Return_Value (x)))                              \
   && (TYPE_SIGNED (type) ? TYPE_MINIMUM (type) <= XFIXNUM (LRH (ELisp_Return_Value (x))) : 0 <= XFIXNUM (LRH (ELisp_Return_Value (x)))) \
   && XFIXNUM (LRH (ELisp_Return_Value (x))) <= TYPE_MAXIMUM (type))

INLINE bool
AUTOLOADP (ELisp_Handle x)
{
  return CONSP (x) && EQ (LSH (Qautoload), LRH (XCAR (x)));
}


/* Test for specific pseudovector types.  */

INLINE bool
WINDOW_CONFIGURATIONP (ELisp_Handle a)
{
  return PSEUDOVECTORP (a, PVEC_WINDOW_CONFIGURATION);
}

INLINE bool
COMPILEDP (ELisp_Handle a)
{
  return PSEUDOVECTORP (a, PVEC_COMPILED);
}

INLINE bool
FRAMEP (ELisp_Handle a)
{
  return PSEUDOVECTORP (a, PVEC_FRAME);
}

INLINE bool
RECORDP (ELisp_Handle a)
{
  return PSEUDOVECTORP (a, PVEC_RECORD);
}

INLINE void
CHECK_RECORD (ELisp_Handle x)
{
  CHECK_TYPE (RECORDP (x), LSH (Qrecordp), x);
}

INLINE void
CHECK_NUMBER (ELisp_Handle x)
{
  lisp_h_CHECK_NUMBER (x);
}

INLINE void
(CHECK_FIXNUM_OR_FLOAT) (ELisp_Handle x)
{
  CHECK_TYPE (FIXED_OR_FLOATP (x), LSH (Qnumberp), x);
}

INLINE void
(CHECK_FIXNAT) (ELisp_Handle x)
{
  CHECK_TYPE (FIXNATP (x), LSH (Qwholenump), x);
}

INLINE void
CHECK_STRING_CAR (ELisp_Handle x)
{
  CHECK_TYPE (STRINGP (LRH (XCAR (x))), LSH (Qstringp), LRH (XCAR (x)));
}
/* This is a bit special because we always need size afterwards.  */
INLINE ptrdiff_t
CHECK_VECTOR_OR_STRING (ELisp_Handle x)
{
  if (VECTORP (x))
    return ASIZE (x);
  if (STRINGP (x))
    return SCHARS (x);
  wrong_type_argument (LSH (Qarrayp), x);
}
INLINE void
CHECK_ARRAY (ELisp_Handle x, ELisp_Handle predicate)
{
  CHECK_TYPE (ARRAYP (x), predicate, x);
}
INLINE void
CHECK_NATNUM (ELisp_Handle x)
{
  CHECK_TYPE (NATNUMP (x), LSH (Qwholenump), x);
}

#define CHECK_TYPE_RANGED_INTEGER(type, x) \
  do {									\
    if (TYPE_SIGNED (type))						\
      CHECK_RANGED_INTEGER (x, TYPE_MINIMUM (type), TYPE_MAXIMUM (type)); \
    else								\
      CHECK_RANGED_INTEGER (x, 0, TYPE_MAXIMUM (type));			\
  } while (false)

#define CHECK_FIXNUM_COERCE_MARKER(x)					\
  do {									\
    if (MARKERP ((x)))							\
      XSETFASTINT (x, marker_position (x));				\
    else								\
      CHECK_TYPE (FIXNUMP (x), LSH (Qinteger_or_marker_p), x);		\
  } while (false)

INLINE double
XFLOATINT (ELisp_Handle n)
{
  return FLOATP (n) ? XFLOAT_DATA (n) : XFIXNUM (n);
}


/* Define a built-in function for calling from Lisp.
 `lname' should be the name to give the function in Lisp,
    as a null-terminated C string.
 `fnname' should be the name of the function in C.
    By convention, it starts with F.
 `sname' should be the name for the C constant structure
    that records information on this function for internal use.
    By convention, it should be the same as `fnname' but with S instead of F.
    It's too bad that C macros can't compute this from `fnname'.
 `minargs' should be a number, the minimum number of arguments allowed.
 `maxargs' should be a number, the maximum number of arguments allowed,
    or else MANY or UNEVALLED.
    MANY means pass a vector of evaluated arguments,
         in the form of an integer number-of-arguments
         followed by the address of a vector of Lisp_Objects
         which contains the argument values.
    UNEVALLED means pass the list of unevaluated arguments
 `intspec' says how interactive arguments are to be fetched.
    If the string starts with a `(', `intspec' is evaluated and the resulting
    list is the list of arguments.
    If it's a string that doesn't start with `(', the value should follow
    the one of the doc string for `interactive'.
    A null string means call interactively with no arguments.
 `doc' is documentation for the user.  */

/* This version of DEFUN declares a function prototype with the right
   arguments, so we can catch errors with maxargs at compile-time.  */
#ifdef _MSC_VER
#define DEFUN(lname, fnname, sname, minargs, maxargs, intspec, doc)	\
   ELisp_Return_Value fnname DEFUN_ARGS_ ## maxargs ;				\
   struct Lisp_Subr alignas (GCALIGNMENT) sname =		\
   { { (PVEC_SUBR << PSEUDOVECTOR_AREA_BITS)				\
       | (sizeof (struct Lisp_Subr) / sizeof (EMACS_INT)) },		\
      { (ELisp_Return_Value (__cdecl *)(void))fnname },                        \
       minargs, maxargs, lname, intspec, 0};				\
   ELisp_Return_Value fnname
#else  /* not _MSC_VER */
#define DEFUN(lname, fnname, sname, minargs, maxargs, intspec, doc)	\
   struct Lisp_Subr alignas (GCALIGNMENT) sname =		\
     { { {}, PVEC_SUBR << PSEUDOVECTOR_AREA_BITS },                    \
       { .a ## maxargs = fnname },					\
       minargs, maxargs, lname, intspec, 0};				\
   ELisp_Return_Value fnname
#endif

/* defsubr (Sname);
   is how we define the symbol for function `name' at start-up time.  */
extern void defsubr (struct Lisp_Subr *);

enum maxargs
  {
    MANY = -2,
    UNEVALLED = -1
  };

#define LV(n,v) ((struct ELisp_Vector){ (n), (v) })
#define LV0 LV(0, (JSReturnValue *)0)

/* Call a function F that accepts many args, passing it ARRAY's elements.  */
#define CALLMANY(f, array) (f) (LV (ARRAYELTS (array), array))

template<typename... As>
ELisp_Return_Value
CALLN(ELisp_Return_Value (*f) (ELisp_Vector_Handle), As... args)
{
  ELisp_Value arr[] = { ELisp_Return_Value (args)... };
  ELisp_Dynvector d;
  d.resize(ARRAYELTS (arr));
  for (size_t i = 0; i < ARRAYELTS(arr); i++)
    d.sref(i, arr[i]);
  ELisp_Vector v = LV (ARRAYELTS (arr), d);
  auto ret = f (v);

  return ret;
}


extern ELisp_Heap_Value Vascii_downcase_table;
extern ELisp_Heap_Value Vascii_canon_table;
struct window;
struct frame;

INLINE_HEADER_END

#endif /* JSLISP_HH_SECTION_N */

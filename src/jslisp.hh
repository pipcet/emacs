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

#if PTRDIFF_MAX == INT_MAX
# define pD ""
#elif PTRDIFF_MAX == LONG_MAX
# define pD "l"
#elif PTRDIFF_MAX == LLONG_MAX
# define pD "ll"
#else
# define pD "t"
#endif


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

#define VAL_MAX (EMACS_INT_MAX >> (GCTYPEBITS - 1))

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


#define INTMASK 0xffffffff
#define case_Lisp_Int case Lisp_Int0: case Lisp_Int1

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
#elif defined (JSLISP_HH_SECTION_4)
#undef JSLISP_HH_SECTION_4
/* Interned state of a symbol.  */

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

#include "thread.h.hh"

#include "jslisp-symbol-accessors.hh"

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

INLINE_HEADER_END

#endif /* JSLISP_HH_SECTION_N */

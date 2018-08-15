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

#define EXFUN(fnname, maxargs) \
  extern ELisp_Return_Value fnname DEFUN_ARGS_ ## maxargs

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

#define TAG_PTR(tag, ptr) \
  (USE_LSB_TAG \
   ? (intptr_t) (ptr) + (tag) \
   : (EMACS_INT) (((EMACS_UINT) (tag) << VALBITS) + (uintptr_t) (ptr)))

#define DEFINE_LISP_SYMBOL(name) \
  DEFINE_GDB_SYMBOL_BEGIN (ELisp_Struct_Value, name) \
  DEFINE_GDB_SYMBOL_END (LISPSYM_INITIALLY (name))

#define SYMBOL_INDEX(sym) i##sym

#define TAG_SYMOFFSET(offset) TAG_PTR (Lisp_Symbol, offset)

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

extern const int chartab_size[4];

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

/***********************************************************************
                               Symbols
 ***********************************************************************/

INLINE ELisp_Return_Value
SYMBOL_ALIAS (ELisp_Handle sym)
{
  return elisp_symbol_value (sym);
}

extern enum symbol_redirect
elisp_symbol_redirect (ELisp_Handle);

INLINE enum symbol_redirect
SYMBOL_REDIRECT (ELisp_Handle sym)
{
  return elisp_symbol_redirect (sym);
}

extern struct Lisp_Buffer_Local_Value *
elisp_symbol_blv (ELisp_Handle sym);

INLINE struct Lisp_Buffer_Local_Value *
SYMBOL_BLV (ELisp_Handle sym)
{
  return elisp_symbol_blv (sym);
}

extern union Lisp_Fwd *
elisp_symbol_fwd (ELisp_Handle sym);

INLINE union Lisp_Fwd *
SYMBOL_FWD (ELisp_Handle sym)
{
  return elisp_symbol_fwd (sym);
}

extern void
elisp_symbol_set_redirect(ELisp_Handle, enum symbol_redirect);

INLINE void
SET_SYMBOL_REDIRECT (ELisp_Handle sym, enum symbol_redirect x)
{
  elisp_symbol_set_redirect(sym, x);
}

extern void
elisp_symbol_set_alias (ELisp_Handle, ELisp_Handle);

INLINE void
SET_SYMBOL_ALIAS (ELisp_Handle sym, ELisp_Handle v)
{
  elisp_symbol_set_alias (sym, v);
}

extern void
elisp_symbol_set_blv (ELisp_Handle, struct Lisp_Buffer_Local_Value *);

INLINE void
SET_SYMBOL_BLV (ELisp_Handle sym, struct Lisp_Buffer_Local_Value *v)
{
  elisp_symbol_set_blv (sym, v);
}
extern void
elisp_symbol_set_fwd (ELisp_Handle, union Lisp_Fwd *);

INLINE void
SET_SYMBOL_FWD (ELisp_Handle sym, union Lisp_Fwd *v)
{
  elisp_symbol_set_fwd (sym, v);
}

extern void
elisp_symbol_set_interned(ELisp_Handle, unsigned);

INLINE void
SET_SYMBOL_INTERNED (ELisp_Handle sym, unsigned interned)
{
  elisp_symbol_set_interned(sym, interned);
}

extern void
elisp_symbol_set_trapped_write(ELisp_Handle sym, enum symbol_trapped_write);

INLINE void
SET_SYMBOL_TRAPPED_WRITE (ELisp_Handle sym, enum symbol_trapped_write trapped_write)
{
  elisp_symbol_set_trapped_write(sym, trapped_write);
}

extern bool elisp_symbol_declared_special_p(ELisp_Handle);

INLINE bool
SYMBOL_DECLARED_SPECIAL_P (ELisp_Handle sym)
{
  return elisp_symbol_declared_special_p (sym);
}

extern void
elisp_symbol_set_declared_special(ELisp_Handle, bool);

INLINE void
SET_SYMBOL_DECLARED_SPECIAL (ELisp_Handle sym, bool declared_special)
{
  elisp_symbol_set_declared_special(sym, declared_special);
}

extern void
elisp_symbol_set_pinned(ELisp_Handle, bool);

INLINE void
SET_SYMBOL_PINNED (ELisp_Handle sym, bool pinned)
{
  elisp_symbol_set_pinned(sym, pinned);
}

/* Value is non-zero if symbol cannot be changed through a simple set,
   i.e. it's a constant (e.g. nil, t, :keywords), or it has some
   watching functions.  */

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

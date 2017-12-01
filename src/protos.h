Lisp_Object XCAR(Lisp_Object);
Lisp_Object XCDR(Lisp_Object);
void *XFRAME(Lisp_Object);
void *XWINDOW(Lisp_Object);
void *XVECTOR(Lisp_Object);
void *XCONS(Lisp_Object);
void *XSTRING(Lisp_Object);
void *XSYMBOL(Lisp_Object);
void *XFLOAT(Lisp_Object);
double XFLOAT_DATA (Lisp_Object);
void *XSUBR(Lisp_Object);
void *XMARKER(Lisp_Object);
void *XBUFFER(Lisp_Object);
void *XFRAME(Lisp_Object);
void *XPROCESS(Lisp_Object);
void *XTHREAD(Lisp_Object);
void *XHASH(Lisp_Object);
void *XSUB_CHAR_TABLE(Lisp_Object);
void *XCHAR_TABLE(Lisp_Object);
int XINT (Lisp_Object);
int XFASTINT (Lisp_Object);
boolean NILP(Lisp_Object);
boolean CONSP(Lisp_Object);
boolean LISTP(Lisp_Object);
boolean STRINGP(Lisp_Object);
boolean SYMBOLP(Lisp_Object);
boolean CHARACTERP(Lisp_Object);
boolean INTEGERP(Lisp_Object);
boolean MARKERP(Lisp_Object);
boolean NUMBERP(Lisp_Object);
boolean NATNUMP(Lisp_Object);
boolean COMPILEDP(Lisp_Object);
boolean CHARSETP(Lisp_Object);
boolean SUBRP(Lisp_Object);
bool KEYMAPP (Lisp_Object);
boolean RANGED_INTEGERP(ptrdiff_t, Lisp_Object, ptrdiff_t);
bool CHECK_RANGED_INTEGER (Lisp_Object, ptrdiff_t, ptrdiff_t);
boolean FLOATP(Lisp_Object);
boolean VECTORP(Lisp_Object);
boolean VECTORLIKEP(Lisp_Object);
boolean CATEGORYP(Lisp_Object);
boolean EQ(Lisp_Object, Lisp_Object);
void CHECK_SYMBOL(Lisp_Object);
void CHECK_STRING(Lisp_Object);
Lisp_Object CALLN(Lisp_Object (*)(ptrdiff_t, Lisp_Object *), Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object);
// Fsetcar, Fsetcdr, CATEGORY_MEMBER, Fmarker_buffer
Lisp_Object CATEGORY_DOCSTRING (Lisp_Object, void *);
void SETTOP (Lisp_Object);
Lisp_Object OVERLAY_START (Lisp_Object);
Lisp_Object OVERLAY_END (Lisp_Object);
Lisp_Object OVERLAY_POSITION (Lisp_Object);
Lisp_Object POSN_POSN (Lisp_Object);
Lisp_Object POSN_WINDOW (Lisp_Object);
Lisp_Object LGSTRING_FONT (Lisp_Object);
Lisp_Object LGSTRING_SLOT (Lisp_Object);
Lisp_Object Fcar (Lisp_Object);
Lisp_Object Fcdr (Lisp_Object);
boolean CHECK_CATEGORY (Lisp_Object);
boolean TYPE_RANGED_INTEGERP (void *, Lisp_Object);
// AUTO_STRING, AUTO_LIST_1
// CHARSET_DEUNIFIER, CHARSET_ENCODER, CHARSET_SUPERSET, CHARSET_MIN_CODE, CHARSET_MAX_CODE, make_fixnum_or_float, CHARSET_SYMBOL_ID, CHARSET_ATTRIBUTES, BVAR?
// INTEGER_TO_CONS
ptrdiff_t COMPOSITION_LENGTH (Lisp_Object);
Lisp_Object listn (enum constype, ptrdiff_t, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object);
void CHECK_TYPE (boolean, Lisp_Object, Lisp_Object);
Lisp_Object Fsetcar (Lisp_Object, Lisp_Object);
Lisp_Object Fsetcdr (Lisp_Object, Lisp_Object);
Lisp_Object CAR_SAFE (Lisp_Object);
Lisp_Object CDR_SAFE (Lisp_Object);
bool XD_DBUS_TYPE_P (Lisp_Object);
Lisp_Object XD_OBJECT_TO_DBUS_TYPE (Lisp_Object);
Lisp_Object AUTO_CONS_EXPR (Lisp_Object, Lisp_Object);
Lisp_Object AUTO_LIST1 (void *, Lisp_Object);
Lisp_Object AUTO_LIST2 (void *, Lisp_Object, Lisp_Object);
Lisp_Object AUTO_LIST3 (void *, Lisp_Object, Lisp_Object, Lisp_Object);
Lisp_Object AUTO_LIST4 (void *, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object);
Lisp_Object AUTO_FRAME_ARG (void *, Lisp_Object, Lisp_Object);
void ADD_IMAGE_TYPE (Lisp_Object);
int COMPOSITION_LENGTH (Lisp_Object);
void DEFVAR_PER_BUFFER (const char *, Lisp_Object *, Lisp_Object, void *);
Lisp_Object Fthrow (Lisp_Object, Lisp_Object);
Lisp_Object Ffboundp (Lisp_Object);
Lisp_Object Fset (Lisp_Object, Lisp_Object);
Lisp_Object Ffset (Lisp_Object, Lisp_Object);
Lisp_Object Fpurecopy (Lisp_Object);
Lisp_Object store_symval_forwarding (void *, Lisp_Object, void *);
Lisp_Object Fset_buffer (Lisp_Object);
Lisp_Object DECODE_SYSTEM (Lisp_Object);
Lisp_Object Findirect_function (Lisp_Object, Lisp_Object);
Lisp_Object Fmake_char_table (Lisp_Object, Lisp_Object);
Lisp_Object DISP_CHAR_VECTOR (void *, void *);
Lisp_Object LOCAL_SELECTION (Lisp_Object, void *);
void CHECK_NUMBER (Lisp_Object);
Lisp_Object encode_coding_string (void *, Lisp_Object, bool);
Lisp_Object decode_coding_string (void *, void *, void *, Lisp_Object);
ptrdiff_t ASIZE (Lisp_Object);
void *LEFT_FRINGE(void *, Lisp_Object, bool);
void *RIGHT_FRINGE(void *, Lisp_Object, bool);
Lisp_Object LGSTRING_HEADER (void *, Lisp_Object);
int LGLYPH_CODE (Lisp_Object);
Lisp_Object EVENT_HEAD_KIND (Lisp_Object);
Lisp_Object EVENT_HEAD (Lisp_Object);
Lisp_Object EVENT_START (Lisp_Object);
Lisp_Object FONTSET_FALLBACK (Lisp_Object);
Lisp_Object ENCODE_FILE (Lisp_Object);
Lisp_Object ENCODE_SYSTEM (Lisp_Object);
void CHECK_CODING_SYSTEM (Lisp_Object);
bool UNSPECIFIEDP (Lisp_Object);
bool TMEM (Lisp_Object, Lisp_Object);
Lisp_Object POSN_SET_POSN (Lisp_Object, Lisp_Object);
bool parse_image_spec (Lisp_Object, void *, int, Lisp_Object);
Lisp_Object safe__call1 (bool, Lisp_Object, Lisp_Object);

void PUT_ERROR (Lisp_Object, Lisp_Object, const char *);
void AINC (Lisp_Object, int);
void XD_SIGNAL1 (Lisp_Object);
void XD_SIGNAL2 (Lisp_Object, Lisp_Object);
void XD_SIGNAL3 (Lisp_Object, Lisp_Object, Lisp_Object);
Lisp_Object CHARSET_SYMBOL_ID (Lisp_Object);
Lisp_Object FONTSET_ADD (void *, Lisp_Object, void *, void *);
void CHECK_CHARSET_GET_CHARSET (Lisp_Object, void *);
void CHECK_CHARSET_GET_ID (Lisp_Object, void *);
void CHECK_CODING_SYSTEM_GET_SPEC (Lisp_Object, void *);
void CHECK_CHARACTER (Lisp_Object);
bool IGNORE_DEFFACE_P (Lisp_Object);
bool LFACEP (Lisp_Object);
int FONT_WEIGHT_NUMERIC (Lisp_Object);
int FONT_WEIGHT_NAME_NUMERIC (Lisp_Object);
int FONT_SLANT_NAME_NUMERIC (Lisp_Object);
int CONS_TO_INTEGER (Lisp_Object);
Lisp_Object  INTEGER_TO_CONS (int);
bool EVENT_HAS_PARAMETERS (Lisp_Object);
Lisp_Object CHARSET_SYMBOL_ID (Lisp_Object);
void FONT_ADD_LOG (void *, Lisp_Object, Lisp_Object);
void FONT_DEFERRED_LOG (void *, Lisp_Object, Lisp_Object);
int LGSTRING_CHAR_LEN (Lisp_Object);
void LGSTRING_SET_ID (Lisp_Object, Lisp_Object);
void LGSTRING_SET_HEADER (Lisp_Object, Lisp_Object);
void LGSTRING_SET_GLYPH (Lisp_Object, int, Lisp_Object);
void FONT_SET_STYLE (Lisp_Object, int, Lisp_Object);
Lisp_Object CODING_ATTR_BASE_NAME (Lisp_Object);
Lisp_Object CODING_ATTR_CATEGORY (Lisp_Object);
Lisp_Object CODING_ATTR_TYPE (Lisp_Object);
Lisp_Object CODING_SYSTEM_SPEC (Lisp_Object);
Lisp_Object CODING_SYSTEM_ID (Lisp_Object);
Lisp_Object CODING_CCL_ENCODER (void *);
Lisp_Object CODING_CCL_DECODER (void *);
Lisp_Object CODING_ID_ATTRS (void *);
Lisp_Object XSETCAR (Lisp_Object, Lisp_Object);
Lisp_Object XSETCDR (Lisp_Object, Lisp_Object);
Lisp_Object DISP_CHAR_VECTOR (void *, void *);
Lisp_Object CHARSET_SYMBOL_ID (Lisp_Object);
Lisp_Object fontset_ref (Lisp_Object, int);
Lisp_Object FONT_DEF_SPEC (Lisp_Object);
Lisp_Object FONT_DEF_REPERTORY (Lisp_Object);
Lisp_Object FONTSET_DEFAULT (Lisp_Object);
Lisp_Object FONTSET_ID (Lisp_Object);
Lisp_Object ftfont_pattern_entity (int, Lisp_Object);
void OTF_SYM_TAG (Lisp_Object, int);
void LGLYPH_SET_ADJUSTMENT (void *, Lisp_Object);
void LGLYPH_SET_CODE (Lisp_Object, void *);
Lisp_Object LGSTRING_FONT (Lisp_Object);
int COMPOSITION_GLYPH (void *);
Lisp_Object FONTSET_REF (Lisp_Object, int);
Lisp_Object FONTSET_SET (Lisp_Object, Lisp_Object, Lisp_Object);
bool CODING_SYSTEM_P (Lisp_Object);
Lisp_Object CHAR_TABLE_REF (Lisp_Object, int);
void image_error (const char *fmt, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object);
Lisp_Object ENCODE_UTF_8 (Lisp_Object);

#!/usr/bin/perl
# TODO#
# a = b = c = d gets parsed as (a = b) = (c = d).
use strict;
use Marpa::R2;
use Data::Dumper;
use Time::HiRes qw(time);
#use Carp::Always;

package EmacsCGrammar;

@EmacsCGrammar::types = qw(
    void
    bool
    char
    short
    int
    long

    signed
    unsigned

    float
    double

    intptr_t
    intmax_t
    uintmax_t

    unichar

    GConfClient
    regnum_t
    id_t
    CGGlyph
    pthread_mutex_t
    HWND
    CFRange
    Lisp_Word
    CGContextRef

    Cursor

    funcptr

    widget_value

    GLYPH

    OFFSET
    md5_ctx
    u64
    FILE

    EMACS_INT
    EMACS_UINT
    INTERVAL
    KBOARD

    printmax_t
    uprintmax_t

    NativeRectangle

    bidi_bracket_type_t
    bidi_category_t
    bidi_dir_t
    bidi_type_t

    map_keymap_function_t

    jmp_buf

    emacs_env
    emacs_value
    dynlib_function_ptr_nonce
    dynlib_function_ptr
    dynlib_handle_ptr
    get_user_finalizer

    gnutls_session_t
    gnutls_rnd_t
    xg_list_node

    map_keymap_fuction_t
    WidgetList

    NSColor
    NSSize
    NSRect
    NSPoint
    NSUInteger
    CGFloat
    EmacsCGFloat

    GCallback

    xcb_query_tree_cookie_t
    xcb_query_tree_reply_t
    dbus_int64_t
    dbus_uint64_t
    xcb_translate_coordinates_cookie_t
    TIFFErrorHandler
    TIFFCloseProc

    GError

    XWMHints

    gint
    gboolean
    boolean
    StorageType
    FcCharSet

    CFIndex

    cairo_t
    cairo_status_t

    xcb_get_geometry_cookie_t
    xcb_get_property_cookie_t

    XtTranslations

    SmPropValue

    Colormap

    PropMotifWmHints

    XdbeSwapInfo

    Gpm_Event
    Gpm_Conect

    tty_menu

    GdkGCValues
    GdkGC
    XFontSet
    XIMStyle
    XIM
    XVaNestedList
    XIC
    XClassHint
    XButtonEvent
    XMotionEvent
    XtActionHookId
    XtPointer
    GtkRange
    GdkEventButton
    Dimension
    gpointer
    LWLIB_ID
    HPALETTE
    Position
    String
    XrmName
    mi_result
    Cardinal
    XrmValuePtr
    SmProp
    SmcCallbacks
    uint32
    dbus_uint32_t
    dbus_int32_t
    dbus_int16_t
    dbus_uint16_t
    ELisp_Dynvector
    size_t
    guchar
    RsvgHandle
    MagickRealType
    DisposeType
    ExceptionType
    gif_memory_source
    GifByteType
    TiffCloseProc
    TiffSizeProc
    JDIMENSION
    TiffMapFileProc
    GifFileType
    tiff_memory_source
    TIFFReadWriteProc
    j_common_ptr
    JOCTET
    png_uint_32
    png_struct
    GtkDialog
    gnutls_datum_t
    KEY_EVENT_RECORD
    GdkCursor
    cairo_matrix_t
    GtkFileChooserAction
    PangoStyle
    XWindowAttributes
    XtConvertArgRec
    sigjmp_buf
    GtkStockItem
    Status
    gnutls_alert_description_t
    gnutls_aead_cipher_hd_t
    gnutls_cipher_hd_t
    gnutls_hmac_hd_t
    gnutls_hash_hd_t
    cmsCIExyY
    thandle_t
    tdata_t
    toff_t
    JSAMPARRAY
    TIFF
    TIFFSeekProc
    TIFFSizeProc
    TIFFMapFileProc
    GifByteType
    j_decompress_ptr
    compile_stack_type
    fail_stack_type
    XMappingEvent
    GSList
    WINDOWPLACEMENT
    GtkOrientation
    GDestroyNotify
    XrmOptionDescRec
    GLogLevelFlags
    xpm_XImage
    PixelWand
    MagickBooleanType
    MagickPixelPacket
    RsvgDimensionData
    GdkPixbuf
    Pixmap
    Widget
    Mouse_HLInfo
    GC
    XGCValues
    XEDataObject
    XExtData
    XColor
    XSetWindowAttributes
    Time
    AppendMenuW_Proc
    Display
    KeySym
    XPropertyEvent
    Visual
    Atom
    XErrorEvent
    XClientMessageEvent
    XChar2b
    Screen
    gnutls_x509_crt_t
    gnutls_mac_algorithm_t
    gnutls_digest_algorithm_t
    gnutls_initstage_t
    NSMenu
    CGFontRef
    u_int32_t
    timer_t
    u_int16_t
    uint64_t
    snd_pcm_sw_params_t
    snd_pcm_uframes_t
    snd_lib_error_handler_t
    MCIERROR
    Pixel
    xmlNode
    cmsCIELab
    cmsFloat64Number
    cmsCIEXYZ
    cmsViewingConditions
    lcmsJab_t
    XrmValue
    cmsJCh
    clocid_t
    log_t
    z_streamp
    z_stream
    lisp_mutex_t
    sys_thread_t
    XPoint
    XFontStruct
    FT_Size
    FT_Matrix
    FcPattern
    FcChar32
    XGlyphInfo
    XftColor
    FT_UInt
    FcResult
    FT_Face
    OTF_GSUB_GPOS
    MFLTGlyphString
    MFLTFont
    MFLTOtfSpec
    OTF
    GPollFD
    cairo_surface_t
    cairo_surface_type_t
    cairo_format_t
    RGB_PIXEL_COLOR
    HGDIOBJ
    HBITMAP
    XpmAttributes
    png_byte
    png_bytep
    png_structp
    png_size_t
    MagickWand
    GLogeLevelFlags
    Font
    gulong
    XtAppContext
    Drawable
    lock_info_type
    XrmDatabase
    GtkMenuPositionFunc
    Arg
    voidfuncptr
    funcptr
    GtkMenu
    Window
    EmacsView
    XCharStruct
    XEvent
    XImage
    XImagePtr
    HDC
    XImagePtr_or_DC
    TEXTMETRICW
    BITMAPINFO
    PROCESS_INFORMATION
    LOGFONT
    LPCSTR
    BOOL
    HMENU
    RECT
    W32Msg
    LRESULT
    CRITICAL_SECTION
    GdkGeometry
    XMenu
    XtPointer
    GtkWidget
    GtkFixed
    GtkFixedClass
    XRectangle
    regex_t
    reg_syntax_t
    regoff_t
    bpa_stack_entry
    cairo_pattern_t
    Gpm_Connect
    XIMStyles
    SmcConn
    SmPointer
    max_align_t
    int_fast64_t

    bool_bf

    bits_word
    keyremap

    atimer_callback
    OTF_Tag
    OTF_DeviceTable
    OTF_ValueRecord
    OTF_Anchor
    socklen_t
    file_offset
    gnutls_session_t
    gnutls_initstage_t
    gnutls_certificate_client_credentials
    gnutls_anon_client_credentials_t
    gnutls_x509_crt_t
    gnutls_rnd_level_t
    gnutls_transport_ptr_t
    HCURSOR

    U8 U16 U32 U64
    S8 S16 S32 S64
    I32 F32 F64
    U32TYPE

    ELisp_Pointer
    ELisp_Return_Value
    ELisp_Value
    ELisp_Struct_Value
    ELisp_Handle
    ELisp_Handle_RW
    Lisp_Object

    SV CV AV HV GV
    MAGIC
    HE HEK

    size_t ssize_t ptrdiff_t
    FILE
    Malloc_t
    SSize_t STRLEN Time64_T
    siginfo_t
    pTHX pTHXx
    PADNAME OP IV UV NV Shmat_t IO  svtype AMT
    XPVHV XPVGV XPVCV XPVMG XPVIO XPVAV

    PTRV
    COP
    COPHH
    Size_t
    Groups_t
    Signal_t
    Sighandler_t
    Sock_size_t
    Mode_t

    fd_callback
    Display_Info

    RE_TRANSLATE_STRUCT

    COLORREF

    frame_parm_handler

    reg_errcode_t

    wctype_t
    re_wctype_t

    signal_handler_t

    pthread_cond_t
    sys_cond_t
    w32thread_critsect
    w32thread_cond_t
    thread_creation_function
    sys_mutex_t

    sys_jmp_buf
    stack_t
    random_seed

    speed_t

    XmString

    XkbDescPtr
    filedesc
    select_func

    HANDLE

    LPBYTE
    LPDWORD

    IMAGE_NT_HEADERS

    HMODULE

    WINAPI
    UINT
    CPINFO
    HINSTANCE
    PALETTEENTRY
    HWND
    CONSOLE_SCREEN_BUFFER_INFO

    IceConn
    IcePointer
    XSelectionEvent
    XWindowChanges
    XrmRepresentation
    List
    GtkTextDirection
    FcBool
    GSettings
    GtkImage
    XrmClass
    CARD16
    CARD32
    gsize
    GdkRGBA
    GtkRequisition
    gchar
    guint
    XMotionEvent
    GdkEvent
    GdkColor
    GObject
    GdkFilterReturn
    GdkXEvent
    XComposeStatus
    GType
    EmacsFixed
    GtkAllocation
    EmacsFixedClass
    XSizeHints
    XTextProperty
    guint
    GdkRectangle
    xg_menu_cb_data
    GList
    DBusError
    DBusMessageIter
    DBusConnection
    DBusWatch
    gdouble
    XVisualInfo
    GtkPrintOperation
    GtkPrintOperationResult
    xcb_get_property_cookie_t
    GdkEventCrossing
    GtkToolItem
    GtkTextDirection

    Bool
    Boolean
    CorePart
    CoreClassPart
    EmacsFramePart
    EmacsFrameClassPart

    uniprop_decoder_t
    uniprop_encoder_t

    tr_stack

    CHILD_SETUP_TYPE
    sigjmp_buf
    emacs_subr


    MSG

    WidgetClass

    fixpt_t
    dev_t
    locale_t
    wint_t
    mode_t

    security_context_t

    MEM_SIZE
    perl_mstats_t
    PerlIO
    MGS
    MGVTBL
    PADOFFSET
    va_list
    sigset_t
    uid_t
    gid_t
    pid_t
    REGEXP
    CLONE_PARAMS
    LISTOP
    UNOP
    OPCODE
    BHK
    UNOP_AUX
    PMOP
    SVOP
    BINOP
    regex_charset
    LOOP
    XSUBADDR_t
    Perl_call_checker
    Optype
    PADLIST
    OPSLAB
    OPSLOT
    PADNAMELIST
    PAD
    PerlInterpreter
    XSINIT_t
    Stat_t
    Uid_t
    Gid_t
    line_t
    off_t
    PerlIOl
    PerlIO_list_t
    PerlIO_funcs
    STDCHAR
    Off_t
    uint32_t
    key_t
    Time_t
    LOGOP
    regexp_engine
    WCHAR
    wchar_t
    Pid_t
    u_short
    gptr
    regnode_ssc
    time_t
    Year
    scan_data_t
    PERL_SI
    u_int
    YYSTYPE
    yy_stack_frame
    yy_parser
    SVCOMPARE_t
    XXXitem
    GP
    SVFUNC_t
    Netdb_net_t
    tempsym_t
    regmatch_info
    regmatch_info_aux
    regmatch_info_aux_eval
    regmatch_state
    scan_frame
    RExC_state_t
    SB_enum
    WB_enum
    regnode
    Free_t
    scan_data_t
    tempsym_t
    caddr_t
    I8
    I16
    METHOP
    PERL_BITFIELD8
    PERL_BITFIELD16
    PERL_BITFIELD32
    DESTRUCTORFUNC_t
    DESTRUCTORFUNC_NOCONTEXT_t
    u_char
    ANY
    PERL_CONTEXT
    Select_fd_set_t
    XINVLIST
    padtidy_type
    SETSOCKOPT_OPTION_VALUE_T
    Direntry_t
    DIR
    fd_set
    UNOP_AUX_item
    YYSTYPE
    Fpos_t
    XPV
    PTR_TBL_t
    DWORD
    WORD

    JSP_SV_Handle
    JSP_AV_Handle
    JSP_HV_Handle
    JSP_CV_Handle
    JSP_GV_Handle

    JSP_SV_Return
    JSP_AV_Return
    JSP_HV_Return
    JSP_CV_Return
    JSP_GV_Return

    JSP_SVP_Return
    JSP_AVP_Return
    JSP_HVP_Return
    JSP_CVP_Return
    JSP_GVP_Return

    JSP_SV
    JSP_AV
    JSP_HV
    JSP_CV
    JSP_GV

    GCB_enum
    LB_enum

    re_scream_pos_data
    re_char

    mode_t
    acl_type_t
    acl_t

    timezone_t
    sdata_union
    uintptr_t


    I8TYPE
    U8TYPE
    I16TYPE
    U16TYPE
    I32TYPE
    U32TYPE
    I64TYPE
    U64TYPE
    IVTYPE
    UVTYPE
    NVTYPE

    LPLIOAccess
    LPLIOChmod
    LPLIOChown
    LPAccept
    LPChdir
    LPRmdir
    LPMakedir
    LPMemMalloc
    LPMemRealloc
    LPProcAbort
    LPProcCrypt
    LPProcExit
    LPProc_Exit
    LPHtonl
    LPNtohl
    LPHtons
    LPNtohs
    LPStdin
    LPStderr
    LPDirClose
    LPDirRead
    LPEnvGetChildIO
    LPEnvGetenv
    LPLIOChsize
    LPMemFree
    LPProcExecl
    SOCKET
    LPBind
    u_long
    INFNAN_NV_U8_DECL
    INFNAN_U8_NV_DECL
    EXT_MGVTBL
    SUBLEXINFO
    Perl_c_backtrace_header
    Perl_c_backtrace_frame
    perl_mutex
    cthread_t
    mutex_t
    condition_t
    PPADDR_t
    ATEXIT_t
    runops_proc_t
    filter_t
    perl_drand48_t
    perl_ppaddr_t
    Perl_ppaddr_t
    Sigjmp_buf
    Sigsave_t
    Perl_check_t

    XOP
    XOPRETANY
    INT_64_T
    Int64

    JSP_Struct
    JSP_Return
    JMPENV
    CHECKPOINT

    pthread_t
    pthread_mutex_t

    PerlExitListEntry
    regmatch_slab
    REENTR
    tTHX

    xop_flags_enum
    re_intuit_string_t

    GtkTooltip
    GtkWidgetClass
    GParamSpec

    dbus_bool_t
    DBusBusType
    mbstate_t
    xg_get_file_func
    PangoWeight
    xg_menu_item_cb_data
    GtkCallback
    rlim_t
    GtkMenuItem
    GtkLabel
    next_element_function
    RROutput
    GtkPrintContext
    GdkEventExpose
    KeyCode
    xt_or_gtk_widget
    GtkIconTheme
    GtkScrollType
    XKeyEvent
    XmTextPosition
    x_special_error_handler
    XIMCallback
    XPointer
    XRegisterIMInstantiateCallback_arg6
    JSReturnValue
    re_opcode_t
    pattern_offset_t
    re_wchar_t
    RE_TRANSLATE_TYPE
    RE_TRANSLATE_HANDLE
    ELisp_Vector_Handle
    context_t
    gnutls_alert_get
    gnutls_certificate_credentials_t
    gnutls_cipher_algorithm_t
    MMRESULT
    cmsContext
    cmsHANDLE
    clockid_t
    cmsCIExyYs
    FT_Library
    LPCRITICAL_SECTION
    BDF_PropertyRec
    FcChar8
    XftDraw
    FcValue
    FT_ULong
    Bitmap_Record
    FT_Int32
    MFLTGlyph
    MFLTGlyphFT
    OTF_GlyphString
    MFLTGlyphString
    png_color_16
    InputFunc
    );
%EmacsCGrammar::types;
for my $type (@EmacsCGrammar::types) {
    $EmacsCGrammar::types{$type} = 1;
}

my @symbols = qw(TLS Directive Expr0 TypeStmt Stmts Stmt Label CaseLabel PExpr CExpr Expr String BinOp ModOp UnOp ModUnOp ModPostOp FunctionDefinition Junk JunkItem VarDefinition Typedef RetType Attrs Attr PArgExprs ArgExprs ArgExpr PArgs Args BBody Arg AttrType PointerType StructUnionType EnumType TypedSymbol FunType Typed BStructBody StructBody BEnumBody EnumBody PSymbol Symbol Number RewriteInit InitRests2 InitRests InitRest InitSuffixes InitRestPrefixes InitPrefix TypeNoStar StructUnion AttrTypeNoStar Type StringSpec);

$EmacsCGrammar::dsl = <<'EODSL';
:default ::= action => [start, length, values] bless => ::lhs

Chunk ::= Empty | TLS Chunk
TLS ::= Directive | Stmt
Directive ::= 'INLINE_HEADER_BEGIN' | 'INLINE_HEADER_END' | 'DEF_DLL_FN' '(' Type ',' Symbol ',' PArgs ')' ';'
Expr0 ::= Empty | CExpr
TypeStmt ::= Type ';'
StmtShouldHaveSemicolon ::= 'WEAK_ALIAS' '(' Symbol ',' Symbol ')'
Stmt ::= StmtShouldHaveSemicolon | 'G_BEGIN_DECLS' | 'G_END_DECLS' | 'dMARK' | 'dSP' | 'dTARGET' | 'dTOPss' | DEFINE_LISP_SYMBOL '(' Symbol ')' | 'FOR_EACH_PROCESS' '(' Expr ',' Expr ')' Stmt | 'FOR_EACH_TAIL' '(' Expr ')' Stmt | 'FOR_EACH_TAIL_SAFE' '(' Expr ')' Stmt | 'FOR_EACH_PER_BUFFER_OBJECT_AT' '(' Expr ')' Stmt | 'FOR_EACH_BUFFER' '(' Expr ')' Stmt | 'FOR_EACH_FRAME' '(' Expr ',' Expr ')' | 'FOR_EACH_LIVE_BUFFER' '(' Expr ',' Expr ')' | '...' | 'IF_LINT' '(' Typed '=' Expr ')' | 'IF_LINT' '(' Stmt ')' ';' | ';' rank => -3 | break ';' | continue ';' | goto Label ';' | goto Symbol ';' | Expr ';' rank => -1 | 'for' '(' Stmt Stmt Expr0 ')' Stmt | 'if' PExpr Stmt else Stmt | 'if' PExpr Stmt | 'while' PExpr Stmt | 'do' Stmt 'while' PExpr ';' | return ';' | return Expr ';' | Typed '=' Expr ';' rank => -2 | Typed 'IF_LINT' '(' '=' Expr ')' ';' | FunctionDefinition rank => 3 | VarDefinition rank => -2 | TypeStmt | Typedef | '{' Stmts '}' | Label Stmt | CaseLabel Stmt | 'switch' PExpr Stmt | 'swapfield' '(' Expr ',' Type ')' | 'FOR_EACH_FRAME' '(' CExpr ')' | 'DEBUG_STATEMENT' '(' Stmts ')' | 'DEBUG_STATEMENT' '(' Stmts Expr ')' | CExpr ';' rank => -2 | 'DECLARE_POINTER_ALIAS' '(' Expr ',' Type ',' Expr ')' ';'
Label ::= Symbol ':'
CaseLabel ::= 'case' Expr ':' | 'CASE' '(' Expr ')' ':' | 'CASE_DEFAULT' | 'CASE_ABORT' ':' | 'FIRST'
PExpr ::= '(' CExpr ')'
CExpr ::= Expr ',' CExpr | Expr
ExprWithType ::= 'INT_STRLEN_BOUND' '(' Type ')' | 'TYPE_MINIMUM' '(' Type ')' | 'TYPE_SIGNED' '(' Type ')' | 'INT_BUFSIZE_BOUND' '(' Type ')' | 'TYPE_RANGED_INTEGERP' '(' Type ',' Expr ')' | 'CHECK_TYPE_RANGED_INTEGER' '(' Type ',' Expr ')' | 'swapfield_' '(' Symbol ',' Type ')'
Expr ::=  ExprWithType | Expr BinOp Expr rank => 3 | '...' | 'VECSIZE' '(' Type ')' | 'FLEXSIZEOF' '(' Type ',' Expr ',' Expr ')' | 'PSEUDOVECSIZE' '(' Type ',' Expr ')' | 'ALLOCATE_PSEUDOVECTOR' '(' Type ',' Expr ',' Expr ')' | 'ALLOCATE_ZEROED_PSEUDOVECTOR' '(' Type ',' Expr ',' Expr ')' | 'REGEX_TALLOC' '(' Expr ',' Type ')' | 'UNSIGNED_CMP' '(' Expr ',' BinOp ',' Expr ')' | 'TYPE_MAXIMUM' '(' Type ')' | 'CONS_TO_INTEGER' '(' Expr ',' Type ',' Expr ')' | Symbol rank => 1 | Number | Expr PArgExprs rank => 1 | 'sizeof' Type | 'sizeof' '(' Type ')' | 'sizeof' Expr | 'alignof' '(' Type ')' | 'offsetof' '(' Type ',' Symbol ')' | 'offsetof' '(' Type ',' Expr ')' | PExpr | Expr '?' Expr ':' Expr rank => 5 | UnOp Expr rank => 2 | '(' Type ')' Expr | Expr '[' Expr ']' | String | '{' '}' | '{' CExpr '}' | '{' CExpr ',' '}' | Attr Expr | 'va_arg' '(' Expr ',' Type ')' | '[' Expr '...' Expr ']' '=' Expr | '[' Expr ']' '=' Expr | '.' Expr '=' Expr | '&&' Symbol | 'ELisp_Array_Imm' '(' Expr ',' CExpr ')' | '(' Expr ')' | Expr ModOp Expr rank => 6 | ModUnOp Expr | Expr ModPostOp
String ::= string | String String | 'FOPEN_TEXT' | 'FOPEN_BINARY' | 'PACKAGE_BUGREPORT' | 'SCNuMAX' | 'WTMP_FILE' | 'L' String | StringSpec
StringSpec ::= 'pI' | 'pD' | 'pMu' | 'pMd' | 'PRIu64' | 'PRIxPTR'
BinOp ::= '!=' | '==' | '>' | '>=' | '<' | '<=' | '||' | '&&' | '|' | '&' | '.' | '->' | '^' | '<<' | '>>' | '%' | '+' | '-' | '*' rank => -1 | '/'
ModOp ::= '=' | '+=' | '-=' | '>>=' | '<<=' | '*=' | '/=' | '&=' | '|=' | '^=' | '%='
UnOp ::= '+' | '-' | '&' | '*' | '~' | '!'
ModUnOp ::= '++' | '--'
ModPostOp ::= '++' | '--'
FunctionDefinition ::= Attrs RetType PSymbol PArgs Attrs BBody | Attrs RetType PSymbol PArgs Attrs ';' | 'DEFUN' '(' Expr ',' Symbol ',' CExpr ',' Junk ')' PArgs BBody | 'DEAFUN' '(' CExpr ',' Junk ')' PArgs BBody
Junk ::= Empty | JunkItem Junk
JunkItem ::= 'doc:' | 'attributes' ':' 'const' | 'attributes' ':' 'noreturn' | 'attributes' ':' 'noinline'
VarDefinition ::= RewriteInit | Attrs Typed ';' | Typed Attrs ';' | Attrs Typed Attrs '=' Expr ';' | 'DEFVAR_LISP' '(' CExpr ',' Junk ')'  | 'DEFVAR_LISP_NOPRO' '(' CExpr ',' Junk ')' | 'DEFVAR_BOOL' '(' CExpr ',' Junk ')' | 'DEFVAR_INT' '(' CExpr ',' Junk ')' | 'DEFVAR_KBOARD' '(' CExpr ',' Junk ')' | 'DEFVAR_BUFFER_DEFAULTS' '(' CExpr ',' Junk ')' | 'DEFVAR_PER_BUFFER' '(' CExpr ',' Junk ')'
Typedef ::= typedef Typed ';' | typedef Type Symbol ';' | typedef Type Type ';' | typedef Type '(' '*' Symbol ')' PArgs ';'

RetType ::= Type

Attrs ::= Empty | Attr Attrs

Attr ::= restrict | '__THROW' | '_Restrict_' | '__restrict' | extern | 'inline' | 'INLINE' | 'NO_INLINE' | '_Noreturn' | static | 'ATTRIBUTE_UNUSED' | 'const' | 'auto' | register | 'ATTRIBUTE_CONST' | 'ATTRIBUTE_UNUSED' | 'EXTERNALLY_VISIBLE' | alignas Expr | const | signed | unsigned | short | long | volatile | auto | 'asm' PExpr | '__cdecl' | '_cdecl' | 'UNINIT' | 'ATTRIBUTE_NO_SANITIZE_ADDRESS' | '__MALLOC_HOOK_VOLATILE' | 'weak_function' | 'CACHEABLE' | 'ALIGN_STACK' | 'CALLBACK' | 'WINAPI' | 'ATTRIBUTE_MALLOC' | 'GCALIGNED' | 'WINDOW_SYSTEM_RETURN' | macro PArgExprs rank => -3 | 'ATTRIBUTE_MAY_ALIAS' | '__attribute__' '(' '(' ArgExprs ')' ')' | 'EMACS_NOEXCEPT'

PArgExprs ::= '(' ArgExprs ')'
ArgExprs ::= Empty | ArgExpr | ArgExpr ',' ArgExprs
ArgExpr ::= Expr

PArgs ::= '(' Args ')'
Args ::= Empty | Arg | Arg ',' Args | Args ',' '...'

# Args = "ptrdiff_t $ownsym, Lisp_Object *$symbol" -> "Lisp_Handle_Vector $symbol"
# Args = ["ptrdiff_t $ownsym, Lisp_Object *$symbol", "Args"]

BBody ::= '{' Stmts '}'

Stmts ::= Empty | Stmt Stmts

Arg ::= Type | Typed

AttrType ::= Attr Type
PointerType ::= Type '*'
StructUnionType ::= StructUnion Attrs Symbol BStructBody | StructUnion Attrs Type BStructBody
EnumType ::= 'enum' Symbol BEnumBody
Type ::= '(' '__type__' ')' Symbol | '(' '__type__' ')' Symbol Type | AttrType | StructUnion Attrs Symbol | StructUnion Attrs BStructBody | StructUnionType | 'enum' Symbol | EnumType | 'enum' BEnumBody | PointerType rank => 1 | Type const | 'ENUM_BF' '(' Type ')' | 'ENUM_BF' '(' Symbol ')' | Type '[' ']' | Type '[' Expr ']' | RetType '(' Attrs '*' ')' PArgs | Type Attr | Attrs RetType '(' '*' ')' PArgs | 'typeof' Expr

TypedSymbol ::= Type Symbol Attrs
TypedType ::= Type Type Attrs

FunType ::= RetType '(' '*' Type ')' PArgs | RetType '(' Type ')' PArgs | RetType '(' Typed ')' PArgs | RetType '(' '*' Symbol ')' PArgs | RetType '(' '*' '*' Symbol ')' PArgs
Typed ::= Typed Attr | '*' Typed | TypedSymbol | TypedType | FunType | Typed ':' Expr | Typed '[' Expr ']' | Typed '[' ']'

BStructBody ::= '{' StructBody '}'
StructBody ::= Empty | Stmt StructBody

BEnumBody ::= '{' EnumBody '}'
EnumBody ::= CExpr | CExpr ','
PSymbol ::= Symbol | '(' Symbol ')'

Symbol ::= symbol
Number ::= number

Empty ::=

RewriteInit ::= InitPrefix InitRests2 ';'
InitRests2 ::= InitRest ',' InitRests
InitRests ::= InitRest | InitRest ',' InitRests
InitRest ::= InitRestPrefixes Symbol InitSuffixes Attrs | InitRestPrefixes Symbol InitSuffixes '=' Expr | InitRestPrefixes Symbol InitSuffixes 'IF_LINT' '(' '=' Expr ')'
InitSuffixes ::= Empty | '[' Expr ']' InitSuffixes | '[' ']' InitSuffixes | ':' Expr InitSuffixes
InitRestPrefixes ::= InitRestPrefix InitRestPrefixes | Empty
InitRestPrefix ::= '*'

InitPrefix ::= TypeNoStar

TypeNoStar ::= '(' '__type__' ')' Symbol | '(' '__type__' ')' Symbol TypeNoStar | AttrTypeNoStar | StructUnion Attrs Symbol | StructUnion Attrs BStructBody | StructUnion Attrs Symbol BStructBody| 'enum' Symbol | 'enum' Symbol BEnumBody | 'enum' BEnumBody | TypeNoStar 'const' | 'ENUM_BF' '(' TypeNoStar ')' | TypeNoStar '[' ']' | TypeNoStar '[' Expr ']' | RetType '(' '*' ')' PArgs

StructUnion ::= struct | union | StructUnion '(' '__type__' ')'

AttrTypeNoStar ::= Attr TypeNoStar

lexeme default = action => [ value ] latm => 1
ws ~ comment
comment ~ commentstart commentbody commentend
commentstart ~ [/][*]
commentend ~ [*][/]
commentbody ~ commentchar*
commentchar ~ [^*]
commentchar ~ [*][^/]

:lexeme ~ DEFINE_LISP_SYMBOL priority => 1
DEFINE_LISP_SYMBOL ~ 'DEFINE_LISP_SYMBOL'
alignas ~ 'alignas'
auto ~ 'auto'
break ~ 'break'
const ~ 'const'
continue ~ 'continue'
:lexeme ~ else priority => 1
else ~ 'else'
:lexeme ~ extern priority => 1
extern ~ 'extern'
goto ~ 'goto'
long ~ 'long'
:lexeme ~ register priority => 1
register ~ 'register'
restrict ~ 'restrict'
:lexeme ~ return priority => 1
return ~ 'return'
short ~ 'short'
signed ~ 'signed'
static ~ 'static'
:lexeme ~ struct priority => 1
struct ~ 'struct'
:lexeme ~ typedef priority => 1
typedef ~ 'typedef'
union ~ 'union'
unsigned ~ 'unsigned'
volatile ~ 'volatile'

symbol ~ alpha alnum
alpha ~ [a-zA-Z_]
alnum ~ [a-zA-Z0-9_]*
macro ~ calpha calnum
calpha ~ [A-Z_]
calnum ~ [A-Z0-9_]*
number ~ digit alnum
digit ~ [0-9]
ws ~ whitespace
whitespace ~ [\s\n\v]+
string ~ ["]["]
:discard ~ ws
EODSL

# create fake terminal symbols to use to inject wildcards into patterns
for my $symbol (@symbols) {
    $EmacsCGrammar::dsl .= "$symbol ::= t_$symbol\n";
    $EmacsCGrammar::dsl .= "t_$symbol ~ \'\\\\\'\n";
}

package Chunker;

sub chunks {
    my @chunks;
    my $chunk = "";
    my $st;
    while (<STDIN>) {
        chomp;
        #    if (/^[^ \t\/#]/) {
        #        $st = 1;
        if (/^[_a-zA-Z0-9]/ && $st == 7 && !/.*:$/) {
            if ($chunk =~ /\/\*([^*]|\*+[^*\/])*$/) {
                $chunk .= "$_\n";
                next;
            }
            push @chunks, $chunk;
            $chunk = "";
            $st = 0;
        } elsif (/^?$/ || /\*\//) {
            $st = 7;
        } else {
            $st = 0;
        }
        $chunk .= "$_\n";
    }

    push @chunks, $chunk;

    return @chunks;
}

package EmacsCTree::Wildcard;


use Data::Dumper;

sub type {
    my ($self) = @_;
    return $self->{type};
}

sub new {
    my ($class, $type, $symbol) = @_;

    return bless {
        type => $type,
        comps => $symbol,
    }, $class;
}

sub match {
    my ($self, $other, $processor) = @_;

    #warn Dumper($self, $other);

    return if $self->type ne $other->type;
    $processor->set("#" . join("#", @{$self->{comps}}), $other);
    #warn "set" . ("#" . join("#", @{$self->{comps}}) . " to " . Node->new_from_ctree($other));


    return 1;
}

sub nodes {
    my ($self) = @_;

    return ();
}

package EmacsCTree::Lookup;

use Data::Dumper;

sub resolve {
    my ($self, $processor) = @_;

    return $processor->lookup($self->{comps})->ctree;
}

sub type {
    my ($self) = @_;
    return $self->{type};
}

sub new {
    my ($class, $type, $symbol) = @_;

    return bless {
        type => $type,
        comps => $symbol,
    }, $class;
}

sub match {
    my ($self, $other, $processor) = @_;

    my $val = $processor->lookup($self->{comps})->ctree;

    return $val->match($other, $processor);
}

sub nodes {
    my ($self) = @_;

    return ();
}

package EmacsCTree;

use Data::Dumper;

sub shortstr {
    my ($self) = @_;

    return $self->type . "#" . $self->{string};
}

sub resolve {
    my ($self, $processor) = @_;
    my %self = %$self;
    my $copy = \%self;
    bless $copy, "EmacsCTree";
    $copy->{children} = [];
    for my $c (@{$self->{children}}) {
        push @{$copy->{children}}, $c->resolve($processor);
    }
    return $copy;
}

sub type {
    my ($self) = @_;
    return $self->{type};
}

sub nodes {
    my ($self, $type) = @_;
    my @ret;

    return @{$self->{nodes}{$type}} if ($self->{nodes}{$type});

    for my $c (@{$self->ctree->{children}}) {
        push @ret, $c
            if ((!defined($type)) or $c->type eq $type);
        push @ret, ($c->nodes($type));
    }

    $self->{nodes}{$type} = \@ret;

    return @ret;
}

sub ctree {
    my ($self) = @_;

    return $self;
}

sub debug {
    my ($self) = @_;
    my $ret = ""; # . $self->type . ":";

    for my $child (@{$self->{children}}) {
        if ($child->{type} eq "string") {
            $ret .= $child->{string};
        } else {
            $ret .= $child->debug;
        }
    }

    return $ret if ($ret =~ /^<.*>$/);
    return "<" . $ret . ">";
}

sub debug2 {
    my ($self) = @_;
    my $ret = ""; # . $self->type . ":";

    for my $child (@{$self->{children}}) {
        if ($child->{type} eq "string") {
            $ret .= $child->{string};
        } else {
            $ret .= $child->debug2;
        }
    }

    return "<" . $self->type . ":" . $ret . ">";
}

sub match {
    my ($self, $other, $processor) = @_;

    #warn Dumper($self, $other);

    if ($self->{type} ne $other->{type}) {
        return 0;
    }

    if ($self->{type} eq "string") {
        return $self->{string} eq $other->{string};
    }

    my @tree = @{$self->{children}};
    my @pattern = @{$other->{children}};

    my $i = 0;
    my $j = 0;

    while ($i < @tree && $j < @pattern) {
        if ($tree[$i]->match($pattern[$j], $processor)) {
            $i++;
            $j++;
        } else {
            return 0;
        }
    }

    return 0 unless ($i == @tree && $j == @pattern);

    return 1;
}

my $rtree = EmacsCParser::parse_verbatim_ctree("Chunk", \(my $x = "72;"));
$EmacsCTree::globals = {};

sub get_global_hash {
    my ($symbol) = @_;
    $EmacsCTree::globals->{$symbol} = PointedHash->new
        unless $EmacsCTree::globals->{$symbol};
    return $EmacsCTree::globals->{$symbol};
}

sub new_from_rawtree {
    my ($class, $rawtree, $chunk, $prevend) = @_;
    my $ret = bless {}, $class;

    $ret->{vars} = PointedHash->new();

    if (!defined $rawtree) {
        die;
        $ret->{children} = [];
        $ret->{type} = "empty";

        return $ret;
    }

    if (!ref $rawtree) {
        $ret->{children} = [];
        $ret->{type} = "string";
        $ret->{string} = $rawtree;
        $ret->{vars}->{G} = get_global_hash($rawtree);
        return $ret;
    }

    if (ref $rawtree eq "Wildcard" or
        ref $rawtree eq "Lookup") {
        return $rawtree;
    }

    if (ref $rawtree eq "ARRAY") {
        return new_from_rawtree($class, $rawtree->[2], $chunk)
            if defined $rawtree->[2];
        return new_from_rawtree($class, $rawtree->[0], $chunk)
            if defined $rawtree->[0];
        return;
    }

    $ret->{prevend} = $prevend
        if defined $prevend;
    $ret->{start} = $rawtree->[0];
    $ret->{length} = $rawtree->[1];
    $ret->{children} = [];
    for (my $i = 2; $i <= $#$rawtree; $i++) {
        next if !defined $rawtree->[$i];
        my $child = EmacsCTree->new_from_rawtree($rawtree->[$i], $chunk, $prevend);
        if (ref $child eq "Wildcard") {
            return EmacsCTree::Wildcard->new($child->{type}, $child->{comps});
        }
        if (ref $child eq "Lookup") {
            return EmacsCTree::Lookup->new($child->{type}, $child->{comps});
        }
        next if !defined $child;
        $prevend = $child->{start} + $child->{length};
        $ret->{children}->[$i-2] = $child;
    }

    $ret->{string} = substr($chunk, $ret->{start}, $ret->{length})
        if defined $chunk;

    $ret->{type} = ref $rawtree;
    $ret->{type} =~ s/^C:://;

    #warn Dumper($rawtree, $ret);
    $ret->{vars}->{G} = get_global_hash($ret->{string});

    return $ret;
}

package EmacsCParser;

use Data::Dumper;
use Digest::MD5 qw(md5_hex);

my %value_keep;
my %value_strings;

my $grammar = Marpa::R2::Scanless::G->new({
    source => \$EmacsCGrammar::dsl,
    bless_package => "C",
                                          });

my %grammars_by_token;
my %memo;

sub memoize_ctree {
    my ($memostr, $value) = @_;

    $memo{$memostr} = $value;

    my $md5 = md5_hex($memostr);
    my $fh;

    open $fh, ">chunkl-cache/ctree/$md5" or return;

    print $fh Dumper([$memostr, $value]);

    close $fh;
}

sub parse_verbatim_ctree {
    my ($type, $rformat) = @_;
    my $format = $$rformat;
    my @comments;
    my $orig = $format;

    pos($format) = 0;
    while (pos $format < length $format) {
        if ($format =~ /\G.*?(\/\*([^*]|\*+[^*\/])*\*+\/|\/\/[^\n]*\n)/msg) {
            my $str = $1;
            my $npos = pos($format);
            substr $format, $npos - length($str), length($str), (" " x length($str));
            pos($format) = $npos;
            push @comments, [$npos - length($str), $str];
        } else {
            last;
        }
    }

    my $input = $format;

    my $memostr = $type . "\0" . $format;

    return $memo{$memostr} if ($memo{$memostr});

    my $grammar;

    if ($grammars_by_token{$type}) {
        $grammar = $grammars_by_token{$type};
    } else {
        my $dsl = $EmacsCGrammar::dsl;

        $dsl = ":start ::= $type\ninaccessible is ok by default\n$dsl";

        $grammar = Marpa::R2::Scanless::G->new({
            source => \$dsl,
            bless_package => 'C',
                                                  });

        $grammars_by_token{$type} = $grammar;
    }

    my $recce = Marpa::R2::Scanless::R->new({
        grammar => $grammar,
        ranking_method => 'high_rule_only',
                                            });

    $recce->read(\$input, 0, 0);

    my $pos = 0;

    my $chunk = $input;
    my $pos0 = $pos;
    my @comments;

    while (pos $chunk < length $chunk) {
        my $pos = $pos0 + pos($chunk);
        if ($chunk =~ /\G([^\/#\"\']+)/msgc) {
            $recce->resume($pos, length($1));
        } elsif ($chunk =~ /\G(\#([^\n\\]|\\\n|\\.)*\n)/msgc) {
            my $str = substr $chunk, $pos, pos($chunk) - $pos;
            $str =~ s/Lisp_Object/Lisp*Object/msg;
            my $npos = pos($chunk);
            substr $chunk, $pos, pos($chunk) - $pos, $str;
            pos($chunk) = $npos;
        } elsif ($chunk =~ /\G(\/\*([^*]|\*+[^*\/])*\*+\/)/msgc) {
            my $str = substr $chunk, $pos, pos($chunk) - $pos;
            my $npos = pos($chunk);
            substr $chunk, $pos, pos($chunk) - $pos, $str;
            pos($chunk) = $npos;
        } elsif ($chunk =~ /\G(\"([^\\\"]|\\.|\\\n)*\")/msgc) {
            $recce->lexeme_read("string", $pos, length($1));
        } elsif ($chunk =~ /\G(\'([^\\\']|\\.|\\\n)*\')/msgc) {
            $recce->lexeme_read("string", $pos, length($1));
        } elsif ($chunk =~ /\G(.[^\/#\"\']+)/msgc) {
            $recce->resume($pos, length($1));
        }
    }

    my $vref = $recce->value;
    die $memostr unless $vref;
    my $value = $$vref;

    $value = EmacsCTree->new_from_rawtree($value, $orig);

    $memo{$memostr} = $value;

    die $memostr unless $value;

    return $value;
}

sub parse_subtree {
#    my ($type, $format, $verbatim, $bindings) = @_;
#
#    my @format;
#    my $length = 0;
#    my $input;
#    $memo_hashes{$type} = $type;
#    $memo_hashes{$format} = $format;
#
#    if ($verbatim) {
#        pos($format) = 0;
#        while (pos $format < length $format) {
#            if ($format =~ /\G.*?(\/\*([^*]|\*+[^*\/])*\*+\/|\/\/[^\n]*\n)/msg) {
#                my $str = $1;
#                $str =~ s/Lisp_Object/Lisp-Object/msg if $main::phase eq "lvnew";
#                my $npos = pos($format);
#                substr $format, $npos - length($str), length($str), (" " x length($str));
#                pos($format) = $npos;
#                push @comments, [$npos - length($str), $str];
#            } else {
#                last;
#            }
#        }
#
#        @format = (["verbatim", $format]);
#        $length = length($format);
#        $input = $format;
#    } else {
#        @format = parse_format($format, $bindings);
#        for my $entry (@format) {
#            my ($type, $arg, $arg2) = @$entry;
#
#            if ($type eq "verbatim") {
#                $length += length($arg);
#                $input .= $arg;
#            } else {
#                $length++;
#                $input .= "?";
#            }
#        }
#    }
#
#    my $memostr = $type . "\0" . $format . "\0" . Dumper(\@format);
#
#    return $memo{$memostr} if ($memo{$memostr});
#
#    my $grammar;
#
#    if ($grammars_by_token{$type}) {
#        $grammar = $grammars_by_token{$type};
#    } else {
#        my $dsl = $dsl;
#
#        $dsl = ":start ::= $type\ninaccessible is ok by default\n$dsl";
#
#        $grammar = Marpa::R2::Scanless::G->new({
#            source => \$dsl,
#            bless_package => 'C',
#                                                  });
#
#        $grammars_by_token{$type} = $grammar;
#    }
#
#    my $recce = Marpa::R2::Scanless::R->new({
#        grammar => $grammar,
#        ranking_method => 'high_rule_only',
#                                            });
#
#    $recce->read(\$input, 0, 0);
#
#    my $pos = 0;
#    for my $entry (@format) {
#        my ($type, $arg, $arg2) = @$entry;
#
#        if ($type eq "verbatim") {
#            $chunk = $arg;
#            my $pos0 = $pos;
#            my @comments;
#
#            while (pos $chunk < length $chunk) {
#                my $pos = $pos0 + pos($chunk);
#                if ($chunk =~ /\G([^\/#\"\']+)/msgc) {
#                    $recce->resume($pos, length($1));
#                } elsif ($chunk =~ /\G(\#([^\n\\]|\\\n|\\.)*\n)/msgc) {
#                    my $str = substr $chunk, $pos, pos($chunk) - $pos;
#                    $str =~ s/Lisp_Object/Lisp*Object/msg if $main::phase eq "lisp-object";
#                    my $npos = pos($chunk);
#                    substr $chunk, $pos, pos($chunk) - $pos, $str;
#                    pos($chunk) = $npos;
#                } elsif ($chunk =~ /\G(\/\*([^*]|\*+[^*\/])*\*+\/)/msgc) {
#                    my $str = substr $chunk, $pos, pos($chunk) - $pos;
#                    $str =~ s/Lisp_Object/Lisp-Object/msg if $phase eq "lisp-object";
#                    my $npos = pos($chunk);
#                    substr $chunk, $pos, pos($chunk) - $pos, $str;
#                    pos($chunk) = $npos;
#                } elsif ($chunk =~ /\G(\"([^\\\"]|\\.|\\\n)*\")/msgc) {
#                    $recce->lexeme_read("string", $pos, length($1));
#                } elsif ($chunk =~ /\G(\'([^\\\']|\\.|\\\n)*\')/msgc) {
#                    $recce->lexeme_read("string", $pos, length($1));
#                } elsif ($chunk =~ /\G(.[^\/#\"\']+)/msgc) {
#                    $recce->resume($pos, length($1));
#                }
#            }
#            $pos += length($arg);
#        } else {
#            $recce->lexeme_read("t_" . $arg, $pos, 1, Wildcard->new($arg, $arg2));
#            $pos++;
#        }
#    }
#
#    my $value = ${$recce->value};
#
#    $memo{$memostr} = $value;
#
#    die $memostr unless $value;
#
#    return $value;
}




package TypedValue;

package PointedHash;

use Data::Dumper;

sub new {
    my ($class, $value) = @_;

    return bless { "" => $value }, $class;
}

sub copy_from {
    my ($self, $root, @others) = @_;

    $self->{""} = $root;
    for my $other (@others) {
        # die ref $other unless ref $other eq "PointedHash";
        for my $key (keys %$other) {
            next if $key eq "" or $key =~ /^#/;
            die if ref $other->{$key} eq "EmacsCTree";
            $self->{$key} = PointedHash->new();
            $self->{$key}->copy_from($other->{$key}->{""},
                                     $other->{$key});
        }
    }

    return $self;
}

package Alt;

sub new {
    my ($class, $var, $str) = @_;

    $str =~ s/^ *//;

    return bless {
        var => $var,
        str => $str,
    }, $class;
}

package Wildcard;

use Data::Dumper;

sub new {
    my ($class, $type, $cb, $comps) = @_;

    return bless {
        type => $type,
        cb => $cb,
        comps => $comps,
    }, $class;
}

package Lookup;

use Data::Dumper;

sub new {
    my ($class, $type, $comps) = @_;

    return bless {
        type => $type,
        comps => $comps,
    }, $class;
}

package Processor;

use Data::Dumper;
use Digest::MD5 qw(md5_hex);
use File::Slurp qw(read_file write_file);
use Carp qw(croak);

my %memo;

mkdir("chunkl-cache/ctree/");

sub memoize_ctree {
    my ($memostr, $value) = @_;

    $memo{$memostr} = $value;

    my $md5 = md5_hex($memostr);
    my $fh;

    open $fh, ">chunkl-cache/ctree/$md5" or return;

    $Data::Dumper::Sortkeys = 1;
    $Data::Dumper::Indent = 0;
    print $fh Dumper([$memostr, $value]);

    close $fh;
}

sub get_memoized_ctree {
    my ($memostr) = @_;

    return if $memo{$memostr};

    my $md5 = md5_hex($memostr);

    -e "chunkl-cache/ctree/$md5" or return;

    {
        my $VAR1;
        eval read_file("chunkl-cache/ctree/$md5");
        $memo{$VAR1->[0]} = $VAR1->[1];
    }
}

sub new {
    my ($class, $ctree, $defns, $replcb, $cb, $vars, $counterref, $outputref) = @_;

    my $ret = bless {
        parent => undef,
        defn => undef,
        pc => undef,
        vars => PointedHash->new($ctree),
        defns => $defns,
        replcb => $replcb,
        cb => $cb,
    }, $class;

    for my $key (keys %$vars) {
        my @comps = split("#", $key);
        my $type = shift @comps;
        my $nkey = "#" . join("#", @comps);
        $ret->set($nkey, EmacsCParser::parse_verbatim_ctree($type, \($vars->{$key})));
    }

    $ret->{counter} = $counterref;
    $ret->{output} = $outputref;

    return $ret;
}

sub fork {
    my ($self) = @_;

    return bless {
        parent => $self,
        defn => $self->{defn},
        pc => $self->{pc},
        vars => PointedHash->new()->copy_from($self->{vars}->{""},
                                              $self->{vars}),
        defns => $self->{defns},
        replcb => $self->{replcb},
        cb => $self->{cb},
        outvar => PointedHash->new,
        counter => $self->{counter},
        output => $self->{output},
    }, ref $self;
}

sub lookup {
    my ($self, $str) = @_;

    return $self->{vars}->{""} if $str eq "#";

    my @comps = split '#', $str;
    shift @comps;

    my @vars = ($self->{vars});
    for my $comp (@comps) {
        my @newvars;
        for my $vars (@vars) {
            push @newvars, $vars->{$comp};
            if ($vars->{$comp}->{"#tree"}) {
                push @newvars, $vars->{$comp}->{"#tree"}->{vars};
            }
        }
        @vars = @newvars;
    }

    for my $vars (@vars) {
        if (exists $vars->{""}) {
            return $vars->{""};
        }
    }

    return;
}

sub set {
    my ($self, $str, $val, $or) = @_;

    my @comps = split '#', $str;
    shift @comps;

    my @vars = ($self->{vars});
    for my $comp (@comps) {
        my @newvars;
        for my $vars (@vars) {
            $vars->{$comp} = PointedHash->new unless $vars->{$comp};
            push @newvars, $vars->{$comp};
            if ($vars->{$comp}->{"#tree"}) {
                push @newvars, $vars->{$comp}->{"#tree"}->{vars};
            }
        }
        @vars = @newvars;
    }

    for my $vars (@vars) {
        next if $or && defined $vars->{""};
        $vars->{""} = $val;
        if (ref $val eq "EmacsCTree") {
            $vars->{"#tree"} = $val;
        }
    }
}

my %memo_hashes;
sub parse_format {
    my ($self, $format) = @_;
    my @result;
    pos($format) = 0;
    while (pos($format) < length($format)) {
        if ($format =~ /\G(.*?)([a-zA-Z]*(#[a-zA-Z]*)+)/gc) {
            push @result, ["verbatim", $1] if $1 ne "";
            my $match = $2;
            if ($match =~ /^#/) {
                my $lu = "";
                $lu = $self->lookup($match);
                return unless $lu;
                $lu = $lu->type;
                $memo_hashes{$lu} = $lu;
                $lu = "" . $lu;
                push @result, ["match", $match, $lu]
            } else {
                push @result, ["wildc", $match]
            }
        } elsif ($format =~ /\G(.*?)$/gc) {
            push @result, ["verbatim", $1] if $1 ne "";
        }
    }

    return @result;
}

my %memo_hash;
my %grammars_by_token;

my %memo_substt;

sub subst_types {
    my ($arg0) = @_;

    return $memo_substt{$arg0} if exists $memo_substt{$arg0};

    my $arg = $arg0;

    for my $type (@EmacsCGrammar::types) {
        $arg =~ s/\b$type\b/(__type__)$type/g;
        $arg =~ s/\(__type__\)($type\([^_*])/$1/g;
        $arg =~ s/\(__type__\)($type \([^_*])/$1/g;
        $arg =~ s/\(__type__\)\(__type__\)/(__type__)/g;
        $arg =~ s/\(__type__\)\(__type__\)/(__type__)/g;
    }

    $memo_substt{$arg0} = $arg;

    return $arg;
}

sub parse_and_bind {
    my ($self, $type, $str) = @_;
    my @format = $self->parse_format($str);

    return unless @format;

    my $length = 0;
    my $input = "";
    for my $entry (@format) {
        my ($type, $arg, $arg2) = @$entry;

        if ($type eq "verbatim") {
            $entry->[1] = $arg = subst_types($arg);

            $length += length($arg);
            $input .= $arg;
        } else {
            $length+=1;
            $input .= "?";
        }
    }

    $Data::Dumper::Sortkeys = 1;
    $Data::Dumper::Indent = 0;
    my $memostr = $type . "\0" . $str . "\0". Dumper(\@format);

    get_memoized_ctree($memostr);
    return $memo{$memostr} if ($memo{$memostr});

    #warn "no memo for $memostr";

    my $grammar;

    if ($grammars_by_token{$type}) {
        $grammar = $grammars_by_token{$type};
    } else {
        my $dsl = $EmacsCGrammar::dsl;

        $dsl = ":start ::= $type\ninaccessible is ok by default\n$dsl";

        $grammar = Marpa::R2::Scanless::G->new({
            source => \$dsl,
            bless_package => 'C',
                                                  });

        $grammars_by_token{$type} = $grammar;
    }

    my $value;

    eval {
    my $recce = Marpa::R2::Scanless::R->new({
        grammar => $grammar,
        ranking_method => 'high_rule_only',
                                            });


    $recce->read(\$input, 0, 0);

    my $pos = 0;

    for my $entry (@format) {
       my ($type, $arg) = @$entry;

        if ($type eq "verbatim") {
            my $chunk = $arg;
            my $pos0 = $pos;
            my @comments;

            while (pos $chunk < length $chunk) {
                my $pos = $pos0 + pos($chunk);
                if ($chunk =~ /\G([^\/#\"\']+)/msgc) {
                    $recce->resume($pos, length($1));
                } elsif ($chunk =~ /\G(\#([^\n\\]|\\\n|\\.)*\n)/msgc) {
                    my $str = substr $chunk, $pos, pos($chunk) - $pos;
                    my $npos = pos($chunk);
                    substr $chunk, $pos, pos($chunk) - $pos, $str;
                    pos($chunk) = $npos;
                } elsif ($chunk =~ /\G(\/\*([^*]|\*+[^*\/])*\*+\/)/msgc) {
                    my $str = substr $chunk, $pos, pos($chunk) - $pos;
                    my $npos = pos($chunk);
                    substr $chunk, $pos, pos($chunk) - $pos, $str;
                    pos($chunk) = $npos;
                } elsif ($chunk =~ /\G(\"([^\\\"]|\\.|\\\n)*\")/msgc) {
                    $recce->lexeme_read("string", $pos, length($1));
                } elsif ($chunk =~ /\G(\'([^\\\']|\\.|\\\n)*\')/msgc) {
                    $recce->lexeme_read("string", $pos, length($1));
                } elsif ($chunk =~ /\G(.[^\/#\"\']+)/msgc) {
                    $recce->resume($pos, length($1));
                }
            }
            $pos += length($arg);
        } elsif ($type eq "wildc") {
            my @comps = split("#", $arg);
            my $var = $arg;
            $var =~ s/^[^#]*//;
            my $type = shift(@comps);
    $Data::Dumper::Sortkeys = 1;
    $Data::Dumper::Indent = 0;
            die "rejected: $memostr " . $type . " " . Dumper(\@format) unless $recce->lexeme_read("t_" . $type, $pos, 1, Wildcard->new($type, sub { $self->set("#" . join("#", @comps), $_[0]) if @comps }, \@comps));
            $pos += 1;
        } else {
            my @comps = split("#", $arg);
            my $var = $arg;
            my $val = $self->lookup($arg);
            return unless $val;
            my $type = $val->type;
    $Data::Dumper::Sortkeys = 1;
    $Data::Dumper::Indent = 0;
            die "rejected: $memostr " . $type . " " . Dumper(\@format) unless $recce->lexeme_read("t_" . $type, $pos, 1, Lookup->new($type, $var));
            $pos += 1;
        }
    }

    $value = $recce->value;
    die $memostr unless $value;
    $value = $$value;
    };

    die $@ if $@;

    $value = EmacsCTree->new_from_rawtree($value);

    memoize_ctree($memostr, $value);

    use Data::Dumper;
    $Data::Dumper::Sortkeys = 1;
    $Data::Dumper::Indent = 0;
    # warn "$memostr => " . Dumper($value);

    die $memostr unless $value;

    return $value;
}

sub rec_print {
    my ($self, $pattern) = @_;

    my $str = $pattern->{string};

    return $str if (defined $str);

    if (ref($pattern) eq "Wildcard") {
        return $pattern->{type} . "#";
    } elsif (ref($pattern) eq "EmacsCTree") {
        my @tokens = map { $self->rec_print($_) } @{$pattern->{children}};
        @tokens = grep { $_ ne "" } @tokens;
        my $ret = join(" ", @tokens);
        $ret =~ s/ +/ /g;
        $ret =~ s/ ?\. ?/./g;
        $ret =~ s/ ?, ?/, /g;
        $ret =~ s/ ?\) ?/)/g;
        $ret =~ s/\( ?/(/g;
        $ret =~ s/\(\{/( {/g;
        $ret =~ s/\}\)/} )/g;
        $ret =~ s/ ?;/;/g;
        $ret =~ s/;([^ \n])/; $1/g;
        return $ret;
    } else {
        return $pattern;
    }
}

sub print {
    my ($self) = @_;

    ${$self->{output}} .= $_[1];
}

sub step {
    my ($self, $pc) = @_;
    die unless $self->{defn};
    my $clause = $self->{defn}->{clauses}->[$pc];

    if (!$clause) {
        die $self unless $self->{outvar};
        return unless $self->{parent}->{defn};
        my $outvar = $self->{defn}->{outvar};
        $outvar =~ s/^#//;
        eval {
            # warn "copying $outvar to " . $self->{outvar};
            # warn $self->{vars}->{$outvar}->{""}->debug;
            $self->{outvar}->copy_from($self->{vars}->{$outvar}->{""},
                                       $self->{vars}->{$outvar}, $self->{vars})
                if ($outvar ne "");
            # warn $self->{outvar}->{""}->debug;
        };

        if ($@) {
            die "$outvar . $@";
        }

        $self->{parent}->step($self->{retpc}) if $self->{parent}->{defn} and defined($self->{retpc});

        return;
    }

    for my $alt (@$clause) {
        my $var;
        my $str;
        ($var, $str) = ($1, $2) if($alt =~ /^(.*?) (.*)$/);
        my $val = $self->lookup($var);
        next unless $val;
        my $type;
        eval {
            $type = $val->type;
        };
        if ($@) {
            die "$val $var $str";
        }

        if ($str =~ /^matches +(.*?)$/) {
            my $fork = $self;
            my $pattern = $1;
            eval {
                my $parsed = $fork->parse_and_bind($type, $pattern);

                next unless $parsed;
                # warn "matches " . $val->debug . " $pattern";
                next unless $parsed->match($val->ctree, $fork);
                # warn "matches " . $val->debug . " $pattern: yes";
            };
            if ($@) {
                warn $@;
                next;
            }

            $fork->step($pc+1);

            next;
        } elsif ($str =~ /^nomatch +(.*?)$/) {
            my $fork = $self;
            my $pattern = $1;
            eval {
                my $parsed = $fork->parse_and_bind($type, $pattern);

                next unless $parsed;
                # warn "nomatch " . $val->debug . " $pattern";
                next if $parsed->match($val->ctree, $fork);
                # warn "nomatch " . $val->debug2 . " $pattern: yes";
            };
            if ($@) {
                warn $@;
                next;
            }

            $fork->step($pc+1);

            next;
        } elsif ($str =~ /^contains +(.*?)$/) {
            my @comps = split("#", $1);
            die if (@comps <= 1);
            my $type = shift @comps;

            my $parsed;
            eval {
                $parsed = $self->parse_and_bind($type, $1);

                next unless $parsed;
            };
            if ($@) {
                warn $@;
                next;
            }

            my @nodes = $val->nodes($type);

            for my $node (@nodes) {
                if ($node->type eq $type and
                    $parsed->match($node->ctree, $self)) {
                    $self->step($pc + 1);
                }
            }

            next;
        } elsif ($str =~ /^has +(.*?)$/) {
            my @comps = split("#", $1);
            die if (@comps <= 1);
            my $type = shift @comps;

            my $parsed;
            eval {
                $parsed = $self->parse_and_bind($type, $1);

                next unless $parsed;
            };
            if ($@) {
                warn $@;
                next;
            }

            my @nodes = (@{$val->ctree->{children}});

            for my $node (@nodes) {
                if ($node->type eq $type and
                    $parsed->match($node->ctree, $self)) {
                    $self->step($pc + 1);
                }
            }

            next;
        } elsif ($str =~ /^ *element (.*?): (.*?)$/) {
            my ($index, $pattern) = ($1, $2);
            my $ctype;
            if ($type eq "Args") {
                $ctype = "Arg";
            } elsif ($type eq "ArgExprs") {
                $ctype = "ArgExpr";
            } else {
                die;
            }

            my $itree;

            $itree = $self->parse_and_bind("Expr", "$index");

            my $parsed;
            eval {
                $parsed = $self->parse_and_bind($ctype, $pattern);

                next unless $parsed;
            };
            if ($@) {
                warn $@;
                next;
            }

            my @nodes = (@{$val->ctree->{children}});

            my $car = sub {
                return ref($_[0]) ? $_[0]->{children}->[0] : $_[0];
            };
            my $cdr = sub {
                return ref($_[0]) ? $_[0]->{children}->[2] : undef;
            };

            my $nodes = $val->ctree;
            for (my $i = 0; $nodes; $i++, $nodes = $cdr->($nodes)) {
                my $node = $car->($nodes);
                next unless $node;
                if ($node->type eq $ctype and
                    $parsed->match($node->ctree, $self)) {
                    if (defined($itree)) {
                        unless ($itree->match(EmacsCParser::parse_verbatim_ctree("Expr", \("$i")), $self)) {
                            warn "itree did not match $i";
                            next;
                        }
                    }
                    $self->step($pc + 1);
                }
            }

            next;
        } elsif ($str =~ /^ *count (.*?)$/) {
            my ($count) = ($1);
            my $ctype;
            if ($type eq "Args") {
                $ctype = "Arg";
            } elsif ($type eq "ArgExprs") {
                $ctype = "ArgExpr";
            } else {
                die;
            }

            my $itree;

            $itree = $self->parse_and_bind("Expr", "$count");

            my @nodes = (@{$val->ctree->{children}});

            my $car = sub {
                return ref($_[0]) ? $_[0]->{children}->[0] : $_[0];
            };
            my $cdr = sub {
                return ref($_[0]) ? $_[0]->{children}->[2] : undef;
            };

            my $nodes = $val->ctree;
            my $i;
            for ($i = 0; $nodes; $i++, $nodes = $cdr->($nodes)) {
            }

            unless ($itree->match(EmacsCParser::parse_verbatim_ctree("Expr", \(my $x = "$i")), $self)) {
                warn "itree did not match $i";
                next;
            }
            $self->step($pc + 1);

            next;
        } elsif ($str =~ /^includes +(.*?): (.*?)$/) {
            my ($runvar, $expr) = ($1, $2);
            my $ctype;
            if ($type eq "Args") {
                $ctype = "Arg";
            } elsif ($type eq "ArgExprs") {
                $ctype = "ArgExpr";
            } else {
                die;
            }
            my $node = $self->lookup($var);
            my $ctree = $node->ctree;

            my $comma0 = $self->parse_and_bind($type, "$expr");
            #my $comma0 = $self->parse_and_bind($type, $expr . "," . "$type#");

            my $car = sub {
                return ref($_[0]) ? $_[0]->{children}->[0] : $_[0];
            };
            my $cdr = sub {
                return ref($_[0]) ? $_[0]->{children}->[2] : undef;
            };

            my @l1;

            for (my $tree = $comma0; $tree; $tree = $cdr->($tree)) {
                push @l1, $car->($tree);
            }

            my @l2;

            for (my $tree = $ctree; $tree; $tree = $cdr->($tree)) {
                push @l2, $car->($tree);
            }

            if (@l1 <= @l2) {
              outer:
                for (my $i = 0; $i <= @l2 - @l1; $i++) {
                    for (my $j = 0; $j < @l1; $j++) {
                        next outer unless $l1[$j]->match($l2[$i+$j], $self);
                    }
                    $self->set($runvar, (
                                   EmacsCTree->new_from_rawtree(bless([$l2[$i]{start}, ($l2[$i+$#l1]{start}-$l2[$i]{start}) + $l2[$i+$#l1]{length}, undef], "C::Dummy"))));
                    $self->step($pc + 1);
                    next;
                }
            }

            next;
        } elsif ($str =~ /^ *<- ?(.*?)$/) {
            my @format;
            eval {
                @format = $self->parse_format($1);
            };
            warn $@ if ($@);
            next if ($@ or ($1 and !@format));
            my $str = "";
            for my $entry (@format) {
                my ($type, $arg) = @$entry;
                if ($type eq "verbatim") {
                    $str .= $arg;
                } else {
                    $str .= $self->rec_print($self->lookup($arg)->ctree);
                }
            }
            $str =~ s/ $//;
            $str =~ s/ ?\. ?/./g;
            $str =~ s/ ?, ?/, /g;
            $str =~ s/ ?\)/)/g;
            $str =~ s/\( +/(/g;
            $str =~ s/\(\{/( {/g;
            $str =~ s/\}\)/} )/g;
            $str =~ s/ ?;/;/g;
            $str =~ s/ ?-> ?/->/g;
            $str =~ s/ ?\[ ?/[/g;
            $str =~ s/ ?\] ?/]/g;

            my $repl = [$val->ctree->{start}, $val->ctree->{length}, $str];

            $self->{replcb}->($repl);
            $self->step($pc + 1);

            next;
        } elsif ($str =~ /^ *<<- *(.*)$/) {
            my @format;
            eval {
                @format = $self->parse_format($1);
            };
            warn $@ if ($@);
            next if ($@ or !@format);
            my $str = "";
            for my $entry (@format) {
                my ($type, $arg) = @$entry;
                if ($type eq "verbatim") {
                    $str .= $arg;
                } else {
                    $str .= $self->rec_print($self->lookup($arg)->ctree);
                }
            }
            $str =~ s/ $//;
            $str =~ s/ ?\. ?/./g;
            $str =~ s/ ?, ?/, /g;
            $str =~ s/ ?\) ?/)/g;
            $str =~ s/\( +/(/g;
            $str =~ s/\(\{/( {/g;
            $str =~ s/\}\)/} )/g;
            $str =~ s/ ?;/;/g;
            $str =~ s/ ?-> ?/->/g;
            $str =~ s/ ?\[ ?/[/g;
            $str =~ s/ ?\] ?/]/g;

            $str .= "\n  ";

            my $repl = [$val->ctree->{start}, 0, $str];

            $self->{replcb}->($repl);
            $self->step($pc + 1);

            next;
        } elsif ($str =~ /^ *pre-chunk (.*)$/) {
            my @format = $self->parse_format($1);
            next if !@format;
            my $str = "";
            for my $entry (@format) {
                my ($type, $arg) = @$entry;
                if ($type eq "verbatim") {
                    $str .= $arg;
                } else {
                    $str .= $self->rec_print($self->lookup($arg)->ctree);
                }
            }
            $str =~ s/ $//;
            $str =~ s/ ?\. ?/./g;
            $str =~ s/ ?, ?/, /g;
            $str =~ s/ ?\)/)/g;
            $str =~ s/\( +/(/g;
            $str =~ s/\(\{/( {/g;
            $str =~ s/\}\)/} )/g;
            $str =~ s/ ?; ?/;/g;

            $self->{cb}->{prechunk}->($str);

            next;
        } elsif ($str =~ /^ *set(-or)? ([^#]*?)(#.*?): (.*)$/) {
            my ($or, $type, $var, $val) = ($1, $2, $3, $4);
            my $parsed;
            eval {
                $parsed = $self->parse_and_bind($type, $val);
            };

            if ($@ or !$parsed) {
                warn $@ if $@;
                next;
            }

            $parsed = $parsed->resolve($self);

            $self->set($var, $parsed, $or);

            $self->step($pc + 1);

            next;
        } elsif ($str =~ /^ *equate(-or)? ([^#]*?)(#.*?): (.*)$/) {
            my ($or, $type, $var, $var2) = ($1, $2, $3, $4);

            $self->set($var, $self->lookup($var2), $or);

            $self->step($pc + 1);

            next;
        } elsif ($str =~ /^ *unique: ([^#]*?)(#.*?)$/) {
            my ($type, $var) = ($1, $2);
            my $counter;
            if (defined $val->{uniq}) {
                $counter = ++$val->{uniq};
            } else {
                my $str = $self->rec_print($val->ctree);

                $counter = 0;
                while (index($str, "__u_$counter") != -1) {
                    $counter++;
                }
                $val->{uniq} = $counter;
            }
            warn "$val " . $val->{uniq} . " $counter";
            my $val = "__u_$counter";
            my $parsed;
            eval {
                $parsed = $self->parse_and_bind($type, $val);
            };

            if ($@ or !$parsed) {
                warn $@ if $@;
                next;
            }

            $parsed = $parsed->resolve($self);

            $self->set($var, $parsed);

            $self->step($pc + 1);

            next;
        } elsif ($str =~ /^ *free/) {
            if ($self->{cb}->{free}->($val)) {
                $self->step($pc + 1);
            }
        } elsif ($str =~ /^ *dump/) {
            $Data::Dumper::Sortkeys = 1;
            $Data::Dumper::Indent = 2;
            warn Dumper($val) if $val->debug =~ /LHH.*x/;
            $self->step($pc + 1);

            next;
        } elsif ($str =~ /^ *debug/) {
            warn $val->debug;
            $self->step($pc + 1);

            next;
        } elsif ($str =~ /^ *flush/) {
            $self->{cb}->{flush}->();
            $self->step($pc + 1);

            next;
        } elsif ($str =~ /^ *print (.*?)$/) {
            my @format = $self->parse_format($1);
            next if !@format;
            my $str = "";
            for my $entry (@format) {
                my ($type, $arg) = @$entry;
                if ($type eq "verbatim") {
                    $str .= $arg;
                } else {
                    $str .= $self->rec_print($self->lookup($arg)->ctree);
                }
            }
            $str =~ s/ $//;
            $str =~ s/ ?\. ?/./g;
            $str =~ s/ ?, ?/, /g;
            $str =~ s/ ?\)/)/g;
            $str =~ s/\( +/(/g;
            $str =~ s/\(\{/( {/g;
            $str =~ s/\}\)/} )/g;
            $str =~ s/ ?; ?/;/g;

            $self->print($str . "\n");

            $self->step($pc + 1);
            next;
        } elsif ($str =~ /^ *check (.*?)$/) {
            my @format = $self->parse_format($1);
            next if !@format;
            my $str = "";
            for my $entry (@format) {
                my ($type, $arg) = @$entry;
                if ($type eq "verbatim") {
                    $str .= $arg;
                } else {
                    $str .= $self->rec_print($self->lookup($arg)->ctree);
                }
            }
            $str =~ s/ $//;
            $str =~ s/ ?\. ?/./g;
            $str =~ s/ ?, ?/, /g;
            $str =~ s/ ?\)/)/g;
            $str =~ s/\( +/(/g;
            $str =~ s/\(\{/( {/g;
            $str =~ s/\}\)/} )/g;
            $str =~ s/ ?; ?/;/g;

            # warn "checking $str";
            if (eval($str)) {
                # warn "yes";
                $self->step($pc + 1);
            } else {
                # warn "no";
            }
        } elsif ($str =~ /^ *exit/) {
            $Data::Dumper::Sortkeys = 1;
            $Data::Dumper::Indent = 0;
            warn Dumper($val);
            return;
        } elsif ($str =~ /^call: (.*?) ([a-zA-Z]*(#[a-zA-Z]*)+)$/ &&
                 $self->{defns}->{$1}) {
            my $defn = $self->{defns}->{$1};
            my $outvar = $2;
            my $invar = $var;

            $self->call($defn, $invar, $outvar, $pc + 1)->step(0);

            $self->step($pc + 1);

            next;
        } elsif ($str =~ /^(.*?) ([a-zA-Z]*(#[a-zA-Z]*)+)$/ &&
                 $self->{defns}->{$1}) {
            my $defn = $self->{defns}->{$1};
            my $outvar = $2;
            my $invar = $var;

            $self->run($defn, $invar, $outvar, $pc + 1)->step(0);

            next;
        } else {
            die "unknown str $str";
        }
    }
}

sub run {
    my ($self, $defn, $invar, $outvar, $retpc) = @_;

    $outvar =~ s/^#//;
    $self->{vars}->{$outvar} = PointedHash->new();
    my $fork = $self->fork;
    $fork->{defn} = $defn;
    $fork->{outvar} = $self->{vars}->{$outvar};
    $fork->set($defn->{invar}, $self->lookup($invar));
    $fork->{pc} = 0;
    $fork->{retpc} = $retpc;

    return $fork;
}

sub call {
    my ($self, $defn, $invar, $outvar, $retpc) = @_;

    $outvar =~ s/^#//;
    $self->{vars}->{$outvar} = PointedHash->new();
    my $fork = $self->fork;
    $fork->{defn} = $defn;
    $fork->{outvar} = $self->{vars}->{$outvar};
    $fork->set($defn->{invar}, $self->lookup($invar));
    $fork->{pc} = 0;

    return $fork;
}

package Defn;

sub new {
    my ($class, $invar, $outvar, $arg, $defn, $clauses) = @_;

    return bless {
        defn => $defn,
        invar => $invar,
        outvar => $outvar,
        arg => $arg,
        clauses => $clauses,
    }, $class;
}

package Parser;

sub parse_defns {
    my ($text, $pos) = @_;
    my %defns;

    pos($text) = $pos;

    while (pos($text) < length($text)) {
        my $defn;
        my @clauses;
        my $orflag;


        while (pos($text) < length($text)) {
            last if ($text =~ /\G\n/msgc);
            $defn = $1 if ($text =~ /\G\[\[([^\n]*?)\]\]:\n/msgc);
            if ($orflag) {
                push @{$clauses[$#clauses]}, $1
                    if ($text =~ /\G\[\[([^\n]*?)\]\]((\|| \|\|)?)\n/msgc);
                $orflag = $2 ne "";
            } else {
                push @clauses, [$1]
                    if ($text =~ /\G\[\[([^\n]*?)\]\]((\|| \|\|)?)\n/msgc);
                $orflag = $2 ne "";
            }
        }

        my ($var, $descr, $arg);

        ($var, $descr, $arg) = ($1, $2, $3)
            if $defn =~ /^(\#.*?) (.*?) (\#.*?)$/;

        $defns{$descr} = Defn->new($arg, $var, $arg, $descr, \@clauses);
    }

    return \%defns;
}






package Variable;

sub new {
    my ($class, $name, $bindings) = @_;
    my $ret = bless { name => $name, bindings => $bindings }, $class;

    $ret->{bindings}->{$name}
}

sub set {
    my ($self, $type, $value) = @_;

    $self->{type} = $type;
    $self->{value} = $value;
}

package main;

use Data::Dumper;

my $chunk = <<'EOF';
int main(int arg0, ptrdiff_t count, Lisp_Object *vector, int arg3)
{
}
EOF

my $defns_header = Parser::parse_defns(<<'EOF', 0);
[[#funtyped FunTyped #chunk]]:
[[#chunk contains Typed#funtyped]]
[[#funtyped matches RetType#ret (* Symbol#) (Args#args)]] ||
[[#funtyped matches RetType#ret (* * Symbol#) (Args#args)]] ||
[[#funtyped matches RetType#ret (* Type#typeb) (Args#args)]]

[[#funtyped FunTypedef #chunk]]:
[[#chunk contains Typedef#typedef]]
[[#typedef matches typedef RetType#ret (* Type#typeb) (Args#args);]]

[[#funtype FunType #chunk]]:
[[#chunk contains Type#funtype]]
[[#funtype matches Attrs# RetType#ret (*) (Args#args)]]

[[#funtypedb FunTypeD #chunkb]]:
[[#chunkb FunTyped #funtypedb]] ||
[[#chunkb FunType #funtypedb]] ||
[[#chunkb FunTypedef #funtypedb]]

[[#fundef FunctionDefinition #chunk]]:
[[#chunk contains FunctionDefinition#fundef]]
[[#fundef matches DEFUN(Expr#, Symbol#symbol, CExpr#, Junk#)(Args#args) BBody#body]] ||
[[#fundef matches Attrs# RetType#ret Symbol#symbol (Args#args) BBody#body]] ||
[[#fundef matches Attrs# RetType#ret Symbol#symbol (Args#args) Attrs# ;]] ||
[[#fundef matches Attrs# RetType#ret (Symbol#symbol) (Args#args) BBody#body]] ||
[[#fundef matches Attrs# RetType#ret (Symbol#symbol) (Args#args) Attrs# ;]]

[[#vector LV arg #args]]:
[[#args includes #run: (__type__)ptrdiff_t Symbol#count,(__type__)Lisp_Object*Symbol#vector]]

[[# AUTO-0010 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args LV arg #vector]]
[[#vector#run <- (__type__)ELisp_Vector_Handle #vector]]

[[# AUTO-0015 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args includes #run: (__type__)ptrdiff_t, (__type__)Lisp_Object *]]
[[#run <- (__type__)ELisp_Vector_Handle]]

[[# AUTO-0020 #]]:
[[# FunTypeD #funtyped]]
[[#funtyped#args LV arg #vector]]
[[#vector#run <- (__type__)ELisp_Vector_Handle #vector]]

[[# AUTO-0100-FLUSH #]]:

[[# AUTO-0300 #]]:
[[# contains Stmt#stmt]]
[[#stmt matches (__type__)ELisp_Value Symbol#symbol;]]
[[# set Type#symbol#G#type: (__type__)ELisp_Value]]

[[# AUTO-030001 #]]:
[[# contains Stmt#stmt]]
[[#stmt matches (__type__)ELisp_Value Symbol#symbol = Expr#rhs;]]
[[# set Type#symbol#G#type: (__type__)ELisp_Value]]

[[# AUTO-07400 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args element Expr#n: Arg#arg]]
[[#arg matches (__type__)Lisp_Object]] ||
[[#arg matches (__type__)Lisp_Object Symbol#]] ||
[[#arg matches register (__type__)Lisp_Object Symbol#]]
[[# print $accepts{#fundef#symbol}[#n] = "Lisp_Object"; #CU;]]

[[# AUTO-07450 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args element Expr#n: Arg#arg]]
[[#arg matches (__type__)void *]] ||
[[#arg matches (__type__)void * Symbol#]] ||
[[#arg matches register (__type__)void * Symbol#]]
[[# print $accepts{#fundef#symbol}[#n] = "void *"; #CU;]]

[[# AUTO-07500 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#symbol matches Symbol#symbol]]
[[#fundef#ret matches (__type__)Lisp_Object]]
[[# print $returns{#symbol} = "Lisp_Object"; #CU;]]

[[# AUTO-08000 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#symbol matches Symbol#symbol]]
[[#fundef#ret matches (__type__)void *]]
[[# print $returns{#symbol} = "void *"; #CU;]]

[[# AUTO-0900 #]]:
[[# contains FunctionDefinition#fundef]]
[[#fundef matches DEFUN(Expr#, Symbol#symbol, CExpr#, Junk#)(Args#args) BBody#body]]
[[#symbol print $returns{#symbol} = "Lisp_Object"; #CU;]]

[[# AUTO-0950 #]]:
[[# contains Stmt#stmt]]
[[#stmt matches EXFUN(Symbol#symbol, Expr#n);]]
[[#n check "#n" ~ /^[0-9]+$/]]
[[#symbol print for my $i (0 .. (#n-1)) { $accepts{#symbol}[$i] = "Lisp_Object"; } $returns{#symbol} = "Lisp_Object"; #CU; ]]
EOF

my $defns_main = Parser::parse_defns(<<'EOF', 0);
[[#funtyped FunTyped #chunk]]:
[[#chunk contains Typed#funtyped]]
[[#funtyped matches RetType#ret (* Symbol#) (Args#args)]] ||
[[#funtyped matches RetType#ret (* Type#typeb) (Args#args)]]

[[#funtype FunType #chunk]]:
[[#chunk contains Type#funtype]]
[[#funtype matches Attrs# RetType#ret (*) (Args#args)]]

[[#funtypedb FunTypeD #chunkb]]:
[[#chunkb FunTyped #funtypedb]] ||
[[#chunkb FunType #funtypedb]]

[[#fundef FunctionDefinition #chunk]]:
[[#chunk contains FunctionDefinition#fundef]]
[[#fundef matches DEFUN(Expr#, Symbol#, CExpr#, Junk#)(Args#args) BBody#body]] ||
[[#fundef matches Attrs# RetType#ret PSymbol#psymbol (Args#args) BBody#body]] ||
[[#fundef matches Attrs# RetType#ret PSymbol#psymbol (Args#args) Attrs# ;]]

[[#vector LV arg #args]]:
[[#args includes #run: (__type__)ptrdiff_t Symbol#count,(__type__)Lisp_Object*Symbol#vector]]

[[#vector LV array arg #args]]:
[[#args includes #run: (__type__)Lisp_Object Symbol#vector[Expr#count]]]

[[#chunk LV chunk arg #vector]]:
[[#chunk FunctionDefinition #fundef]]
[[#fundef#args LV arg #vector]]

[[#chunk LV chunk array arg #vector]]:
[[#chunk FunctionDefinition #fundef]]
[[#fundef#args LV arg #vector]]

[[#chunk LV array var #vector]]:
[[#chunk FunctionDefinition #fundef]]
[[#fundef#args LV arg #vector]]
[[#fundef#body contains Stmt#decl]]
[[#decl matches (__type__)Lisp_Object Symbol#symbol[Expr#count];]]
[[#decl <- ELisp_Array(#symbol, #count);]]
[[# set Expr#newcount: #count]]

[[#chunk LV initialized array var #vector]]:
[[#chunk contains Stmt#declinit]]
[[#declinit matches (__type__)Lisp_Object Symbol#symbol[] = { CExpr#cexpr };]] ||
[[#declinit matches (__type__)Lisp_Object Symbol#symbol[] = { CExpr#cexpr, };]]
[[#declinit <- ELisp_Array_Imm(#symbol, #cexpr);]]
[[# set Expr#newcount: #symbol.n]]

[[#vector LV #chunk]]:
[[#chunk LV chunk arg #vector]] ||
[[#chunk LV chunk array arg #vector]] ||
[[#chunk LV array var #vector]] ||
[[#chunk LV initialized array var #vector]]

[[# AUTO-0005-REPEAT #]]:
[[# contains RewriteInit#stmt]]
[[#stmt matches InitPrefix#initprefix InitRest#rest, InitRests#rests;]]
[[#stmt <- #initprefix #rest; #initprefix #rests;]]

[[# AUTO-0006 #]]:
[[# contains Attr#attr]]
[[#attr matches register]] ||
[[#attr matches UNINIT]] ||
[[#attr matches auto]]
[[#attr <-]]

[[# AUTO-0007-0001 #]]:
[[# contains Symbol#symbol]]
[[#symbol matches min]]
[[#symbol <- c_min]]

[[# AUTO-0007-0002 #]]:
[[# contains Symbol#symbol]]
[[#symbol matches max]]
[[#symbol <- c_max]]

[[# AUTO-0007-0003 #]]:
[[# contains Symbol#symbol]]
[[#symbol matches this]]
[[#symbol <- c_this]]

[[# AUTO-0007-0004 #]]:
[[# contains Symbol#symbol]]
[[#symbol matches not]]
[[#symbol <- c_not]]

[[# AUTO-0007-0005 #]]:
[[# contains Symbol#symbol]]
[[#symbol matches new]]
[[#symbol <- c_new]]

[[# AUTO-0007-0006 #]]:
[[# contains Symbol#symbol]]
[[#symbol matches and]]
[[#symbol <- c_and]]

[[# AUTO-0007-0007 #]]:
[[# contains Symbol#symbol]]
[[#symbol matches or]]
[[#symbol <- c_or]]

[[# AUTO-0007-0008 #]]:
[[# contains Symbol#symbol]]
[[#symbol matches class]]
[[#symbol <- c_class]]

[[# AUTO-0007-0009 #]]:
[[# contains Symbol#symbol]]
[[#symbol matches delete]]
[[#symbol <- c_delete]]

[[# AUTO-0007-0010 #]]:
[[# contains Symbol#symbol]]
[[#symbol matches try]]
[[#symbol <- c_try]]

[[# AUTO-0007-0011 #]]:
[[# contains Symbol#symbol]]
[[#symbol matches private]]
[[#symbol <- c_private]]

[[# AUTO-0007-0012 #]]:
[[# contains Symbol#symbol]]
[[#symbol matches explicit]]
[[#symbol <- c_explicit]]

[[# AUTO-0007-0013 #]]:
[[# contains Symbol#symbol]]
[[#symbol matches min]]
[[#symbol <- c_min]]

[[# AUTO-0007-0014 #]]:
[[# contains Symbol#symbol]]
[[#symbol matches catch]]
[[#symbol <- c_catch]]

[[# AUTO-0007-0015 #]]:
[[# contains Symbol#symbol]]
[[#symbol matches INFINITY]]
[[#symbol <- C_INFINITY]]

[[# AUTO-0007-0016 #]]:
[[# contains StringSpec#stringspec]]
[[#stringspec <-  #stringspec  ]]

[[# AUTO-0008-0001 #]]:
[[# contains Stmt#stmt]]
[[#stmt matches Type#type Symbol#symbol = xmalloc(Expr#expr);]]
[[#stmt <- #type #symbol = (#type)xmalloc(#expr);]]

[[# AUTO-0008-0002 #]]:
[[# contains Stmt#stmt]]
[[#stmt matches Type#type Symbol#symbol = Expr#rhs;]] ||
[[#stmt matches Type#type Symbol#symbol;]]
[[#stmt set Type#symbol#type: #type]]

[[# AUTO-0008-0003 #]]:
[[# contains Expr#expr]]
[[#expr matches Symbol#symbol = Symbol#alloc(ArgExprs#argexprs)]]
[[#alloc matches malloc]] ||
[[#alloc matches xmalloc]] ||
[[#alloc matches xzalloc]] ||
[[#alloc matches xrealloc]] ||
[[#alloc matches xpalloc]] ||
[[#alloc matches alloca]] ||
[[#alloc matches SAFE_ALLOCA]] ||
[[#alloc matches SAFE_NALLOCA]]
[[#symbol#type matches Type#type *]]
[[#expr <- #symbol = (#symbol#type)#alloc(#argexprs)]]

[[# AUTO-0008-0004 #]]:
[[# contains Stmt#stmt]]
[[#stmt matches Type#type Symbol#symbol = Symbol#alloc(ArgExprs#argexprs);]]
[[#alloc matches malloc]] ||
[[#alloc matches xmalloc]] ||
[[#alloc matches xzalloc]]
[[#symbol#type matches Type#typeb *]]
[[#stmt <- #type #symbol = (#symbol#type)#alloc(#argexprs);]]

[[# XAUTO-0008-0005 #]]:
[[# contains Expr#expr]]
[[#expr matches Expr#symbol = Symbol#alloc(ArgExprs#argexprs)]]
[[#alloc matches malloc]] ||
[[#alloc matches xmalloc]] ||
[[#alloc matches xzalloc]] ||
[[#alloc matches xrealloc]] ||
[[#alloc matches xpalloc]] ||
[[#alloc matches xnmalloc]] ||
[[#alloc matches alloca]] ||
[[#alloc matches SAFE_ALLOCA]] ||
[[#alloc matches SAFE_NALLOCA]]
[[#expr <- #symbol = (typeof #symbol)#alloc(#argexprs)]]

[[# XAUTO-0008-0006 #]]:
[[# contains Stmt#stmt]]
[[#stmt matches Type#type Symbol#symbol = Symbol#alloc(ArgExprs#argexprs);]]
[[#alloc matches malloc]] ||
[[#alloc matches xmalloc]] ||
[[#alloc matches xzalloc]] ||
[[#alloc matches xrealloc]] ||
[[#alloc matches xpalloc]] ||
[[#alloc matches xnmalloc]] ||
[[#alloc matches alloca]] ||
[[#alloc matches SAFE_ALLOCA]] ||
[[#alloc matches SAFE_NALLOCA]]
[[#stmt <- #type #symbol = (typeof #symbol)#alloc(#argexprs);]]

[[# AUTO-0009-FLUSH #]]:

[[# AUTO-0010 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args LV arg #vector]]
[[#vector#run <- (__type__)ELisp_Vector_Handle #vector]]

[[# AUTO-0015 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args includes #run: (__type__)ptrdiff_t, (__type__)Lisp_Object *]]
[[#run <- (__type__)ELisp_Vector_Handle]]

[[# AUTO-0020 #]]:
[[# FunTypeD #funtyped]]
[[#funtyped#args LV arg #vector]]
[[#vector#run <- (__type__)ELisp_Vector_Handle #vector]]

[[# AUTO-0030 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args LV arg #vector]]
[[#fundef#body contains ArgExprs#argexprs]]
[[#argexprs includes #run: #vector#count, #vector]]
[[#run <- #vector]]

[[# AUTO-0040 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args LV arg #vector]]
[[#fundef#body contains Expr#callmany]]
[[#callmany matches CALLMANY(Symbol#f, #vector)]]
[[#callmany <- #f(#vector)]]

[[# AUTO-0045 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args LV arg #vector]]
[[#fundef#body contains Expr#expr]]
[[#expr matches #vector#count]]
[[#expr <- #vector.n]]

[[# AUTO-0050 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args LV arg #vector]]
[[#fundef#body contains Expr#expr]]
[[#expr matches ARRAYELTS(#vector)]]
[[#expr <- #vector.n]]

[[# AUTO-0051 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args LV arg #vector]]
[[#fundef#body contains Expr#expr]]
[[#expr matches sizeof(#vector)]] ||
[[#expr matches sizeof #vector]]
[[#expr <- #vector.n * sizeof((__type__)ELisp_Value)]]

[[# AUTO-0060 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args LV arg #vector]]
[[#fundef#body contains Symbol#countb]]
[[#countb matches #vector#count]]
[[#countb <- #vector.n]]

[[# AUTO-0070 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args LV arg #vector]]
[[#fundef#body contains Symbol#vec]]
[[#vec matches #vector]]
[[#vec <- #vector.vec]]

[[# AUTO-0080 #]]:
[[# contains Stmt#decl]]
[[#decl matches (__type__)Lisp_Object *Symbol#symbol;]]
[[#decl <- (__type__)ELisp_Pointer #symbol;]]

[[# AUTO-0083 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#decl]]
[[#decl matches (__type__)Lisp_Object const*Symbol#symbol;]]
[[#decl <- (__type__)ELisp_Pointer #symbol;]]

[[# AUTO-0085 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#decl]]
[[#decl matches (__type__)Lisp_Object *Symbol#symbol = ((__type__)Lisp_Object *)Expr#rhs;]]
[[#decl <- (__type__)ELisp_Pointer #symbol = ((__type__)ELisp_Pointer)#rhs;]]

[[# AUTO-0086 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#decl]]
[[#decl matches (__type__)Lisp_Object const *Symbol#symbol = ((__type__)Lisp_Object *)Expr#rhs;]]
[[#decl <- (__type__)ELisp_Pointer #symbol = ((__type__)ELisp_Pointer)#rhs;]]

[[# AUTO-0090 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Expr#cast]]
[[#cast matches ((__type__)Lisp_Object *)Expr#expr]]
[[#cast <- ((__type__)ELisp_Pointer)#expr]]

[[# AUTO-0093 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#arg]]
[[#arg matches (__type__)Lisp_Object *Symbol#symbol;]]
[[#arg <- (__type__)ELisp_Pointer #symbol;]]

[[# AUTO-0095 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args contains Arg#arg]]
[[#arg matches (__type__)Lisp_Object *Symbol#symbol]]
[[#arg <- (__type__)ELisp_Pointer #symbol]]

[[# AUTO-0096 #]]:
[[# FunTypeD #funtype]]
[[#funtype#args contains Arg#arg]]
[[#arg matches (__type__)Lisp_Object *Symbol#symbol]]
[[#arg <- (__type__)ELisp_Pointer #symbol]]

[[# AUTO-0097 #]]:
[[# FunTypeD #funtype]]
[[#funtype#args contains Arg#arg]]
[[#arg matches Type#type *]]
[[#type matches (__type__)Lisp_Object]]
[[#arg <- (__type__)ELisp_Pointer]]

[[# AUTO-0098 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args contains Arg#arg]]
[[#arg matches Type#type *]]
[[#type matches (__type__)Lisp_Object]]
[[#arg <- (__type__)ELisp_Pointer]]

[[# AUTO-0099 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args contains Arg#arg]]
[[#arg matches Type#type **]]
[[#type matches (__type__)Lisp_Object]]
[[#arg <- (__type__)ELisp_Pointer *]]

[[# AUTO-0100 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#decl]]
[[#decl matches (__type__)Lisp_Object *Symbol#symbol = Expr#rhs;]]
[[#decl <- (__type__)ELisp_Pointer #symbol = #rhs;]]

[[# AUTO-0101 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#decl]]
[[#decl matches (__type__)Lisp_Object *Symbol#symbol;]]
[[#decl <- (__type__)ELisp_Pointer #symbol;]]

[[# AUTO-0065 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#declinit]]
[[#declinit matches (__type__)Lisp_Object Symbol#symbol[] = { CExpr#cexpr };]] ||
[[#declinit matches (__type__)Lisp_Object Symbol#symbol[] = { CExpr#cexpr, };]]
[[#declinit <- ELisp_Array_Imm(#symbol, #cexpr);]]

[[# AUTO-0110 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args LV array arg #vector]]
[[#fundef#body contains Expr#expr]]
[[#expr matches ARRAYELTS(#vector)]]
[[#expr <- #vector.n]]

[[# AUTO-0120 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args LV array arg #vector]]
[[#fundef#body contains Expr#expr]]
[[#expr matches sizeof(#vector)]] ||
[[#expr matches sizeof #vector]]
[[#expr <- #vector.n * sizeof((__type__)ELisp_Value)]]

[[# AUTO-0140 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args LV array arg #vector]]
[[#fundef#body contains Symbol#vec]]
[[#vec matches #vector]]
[[#vec <- #vector.vec]]

[[# AUTO-0105 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args LV array arg #vector]]
[[#vector#run <- (__type__)ELisp_Vector_Handle #vector]]

[[# AUTO-0180 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#declinit]]
[[#declinit matches (__type__)Lisp_Object Symbol#symbol[] = { CExpr#cexpr };]] ||
[[#declinit matches (__type__)Lisp_Object Symbol#symbol[] = { CExpr#cexpr, };]]
[[#fundef#body contains Expr#expr]]
[[#expr matches CALLMANY(Symbol#f, #symbol)]]
[[#expr <- #f(#symbol)]]

[[# AUTO-0192 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#declinit]]
[[#declinit matches (__type__)Lisp_Object Symbol#symbol[] = { CExpr#cexpr };]] ||
[[#declinit matches (__type__)Lisp_Object Symbol#symbol[] = { CExpr#cexpr, };]]
[[#fundef#body contains Expr#expr]]
[[#expr matches ARRAYELTS(#symbol)]]
[[#expr <- #symbol.n]]

[[# AUTO-0195 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#declinit]]
[[#declinit matches (__type__)Lisp_Object Symbol#symbol[] = { CExpr#cexpr };]] ||
[[#declinit matches (__type__)Lisp_Object Symbol#symbol[] = { CExpr#cexpr, };]]
[[#fundef#body contains Symbol#symbolb]]
[[#symbolb matches #symbol]]
[[#symbolb <- #symbol.vec]]

[[# AUTO-0197 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#declinit]]
[[#declinit matches (__type__)Lisp_Object Symbol#vector[] = { CExpr#cexpr };]] ||
[[#declinit matches (__type__)Lisp_Object Symbol#vector[] = { CExpr#cexpr, };]]
[[#fundef#body contains ArgExprs#argexprs]]
[[#argexprs includes #run: ARRAYELTS(#vector), #vector]]
[[#run <- #vector]]

[[# AUTO-0200 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#decl]]
[[#decl matches (__type__)Lisp_Object Symbol#symbol[Expr#count];]]
[[#decl <- ELisp_Array(#symbol, #count);]]

[[# AUTO-0203 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#decl]]
[[#decl matches (__type__)Lisp_Object Symbol#symbol[Expr#count];]]
[[#fundef#body contains Expr#expr]]
[[#expr matches sizeof(#symbol)]] ||
[[#expr matches sizeof #symbol]]
[[#expr <- #symbol.n * sizeof((__type__)ELisp_Value)]]

[[# AUTO-0205 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#declinit]]
[[#declinit matches (__type__)Lisp_Object Symbol#symbol[Expr#count];]]
[[#fundef#body contains CExpr#expr]]
[[#expr matches CALLMANY(Symbol#f, #symbol)]]
[[#expr <- #f(#symbol)]]

[[# AUTO-0206 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#declinit]]
[[#declinit matches (__type__)Lisp_Object Symbol#symbol[Expr#count];]]
[[#fundef#body contains Expr#expr]]
[[#expr matches CALLMANY(Symbol#f, #symbol)]]
[[#expr <- #f(#symbol)]]

[[# AUTO-0210 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#decl]]
[[#decl matches (__type__)Lisp_Object Symbol#symbol[Expr#count];]]
[[#fundef#body contains Expr#expr]]
[[#expr matches ARRAYELTS(#symbol)]]
[[#expr <- #symbol.n]]

[[# AUTO-0215 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#decl]]
[[#decl matches (__type__)Lisp_Object Symbol#symbol[Expr#count];]]
[[#fundef#body contains CExpr#expr]]
[[#expr matches ARRAYELTS(#symbol)]]
[[#expr <- #symbol.n]]

[[# AUTO-0220 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#decl]]
[[#decl matches (__type__)Lisp_Object Symbol#symbol[Expr#count];]]
[[#fundef#body contains Expr#expr]]
[[#expr matches #symbol]]
[[#expr <- #symbol.vec]]

[[# AUTO-0225 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#decl]]
[[#decl matches (__type__)Lisp_Object Symbol#symbol[Expr#count];]]
[[#fundef#body contains CExpr#expr]]
[[#expr matches #symbol]]
[[#expr <- #symbol.vec]]

[[# AUTO-0230 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#decl]]
[[#decl matches (__type__)Lisp_Object Symbol#symbol;]]
[[#decl <- (__type__)ELisp_Value #symbol;]]

[[# AUTO-0240 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#decl]]
[[#decl matches Typed#typed = Expr#rhs;]]
[[#typed matches (__type__)Lisp_Object Symbol#symbol]]
[[#typed <- (__type__)ELisp_Value #symbol]]

[[# AUTO-0241 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#decl]]
[[#decl matches Typed#typed = Expr#rhs;]]
[[#typed matches (__type__)Lisp_Object volatile Symbol#symbol]]
[[#typed <- (__type__)ELisp_Value volatile #symbol]]

[[# AUTO-0299-FLUSH #]]:

[[# AUTO-0300 #]]:
[[# contains Stmt#stmt]]
[[#stmt matches (__type__)ELisp_Value Symbol#symbol;]]
[[# contains Symbol#symbolb]]
[[#symbolb matches #symbol]]
[[# set Type#symbolb#type: (__type__)ELisp_Value]]

[[# AUTO-030001 #]]:
[[# contains Stmt#stmt]]
[[#stmt matches (__type__)ELisp_Value Symbol#symbol = Expr#rhs;]]
[[# contains Symbol#symbolb]]
[[#symbolb matches #symbol]]
[[# set Type#symbolb#type: (__type__)ELisp_Value]]

[[# AUTO-03001 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args contains Arg#arg]]
[[#arg matches (__type__)ELisp_Handle Symbol#symbol]]
[[#fundef contains Symbol#symbolb]]
[[#symbolb matches #symbol]]
[[# set Type#symbolb#type: (__type__)ELisp_Value]]

[[# AUTO-03002 #]]:
[[# contains Stmt#stmt]]
[[#stmt matches (__type__)ELisp_Pointer Symbol#symbol;]]
[[# contains Symbol#symbolb]]
[[#symbolb matches #symbol]]
[[# set Type#symbolb#type: (__type__)ELisp_Pointer]]

[[# AUTO-030021 #]]:
[[# contains Stmt#stmt]]
[[#stmt matches (__type__)ELisp_Pointer Symbol#symbol = Expr#rhs;]]
[[# contains Symbol#symbolb]]
[[#symbolb matches #symbol]]
[[# set Type#symbolb#type: (__type__)ELisp_Pointer]]

[[# AUTO-03003 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args contains Arg#arg]]
[[#arg matches (__type__)ELisp_Pointer Symbol#symbol]]
[[#fundef contains Symbol#symbolb]]
[[#symbolb matches #symbol]]
[[# set Type#symbolb#type: (__type__)ELisp_Pointer]]

[[# AUTO-03004 #]]:
[[# contains Stmt#stmt]]
[[#stmt matches ELisp_Array(Symbol#symbol, Expr#);]]
[[# contains Symbol #symbolb]]
[[#symbolb matches #symbol]]
[[# set Type#symbolb#type: (__type__)ELisp_Array]]

[[# AUTO-03005 #]]:
[[# contains Stmt#stmt]]
[[#stmt matches ELisp_Array_Imm(Symbol#symbol, CExpr#);]]
[[# contains Symbol #symbolb]]
[[#symbolb matches #symbol]]
[[# set Type#symbolb#type: (__type__)ELisp_Array]]

[[# AUTO-03006 #]]:
[[# contains Stmt#stmt]]
[[#stmt matches (__type__)void *Symbol#symbol;]] ||
[[#stmt matches (__type__)void *Symbol#symbol = Expr#rhs;]]
[[# set Type#symbol#type: (__type__)void *]]

[[# AUTO-0301 #]]:
[[# contains Expr#expr]]
[[#expr matches Symbol#symbol]]
[[# set Type#expr#type: #symbol#type]]

[[# AUTO-030401 #]]:
[[# contains Expr#a]]
[[#a matches Qnil]] ||
[[#a matches Qt]] ||
[[#a matches BVAR(Expr#, Expr#)]] ||
[[#a matches XCAR(Expr#)]] ||
[[#a matches XCDR(Expr#)]] ||
[[#a matches make_number(Expr#)]]
[[# set Type#a#type: (__type__)ELisp_Value]]

[[# AUTO-030402 #]]:
[[# contains Expr#expr]]
[[#expr matches Expr#lhs = Expr#a ? Expr#b : Expr#c]]
[[#lhs#type matches (__type__)ELisp_Value]]
[[#expr <- #lhs = #a ? ELisp_Return_Value(#b) : ELisp_Return_Value(#c)]]

[[# AUTO-0304025 #]]:
[[# contains Stmt#stmt]]
[[#stmt matches Type#type Symbol#lhs = Expr#a ? Expr#b : Expr#c;]]
[[#type matches (__type__)ELisp_Value]]
[[#stmt <- #type #lhs = #a ? ELisp_Return_Value(#b) : ELisp_Return_Value(#c);]]

[[# AUTO-0305 #]]:
[[# contains Expr#expr]]
[[#expr matches Expr#a ? Expr#b : Expr#c]]
[[#b#type matches (__type__)ELisp_Value]] ||
[[#c#type matches (__type__)ELisp_Value]]
[[#b <- ELisp_Return_Value(#b)]]
[[#c <- ELisp_Return_Value(#c)]]

[[# AUTO-0306 #]]:
[[# contains Expr#expr]]
[[#expr matches Expr#a = Expr#b]]
[[#a matches *Expr#ptr]]
[[#ptr#type matches (__type__)ELisp_Pointer]]
[[#expr <- #ptr.set(#b)]]

[[# AUTO-0307 #]]:
[[# contains Expr#expr]]
[[#expr matches &Expr#ptr[Expr#index] ]]
[[#expr <- (#ptr + (#index))]]

[[# AUTO-03075 #]]:
[[# contains Expr#expr]]
[[#expr matches &Expr#ptr.vec[Expr#index] ]]
[[#expr <- (#ptr + (#index))]]

[[# AUTO-0308 #]]:
[[# contains Expr#expr]]
[[#expr matches Expr#a = Expr#b]]
[[#a nomatch Expr# = Expr#]]
[[#b nomatch Expr# = Expr#]]
[[#a matches Expr#ptr[Expr#index] ]]
[[#ptr#type matches (__type__)ELisp_Pointer]]
[[#expr <- #ptr.sref(#index, #b)]]

[[# AUTO-0309 #]]:
[[# contains Expr#expr]]
[[#expr matches Expr#ptr[Expr#index] ]]
[[#ptr#type matches (__type__)ELisp_Pointer]]
[[#expr <- #ptr.ref(#index)]]

[[# AUTO-0310 #]]:
[[# contains Expr#expr]]
[[#expr matches Expr#ptr.vec[Expr#index] ]]
[[#ptr#type matches (__type__)ELisp_Pointer]]
[[#expr <- #ptr.ref(#index)]]

[[# AUTO-03108 #]]:
[[# contains Expr#expr]]
[[#expr matches *Expr#ptr ]]
[[#ptr#type matches (__type__)ELisp_Pointer]]
[[#expr <- #ptr.ref(0)]]

[[# AUTO-0311 #]]:
[[# contains Expr#expr]]
[[#expr matches sizeof *#ptr]] ||
[[#expr matches sizeof(*#ptr)]] ||
[[#expr matches sizeof #ptr[0])]] ||
[[#expr matches sizeof(*#ptr[0])]]
[[#ptr#type matches (__type__)ELisp_Pointer]]
[[#expr <- sizeof((__type__)ELisp_Struct_Value)]]

[[# AUTO-0312-REPEAT #]]:
[[# contains Expr#expr]]
[[#expr matches Expr#a = Expr#b]]
[[#a matches Expr#ptr.vec[Expr#index] ]]
[[#expr <- #ptr.vec.sref(#index, #b)]]

[[# AUTO-0314-REPEAT #]]:
[[# contains Expr#expr]]
[[#expr matches Expr#ptr.vec[Expr#index] ]]
[[#expr <- #ptr.vec.ref(#index)]]

[[# AUTO-0315-REPEAT #]]:
[[# contains Expr#expr]]
[[#expr matches Expr#a = Expr#b]]
[[#a nomatch Expr# = Expr#]]
[[#b nomatch Expr# = Expr#]]
[[#a matches Expr#ptr.vec[Expr#index] ]]
[[#expr <- #ptr.vec.sref(#index, #b)]]

[[# AUTO-0316-REPEAT #]]:
[[# contains Expr#expr]]
[[#expr matches Expr#ptr.vec[Expr#index] ]]
[[#expr <- #ptr.vec.ref(#index)]]

[[# XXXAUTO-0248 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args contains Arg#arg]]
[[#arg matches (__type__)Lisp_Object Symbol#symbol]]
[[#fundef#body contains Expr#rwexpr]]
[[#rwexpr modifies #arg]]
[[#arg <- (__type__)ELisp_Handle_RW #symbol]]

[[# XXXAUTO-0249 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args contains Arg#arg]]
[[#arg matches (__type__)Lisp_Object Symbol#symbol]]
[[#fundef#body contains Expr#expr]]
[[#expr matches &#arg]]
[[#arg <- (__type__)ELisp_Handle_RW #symbol]]

[[# AUTO-0250 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args contains Arg#arg]]
[[#arg matches (__type__)Lisp_Object Symbol#symbol]]
[[#arg <- (__type__)ELisp_Handle #symbol]]

[[# AUTO-0255 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args contains Arg#arg]]
[[#arg matches (__type__)Lisp_Object]]
[[#arg <- (__type__)ELisp_Handle]]

[[# AUTO-02552 #]]:
[[# FunTypeD #fundef]]
[[#fundef#args contains Arg#arg]]
[[#arg matches Type#type Symbol#symbol]]
[[#type matches (__type__)Lisp_Object]]
[[#type <- (__type__)ELisp_Handle]]

[[# AUTO-0256 #]]:
[[# FunTypeD #fundef]]
[[#fundef#args contains Arg#arg]]
[[#arg matches Type#type]]
[[#type matches (__type__)Lisp_Object]]
[[#arg <- (__type__)ELisp_Handle]]

[[# AUTO-0260 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#ret matches (__type__)Lisp_Object]]
[[#fundef#ret <- (__type__)ELisp_Return_Value]]

[[# AUTO-0265 #]]:
[[# FunTypeD #funtyped]]
[[#funtyped#ret matches (__type__)Lisp_Object]]
[[#funtyped#ret <- (__type__)ELisp_Return_Value]]

[[# AUTO-0270 #]]:
[[# contains StructBody#structbody]]
[[#structbody contains Stmt#decl]]
[[#decl matches (__type__)Lisp_Object Symbol#symbol;]]
[[#decl <- (__type__)ELisp_Struct_Value #symbol;]]

[[# AUTO-0272 #]]:
[[# contains StructBody#structbody]]
[[#structbody contains Stmt#decl]]
[[#decl matches extern (__type__)Lisp_Object Symbol#symbol;]]
[[#decl <- extern (__type__)ELisp_Struct_Value #symbol;]]

[[# AUTO-0275 #]]:
[[# contains StructBody#structbody]]
[[#structbody contains Stmt#decl]]
[[#decl matches (__type__)Lisp_Object Symbol#symbol[Expr#expr];]]
[[#decl <- (__type__)ELisp_Struct_Value #symbol[#expr];]]

[[# AUTO-0277 #]]:
[[# contains StructBody#structbody]]
[[#structbody contains Stmt#decl]]
[[#decl matches extern (__type__)Lisp_Object Symbol#symbol[Expr#expr];]]
[[#decl <- extern (__type__)ELisp_Struct_Value #symbol[#expr];]]

[[# AUTO-0280 #]]:
[[# contains Stmt#decl]]
[[#decl matches static (__type__)Lisp_Object Symbol#symbol;]]
[[#decl <- static (__type__)ELisp_Heap_Value #symbol = builtin_lisp_symbol(0);]]

[[# AUTO-0285 #]]:
[[# contains Stmt#decl]]
[[#decl matches static (__type__)Lisp_Object Symbol#symbol[Expr#expr];]]
[[#decl <- static (__type__)ELisp_Heap_Value #symbol[#expr];]]

[[# AUTO-0290 #]]:
[[# contains Stmt#decl]]
[[#decl matches (__type__)Lisp_Object Symbol#symbol;]]
[[#decl <- (__type__)ELisp_Heap_Value #symbol = builtin_lisp_symbol(0);]]

[[# AUTO-0292 #]]:
[[# contains Stmt#decl]]
[[#decl matches (__type__)Lisp_Object Symbol#symbol[Expr#expr];]]
[[#decl <- (__type__)ELisp_Heap_Value #symbol[#expr];]]

[[# AUTO-0293 #]]:
[[# contains Stmt#decl]]
[[#decl matches (__type__)Lisp_Object Symbol#symbol[Expr#expr] Attrs#attrs;]]
[[#decl <- (__type__)ELisp_Heap_Value #symbol[#expr] #attrs;]]

[[# AUTO-0295 #]]:
[[# contains Stmt#decl]]
[[#decl matches extern (__type__)Lisp_Object Symbol#symbol;]]
[[#decl <- extern (__type__)ELisp_Heap_Value #symbol;]]

[[# AUTO-0297-FLUSH #]]:
[[# contains Stmt#stmt]]

[[# AUTO-0400 #]]:
[[# contains Stmt#decl]]
[[#decl matches typedef struct Symbol#struct BStructBody#body Symbol#symbol;]] ||
[[#decl matches typedef struct (__type__)Symbol#struct BStructBody#body Symbol#symbol;]] ||
[[#decl matches typedef struct Symbol#struct BStructBody#body (__type__)Symbol#symbol;]] ||
[[#decl matches typedef struct Type#struct BStructBody#body Type#symbol;]] ||
[[#decl matches typedef struct (__type__)Symbol#struct BStructBody#body (__type__)Symbol#symbol;]]
[[#decl <- typedef struct #struct #symbol;]]
[[# pre-chunk struct #struct #body;]]

[[# AUTO-0410 #]]:
[[# contains Stmt#sd]]
[[#sd contains Stmt#sdb]]
[[#sdb matches struct Symbol#symbol BStructBody#body;]]
[[#sdb <- ]]
[[# pre-chunk struct #symbol #body;]]

[[# AUTO-0420 #]]:
[[# contains StructBody#outerstruct]]
[[#outerstruct contains Stmt#sd]]
[[#sd matches struct Symbol#struct BStructBody#body Symbol#symbol;]]
[[#sd <- struct #struct #symbol;]]
[[# pre-chunk struct #struct #body;]]

[[# AUTO-0425 #]]:
[[# contains StructBody#outerstruct]]
[[#outerstruct contains Stmt#sd]]
[[#sd matches struct (__type__)Symbol#struct BStructBody#body Symbol#symbol;]]
[[#sd <- struct #struct #symbol;]]
[[# pre-chunk struct (__type__)#struct #body;]]

[[# AUTO-0430 #]]:
[[# contains StructBody#outerstruct]]
[[#outerstruct contains Stmt#sd]]
[[#sd matches union Symbol#struct BStructBody#body Symbol#symbol;]]
[[#sd <- union #struct #symbol;]]
[[# pre-chunk union #struct #body;]]

[[# XXXAUTO-0500 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#psymbol matches Symbol#symbol]]
[[#fundef#ret matches Type#ret]]
[[# set RetType#symbol#G#rettype: #ret]]

[[# XXXAUTO-0510 #]]:
[[# contains TLS#stmt]]
[[#stmt matches Type#type Symbol#symbol;]] ||
[[#stmt matches Type#type Symbol#symbol = Expr#;]]
[[# set Type#symbol#G#type: #type]]

[[# AUTO-0600 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args contains Arg#arg]]
[[#arg matches (__type__)ELisp_Handle Symbol#symbol]]
[[#fundef#body matches { Stmts#stmts }]]
[[#fundef#body contains Expr#expr]]
[[#expr modifies #rwexpr]]
[[#rwexpr matches #symbol]]
[[#stmts <<- (__type__)ELisp_Value #symbol = ARG(#symbol);]]
[[#arg <- (__type__)ELisp_Handle ARG(#symbol)]]

[[#expr xsets #rwexpr]]:
[[#rwexpr matches Symbol#symbol(Expr#expr, Expr#exprb)]]
[[#symbol check "#symbol" =~ /^XSET/]]
[[#symbol check "#symbol" !~ /^XSETC[AD]R/]]

[[#expr coerces #rwexpr]]:
[[#rwexpr matches Symbol#symbolb(Expr#expr)]]
[[#symbol check "#symbolb" =~ /^(CHECK_NUMBER_COERCE_MARKER|CHECK_NUMBER_OR_FLOAT_COERCE_MARKER)$/]]

[[#expr modifies #rwexpr]]:
[[#rwexpr matches Expr#expr ModOp# Expr#]] ||
[[#rwexpr matches Expr#expr ModPostOp#]] ||
[[#rwexpr matches ModUnOp# Expr#expr]] ||
[[#rwexpr matches & Expr#expr]] ||
[[#rwexpr matches FOR_EACH_FRAME(Expr#, Expr#expr)]] ||
[[#rwexpr xsets #expr]] ||
[[#rwexpr coerces #expr]]

[[# AUTO-0700 #]]:
[[# contains Expr#expr]]
[[#expr matches swapfield_(Expr#exprb, Symbol#symbol)]]
[[#symbol matches (__type__)Lisp_Object]]
[[#symbol <- (__type__)ELisp_Struct_Value]]

[[# AUTO-0750 #]]:
[[# contains Stmt#decl]]
[[#decl matches Type#type Symbol#symbol;]]
[[#type matches (__type__)ELisp_Pointer]]
[[# contains Expr#expr]]
[[#expr matches SAFE_ALLOCA_LISP(#symbol, Expr#size)]]
[[#type <- (__type__)ELisp_Dynvector]]
[[#expr <- #symbol.resize(#size)]]

[[# AUTO-0799-FLUSH #]]:

[[# AUTO-07998 #]]:
[[# contains Expr#expr]]
[[#expr matches Qt]] ||
[[#expr matches Qnil]] ||
[[#expr matches BVAR(Expr#, Expr#)]]
[[# set Type#expr#type: (__type__)ELisp_Struct_Value]]
[[#expr set Type#expr#type: (__type__)ELisp_Struct_Value]]

[[# AUTO-07999 #]]:
[[# contains Expr#a]]
[[#a matches XCAR(Expr#)]] ||
[[#a matches XCDR(Expr#)]] ||
[[#a matches make_number(Expr#)]]
[[# set Type#a#type: (__type__)ELisp_Return_Value]]
[[#a set Type#a#type: (__type__)ELisp_Return_Value]]

[[# XAUTO-0800 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args element Expr#n: Arg#arg]]
[[#arg matches (__type__)ELisp_Handle Symbol#symbol]]
[[#fundef#body contains Expr#expr]]
[[#expr matches #symbol]]
[[# set Type#expr#type: (__type__)ELisp_Handle]]

[[# XAUTO-0801 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#decl]]
[[#decl matches (__type__)ELisp_Value Symbol#symbol;]] ||
[[#decl matches (__type__)ELisp_Value Symbol#symbol = Expr#rhs;]] ||
[[#decl matches AUTO_CONS(Symbol#symbol, ArgExprs#);]] ||
[[#decl matches AUTO_LIST1(Symbol#symbol, ArgExprs#);]] ||
[[#decl matches AUTO_LIST2(Symbol#symbol, ArgExprs#);]] ||
[[#decl matches AUTO_LIST3(Symbol#symbol, ArgExprs#);]] ||
[[#decl matches AUTO_LIST4(Symbol#symbol, ArgExprs#);]] ||
[[#decl matches AUTO_LIST5(Symbol#symbol, ArgExprs#);]] ||
[[#decl matches AUTO_STRING(Symbol#symbol, Expr#);]] ||
[[#decl matches AUTO_STRING_WITH_LEN(Symbol#symbol, Expr#str, Expr#len);]]
[[#fundef#body contains Expr#exprb]]
[[#exprb matches #symbol]]
[[# set Type#exprb#type: (__type__)ELisp_Value]]

[[# XAUTO-08015 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Expr#expr]]
[[#expr matches AUTO_FRAME_ARG (Symbol#symbol, Expr#, Expr#)]] ||
[[#expr matches AUTO_CONS_EXPR (Symbol#symbol, Expr#)]] ||
[[#expr matches AUTO_LIST1 (Symbol#symbol, Expr#)]] ||
[[#expr matches AUTO_LIST2 (Symbol#symbol, Expr#, Expr#)]] ||
[[#expr matches AUTO_LIST3 (Symbol#symbol, Expr#, Expr#, Expr#)]] ||
[[#expr matches AUTO_LIST4 (Symbol#symbol, Expr#, Expr#, Expr#, Expr#)]]
[[#fundef#body contains Expr#exprb]]
[[#exprb matches #symbol]]
[[# set Type#exprb#type: (__type__)ELisp_Value]]

[[# XAUTO-0802 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Expr#expr]]
[[#expr matches Symbol#symbol(ArgExprs#)]]
[[#symbol check $main::returns->{#symbol} eq "Lisp_Object"]]
[[# set Type#expr#type: (__type__)ELisp_Return_Value]]

[[# XAUTO-0803 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Expr#expr]]
[[#expr matches Symbol#symbol(ArgExprs#)]]
[[#symbol check $main::returns->{#symbol} eq "void *"]]
[[# set Type#expr#type: (__type__)void *]]

[[# XAUTO-0804 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Expr#expr]]
[[#expr matches Expr#lhs = Expr#rhs]]
[[#rhs#type matches (__type__)void *]]
[[#rhs <- (typeof (#lhs))#rhs]]

[[# XAUTO-08045 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#body contains Stmt#declinit]]
[[#declinit matches Type#type Symbol#lhs = Expr#rhs;]]
[[#rhs#type matches (__type__)void *]]
[[#rhs <- (typeof (#lhs))#rhs]]

[[# XAUTO-0805 #]]:
[[# contains Expr#expr]]
[[#expr matches Symbol#symbol(ArgExprs#argexprs)]]
[[#symbol check "#symbol" !~ /^L(SH|HH|VH|RH)$/]]
[[#symbol nomatch LISPSYM_INITIALLY]]
[[#argexprs element Expr#n: ArgExpr#argexpr]]
[[#symbol check "#symbol" !~ /XSET/]] ||
[[#symbol check "#symbol" =~ /XSETC[AD]R/]] ||
[[#n check #n > 0]]
[[#argexpr nomatch Symbol#symbolb(ArgExprs#argexprsb)]] ||
[[#symbolb check "#symbolb" !~ /^(L(SH|HH|VH|RH)|LISPSYM_INITIALLY)$/]]
[[#symbol check $main::accepts->{#symbol}[#n] eq "Lisp_Object" ]]
[[#argexpr matches Expr#exprb]]
[[#exprb#type matches (__type__)ELisp_Value]]
[[#argexpr <- LVH(#argexpr)]]

[[# XAUTO-0806 #]]:
[[# contains Expr#expr]]
[[#expr matches Symbol#symbol(ArgExprs#argexprs)]]
[[#symbol check "#symbol" !~ /^L(SH|HH|VH|RH)$/]]
[[#symbol nomatch LISPSYM_INITIALLY]]
[[#argexprs element Expr#n: ArgExpr#argexpr]]
[[#symbol check "#symbol" !~ /XSET/]] ||
[[#symbol check "#symbol" =~ /XSETC[AD]R/]] ||
[[#n check #n > 0]]
[[#argexpr nomatch Symbol#symbolb(ArgExprs#argexprsb)]] ||
[[#symbolb check "#symbolb" !~ /^(L(SH|HH|VH|RH)|LISPSYM_INITIALLY)$/]]
[[#symbol check $main::accepts->{#symbol}[#n] eq "Lisp_Object"]]
[[#argexpr matches Expr#exprb]]
[[#exprb#type matches (__type__)ELisp_Handle]]
[[#argexpr <- LHH(#argexpr)]]

[[# XAUTO-0806125 #]]:
[[# contains Expr#a]]
[[#a contains Expr#b]]
[[#b equate-or #b#cfe: #a]]

[[# XAUTO-080625 #]]:
[[# contains BBody#stmt]]
[[#stmt matches { Stmts#stmts }]]
[[#stmt contains Expr#expr]]
[[#expr equate Stmts#expr#ics: #stmts]]

[[# XAUTO-08065 #]]:
[[# contains Stmt#stmt]]
[[#stmt matches { Stmts#stmts }]]
[[#stmt contains Expr#expr]]
[[#expr equate Stmts#expr#ics: #stmts]]

[[# XAUTO-080675 #]]:
[[# contains Stmt#stmt]]
[[#stmt contains Expr#expr]]
[[#expr equate Stmts#expr#icss: #stmt]]

[[# XAUTO-0807 #]]:
[[# contains Expr#expr]]
[[#expr matches Symbol#symbol(ArgExprs#argexprs)]]
[[#symbol check "#symbol" !~ /^L(SH|HH|VH|RH)$/]]
[[#symbol nomatch LISPSYM_INITIALLY]]
[[#argexprs element Expr#n: ArgExpr#argexpr]]
[[#argexpr matches Expr#exprb]]
[[#exprb#type matches (__type__)ELisp_Return_Value]]
[[#argexpr free]]
[[#symbol check "#symbol" !~ /XSET/]] ||
[[#symbol check "#symbol" =~ /XSETC[AD]R/]] ||
[[#n check #n > 0]]
[[#argexpr nomatch Symbol#symbolc = Expr#exprd]] ||
[[#symbolc check "#symbol" !~ /^__u_/]]
[[#argexpr nomatch Symbol#symbolb(ArgExprs#argexprsb)]] ||
[[#symbolb check "#symbolb" !~ /^(ELisp_Handle|L(SH|HH|VH|RH)|LISPSYM_INITIALLY)$/]]
[[#symbol check $main::accepts->{#symbol}[#n] eq "Lisp_Object"]]
[[#argexpr free]]
[[#argexpr <- LRH(#argexpr)]]

[[# XAUTO-08075 #]]:
[[# contains Expr#expr]]
[[#expr matches Symbol#symbol(ArgExprs#argexprs)]]
[[#symbol check "#symbol" !~ /^ELisp_Handle|L(SH|HH|VH|RH)$/]]
[[#symbol nomatch LISPSYM_INITIALLY]]
[[#argexprs element Expr#n: ArgExpr#argexpr]]
[[#argexpr matches Expr#exprb]]
[[#exprb#type matches (__type__)ELisp_Return_Value]]
[[#argexpr free]]
[[#symbol check "#symbol" !~ /XSET/]] ||
[[#symbol check "#symbol" =~ /XSETC[AD]R/]] ||
[[#n check #n > 0]]
[[#argexpr matches Symbol#symbolb.vec.ref(ArgExprs#)]]
[[#symbol check $main::accepts->{#symbol}[#n] eq "Lisp_Object"]]
[[#argexpr free]]
[[#argexpr <- LRH(#argexpr)]]

[[# XAUTO-0808 #]]:
[[# contains Expr#expr]]
[[#expr matches Symbol#symbol(ArgExprs#argexprs)]]
[[#symbol check "#symbol" !~ /^L(SH|HH|VH|RH)$/]]
[[#symbol nomatch LISPSYM_INITIALLY]]
[[#argexprs element Expr#n: ArgExpr#argexpr]]
[[#argexpr free]]
[[#symbol check "#symbol" !~ /XSET/]] ||
[[#symbol check "#symbol" =~ /XSETC[AD]R/]] ||
[[#n check #n > 0]]
[[#argexpr nomatch Symbol#symbolc = Expr#exprc]]
[[#argexpr nomatch Symbol#symbolb(ArgExprs#argexprsb)]] ||
[[#symbolb check "#symbolb" !~ /^(ELisp_Handle|L(SH|HH|VH|RH)|LISPSYM_INITIALLY)$/]]
[[#symbol check $main::accepts->{#symbol}[#n] eq "Lisp_Object"]]
[[#argexpr matches Expr#exprb]]
[[#argexpr free]]
[[#argexpr <- LRH(#argexpr)]]

[[# XAUTO-080825 #]]:
[[# contains Stmt#stmt]]
[[#stmt matches for ((__type__)ELisp_Value Symbol#symbol = Expr#rhs; Expr#expra; Expr#exprb) Stmt#inner]]
[[#stmt free]]
[[#expra#ics <<- (__type__)ELisp_Value #symbol;]]
[[#stmt <- for (#symbol = #rhs; #expra; #exprb) #inner]]

[[# XAUTO-08085 #]]:
[[# contains Stmt#stmt]]
[[#stmt matches (__type__)ELisp_Value Symbol#symbol = Expr#rhs;]]
[[#stmt <- (__type__)ELisp_Value #symbol; #symbol = #rhs;]]

[[# XAUTO-0809-FLUSH #]]:

[[#dummy the eighthundreds #chunk]]:
[[#chunk call: XAUTO-0800 #dummy]]
[[#chunk call: XAUTO-0801 #dummy]]
[[#chunk call: XAUTO-08015 #dummy]]
[[#chunk call: XAUTO-0802 #dummy]]
[[#chunk call: XAUTO-0803 #dummy]]
[[#chunk call: XAUTO-0805 #dummy]]
[[#chunk call: XAUTO-0806 #dummy]]
[[#chunk call: XAUTO-0806125 #dummy]]
[[#chunk call: XAUTO-080625 #dummy]]
[[#chunk call: XAUTO-0807 #dummy]]
[[#chunk call: XAUTO-08075 #dummy]]
[[#chunk call: XAUTO-0808 #dummy]]
[[#chunk call: XAUTO-080825 #dummy]]
[[#chunk call: XAUTO-08085 #dummy]]
[[#chunk call: XAUTO-0804 #dummy]]
[[#chunk call: XAUTO-08045 #dummy]]

[[#dummy AUTO-0890-REPEAT #chunk]]:
[[#chunk the eighthundreds #dummy]]

[[# AUTO-08905-FLUSH #]]:

[[# XXXAUTO-0000 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args contains Arg#arg]]
[[#arg matches (__type__)Lisp_Object *Symbol#symbol]]
[[#fundef#body contains Expr#rwexpr]]
[[#rwexpr modifies #expr]]
[[#expr matches *#symbol]] ||
[[#expr matches #symbol[Expr#]]]
[[#arg <- (__type__)ELisp_Pointer_RW #symbol]]

[[# XXXAUTO-0001 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args contains Arg#arg]]
[[#arg matches (__type__)Lisp_Object *Symbol#symbol]]
[[#fundef#body contains Expr#expr]]
[[#expr matches &#symbol]]
[[#arg <- (__type__)ELisp_Pointer_RW #symbol]]

[[# XXXAUTO-0002 #]]:
[[# FunctionDefinition #fundef]]
[[#fundef#args contains Arg#arg]]
[[#arg matches (__type__)Lisp_Object *Symbol#symbol]]
[[#arg <- (__type__)ELisp_Pointer_RO #symbol]]

[[# find structs #]]:
EOF

my $global_defns;

sub read_globals {
    my ($cu) = @_;
    my $global_text = "";
    for my $path ("chunkl.all") {
        $global_text .= read_file($path);
    }

    my %returns;
    my %accepts;
    eval $global_text;
    $main::returns = \%returns;
    $main::accepts = \%accepts;
}

my %globals;
my $count = 0;

sub update_globals {
    my ($cu, $processor) = @_;
    for my $symbol (keys %$EmacsCTree::globals) {
        next if $symbol eq "" or $symbol =~ /^#/;

        $globals{$symbol} = "";
        for my $key (keys %{$EmacsCTree::globals->{$symbol}}) {
            next if $key eq "";
            my $val = $EmacsCTree::globals->{$symbol}->{$key}->{""};
            my $str = $processor->rec_print($val);
            my $type = $val->type;
            $globals{$symbol} .=
                "[[global-$cu-$count]]:\n" .
                "[[# set Symbol#tmp: " . $symbol . "]]\n" .
                "[[# set $type#tmp#G#$key: $str]]\n\n";
            $count++;
        }
    }
}

sub write_globals {
    my ($cu) = @_;
    for my $key (sort keys %globals) {
        print $globals{$key};
    }
}

sub test_replacement {
    my ($val, @repl) = @_;
    my $repl_ok = [ $val->ctree->{start}, $val->ctree->{length} ];

    for my $repl (@repl) {
        return 0
            if ($repl_ok->[0] + $repl_ok->[1] > $repl->[0] &&
                $repl->[0] + $repl->[1] > $repl_ok->[0]);
    }

    return 1;
}

sub perform_replacements {
    my ($chunk, @repl) = @_;
    my @repl_ok;
  outer:
    for my $repl (@repl) {
        for my $repl_ok (@repl_ok) {
            next outer
                if ($repl_ok->[0] + $repl_ok->[1] > $repl->[0] &&
                    $repl->[0] + $repl->[1] > $repl_ok->[0]);
            next outer
                if ($repl_ok->[0] == $repl->[0] &&
                    $repl_ok->[1] == $repl->[1] &&
                    $repl_ok->[1] == 0 &&
                    $repl_ok->[2] eq $repl->[2]);
        }
        push @repl_ok, $repl;
  }

    my @repls = sort { ($a->[0] <=> $b->[0]) || ($a->[2] cmp $b->[2]) } @repl_ok;

    my $last = 1e9;
    for my $repl (reverse @repls) {
        if ($repl->[0] + $repl->[1] > $last) {
            warn "overlapping repls";
        }
        substr $chunk, $repl->[0], $repl->[1], $repl->[2];
        if ($repl->[2] eq "") {
            my $bridge = substr $chunk, $repl->[0] - 1, 2;
            if (substr($bridge, 1) eq " ") {
                substr $chunk, $repl->[0] - 1, 2, substr($bridge, 0, 1);
            }
        }
        $last = $repl->[0];
    }

    return $chunk;
}

my $cu;

$cu = $ARGV[0] if ($ARGV[0]);

my @chunks = Chunker::chunks();

my %timebyrule;

my $defns = (grep { $_ eq "--header" } @ARGV) ? $defns_header : $defns_main;
my $nomd5 = grep { $_ eq "--nomd5" } @ARGV;
read_globals($cu) if $cu && $defns == $defns_main;

use Digest::MD5 qw(md5_hex);
use File::Slurp qw(read_file write_file);

my @md5s;

my $gstarttime = time;
for my $chunk (@chunks) {
    my $counter = 10;
    my $md5 = md5_hex($chunk);
    push @md5s, $md5;
    if (!$nomd5 && $defns == $defns_main && -e "chunkl-cache/main/$md5") {
        print read_file("chunkl-cache/main/$md5");
        next;
    }
    $chunk =~ s/^(\#[ \t]*include[ \t]+)TERM_HEADER.*$/$1\"gtkutil.h.hh\"/mg;
    $chunk =~ s/^(\#[ \t]*include[ \t]+)\"(.*\.h)\"/$1\"$2.hh\"/mg;

    for my $type (@EmacsCGrammar::types) {
        $chunk =~ s/\b$type\b/(__type__)$type/g;
        $chunk =~ s/\(__type__\)($type\([^_*])/$1/g;
        $chunk =~ s/\(__type__\)($type \([^_*])/$1/g;
        $chunk =~ s/\(__type__\)\(__type__\)/(__type__)/g;
    }

    my @prechunks;
    my $output = "";
    eval {
        my @repl;
        my $ctree = EmacsCParser::parse_verbatim_ctree("Chunk", \$chunk);
        for my $key (sort keys %$defns) {
            next unless $key =~ /^AUTO/;
            my $start = time();
            my $flush = 0;
            while (1) {
                # warn $key;
                # warn $key . $chunk;
                if ($key =~ /^AUTO.*FLUSH$/ or $flush) {
                    $chunk = perform_replacements($chunk, @repl);
                    @repl = ();
                    $ctree = EmacsCParser::parse_verbatim_ctree("Chunk", \$chunk);
                    #warn $ctree->debug2;
                    $flush = 0;
                }
                my $processor = Processor->new($ctree, $defns, sub { push @repl, @_; }, { prechunk => sub { push @prechunks, $_[0] }, flush => sub { $flush = 1 }, free => sub { test_replacement($_[0], @repl); } }, { "Expr#CU" => "\"$cu\"" }, \$counter, \$output);
                my $outvar = PointedHash->new();
                $processor->run($defns->{$key}, "", "#dummy")->step(0);
                # update_globals($cu, $processor) if $defns == $defns_header && $cu;
                if ($key =~ /^AUTO.*FLUSH$/ or $flush) {
                    $chunk = perform_replacements($chunk, @repl);
                    @repl = ();
                    $ctree = EmacsCParser::parse_verbatim_ctree("Chunk", \$chunk);
                    $flush = 0;
                }
                if ($key =~ /^AUTO.*REPEAT$/) {
                    $chunk = perform_replacements($chunk, @repl);
                    $ctree = EmacsCParser::parse_verbatim_ctree("Chunk", \$chunk);
                    last if @repl == 0;
                    @repl = ();
                } else {
                    last;
                }
            }
            my $end = time();

            $timebyrule{$key} += ($end - $start);
        }

        $chunk = perform_replacements($chunk, @repl);
    };
    warn "$cu:\n" . $@ if $@;

    if ($defns == $defns_header) {
        mkdir("chunkl-cache/header");
        write_file("chunkl-cache/header/$md5", "$output\n");
        print "$output\n";
        next;
    }

    my %prechunks;
    for my $prechunk (@prechunks) {
        if ($prechunks{$prechunk}++ == 0) {
            $chunk = $prechunk . "\n\n" . $chunk;
        }
    }

    $chunk =~ s/\(__type__\)//g;

    print $chunk;

    mkdir("chunkl-cache/main");
    write_file("chunkl-cache/main/$md5", $chunk) if $cu && !$nomd5;
}

if ($cu) {
    my $fh;
    open $fh, ">stats/$cu";
    for my $key (sort { $timebyrule{$b} <=> $timebyrule{$a} } keys %timebyrule) {
        print $fh "$key took " . $timebyrule{$key} . " s\n";
    }
    close $fh;

    my $fh;
    open $fh, ">chunks/$cu";
    for my $md5 (@md5s) {
        print $fh "$md5\n";
    }
    close $fh;
}

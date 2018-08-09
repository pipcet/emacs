// shell g++ -ggdb -g3 -std=c++11 -I ../src/ -I ../js/dist/include/ ./js.cpp -L ../js/dist/bin/ -lz -lpthread -ldl -lmozjs-58a1 -Wl,--whole-archive ../js/mozglue/build/libmozglue.a -Wl,--no-whole-archive -pthread
#include "config.h.hh"

//#define DEBUG
#include "js-config.h"
#include "jsapi.h"

#include "js/Class.h"
#include "js/Initialization.h"
#include "js/RootingAPI.h"
#include "js/Conversions.h" // as of SpiderMonkey 38; previously in jsapi.h

#include "thread.h.hh"
#include "jslisp.hh"

#include "frame.h.hh"
#include "intervals.h.hh"
#include "keyboard.h.hh"
#include "buffer.h.hh"
#include "puresize.h.hh"

sys_jmp_buf *catchall_jmpbuf;
sys_jmp_buf *catchall_real_jmpbuf;
int catchall_real_value;

typedef int64_t EMACS_INT;
typedef uint64_t EMACS_UINT;

extern JSClass elisp_exception_class;
static JSClassOps cons_class_ops =
  {
   NULL, NULL, NULL, NULL,
   NULL, NULL, NULL,
  };

static const JSClass cons_class = {
                                   "ELisp_Cons",
                                   JSCLASS_HAS_RESERVED_SLOTS(2),
                                   &cons_class_ops,
};

static bool
cons_get_property(JSContext *cx, JS::HandleObject obj, JS::HandleId id,
                  JS::MutableHandleValue vp)
{
  if (!JSID_IS_INT(id))
    return false;

  vp.set(JS_GetReservedSlot(obj, JSID_TO_INT(id) ? 1 : 0));

  return true;
}

static bool
cons_construct(JSContext *cx, unsigned argc, JS::Value* vp)
{
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JSObject *obj = JS_NewObjectForConstructor(cx, &cons_class, args);
  JS::RootedObject o(cx, obj);
  JS_SetElement(cx, o, 0, args[0]);
  JS_SetElement(cx, o, 1, args[1]);

  args.rval().setObject(*obj);

  return true;
}

static void
string_finalize(JSFreeOp* freeop, JSObject *obj)
{
  //JS_freeop(freeop, (void *)JS_GetPrivate(obj));
}

static bool
string_toString(JSContext* cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args.thisv().isObject())
    return false;

  JS::RootedObject obj(cx, &args.thisv().toObject());
  char *bytes = (char *)JS_GetPrivate(obj);
  JS::RootedString res(cx, JS_NewStringCopyZ(cx, bytes));
  if (!res)
    return false;

  args.rval().setString(res);

  return true;
}

static bool
string_intervals(JSContext* cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args.thisv().isObject())
    return false;

  JS::RootedObject obj(cx, &args.thisv().toObject());

  args.rval().set(JS_GetReservedSlot(obj, 0));

  return true;
}

static bool
string_size(JSContext* cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args.thisv().isObject())
    return false;

  JS::RootedObject obj(cx, &args.thisv().toObject());

  args.rval().set(JS_GetReservedSlot(obj, 1));

  return true;
}

static bool
string_size_byte(JSContext* cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args.thisv().isObject())
    return false;

  JS::RootedObject obj(cx, &args.thisv().toObject());

  args.rval().set(JS_GetReservedSlot(obj, 2));

  return true;
}

static JSClassOps string_class_ops =
  {
   NULL, NULL, NULL, NULL,
   NULL, NULL, string_finalize
  };

static JSFunctionSpec string_fns[] = {
                                      JS_FN("toString", string_toString, 0, 0),
                                      JS_FS_END
};

static JSPropertySpec string_props[] = {
                                        JS_PSG("intervals", string_intervals, JSPROP_ENUMERATE|JSPROP_PERMANENT),
                                        JS_PSG("size", string_size, JSPROP_ENUMERATE|JSPROP_PERMANENT),
                                        JS_PSG("size_byte", string_size_byte, JSPROP_ENUMERATE|JSPROP_PERMANENT),
                                        JS_PS_END
};

static JSClass string_class = {
                               "ELisp_String",
                               JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(3) | JSCLASS_FOREGROUND_FINALIZE,
                               &string_class_ops,
};

static bool
string_construct(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JSObject *obj = JS_NewObjectForConstructor(cx, &string_class, args);;
  JS::RootedObject o(cx, obj);

  JS::RootedString str(cx, JS::ToString(cx, args[0]));

  if (!str)
    return false;

  char *bytes = JS_EncodeStringToUTF8(cx, str);

  if (!bytes)
    return false;

  JS_SetPrivate(obj, bytes);

  JS_SetReservedSlot(obj, 1, JS::NumberValue(strlen(bytes)));
  JS_SetReservedSlot(obj, 2, JS::NumberValue(1));

  args.rval().setObject(*obj);

  return true;
}

static void
marker_finalize(JSFreeOp* freeop, JSObject* obj)
{
  // unchain_marker
  // free C struct
}

static JSClassOps marker_class_ops = {
                                      NULL, NULL, NULL, NULL,
                                      NULL, NULL, marker_finalize,
};

static JSClass marker_class = {
                               "ELisp_Marker",
                               JSCLASS_HAS_PRIVATE|JSCLASS_HAS_RESERVED_SLOTS(5)|JSCLASS_FOREGROUND_FINALIZE,
                               &marker_class_ops,
};

static void
misc_finalize(JSFreeOp* cx, JSObject *obj)
{
  union Lisp_Misc *misc = (union Lisp_Misc *)JS_GetPrivate(obj);

  if (!misc)
    return;

  if (misc->u_any.type == Lisp_Misc_Marker)
    {
      struct Lisp_Marker *marker = (struct Lisp_Marker *)misc;

      unchain_marker (marker);
    }

  xfree(misc);
}


static JSClassOps misc_class_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, misc_finalize,
};

static JSClass misc_class = {
                             "ELisp_Misc",
                             JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                             &misc_class_ops,
};

static bool
misc_construct(JSContext* cx, unsigned argc, JS::Value* vp)
{
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  /*
  struct Lisp_Misc_Any *misc_any = xmalloc (sizeof *misc_any);
  misc_any->type = 0;
  misc_any->gcmarkbit = 0;
  misc_any_spacer = 0;

  JS_SetPrivate(cx, obj, misc_any);
  */

  return true;
}

static void misc_init(JSContext* cx, JS::HandleObject global)
{
  JS_InitClass(cx, global, nullptr, &misc_class, misc_construct, 1,
               nullptr, nullptr, nullptr, nullptr);
}

static bool
Q_resolve(JSContext *cx, JS::HandleObject obj, JS::HandleId id, bool *resolvedp)
{
  *resolvedp = false;
  if (JSID_IS_STRING (id))
    {
      JS::RootedString fs(cx, JS_FORGET_STRING_FLATNESS(JSID_TO_FLAT_STRING (id)));
      char *bytes = JS_EncodeStringToUTF8 (cx, fs);

      if (!bytes)
        return false;

      //fprintf(stderr, "resolving Q.%s\n", bytes);

      if (!strcmp(bytes, "valueOf"))
        return true;

      if (!strcmp(bytes, "toString"))
        return true;

#if 0
      for (char *p = bytes; *p; p++)
        {
          if (*p == '_')
            *p = '-';
          else if (*p == '-')
            *p = '_';
        }
#endif

      ELisp_Value obarray;
      obarray.v.v = JS_GetReservedSlot(obj, 0);
      ELisp_Value tem;

      tem = oblookup (obarray, bytes, strlen (bytes), strlen (bytes));

      //tem = find_symbol_value (tem);

      if (INTEGERP (tem))
        {
          //*resolvedp = false;
          tem = intern_1 (bytes, strlen(bytes));
          JS_SetProperty (cx, obj, bytes, tem.v.v);
          *resolvedp = true;
        }
      else
        {
          JS_SetProperty (cx, obj, bytes, tem.v.v);
          *resolvedp = true;
        }

      JS_free(cx, bytes);
    }
    return true;
}

static bool Q_call(JSContext *cx, unsigned argc, JS::Value *vp);
static bool V_call(JSContext *cx, unsigned argc, JS::Value *vp);
static bool F_call(JSContext *cx, unsigned argc, JS::Value *vp);

static const JSClassOps Q_classOps =
  {
   nullptr, nullptr, nullptr, nullptr,
   Q_resolve, nullptr, nullptr, Q_call, nullptr, nullptr, nullptr,
  };

static const JSClass Q_class =
  {
   "EmacsSymbols", JSCLASS_HAS_RESERVED_SLOTS(1),
   &Q_classOps,
  };

static bool Q_call(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = CallArgsFromVp(argc, vp);
  JS::RootedObject obj(cx, &args.callee());
  if (JS_GetClass(obj) != &Q_class)
    return false;

  if (args.length() != 1)
    return false;

  if (args[0].isString())
    {
      JS::RootedString str(cx, args[0].toString());
      char *bytes = JS_EncodeStringToUTF8 (cx, str);

      //fprintf(stderr, "resolving Q.%s\n", bytes);

      if (!bytes)
        return false;

      ELisp_Value obarray;
      obarray.v.v = JS_GetReservedSlot(obj, 0);
      ELisp_Value tem;

      tem = oblookup (obarray, bytes, strlen (bytes), strlen (bytes));

      //tem = find_symbol_value (tem);

      if (INTEGERP (tem))
        {
          //*resolvedp = false;
          tem = intern_1 (bytes, strlen(bytes));
        }

      JS_free(cx, bytes);

      args.rval().set(tem.v.v);
    }
  else
    return false;

  return true;
}


static bool
F_resolve(JSContext *cx, JS::HandleObject obj, JS::HandleId id, bool *resolvedp)
{
  *resolvedp = false;
  if (JSID_IS_STRING (id))
    {
      JS::RootedString fs(cx, JS_FORGET_STRING_FLATNESS(JSID_TO_FLAT_STRING (id)));
      char *bytes = JS_EncodeStringToUTF8 (cx, fs);

      if (!bytes)
        return false;

      //fprintf(stderr, "resolving F.%s\n", bytes);

#if 0
      for (char *p = bytes; *p; p++)
        {
          if (*p == '_')
            *p = '-';
          else if (*p == '-')
            *p = '_';
        }
#endif

      ELisp_Value obarray;
      obarray.v.v = JS_GetReservedSlot(obj, 0);
      ELisp_Value tem;

      tem = oblookup (obarray, bytes, strlen (bytes), strlen (bytes));

      if (INTEGERP (tem))
        *resolvedp = false;
      else
        {
          tem = XSYMBOL (tem)->function;

          JS_SetProperty (cx, obj, bytes, tem.v.v);
          *resolvedp = true;
        }

      JS_free(cx, bytes);
    }
    return true;
}

static const JSClassOps F_classOps =
  {
   nullptr, nullptr, nullptr, nullptr,
   F_resolve, nullptr, nullptr, F_call, nullptr, nullptr,
  };

static const JSClass F_class =
  {
   "EmacsFunctions", JSCLASS_HAS_RESERVED_SLOTS(1),
   &F_classOps,
  };

static bool F_call(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = CallArgsFromVp(argc, vp);
  JS::RootedObject obj(cx, &args.callee());
  if (JS_GetClass(obj) != &F_class)
    return false;

  if (args.length() != 1)
    return false;

  if (args[0].isString())
    {
      JS::RootedString str(cx, args[0].toString());
      char *bytes = JS_EncodeStringToUTF8 (cx, str);

      //fprintf(stderr, "resolving F.%s\n", bytes);

      if (!bytes)
        return false;

      ELisp_Value obarray;
      obarray.v.v = JS_GetReservedSlot(obj, 0);
      ELisp_Value tem;

      tem = oblookup (obarray, bytes, strlen (bytes), strlen (bytes));

      //tem = find_symbol_value (tem);

      if (INTEGERP (tem))
        {
          //*resolvedp = false;
          args.rval().set(LRH (Qnil).v.v);
        }
      else
        {
          tem = XSYMBOL (tem)->function;
          args.rval().set(tem.v.v);
        }

      JS_free(cx, bytes);
    }
  else
    return false;

  return true;
}

static bool
V_resolve(JSContext *cx, JS::HandleObject obj, JS::HandleId id, bool *resolvedp)
{
  *resolvedp = false;
  if (JSID_IS_STRING (id))
    {
      JS::RootedString fs(cx, JS_FORGET_STRING_FLATNESS(JSID_TO_FLAT_STRING (id)));
      char *bytes = JS_EncodeStringToUTF8 (cx, fs);

      if (!bytes)
        return false;

      //fprintf(stderr, "resolving V.%s\n", bytes);

#if 0
      for (char *p = bytes; *p; p++)
        {
          if (*p == '_')
            *p = '-';
          else if (*p == '-')
            *p = '_';
        }
#endif

      ELisp_Value obarray;
      obarray.v.v = JS_GetReservedSlot(obj, 0);
      ELisp_Value tem;

      tem = oblookup (obarray, bytes, strlen (bytes), strlen (bytes));

      if (INTEGERP (tem))
        *resolvedp = false;
      else
        {
          tem = find_symbol_value (tem);

          JS_SetProperty (cx, obj, bytes, tem.v.v);
          *resolvedp = true;
        }

      JS_free(cx, bytes);
    }
    return true;
}

static const JSClassOps V_classOps =
  {
   nullptr, nullptr, nullptr, nullptr,
   V_resolve, nullptr, nullptr, V_call, nullptr, nullptr,
  };

static const JSClass V_class =
  {
   "EmacsVariables", JSCLASS_HAS_RESERVED_SLOTS(1),
   &V_classOps,
  };

static bool V_call(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = CallArgsFromVp(argc, vp);
  JS::RootedObject obj(cx, &args.callee());
  if (JS_GetClass(obj) != &V_class)
    return false;

  if (args.length() != 1)
    return false;

  if (args[0].isString())
    {
      JS::RootedString str(cx, args[0].toString());
      char *bytes = JS_EncodeStringToUTF8 (cx, str);

      //fprintf(stderr, "resolving V.%s\n", bytes);

      if (!bytes)
        return false;

      ELisp_Value obarray;
      obarray.v.v = JS_GetReservedSlot(obj, 0);
      ELisp_Value tem;

      tem = oblookup (obarray, bytes, strlen (bytes), strlen (bytes));

      //tem = find_symbol_value (tem);

      if (INTEGERP (tem))
        {
          //*resolvedp = false;
          args.rval().set(LRH (Qnil).v.v);
        }
      else
        {
          tem = find_symbol_value (tem);
          args.rval().set(tem.v.v);
        }

      JS_free(cx, bytes);
    }
  else
    return false;

  return true;
}


static bool
global_enumerate(JSContext* cx, JS::HandleObject obj, JS::AutoIdVector& properties,
                 bool enumerableOnly)
{
    return true;
}

static bool
global_resolve(JSContext* cx, JS::HandleObject obj, JS::HandleId id, bool* resolvedp)
{
  *resolvedp = false;
  if (JSID_IS_STRING (id))
    {
      JS::RootedString fs(cx, JS_FORGET_STRING_FLATNESS(JSID_TO_FLAT_STRING (id)));
      char *bytes = JS_EncodeStringToUTF8 (cx, fs);

      if (!bytes)
        return false;

      //fprintf(stderr, "resolving %s\n", bytes);

      if (strcmp(bytes, "Q") == 0)
        {
          JS::RootedValue q(cx);
          JS::RootedObject qobj(cx, JS_NewObject(cx, &Q_class));
          JS_SetReservedSlot(qobj, 0, Vobarray);
          q = JS::ObjectValue(*qobj);
          JS_SetProperty(cx, obj, bytes, q);
          JS_free(cx, bytes);
          *resolvedp = true;
          return true;
        }
      if (strcmp(bytes, "F") == 0)
        {
          JS::RootedValue q(cx);
          JS::RootedObject qobj(cx, JS_NewObject(cx, &F_class));
          JS_SetReservedSlot(qobj, 0, Vobarray);
          q = JS::ObjectValue(*qobj);
          JS_SetProperty(cx, obj, bytes, q);
          JS_free(cx, bytes);
          *resolvedp = true;
          return true;
        }
      if (strcmp(bytes, "V") == 0)
        {
          JS::RootedValue q(cx);
          JS::RootedObject qobj(cx, JS_NewObject(cx, &V_class));
          JS_SetReservedSlot(qobj, 0, Vobarray);
          q = JS::ObjectValue(*qobj);
          JS_SetProperty(cx, obj, bytes, q);
          JS_free(cx, bytes);
          *resolvedp = true;
          return true;
        }

        {
          JS_free(cx, bytes);
          *resolvedp = false;
          return true;
        }
    }
    return true;
}

static bool
global_mayResolve(const JSAtomState& names, jsid id, JSObject* maybeObj)
{
  return true; // JS_MayResolveStandardClass(names, id, maybeObj);
}

static const JSClassOps global_classOps = {
    nullptr, nullptr, nullptr,
    global_enumerate, global_resolve, global_mayResolve,
    nullptr,
    nullptr, nullptr, nullptr,
    JS_GlobalObjectTraceHook
};

static const JSClass global_class = {
    "global", JSCLASS_GLOBAL_FLAGS,
    &global_classOps
};

static bool
Print(JSContext* cx, unsigned argc, JS::Value* vp)
{
  JS::CallArgs args = CallArgsFromVp(argc, vp);
    for (unsigned i = 0; i < args.length(); i++) {
      JS::RootedString str(cx, JS::ToString(cx, args[i]));
        if (!str)
            return false;
        char* bytes = JS_EncodeStringToUTF8(cx, str);
        if (!bytes)
            return false;
        fprintf(stdout, "%s%s", i ? " " : "", bytes);
        JS_free(cx, bytes);
    }

    fputc('\n', stdout);
    fflush(stdout);

    args.rval().setUndefined();
    return true;
}

static bool
console_log(JSContext* cx, unsigned argc, JS::Value* vp)
{
  JS::CallArgs args = CallArgsFromVp(argc, vp);
  for (unsigned i = 0; i < args.length(); i++) {
    JS::RootedString str(cx, JS::ToString(cx, args[i]));
    if (!str)
      return false;
    char* bytes = JS_EncodeStringToUTF8(cx, str);
    if (!bytes)
      return false;
    fprintf(stdout, "%s%s", i ? " " : "", bytes);
    JS_free(cx, bytes);
  }

  fputc('\n', stdout);
  fflush(stdout);

  args.rval().setUndefined();
  return true;
}

static bool
Oblookup(JSContext *cx, unsigned argc, JS::Value* vp)
{
  JS::CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() < 1)
    return false;

  JS::RootedString str(cx, JS::ToString(cx, args[0]));
  ELisp_Value obarray = Vobarray;
  ELisp_Value tem;
  char* bytes = JS_EncodeStringToUTF8(cx, str);
  if (!bytes)
    return false;

  tem = oblookup (obarray, bytes, strlen (bytes), strlen (bytes));
  if (INTEGERP (tem))
    args.rval().setUndefined();
  else
    args.rval().set(tem.v.v);

  return true;
}

extern ELisp_Return_Value jsval_to_elisp(ELisp_Handle arg);
extern ELisp_Return_Value elisp_to_jsval(ELisp_Handle arg);

static bool
D(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() < 1)
    return false;

  ELisp_Value arg;
  arg.v.v = args[0];
  arg = elisp_to_jsval(arg);

  args.rval().set(arg.v.v);

  return true;
}

static bool
E(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = CallArgsFromVp(argc, vp);
  if (args.length() < 1)
    return false;

  ELisp_Value arg;
  arg.v.v = args[0];
  arg = jsval_to_elisp(arg);

  args.rval().set(arg.v.v);

  return true;
}

static const JSFunctionSpec emacs_functions[] =
  {
   JS_FN("Print", Print, 0, 0),
   JS_FN("Oblookup", Oblookup, 0, 0),
   JS_FN("D", D, 0, 0),
   JS_FN("E", E, 0, 0),
   JS_FN("console_log", console_log, 0, 0),
   JS_FS_END
  };

void
WarningReporter(JSContext* cx, JSErrorReport* report)
{
  fprintf(stderr, "Warning!\n");
}

//extern JS_PUBLIC_API(bool)
//JS_AddExtraGCRootsTracer(JSContext* cx, JSTraceDataOp traceOp, void* data);

enum { NSTATICS = 2048 };
extern ELisp_Struct_Value *staticvec[NSTATICS];
extern ELisp_Heap_Value executing_kbd_macro;
extern KBOARD *all_kboards;
extern struct buffer alignas (GCALIGNMENT) buffer_defaults;
extern struct buffer alignas (GCALIGNMENT) buffer_local_symbols;
extern struct buffered_input_event kbd_buffer[KBD_BUFFER_SIZE];

static void
elisp_vector_trace(JSTracer *trc, JSObject *obj);

extern struct Lisp_Subr Sframe_windows_min_size;

#define PRINT_CIRCLE 200
extern ptrdiff_t print_depth;
extern ELisp_Struct_Value being_printed[PRINT_CIRCLE];

static JS::GCVector<JS::Value, 0, js::TempAllocPolicy> *js_promise_jobs;

static void
js_gc_trace(JSTracer* tracer, void* data)
{
  fprintf(stderr, "that one's mine! And that one! And...\n");

  (*js_promise_jobs).trace(tracer);

  for (ptrdiff_t i = 0; i < PRINT_CIRCLE; i++)
    TraceEdge(tracer, &being_printed[i].v.v, "being_printed");

  // XXX: all subrs can be collected after they're redefined.  This
  // one actually is.  The same applies to builtin symbols, which
  // might be made unbound.
  TraceEdge(tracer, &Sframe_windows_min_size.header.jsval, "Sframe_windows_min_size");

  TraceEdge(tracer, &elisp_cons_class_proto, "cons class proto");
  TraceEdge(tracer, &elisp_string_class_proto, "string class proto");
  TraceEdge(tracer, &elisp_symbol_class_proto, "symbol class proto");
  TraceEdge(tracer, &elisp_vector_class_proto, "vector class proto");
  TraceEdge(tracer, &elisp_misc_class_proto, "misc class proto");

  for (size_t i = 0; i < GLOBAL_OBJECTS; i++) {
    TraceEdge(tracer, &(((ELisp_Struct_Value *)&globals)+i)->v.v, "global");
  }

  for (size_t i = 0; i < ARRAYELTS (lispsym); i++) {
    TraceEdge(tracer, &lispsym[i].jsval, "global symbol");
  }

  for (struct handler *h = handlerlist; h; h = h->next)
    {
      TraceEdge(tracer, &h->tag_or_ch.v.v, "tag_or_ch");
      TraceEdge(tracer, &h->val.v.v, "val");
    }

  for (struct specbinding *pdl = specpdl_invalid_ptr - 1; pdl >= specpdl; pdl--) {
    TraceEdge(tracer, &pdl->unwind.arg.v.v, "unwind");
    TraceEdge(tracer, &pdl->let.where.v.v, "where");
    TraceEdge(tracer, &pdl->let.symbol.v.v, "symbol");
    TraceEdge(tracer, &pdl->let.old_value.v.v, "old value");
    TraceEdge(tracer, &pdl->let.saved_value.v.v, "saved_value");
    TraceEdge(tracer, &pdl->bt.function.v.v, "backtrace function");
    TraceEdge(tracer, &pdl->unwind_excursion.marker.v.v, "backtrace function");
    TraceEdge(tracer, &pdl->unwind_excursion.window.v.v, "backtrace function");
    //TraceEdge(tracer, &pdl->unwind_array.vector.v);
  }

  for (struct specbinding *pdl = specpdl_invalid_ptr; pdl < specpdl + specpdl_size; pdl++) {
    memset(pdl, 0, sizeof *pdl);
    pdl->unwind.arg.v.v = JS::UndefinedValue();
    pdl->let.where.v.v = JS::UndefinedValue();
    pdl->let.symbol.v.v = JS::UndefinedValue();
    pdl->let.old_value.v.v = JS::UndefinedValue();
    pdl->let.saved_value.v.v = JS::UndefinedValue();
    pdl->bt.function.v.v = JS::UndefinedValue();
  }

  for (size_t i = 0; i < NSTATICS; i++) {
    if (staticvec[i])
      TraceEdge(tracer, (JS::Heap<JS::Value> *)staticvec[i], "staticprod");
  }

  TraceEdge(tracer, &empty_unibyte_string.v.v, "empty unibyte string");
  TraceEdge(tracer, &empty_multibyte_string.v.v, "empty multibyte string");
  TraceEdge(tracer, &zero_vector.v.v, "zero vector");
  TraceEdge(tracer, &executing_kbd_macro.v.v, "executing_kbd_macro");

  for (KBOARD *kb = all_kboards; kb; kb = kb->next_kboard)
    {
      if (kb->kbd_macro_buffer)
        for (ELisp_Struct_Value *p = kb->kbd_macro_buffer;
             p < kb->kbd_macro_buffer + kb->kbd_macro_bufsize;
             p++)
          {
            TraceEdge(tracer, &p->v.v, "kboard buffer entry");
          }
      TraceEdge(tracer, &KVAR (kb, Voverriding_terminal_local_map).v.v, "kboard var");
      TraceEdge(tracer, &KVAR (kb, Vlast_command).v.v, "kboard var");
      TraceEdge(tracer, &KVAR (kb, Vreal_last_command).v.v, "kboard var");
      TraceEdge(tracer, &KVAR (kb, Vkeyboard_translate_table).v.v, "kboard var");
      TraceEdge(tracer, &KVAR (kb, Vlast_repeatable_command).v.v, "kboard var");
      TraceEdge(tracer, &KVAR (kb, Vprefix_arg).v.v, "kboard var");
      TraceEdge(tracer, &KVAR (kb, Vlast_prefix_arg).v.v, "kboard var");
      TraceEdge(tracer, &KVAR (kb, kbd_queue).v.v, "kboard var");
      TraceEdge(tracer, &KVAR (kb, defining_kbd_macro).v.v, "kboard var");
      TraceEdge(tracer, &KVAR (kb, Vlast_kbd_macro).v.v, "kboard var");
      TraceEdge(tracer, &KVAR (kb, Vsystem_key_alist).v.v, "kboard var");
      TraceEdge(tracer, &KVAR (kb, system_key_syms).v.v, "kboard var");
      TraceEdge(tracer, &KVAR (kb, Vwindow_system).v.v, "kboard var");
      TraceEdge(tracer, &KVAR (kb, Vinput_decode_map).v.v, "kboard var");
      TraceEdge(tracer, &KVAR (kb, Vlocal_function_key_map).v.v, "kboard var");
      TraceEdge(tracer, &KVAR (kb, Vdefault_minibuffer_frame).v.v, "kboard var");
      TraceEdge(tracer, &KVAR (kb, echo_string).v.v, "kboard var");
      TraceEdge(tracer, &KVAR (kb, echo_prompt).v.v, "kboard var");
    }

  for (struct terminal *t = terminal_list; t; t = t->next_terminal)
    {
      struct Lisp_Vector *v = (struct Lisp_Vector *)t;
      TraceEdge(tracer, &v->header.jsval, "terminal");
      struct image_cache *icache = t->image_cache;
      if (icache)
        for (size_t i = 0; i < icache->used; i++)
          if (icache->images[i])
            {
              TraceEdge(tracer, &icache->images[i]->spec.v.v, "img spec");
              TraceEdge(tracer, &icache->images[i]->dependencies.v.v, "img spec");
              TraceEdge(tracer, &icache->images[i]->lisp_data.v.v, "img spec");
            }
    }

  TraceEdge(tracer, &buffer_defaults.header.jsval, "buffer_defaults");
  TraceEdge(tracer, &buffer_local_symbols.header.jsval, "buffer_local_symbols");

  TraceEdge(tracer, &hashtest_eq.name.v.v, "name");
  TraceEdge(tracer, &hashtest_eq.user_hash_function.v.v, "user_hash_function");
  TraceEdge(tracer, &hashtest_eq.user_cmp_function.v.v, "user_cmp_function");

  TraceEdge(tracer, &hashtest_eql.name.v.v, "name");
  TraceEdge(tracer, &hashtest_eql.user_hash_function.v.v, "user_hash_function");
  TraceEdge(tracer, &hashtest_eql.user_cmp_function.v.v, "user_cmp_function");

  TraceEdge(tracer, &hashtest_equal.name.v.v, "name");
  TraceEdge(tracer, &hashtest_equal.user_hash_function.v.v, "user_hash_function");
  TraceEdge(tracer, &hashtest_equal.user_cmp_function.v.v, "user_cmp_function");

  for (struct x_display_info *dpyinfo = x_display_list; dpyinfo; dpyinfo = dpyinfo->next)
    {
      TraceEdge(tracer, &dpyinfo->name_list_element.v.v, "name_list_element");
    }

  for (size_t i = 0; i < KBD_BUFFER_SIZE; i++)
    {
      TraceEdge(tracer, &kbd_buffer[i].ie.x.v.v, "x");
      TraceEdge(tracer, &kbd_buffer[i].ie.y.v.v, "y");
      TraceEdge(tracer, &kbd_buffer[i].ie.frame_or_window.v.v, "frame_or_window");
      TraceEdge(tracer, &kbd_buffer[i].ie.arg.v.v, "arg");
    }

  fprintf(stderr, "You can have the leftovers, I guess.\n");
}

static void
elisp_gc_callback_register(JSContext *cx)
{
  JS_AddExtraGCRootsTracer(cx, js_gc_trace, NULL);
}

static void
elisp_classes_init(JSContext *cx, JS::HandleObject glob);

bool js_job_callback(JSContext *cx, JS::HandleObject job,
                     JS::HandleObject allocationSite, JS::HandleObject incumbentGlobal,
                     void *data)
{
  JS::RootedValue val(cx, JS::ObjectValue(*job));
  JS::GCVector<JS::Value, 0, js::TempAllocPolicy> *vecp = static_cast<JS::GCVector<JS::Value, 0, js::TempAllocPolicy> *>(data);

  return vecp->append(val);
}

bool js_init()
{
  //js::DisableExtraThreads()

  if (!JS_Init())
    return false;

  JSContext *cx = JS_NewContext(JS::DefaultHeapMaxBytes, JS::DefaultNurseryBytes);
  global_js_context = cx;
  if (!cx)
    return false;
  JS_SetFutexCanWait(cx);
  JS::SetWarningReporter(cx, WarningReporter);
  JS_SetGCParameter(cx, JSGC_MAX_BYTES, 0xfffffffffL);

  JS_SetNativeStackQuota(cx, 8 * 1024 * 1024);

  if (!JS::InitSelfHostedCode(cx))
    return false;

  JS_AddInterruptCallback(cx, (void *)0x55555555);
  //JS::ContextOptionsRef(cx).setBaseline(false).setIon(false);
  //JS_SetGCParameter(cx, JSGC_MODE, JSGC_MODE_INCREMENTAL);
  JS::GCVector<JS::Value, 0, js::TempAllocPolicy> *vecp = new JS::GCVector<JS::Value, 0, js::TempAllocPolicy>(cx);
  js_promise_jobs = vecp;
  JS::SetEnqueuePromiseJobCallback(cx, js_job_callback, static_cast<void *>(vecp));

  {
    JSAutoRequest ar(cx);
    JS_BeginRequest(cx);
    JS::RealmOptions compartment_options;
    compartment_options.creationOptions().setNewCompartmentAndZone();
    JS::RootedObject glob(cx, JS_NewGlobalObject(cx, &global_class, nullptr, JS::FireOnNewGlobalHook, compartment_options));

    if (!glob)
      return false;

    {
      JS::Realm* oldRealm = JS::EnterRealm(cx, glob);
      if (!JS::InitRealmStandardClasses(cx))
        return false;

      if (!JS_DefineFunctions(cx, glob, emacs_functions))
        return false;
      elisp_classes_init(cx, glob);
      elisp_gc_callback_register(cx);
      JS_InitClass(cx, glob, nullptr, &cons_class, cons_construct, 2,
                   nullptr, nullptr, nullptr, nullptr);
      JS_InitClass(cx, glob, nullptr, &string_class, string_construct, 1,
                   string_props, string_fns, nullptr, nullptr);
    }

  }

  return true;
}

char *jsval_to_string(ELisp_Handle val)
{
  JS::RootedString str(jsg.cx, JS::ToString(jsg.cx, val.v.v));
  if (!str)
    return NULL;
  char* bytes = JS_EncodeStringToUTF8(jsg.cx, str);
  if (!bytes)
    return NULL;

  return bytes;
}

static void eval_js(const char *source)
{
  JSContext* cx = jsg.cx;
  JS::RootedScript script(cx);
  size_t sourcelen = strlen(source);
  JS::CompileOptions options(cx);
  options.setIntroductionType("js shell interactive")
    .setUTF8(true)
    .setIsRunOnce(true)
    .setFileAndLine("typein", 1);

  if (!JS::Compile(cx, options, source, sourcelen, &script))
    return;

  if (!JS_ExecuteScript(cx, script))
    return;
}

void
late_js_init(void)
{
  eval_js("Object.getPrototypeOf(F.list(3,4,5))[Symbol.iterator] = function* () { yield this.car; yield* this.cdr; }");
  eval_js("Q.nil[Symbol.iterator] = function* () {}");
}

ELisp_Return_Value jsval_to_elisp(ELisp_Handle ARG(arg))
{
  ELisp_Value arg = ARG(arg);

  if (arg.v.v.isUndefined() || arg.v.v.isNull())
    return Qnil;
  else if (arg.v.v.isBoolean())
    {
      if (arg.v.v.toBoolean())
        return Qt;
      else
        return Qnil;
    }
  else if (arg.v.v.isDouble() && (double)(int32_t)arg.v.v.toDouble() == arg.v.v.toDouble())
    {
      arg.v.v.setInt32((int32_t)arg.v.v.toDouble());
      return arg;
    }
  else if (arg.v.v.isInt32() || arg.v.v.isDouble() || arg.v.v.isObject())
    return arg;
  else if (arg.v.v.isString())
    {
      char *bytes = jsval_to_string (arg);
      return build_string (bytes);
    }
  else
    return arg;
}

ELisp_Return_Value elisp_to_jsval(ELisp_Handle ARG(arg))
{
  ELisp_Value arg = ARG(arg);

  if (NILP (arg))
    arg.v.v.setBoolean(false);
  else if (EQ (arg, LRH(Qt)))
    arg.v.v.setBoolean(true);

  return arg;
}

static ELisp_Return_Value elisp_exception_to_js_exception(ELisp_Handle exc)
{
  return exc;
}

JS::Heap<JSObject*> elisp_exception_class_proto __attribute__((init_priority(103)));
static ELisp_Return_Value js_exception_to_elisp_exception(ELisp_Handle exc)
{
  //if (exc.isObject() && JS_GetClass(&exc.toObject()) == &elisp_exception_class)
    return exc;

  JSContext *cx = jsg.cx;
  JS::RootedObject proto(cx);
  proto = elisp_exception_class_proto;
  JS::RootedObject obj(cx);
  obj = JS_NewObjectWithGivenProto(cx, &elisp_exception_class, proto);

  JS_SetProperty(cx, obj, "e", exc.v.v);

  return JS::ObjectValue(*obj);
}

void jsprint(JS::Value v);
void jsprint(JS::HandleValue v);

static void js_handle_exception(void)
{
  if (JS_IsExceptionPending(jsg.cx))
    {
      ELisp_Value jsexc;
      JS_GetPendingException(jsg.cx, &jsexc.v.v);
      ELisp_Value exc = js_exception_to_elisp_exception(jsexc);
      if (CONSP (LVH (exc)) && EQ (LRH (XCAR (LVH (exc))), LRH (Qno_catch)))
        {
          JS_ClearPendingException(jsg.cx);
          Fthrow(LRH (XCAR (LRH (XCDR (LVH (exc))))),
                 LRH (XCDR (LRH (XCDR (LVH (exc))))));
        }
      else if (CONSP (LVH (exc)))
        {
          JS_ClearPendingException(jsg.cx);
          Fsignal(LRH (XCAR (LVH (exc))),
                  LRH (XCDR (LVH (exc))));
        }
      else
        {
          fprintf(stderr, "unhandled exception\n");
          jsprint(jsexc);
          JS_ClearPendingException(jsg.cx);
        }
    }
}

EXFUN (Fjs, 1);

DEFUN ("js", Fjs, Sjs, 1, 1, 0,
       doc: /* Evaluate JavaScript.
usage: (js SOURCE)  */)
  (ELisp_Handle ARG(arg))
{
  ELisp_Value arg = ARG(arg);
  ELisp_Value result;
  CHECK_STRING (arg);

  const unsigned char *source = SDATA(arg);
  ptrdiff_t len = SCHARS(arg);

  JSContext* cx = jsg.cx;
  JS::RootedScript script(cx);
  size_t sourcelen = strlen((const char*)source);
  JS::CompileOptions options(cx);
  options.setIntroductionType("evaluation from Emacs")
    .setUTF8(true)
    .setIsRunOnce(true)
    .setFileAndLine("typein", 1);

  if (!JS::Compile(cx, options, (const char *)source, sourcelen, &script))
    return Qnil;

  if (!JS_ExecuteScript(cx, script, &result.v.v))
    {
      js_handle_exception();
      return Qnil;
    }

  return jsval_to_elisp(result);
}

EXFUN (Fjsdrain, 0);

DEFUN ("jsdrain", Fjsdrain, Sjsdrain, 0, 0, 0,
       doc: /* Drain the job queue.
usage: (jsdrain)  */)
     (void)
{
  JSContext* cx = jsg.cx;
  size_t i = 0;
  while (i < js_promise_jobs->length())
    {
      JS::RootedValue jobv(cx, (*js_promise_jobs)[i]);
      i++;
      JS::RootedObject job(cx, &jobv.toObject());

      JS::Call(cx, jobv, job, JS::HandleValueArray::empty(), &jobv);
    }

  js_promise_jobs->clear();

  return make_number(i);
}

EXFUN (Fjsglobal, 0);

DEFUN ("jsglobal", Fjsglobal, Sjsglobal, 0, 0, 0,
       doc: /* */)
     (void)
{
  JSContext* cx = jsg.cx;
  ELisp_Value ret;
  JS::RootedObject glob(cx, JS::CurrentGlobalOrNull(cx));
  if (!cx)
    return Qnil;

  ret.v.v = JS::ObjectValue(*glob);

  return ret;
}

EXFUN (Fjsmethod, MANY);
DEFUN ("jsmethod", Fjsmethod, Sjsmethod, 2, MANY, 0,
       doc: /* */)
     (ELisp_Vector_Handle args)
{
  JSContext* cx = jsg.cx;
  ELisp_Value thisv = args.vec.ref(0);
  if (!thisv.isObject())
    return Qnil;
  JS::RootedObject obj(cx, &thisv.toObject());
  ELisp_Value namev = args.vec.ref(1);
  ELisp_Value methv;
  CHECK_STRING (namev);
  if (!JS_GetProperty(cx, obj, (const char *)SDATA (namev), &methv.v.v))
    return Qnil;
  size_t nargs = args.n - 2;
  ELisp_Dynvector rargs;
  rargs.resize(nargs);
  for (size_t i = 0; i < nargs; i++)
    rargs.sref(i, args.vec.ref(i + 2));
  ELisp_Value rval;
  catchall_real_jmpbuf = NULL;

  if (!JS::Call(cx, thisv.v.v, methv.v.v, rargs.vec.vec, &rval.v.v))
    {
      js_handle_exception();
      return Qnil;
    }

  return rval;
}

EXFUN (Fjsderef, 2);
DEFUN ("jsderef", Fjsderef, Sjsderef, 2, 2, 0,
       doc: /* */)
     (ELisp_Handle ARG(object), ELisp_Handle ARG(name))
{
  JSContext* cx = jsg.cx;
  ELisp_Value object = ARG(object);

  if (!object.isObject())
    return Qnil;

  ELisp_Value name = ARG(name);
  CHECK_STRING (name);
  const unsigned char *namestr = SDATA(name);

  JS::RootedObject obj(cx, &object.v.v.toObject());
  JS_GetProperty(cx, obj, (const char *)namestr, &object.v.v);

  return object;
}

EXFUN (Fjspromise, 1);

DEFUN ("jspromise", Fjspromise, Sjspromise, 1, 1, 0,
       doc: /* make a JavaScript promise */)
     (ELisp_Handle ARG(arg))
{
  JSContext* cx = jsg.cx;
  size_t i = 0;
  ELisp_Value arg = ARG(arg);

  JS::RootedObject glob(cx, JS::CurrentGlobalOrNull(cx));
  if (!cx)
    return Qnil;

  JS::RootedValue promv(cx);
  JS_GetProperty(cx, glob, "Promise", &promv);

  if (!promv.isObject())
    return Qnil;
  JS::RootedObject prom(cx, &promv.toObject());

  JS::AutoValueArray<1> vp(cx);
  vp[0].set(arg.v.v);
  JS::RootedObject promise(cx, JS_New(cx, prom, vp));

  arg.v.v = JS::ObjectValue(*promise);

  return arg;
}

/* This is from js.cpp in the Mozilla distribution. */
const char *
read_file_as_string(JSContext* cx, const char* pathname)
{
  FILE* file;

  file = fopen(pathname, "rb");
  if (!file)
    return nullptr;

  if (fseek(file, 0, SEEK_END) != 0) {
    JS_ReportErrorUTF8(cx, "can't seek end of %s", pathname);
    goto err_exit;
  }

  {
    size_t len = ftell(file);
    if (fseek(file, 0, SEEK_SET) != 0) {
      JS_ReportErrorUTF8(cx, "can't seek start of %s", pathname);
      goto err_exit;
    }

    {
      char *buf = static_cast<char*>(js_malloc(len + 1));
      if (!buf)
        goto err_exit;

      buf[len] = 0;
      size_t cc = fread(buf, 1, len, file);
      if (cc != len) {
        if (ptrdiff_t(cc) < 0) {
          JS_ReportErrorUTF8(cx, "can't read %s: error %d", pathname, errno);
        } else {
          JS_ReportErrorUTF8(cx, "can't read %s: short read", pathname);
        }
        goto err_exit;
      }

      return buf;
    }
  }

 err_exit:
  fclose(file);
  return nullptr;
}

ELisp_Return_Value
js_call_function(ELisp_Handle fun, ELisp_Dynvector& vals)
{
  ELisp_Value thisv;
  ELisp_Value rval;
  thisv.v.v = JS::NullValue();
  catchall_real_jmpbuf = NULL;
  if (!JS::Call(jsg.cx, thisv.v.v, fun.v.v, vals.vec.vec, &rval.v.v))
    {
      js_handle_exception();
      return LRH(Qnil);
    }

  return rval;
}

EXFUN (Fjsread, 2);

DEFUN ("jsread", Fjsread, Sjsread, 1, 2, 0,
       doc: /* Evaluate JavaScript.
usage: (jsread SOURCE)  */)
     (ELisp_Handle ARG(arg), ELisp_Handle ARG(dir))
{
  ELisp_Value arg = ARG(arg);
  ELisp_Value dir = ARG(dir);
  ELisp_Value result;
  CHECK_STRING (arg);

  ptrdiff_t count = SPECPDL_INDEX ();
  if (!NILP (dir))
    {
      CHECK_STRING (dir);
      specbind (LRH (Qdefault_directory), dir);
    }
  arg = Fexpand_file_name(arg, dir);

  const unsigned char *source = SDATA(arg);
  ptrdiff_t len = SCHARS(arg);

  JSContext* cx = jsg.cx;
  const char *str = read_file_as_string(cx, (const char *)source);
  if (!str)
    goto error;

  {
    JS::RootedScript script(cx);
    JS::CompileOptions options(cx);
    options.setIntroductionType("jsread run")
      .setUTF8(true)
      .setIsRunOnce(true)
      .setFileAndLine(reinterpret_cast<const char *>(source), 1);

    if (!JS_CompileScript(cx, str, strlen(str), options, &script))
      goto error;

    if (!JS_ExecuteScript(cx, script, &result.v.v))
      goto error;

    result = unbind_to (count, result);
  }

  return jsval_to_elisp(result);

 error:
  result = unbind_to (count, LRH(Qnil));

  return result;
}

EXFUN (Fspecbind, 2);

DEFUN ("specbind", Fspecbind, Sspecbind, 2, 2, 0,
       doc: /* Don't call this. */)
(ELisp_Handle arg, ELisp_Handle arg2)
{
  specbind(arg, arg2);

  return Qnil;
}

EXFUN (Fsetinternal, 3);

DEFUN ("setinternal", Fsetinternal, Ssetinternal, 3, 3, 0,
       doc: /* Don't call this. */)
     (ELisp_Handle arg, ELisp_Handle arg2, ELisp_Handle arg3)
{
  set_internal (arg, arg2, arg3, SET_INTERNAL_SET);

  return Qnil;
}

EXFUN (Funbind_to_rel, 2);

DEFUN ("unbind_to_rel", Funbind_to_rel, Sunbind_to_rel, 2, 2, 0,
       doc: /* Don't call this. */)
     (ELisp_Handle arg, ELisp_Handle arg2)
{
  unbind_to (SPECPDL_INDEX () - XINT (arg), arg2);
  return Qnil;
}

JSG jsg __attribute__((init_priority(102)));

void *elisp_cons_class_proto_backup;
void *elisp_string_class_proto_backup;
void *elisp_symbol_class_proto_backup;
void *elisp_vector_class_proto_backup;
void *elisp_misc_class_proto_backup;
void *elisp_exception_class_proto_backup;

JS::Heap<JSObject*> elisp_cons_class_proto __attribute__((init_priority(103)));
JS::Heap<JSObject*> elisp_string_class_proto __attribute__((init_priority(103)));
JS::Heap<JSObject*> elisp_symbol_class_proto __attribute__((init_priority(103)));
JS::Heap<JSObject*> elisp_vector_class_proto __attribute__((init_priority(103)));
JS::Heap<JSObject*> elisp_misc_class_proto __attribute__((init_priority(103)));

class JSG2 {
public:
  JSG2() {
  memcpy(&elisp_cons_class_proto, &elisp_cons_class_proto_backup, sizeof elisp_cons_class_proto);
  memcpy(&elisp_string_class_proto, &elisp_string_class_proto_backup, sizeof elisp_cons_class_proto);
  memcpy(&elisp_symbol_class_proto, &elisp_symbol_class_proto_backup, sizeof elisp_cons_class_proto);
  memcpy(&elisp_vector_class_proto, &elisp_vector_class_proto_backup, sizeof elisp_cons_class_proto);
  memcpy(&elisp_misc_class_proto, &elisp_misc_class_proto_backup, sizeof elisp_cons_class_proto);
  memcpy(&elisp_exception_class_proto, &elisp_exception_class_proto_backup, sizeof elisp_cons_class_proto);
    
  }
};

JSG2 jsg2 __attribute__((init_priority(104)));

  JSContext* global_js_context;

extern JSContext* global_js_context;

static bool
elisp_cons_resolve(JSContext *cx, JS::HandleObject obj,
                     JS::HandleId id, bool *resolvedp)
{
  struct Lisp_Cons *s = (struct Lisp_Cons *)JS_GetPrivate(obj);

  *resolvedp = false;
  if (JSID_IS_STRING (id))
    {
      if (JS_FlatStringEqualsAscii (JSID_TO_FLAT_STRING (id), "car"))
        {
          JS::RootedValue val(cx, s->car);
          JS_SetProperty (cx, obj, "car", val);
          *resolvedp = true;

          return true;
        }
      else if (JS_FlatStringEqualsAscii (JSID_TO_FLAT_STRING (id), "cdr"))
        {
          JS::RootedValue val(cx, s->u.cdr);
          JS_SetProperty (cx, obj, "cdr", val);
          *resolvedp = true;

          return true;
        }
    }
  else if (JSID_IS_INT(id))
    {
      if (JSID_TO_INT (id) == 0)
        {
          JS::RootedValue val(cx, s->car);
          JS_SetPropertyById (cx, obj, id, val);
          *resolvedp = true;

          return true;
        }
      else
        {
          JS::RootedValue cdr(cx, s->u.cdr);
          if (!cdr.isObject())
            return true;

          JS::RootedObject obj2(cx, &cdr.toObject());
          JS::RootedValue val(cx);

          if (!JS_GetElement(cx, obj2, JSID_TO_INT (id) - 1, &val))
            return false;

          JS_SetElement (cx, obj, JSID_TO_INT (id), val);
          *resolvedp = true;

          return true;
        }
    }

  *resolvedp = false;
  return true;
}

static void
elisp_cons_finalize(JSFreeOp* cx, JSObject *obj)
{
  struct Lisp_Cons *s = (struct Lisp_Cons *)JS_GetPrivate(obj);
  if (!s)
    return;

  if (PURE_P (s))
    return;

  xfree(s);
}

static bool elisp_vector_call(JSContext *cx, unsigned argc, JS::Value *vp);

static void
elisp_cons_trace(JSTracer *trc, JSObject *obj)
{
  struct Lisp_Cons *s = (struct Lisp_Cons *)JS_GetPrivate(obj);

  if (!s) return;

  //fprintf(stderr, "tracing cons at %p\n", s);

  TraceEdge(trc, &s->jsval, "jsval");
  TraceEdge(trc, &s->car.v.v, "car");
  TraceEdge(trc, &s->u.cdr.v.v, "cdr");
}

static JSClassOps elisp_cons_ops =
{
  NULL, NULL, NULL, NULL,
  elisp_cons_resolve, NULL, elisp_cons_finalize,
  elisp_vector_call, NULL, NULL, elisp_cons_trace
};

JSClass elisp_cons_class =
  {
   "ELisp_Cons", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
   &elisp_cons_ops,
  };

static bool
elisp_symbol_function_getter(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = CallArgsFromVp(argc, vp);
  JS::RootedValue thisv(cx, args.thisv());
  if (!thisv.isObject())
    return false;
  JS::RootedObject obj(cx, &thisv.toObject());
  if (JS_GetClass(obj) != &elisp_symbol_class)
    return false;

  struct Lisp_Symbol *s = (struct Lisp_Symbol *)JS_GetPrivate(obj);

  args.rval().set(s->function.v.v);

  return true;
}

static bool
elisp_symbol_function_setter(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = CallArgsFromVp(argc, vp);
  JS::RootedValue thisv(cx, args.thisv());
  if (!thisv.isObject())
    return false;
  JS::RootedObject obj(cx, &thisv.toObject());
  if (JS_GetClass(obj) != &elisp_symbol_class)
    return false;

  struct Lisp_Symbol *s = (struct Lisp_Symbol *)JS_GetPrivate(obj);

  if (args.length() != 1)
    return false;

  s->function.v.v = args[0];
  args.rval().setUndefined();

  return true;
}

static bool
elisp_symbol_value_getter(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = CallArgsFromVp(argc, vp);
  JS::RootedValue thisv(cx, args.thisv());
  if (!thisv.isObject())
    return false;
  JS::RootedObject obj(cx, &thisv.toObject());
  if (JS_GetClass(obj) != &elisp_symbol_class)
    return false;

  struct Lisp_Symbol *s = (struct Lisp_Symbol *)JS_GetPrivate(obj);
  ELisp_Value sym;

  sym.v.v = s->jsval;
  args.rval().set(find_symbol_value (sym).v);

  return true;
}

static bool
elisp_symbol_value_setter(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = CallArgsFromVp(argc, vp);
  JS::RootedValue thisv(cx, args.thisv());
  if (!thisv.isObject())
    return false;
  JS::RootedObject obj(cx, &thisv.toObject());
  if (JS_GetClass(obj) != &elisp_symbol_class)
    return false;

  struct Lisp_Symbol *s = (struct Lisp_Symbol *)JS_GetPrivate(obj);
  ELisp_Value sym;

  sym.v.v = s->jsval;

  if (args.length() != 1)
    return false;

  ELisp_Value newval;
  newval.v.v = args[0];

  args.rval().set (Fset (sym, newval).v);

  return true;
}

static bool
elisp_symbol_resolve(JSContext *cx, JS::HandleObject obj,
                     JS::HandleId id, bool *resolvedp)
{
  struct Lisp_Symbol *s = (struct Lisp_Symbol *)JS_GetPrivate(obj);

  if (JSID_IS_STRING (id))
    {
      if (JS_FlatStringEqualsAscii (JSID_TO_FLAT_STRING (id), "name"))
        {
          JS::RootedValue val(cx, s->name);
          JS_SetProperty (cx, obj, "name", val);
          *resolvedp = true;

          return true;
        }
      else if (JS_FlatStringEqualsAscii (JSID_TO_FLAT_STRING (id), "function"))
        {
          JS::RootedValue val(cx, s->function);
          JS_DefinePropertyById (cx, obj, id,
                                 elisp_symbol_function_getter,
                                 elisp_symbol_function_setter,
                                 0);
          *resolvedp = true;

          return true;
        }
      else if (JS_FlatStringEqualsAscii (JSID_TO_FLAT_STRING (id), "plist"))
        {
          JS::RootedValue val(cx, s->plist);
          JS_SetProperty (cx, obj, "plist", val);
          *resolvedp = true;

          return true;
        }
      else if (JS_FlatStringEqualsAscii (JSID_TO_FLAT_STRING (id), "value"))
        {
          ELisp_Value sym;
          sym.v.v = JS::ObjectValue(*obj);
          JS::RootedValue val(cx, find_symbol_value (sym).v);

          JS_DefinePropertyById (cx, obj, id,
                                 elisp_symbol_value_getter,
                                 elisp_symbol_value_setter,
                                 0);
          *resolvedp = true;

          return true;
        }
    }

  *resolvedp = false;
  return true;
}

static bool elisp_symbol_call(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = CallArgsFromVp(argc, vp);
  JS::RootedObject obj(cx, &args.callee());
  if (JS_GetClass(obj) != &elisp_symbol_class)
    return false;

  ELisp_Value fun;
  ELisp_Value ret;
  fun.v.v = args.calleev();
  ELisp_Dynvector argv;
  argv.resize (args.length() + 1);
  argv.sref(0, fun);
  for (ptrdiff_t i = 0; i < args.length(); i++)
    argv.sref(i+1, args[i]);
  fprintf(stderr, "calling 2\n");
  ret = Ffuncall (LV (args.length() + 1, argv));
  fprintf(stderr, "called\n");
  args.rval().set(ret.v.v);

  return true;
}

static void
elisp_symbol_trace(JSTracer *trc, JSObject *obj)
{
  struct Lisp_Symbol *s = (struct Lisp_Symbol *)JS_GetPrivate(obj);

  if (!s) return;

  TraceEdge(trc, &s->jsval, "jsval");
  TraceEdge(trc, &s->name.v.v, "name");
  if (s->redirect == SYMBOL_PLAINVAL)
    TraceEdge(trc, &s->val.value.v.v, "value");
  else if (s->redirect == SYMBOL_VARALIAS)
    TraceEdge(trc, &s->val.alias->jsval, "alias");
  else if (s->redirect == SYMBOL_LOCALIZED)
    {
      struct Lisp_Buffer_Local_Value *blv = s->val.blv;

      TraceEdge(trc, &blv->where.v.v, "where");
      TraceEdge(trc, &blv->defcell.v.v, "defcell");
      TraceEdge(trc, &blv->valcell.v.v, "valcell");
    }
  else if (s->redirect == SYMBOL_FORWARDED)
    {
      union Lisp_Fwd *fwd = s->val.fwd;

      switch (XFWDTYPE (fwd)) {
      case Lisp_Fwd_Obj:
        break;
      case Lisp_Fwd_Buffer_Obj:
        TraceEdge(trc, &fwd->u_buffer_objfwd.predicate.v.v, "predicate");
        break;
      case Lisp_Fwd_Kboard_Obj:
        break;
      }
    }
  TraceEdge(trc, &s->function.v.v, "function");
  TraceEdge(trc, &s->plist.v.v, "plist");

  if (s->next) {
    TraceEdge(trc, &s->next->jsval, "next");
  }
}

static void
elisp_symbol_finalize(JSFreeOp* cx, JSObject *obj)
{
  xfree(JS_GetPrivate(obj));
}

static JSClassOps elisp_symbol_ops =
{
  NULL, NULL, NULL, NULL,
  elisp_symbol_resolve, NULL, elisp_symbol_finalize,
  NULL, NULL, NULL, elisp_symbol_trace,
};

JSClass elisp_symbol_class =
  {
   "ELisp_Symbol", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
   &elisp_symbol_ops,
  };

static void
elisp_marker_finalize(JSFreeOp* cx, JSObject *obj)
{
  // XXX unchain marker
  // xfree(JS_GetPrivate(obj));
}

static JSClassOps elisp_marker_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_marker_finalize,
};

JSClass elisp_marker_class = {
                            "ELisp_Marker", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_marker_ops,
};
static void
elisp_overlay_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}

static JSClassOps elisp_overlay_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_overlay_finalize,
};

JSClass elisp_overlay_class = {
                            "ELisp_Overlay", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_overlay_ops,
};
static void
elisp_buffer_finalize(JSFreeOp* cx, JSObject *obj)
{
  xfree(JS_GetPrivate(obj));
}


static JSClassOps elisp_buffer_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_buffer_finalize,
};

JSClass elisp_buffer_class = {
                            "ELisp_Buffer", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_buffer_ops,
};
static void
elisp_module_function_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}

static JSClassOps elisp_module_function_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_module_function_finalize,
};

JSClass elisp_module_function_class = {
                            "ELisp_Module_Function", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_module_function_ops,
};

static void
elisp_string_finalize(JSFreeOp* cx, JSObject *obj)
{
  //fprintf(stderr, "finalizing string %p\n", JS_GetPrivate(obj));
  struct Lisp_String *str = (struct Lisp_String *)JS_GetPrivate(obj);
  if (str && !PURE_P (str))
    {
      //xfree(str->data);
    }
  if (!PURE_P (str))
    xfree(str);
}

static bool elisp_string_call(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = CallArgsFromVp(argc, vp);
  JS::RootedObject obj(cx, &args.callee());
  if (JS_GetClass(obj) != &elisp_string_class)
    return false;

  ELisp_Value fun;
  ELisp_Value ret;
  fun.v.v = args.calleev();
  ELisp_Dynvector argv;
  argv.resize (args.length() + 1);
  argv.sref(0, fun);
  for (ptrdiff_t i = 0; i < args.length(); i++)
    argv.sref(i+1, args[i]);
  fprintf(stderr, "calling 3\n");
  ret = Ffuncall (LV (args.length() + 1, argv));
  fprintf(stderr, "called\n");
  args.rval().set(ret.v.v);

  return true;
}

static void
interval_trace (JSTracer *trc, struct interval *i)
{
  if (i->up_obj)
    TraceEdge(trc, &i->up.obj.v.v, "up object");
  TraceEdge(trc, &i->plist.v.v, "plist");
  if (i->left)
    interval_trace(trc, i->left);
  if (i->right)
    interval_trace(trc, i->right);
}

static void
elisp_string_trace(JSTracer *trc, JSObject *obj)
{
  struct Lisp_String *s = (struct Lisp_String *)JS_GetPrivate(obj);

  if (!s) return;

  TraceEdge(trc, &s->jsval, "jsval");

  if (s->intervals)
    interval_trace(trc, s->intervals);
}

static JSClassOps elisp_string_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_string_finalize,
  NULL, NULL, NULL, elisp_string_trace,
};

JSClass elisp_string_class = {
                            "ELisp_String", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_string_ops,
};
static void
elisp_vector_finalize(JSFreeOp* cx, JSObject *obj)
{
  struct Lisp_Vector *s = (struct Lisp_Vector *)JS_GetPrivate(obj);
  if (PSEUDOVECTOR_TYPEP((struct vectorlike_header *)s, PVEC_BUFFER))
    {
      while (all_buffers == (struct buffer *)s)
        all_buffers = ((struct buffer *)s)->next;
      for (struct buffer *b = all_buffers; b; b = b->next)
        if (b->next == (struct buffer *)s)
          b->next = ((struct buffer *)s)->next;
    }
  else if (PSEUDOVECTOR_TYPEP((struct vectorlike_header *)s, PVEC_SUBR))
    return;
  else if (PSEUDOVECTOR_TYPEP((struct vectorlike_header *)s, PVEC_THREAD))
    return;

  xfree(JS_GetPrivate(obj));
}

#define FACE_CACHE_BUCKETS_SIZE 1001

static void
elisp_vector_trace(JSTracer *trc, JSObject *obj)
{
  struct Lisp_Vector *s = (struct Lisp_Vector *)JS_GetPrivate(obj);

  if (!s) return;

  TraceEdge(trc, &s->header.jsval, "jsval");
  //fprintf(stderr, "tracing vector at %p\n", s);

  ptrdiff_t size = s->header.size;

  if (size & PSEUDOVECTOR_FLAG)
    size &= PSEUDOVECTOR_SIZE_MASK;

  for (ptrdiff_t i = 0; i < size; i++)
    TraceEdge(trc, &s->contents[i].v.v, "vector element");

  if (PSEUDOVECTOR_TYPEP((struct vectorlike_header *)s, PVEC_HASH_TABLE))
    {
      struct Lisp_Hash_Table *h = (struct Lisp_Hash_Table *)s;

      TraceEdge(trc, &h->key_and_value.v.v, "key/value vector");
      TraceEdge(trc, &h->test.name.v.v, "hash table test");
      TraceEdge(trc, &h->test.user_hash_function.v.v, "hash table hash function");
      TraceEdge(trc, &h->test.user_cmp_function.v.v, "hash table cmp function");
    }
  else if (PSEUDOVECTOR_TYPEP((struct vectorlike_header *)s, PVEC_FRAME))
    {
      struct frame *f = (struct frame *)s;
      for (ptrdiff_t i = 0; i < FACE_CACHE_BUCKETS_SIZE; i++)
        {
          struct face *face;
          if (FRAME_FACE_CACHE (f))
            for (face = FRAME_FACE_CACHE (f)->buckets[i]; face; face = face->next)
              {
                for (ptrdiff_t j = 0; j < LFACE_VECTOR_SIZE; j++)
                  TraceEdge (trc, &face->lface[j].v.v, "face cache entry");
              }
        }
  //    struct x_output *x = f->output_data.x;
  //    TraceEdge(trc, &x->name_list_element.v.v, "name_list_element");
    }
  else if (PSEUDOVECTOR_TYPEP((struct vectorlike_header *)s, PVEC_BUFFER))
    {
      struct buffer *b = (struct buffer *)s;
      struct buffer_text *text = b->text;
      if (text && text->intervals)
        interval_trace (trc, text->intervals);
    }
}

extern JSClass elisp_vector_class;

static bool elisp_vector_resolve(JSContext *cx, JS::HandleObject obj,
                                 JS::HandleId id, bool *resolvedp)
{
  struct Lisp_Vector *v = (struct Lisp_Vector *)JS_GetPrivate(obj);
  *resolvedp = false;
  if (JSID_IS_INT(id))
    {
      if (v->header.size & PSEUDOVECTOR_FLAG)
        {
          if (JSID_TO_INT(id) > (v->header.size & PSEUDOVECTOR_SIZE_MASK))
            return true;
        }
      else
        {
          if (JSID_TO_INT(id) > (v->header.size))
            return true;
        }

      JS_SetElement (cx, obj, JSID_TO_INT(id), JS::Rooted<JS::Value>(cx, v->contents[JSID_TO_INT(id)].v.v));
      *resolvedp = true;
      return true;
    }

  return true;
}

static ELisp_Return_Value elisp_vector_call_inner(ELisp_Vector) __attribute__((noinline));

static ELisp_Return_Value elisp_vector_call_voidp(void *lvp)
{
  ELisp_Vector *lv = (ELisp_Vector *)lvp;

  return Ffuncall (*lv);
}

static ELisp_Return_Value elisp_vector_call_handler(ELisp_Handle arg)
{
  if (JS_IsExceptionPending(jsg.cx))
    while (1);

  ELisp_Value exc;
  exc = js_exception_to_elisp_exception(arg);
  JS_SetPendingException(jsg.cx, exc.v.v);

  return Qnil;
}

static ELisp_Return_Value elisp_vector_call_inner(ELisp_Vector *lv, bool *successp)
{
  sys_jmp_buf jmpbuf;
  sys_jmp_buf *volatile old_jmpbuf = catchall_jmpbuf;
  catchall_jmpbuf = &jmpbuf;
  if (sys_setjmp(jmpbuf))
    {
      catchall_jmpbuf = old_jmpbuf;
      *successp = false;
      return Qnil;
    }

  auto ret = internal_catch_all (elisp_vector_call_voidp, static_cast<void *>(lv), elisp_vector_call_handler);
  catchall_jmpbuf = old_jmpbuf;
  *successp = true;
  if (JS_IsExceptionPending(jsg.cx))
    *successp = false;
  return ret;
}

static bool elisp_vector_call(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = CallArgsFromVp(argc, vp);
  JS::RootedObject obj(cx, &args.callee());
  if (JS_GetClass(obj) != &elisp_vector_class &&
      JS_GetClass(obj) != &elisp_cons_class)
    return false;

  ELisp_Value fun;
  fun.v.v = args.calleev();
  ELisp_Dynvector argv;
  argv.resize (args.length() + 1);
  argv.sref(0, fun);
  for (ptrdiff_t i = 0; i < args.length(); i++)
    argv.sref(i+1, args[i]);
  auto lv = LV (args.length() + 1, argv);
  bool success;
  maybe_quit ();
  auto ret = elisp_vector_call_inner(&lv, &success);
  if (success)
    {
      args.rval().set(ret.v);
    }

  return success;
}

static JSClassOps elisp_vector_ops =
{
  NULL, NULL, NULL, NULL,
  elisp_vector_resolve, NULL, elisp_vector_finalize,
  elisp_vector_call, NULL, NULL, elisp_vector_trace,
};

JSClass elisp_vector_class = {
                            "ELisp_Vector", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_vector_ops,
};
static void
elisp_bool_vector_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}

static JSClassOps elisp_bool_vector_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_bool_vector_finalize,
};

JSClass elisp_bool_vector_class = {
                            "ELisp_Bool_Vector", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_bool_vector_ops,
};
static void
elisp_char_table_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}


static JSClassOps elisp_char_table_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_char_table_finalize,
};

JSClass elisp_char_table_class = {
                            "ELisp_Char_Table", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_char_table_ops,
};
static void
elisp_sub_char_table_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}

static JSClassOps elisp_sub_char_table_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_sub_char_table_finalize,
};

JSClass elisp_sub_char_table_class = {
                            "ELisp_Sub_Char_Table", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_sub_char_table_ops,
};
static void
elisp_subr_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}

static JSClassOps elisp_subr_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_subr_finalize,
};

JSClass elisp_subr_class = {
                            "ELisp_subr", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_subr_ops,
};
static void
elisp_thread_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}


static JSClassOps elisp_thread_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_thread_finalize,
};

JSClass elisp_thread_class = {
                            "ELisp_Thread", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_thread_ops,
};
static void
elisp_mutex_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}

static JSClassOps elisp_mutex_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_mutex_finalize,
};

JSClass elisp_mutex_class = {
                            "ELisp_Mutex", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_mutex_ops,
};
static void
elisp_condvar_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}

static JSClassOps elisp_condvar_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_condvar_finalize,
};

JSClass elisp_condvar_class = {
                            "ELisp_CondVar", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_condvar_ops,
};
static void
elisp_save_value_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}

static JSClassOps elisp_save_value_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_save_value_finalize,
};

JSClass elisp_save_value_class = {
                            "ELisp_Save_Value", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_save_value_ops,
};
static void
elisp_finalizer_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}

static JSClassOps elisp_finalizer_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_finalizer_finalize,
};

JSClass elisp_finalizer_class = {
                            "ELisp_Finalizer", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_finalizer_ops,
};
static void
elisp_hash_table_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}

static JSClassOps elisp_hash_table_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_hash_table_finalize,
};

JSClass elisp_hash_table_class = {
                            "ELisp_Hash_Table", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_hash_table_ops,
};
static void
elisp_frame_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}

static JSClassOps elisp_frame_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_frame_finalize,
};

JSClass elisp_frame_class = {
                            "ELisp_Frame", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_frame_ops,
};
static void
elisp_font_spec_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}

static JSClassOps elisp_font_spec_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_font_spec_finalize,
};

JSClass elisp_font_spec_class = {
                            "ELisp_Font_Spec", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_font_spec_ops,
};
static void
elisp_font_entity_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}

static JSClassOps elisp_font_entity_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_font_entity_finalize,
};

JSClass elisp_font_entity_class = {
                            "ELisp_Font_Entity", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_font_entity_ops,
};
static void
elisp_font_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}

static JSClassOps elisp_font_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_font_finalize,
};

JSClass elisp_font_object_class = {
                            "ELisp_Font", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_font_ops,
};
static void
elisp_terminal_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}

static JSClassOps elisp_terminal_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_terminal_finalize,
};

JSClass elisp_terminal_class = {
                            "ELisp_Terminal", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_terminal_ops,
};
static void
elisp_window_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}


static JSClassOps elisp_window_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_window_finalize,
};

JSClass elisp_window_class = {
                            "ELisp_Window", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_window_ops,
};
static void
elisp_window_configuration_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}


static JSClassOps elisp_window_configuration_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_window_configuration_finalize,
};

JSClass elisp_window_configuration_class = {
                            "ELisp_Window_Configuration", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_window_configuration_ops,
};
static void
elisp_process_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}


static JSClassOps elisp_process_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_process_finalize,
};

JSClass elisp_process_class = {
                            "ELisp_Process", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_process_ops,
};
static void
elisp_scroll_bar_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}


static JSClassOps elisp_scroll_bar_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_scroll_bar_finalize,
};

JSClass elisp_scroll_bar_class = {
                            "ELisp_Scroll_Bar", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_scroll_bar_ops,
};
static void
elisp_compiled_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}


static JSClassOps elisp_compiled_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_compiled_finalize,
};

JSClass elisp_compiled_class = {
                            "ELisp_Compiled", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_compiled_ops,
};
static void
elisp_misc_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}

static void
elisp_misc_trace(JSTracer *trc, JSObject *obj)
{
  struct Lisp_Misc_Any *s = (struct Lisp_Misc_Any *)JS_GetPrivate(obj);

  if (!s) return;

  TraceEdge(trc, &s->jsval, "vector element");
  //fprintf(stderr, "tracing misc at %p\n", s);
  switch (s->type) {
  case Lisp_Misc_Marker: {
    struct Lisp_Marker *m = (struct Lisp_Marker *)s;
    if (m->buffer)
      {
        TraceEdge(trc, &((struct Lisp_Vector *)m->buffer)->header.jsval, "buffer");
      }
    break;
  }
  case Lisp_Misc_Overlay: {
    struct Lisp_Overlay *o = (struct Lisp_Overlay *)s;

    TraceEdge(trc, &o->start.v.v, "start");
    TraceEdge(trc, &o->end.v.v, "end");
    TraceEdge(trc, &o->plist.v.v, "plist");

    break;
  }
  case Lisp_Misc_Save_Value: {
    struct Lisp_Save_Value *v = (struct Lisp_Save_Value *)s;

    switch (v->save_type) {
    case SAVE_OBJECT:
      TraceEdge(trc, &v->data[0].object.v.v, "saved object");
      break;
    case SAVE_TYPE_OBJ_OBJ:
      TraceEdge(trc, &v->data[0].object.v.v, "saved object");
      TraceEdge(trc, &v->data[1].object.v.v, "saved object");
      break;
    case SAVE_TYPE_OBJ_OBJ_OBJ:
      TraceEdge(trc, &v->data[0].object.v.v, "saved object");
      TraceEdge(trc, &v->data[1].object.v.v, "saved object");
      TraceEdge(trc, &v->data[2].object.v.v, "saved object");
      break;
    case SAVE_TYPE_OBJ_OBJ_OBJ_OBJ:
      TraceEdge(trc, &v->data[0].object.v.v, "saved object");
      TraceEdge(trc, &v->data[1].object.v.v, "saved object");
      TraceEdge(trc, &v->data[2].object.v.v, "saved object");
      TraceEdge(trc, &v->data[3].object.v.v, "saved object");
      break;
    case SAVE_TYPE_PTR_OBJ:
      TraceEdge(trc, &v->data[1].object.v.v, "saved object");
      break;
    case SAVE_TYPE_FUNCPTR_PTR_OBJ:
      TraceEdge(trc, &v->data[2].object.v.v, "saved object");
      break;
    }
  }
  case Lisp_Misc_Finalizer:
    ;
  }
}

static JSClassOps elisp_misc_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_misc_finalize,
  NULL, NULL, NULL, elisp_misc_trace,
};

JSClass elisp_misc_class = {
                            "ELisp_Misc", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_misc_ops,
};
static void
elisp_miscany_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}

static JSClassOps elisp_exception_ops =
  {
   NULL,
  };

JSClass elisp_exception_class =
  {
   "ELisp_Exception", 0,
   &elisp_exception_ops
  };

static JSClassOps elisp_miscany_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_miscany_finalize,
};

JSClass elisp_misc_any_class = {
                            "ELisp_MiscAny", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_miscany_ops,
};
static void
elisp_vectorlike_finalize(JSFreeOp* cx, JSObject *obj)
{
  xfree(JS_GetPrivate(obj));
}


static JSClassOps elisp_vectorlike_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_vectorlike_finalize,
};

JSClass elisp_vectorlike_class = {
                            "ELisp_Vectorlike", JSCLASS_HAS_PRIVATE|JSCLASS_FOREGROUND_FINALIZE,
                            &elisp_vectorlike_ops,
};

static ELisp_Return_Value
allocate_misc(int type)
{
  struct Lisp_Misc_Any *misc = (struct Lisp_Misc_Any *)xmalloc(sizeof Lisp_Misc);

  JS::RootedObject obj(global_js_context, JS_NewObject(global_js_context, &marker_class));

  JS_SetPrivate(obj, misc);

  ELisp_Value ret;
  ret.v.setObject(*obj);
  return ret;
}

static bool
elisp_string_toString(JSContext* cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  if (!args.thisv().isObject())
    return false;

  ELisp_Value s;
  s.v.v = args.thisv();

  unsigned char *bytes = SDATA(s);
  size_t len = SBYTES(s);
  JS::RootedString res(cx, JS_NewStringCopyN(cx, reinterpret_cast<char *>(bytes), len));
  if (!res)
    return false;

  args.rval().setString(res);

  return true;
}

static JSFunctionSpec elisp_string_fns[] = {
                                      JS_FN("toString", elisp_string_toString, 0, 0),
                                      JS_FS_END
};


static void
elisp_classes_init(JSContext *cx, JS::HandleObject glob)
{
  JS_InitClass(cx, glob, nullptr, &F_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS::RootedObject elisp_cons_proto
    (cx, JS_InitClass(cx, glob, nullptr, &elisp_cons_class, nullptr, 0,
                      nullptr, nullptr, nullptr, nullptr));

  elisp_cons_class_proto = elisp_cons_proto;

  JS_SetProperty (cx, glob, "ELisp_Cons_Proto", JS::Rooted<JS::Value>(cx, JS::ObjectValue(*elisp_cons_class_proto)));
  elisp_vector_class_proto = JS_InitClass(cx, glob, nullptr, &elisp_vector_class, nullptr, 0,
                                    nullptr, nullptr, nullptr, nullptr);
  elisp_symbol_class_proto = JS_InitClass(cx, glob, nullptr, &elisp_symbol_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_marker_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_overlay_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_buffer_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_module_function_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  elisp_string_class_proto = JS_InitClass(cx, glob, nullptr, &elisp_string_class, nullptr, 0,
               nullptr, elisp_string_fns, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_overlay_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_bool_vector_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS::RootedObject elisp_vector_proto(cx, elisp_vector_class_proto);
  JS_InitClass(cx, glob, elisp_vector_proto, &elisp_char_table_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, elisp_vector_proto, &elisp_sub_char_table_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_subr_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_thread_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_mutex_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_condvar_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_save_value_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_finalizer_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_hash_table_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_frame_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_font_entity_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_font_spec_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_font_object_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_terminal_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_window_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_window_configuration_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_process_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_scroll_bar_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_compiled_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  elisp_misc_class_proto = JS_InitClass(cx, glob, nullptr, &elisp_misc_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_misc_any_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_vectorlike_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  elisp_exception_class_proto = JS_InitClass(cx, glob, nullptr, &elisp_exception_class, nullptr, 0, nullptr, nullptr, nullptr, nullptr);
  memcpy(&elisp_cons_class_proto_backup, &elisp_cons_class_proto, sizeof elisp_cons_class_proto);
  memcpy(&elisp_string_class_proto_backup, &elisp_string_class_proto, sizeof elisp_cons_class_proto);
  memcpy(&elisp_symbol_class_proto_backup, &elisp_symbol_class_proto, sizeof elisp_cons_class_proto);
  memcpy(&elisp_vector_class_proto_backup, &elisp_vector_class_proto, sizeof elisp_cons_class_proto);
  memcpy(&elisp_misc_class_proto_backup, &elisp_misc_class_proto, sizeof elisp_cons_class_proto);
  memcpy(&elisp_exception_class_proto_backup, &elisp_exception_class_proto, sizeof elisp_cons_class_proto);
}

void jsprint(JS::Value v)
{
  ELisp_Value x;
  x.v.v = v;
  printf("%lx\n", *(long *)&v);
  debug_print(x);
  printf("%lx\n", *(long *)&v);
}

void jsprintl(unsigned long l)
{
  jsprint(*(JS::Value *)&l);
}

void
syms_of_js (void)
{
  defsubr(&Sjs);
  defsubr(&Sjsread);
  defsubr(&Sjsdrain);
  defsubr(&Sjsglobal);
  defsubr(&Sjsderef);
  defsubr(&Sjsmethod);
  defsubr(&Sjspromise);
  defsubr(&Sspecbind);
  defsubr(&Ssetinternal);
  defsubr(&Sunbind_to_rel);
}

void js::ReportOutOfMemory(JSContext* cx)
{
  while (1);
}

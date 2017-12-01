// shell g++ -ggdb -g3 -std=c++11 -I ../src/ -I ../js/dist/include/ ./js.cpp -L ../js/dist/bin/ -lz -lpthread -ldl -lmozjs-58a1 -Wl,--whole-archive ../js/mozglue/build/libmozglue.a -Wl,--no-whole-archive -pthread
#include "config.h.hh"

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

typedef int64_t EMACS_INT;
typedef uint64_t EMACS_UINT;

class CLisp_Vectorlike {
};

class CLisp_String {
};

class CLisp_Symbol {
};

class CLisp_Misc {
};

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
                               JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(3),
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
                               JSCLASS_HAS_PRIVATE|JSCLASS_HAS_RESERVED_SLOTS(5),
                               &marker_class_ops,
};

static void
misc_finalize(JSFreeOp* cx, JSObject *obj)
{
  // xfree(JS_GetPrivate(obj));
}


static JSClassOps misc_class_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, misc_finalize,
};

static JSClass misc_class = {
                             "ELisp_Misc",
                             JSCLASS_HAS_PRIVATE,
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
global_enumerate(JSContext* cx, JS::HandleObject obj, JS::AutoIdVector& properties,
                 bool enumerableOnly)
{
    return true;
}

static bool
global_resolve(JSContext* cx, JS::HandleObject obj, JS::HandleId id, bool* resolvedp)
{
    return true;
}

static bool
global_mayResolve(const JSAtomState& names, jsid id, JSObject* maybeObj)
{
    return JS_MayResolveStandardClass(names, id, maybeObj);
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

static const JSFunctionSpec shell_functions[] =
  {
   JS_FN("print", Print, 0, 0),
   JS_FS_END
  };
                                                         void
WarningReporter(JSContext* cx, JSErrorReport* report)
{
  fprintf(stderr, "Warning!\n");
}

extern JS_PUBLIC_API(bool)
JS_AddExtraGCRootsTracer(JSContext* cx, JSTraceDataOp traceOp, void* data);

enum { NSTATICS = 2048 };
extern ELisp_Struct_Value *staticvec[NSTATICS];
extern ELisp_Heap_Value executing_kbd_macro;
extern KBOARD *all_kboards;
extern struct buffer alignas (GCALIGNMENT) buffer_defaults;
extern struct buffer alignas (GCALIGNMENT) buffer_local_symbols;
extern struct buffered_input_event kbd_buffer[KBD_BUFFER_SIZE];

static void
elisp_vector_trace(JSTracer *trc, JSObject *obj);

static void
js_gc_trace(JSTracer* tracer, void* data)
{
  fprintf(stderr, "that one's mine! And that one! And...\n");

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

  for (struct specbinding *pdl = specpdl_ptr - 1; pdl >= specpdl; pdl--) {
    TraceEdge(tracer, &pdl->unwind.arg.v.v, "unwind");
    TraceEdge(tracer, &pdl->let.where.v.v, "where");
    TraceEdge(tracer, &pdl->let.symbol.v.v, "symbol");
    TraceEdge(tracer, &pdl->let.old_value.v.v, "old value");
    TraceEdge(tracer, &pdl->let.saved_value.v.v, "where");
    ptrdiff_t nargs = pdl->bt.nargs;
    TraceEdge(tracer, &pdl->bt.function.v.v, "backtrace function");
  }

  for (struct specbinding *pdl = specpdl_ptr; pdl < specpdl + specpdl_size; pdl++) {
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

bool js_init()
{
  //js::DisableExtraThreads()

  if (!JS_Init())
    return false;

  JSContext *cx = JS_NewContext(2 * JS::DefaultHeapMaxBytes, JS::DefaultNurseryBytes);
  global_js_context = cx;
  if (!cx)
    return false;
  //JS_SetFutexCanWait(cx);
  JS::SetWarningReporter(cx, WarningReporter);
  //JS_SetGCParameter(cx, JSGC_MAX_BYTES, 0x7ffffffffL);

  JS_SetNativeStackQuota(cx, 8 * 1024 * 1024);

  if (!JS::InitSelfHostedCode(cx))
    return false;

  JS_SetGCParameter(cx, JSGC_MODE, JSGC_MODE_INCREMENTAL);

  {
    JS_BeginRequest(cx);
    JS::CompartmentOptions compartment_options;
    JS::RootedObject glob(cx, JS_NewGlobalObject(cx, &global_class, nullptr, JS::FireOnNewGlobalHook, compartment_options));

    if (!glob)
      return false;

    {
      JS_EnterCompartment (cx, glob);
      if (!JS_DefineFunctions(cx, glob, shell_functions))
        {
          return false;
        }
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
    return false;

  if (!JS_ExecuteScript(cx, script))
    return false;
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
  else if (arg.v.v.isInt32() || arg.v.v.isDouble() || arg.v.v.isObject())
    return arg;
  else
    return Qnil;
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
  size_t sourcelen = strlen(source);
  JS::CompileOptions options(cx);
  options.setIntroductionType("js shell interactive")
    .setUTF8(true)
    .setIsRunOnce(true)
    .setFileAndLine("typein", 1);

  if (!JS::Compile(cx, options, source, sourcelen, &script))
    return Qnil;

  if (!JS_ExecuteScript(cx, script, &result.v.v))
    return Qnil;

  return jsval_to_elisp(result);
}

JSG jsg __attribute__((init_priority(101)));

JSContext* global_js_context;

extern JSContext* global_js_context;

static void
elisp_cons_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}

static void
elisp_cons_trace(JSTracer *trc, JSObject *obj)
{
  struct Lisp_Cons *s = JS_GetPrivate(obj);

  if (!s) return;

  //fprintf(stderr, "tracing cons at %p\n", s);

  TraceEdge(trc, &s->jsval, "jsval");
  TraceEdge(trc, &s->car.v.v, "car");
  TraceEdge(trc, &s->u.cdr.v.v, "cdr");
}

static JSClassOps elisp_cons_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_cons_finalize,
  NULL, NULL, NULL, elisp_cons_trace
};

JSClass elisp_cons_class = {
                            "ELisp_Cons", JSCLASS_HAS_PRIVATE,
                            &elisp_cons_ops,
};


static void
elisp_symbol_trace(JSTracer *trc, JSObject *obj)
{
  struct Lisp_Symbol *s = JS_GetPrivate(obj);

  if (!s) return;

  //fprintf(stderr, "tracing symbol at %p: %s\n", s, (XSTRING (s->name))->data);

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
  NULL, NULL, elisp_symbol_finalize,
  NULL, NULL, NULL, elisp_symbol_trace,
};

JSClass elisp_symbol_class = {
                            "ELisp_Symbol", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Marker", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Overlay", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Buffer", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Module_Function", JSCLASS_HAS_PRIVATE,
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
  struct Lisp_String *s = JS_GetPrivate(obj);

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
                            "ELisp_String", JSCLASS_HAS_PRIVATE,
                            &elisp_string_ops,
};
static void
elisp_vector_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}

#define FACE_CACHE_BUCKETS_SIZE 1001

static void
elisp_vector_trace(JSTracer *trc, JSObject *obj)
{
  struct Lisp_Vector *s = JS_GetPrivate(obj);

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

static JSClassOps elisp_vector_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_vector_finalize,
  NULL, NULL, NULL, elisp_vector_trace,
};

JSClass elisp_vector_class = {
                            "ELisp_Vector", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Bool_Vector", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Char_Table", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Sub_Char_Table", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_subr", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Thread", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Mutex", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_CondVar", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Save_Value", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Finalizer", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Hash_Table", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Frame", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Font_Spec", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Font_Entity", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Font", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Terminal", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Window", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Window_Configuration", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Process", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Scroll_Bar", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Compiled", JSCLASS_HAS_PRIVATE,
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
  struct Lisp_Misc_Any *s = JS_GetPrivate(obj);

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
                            "ELisp_Misc", JSCLASS_HAS_PRIVATE,
                            &elisp_misc_ops,
};
static void
elisp_miscany_finalize(JSFreeOp* cx, JSObject *obj)
{
  //xfree(JS_GetPrivate(obj));
}



static JSClassOps elisp_miscany_ops =
{
  NULL, NULL, NULL, NULL,
  NULL, NULL, elisp_miscany_finalize,
};

JSClass elisp_misc_any_class = {
                            "ELisp_MiscAny", JSCLASS_HAS_PRIVATE,
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
                            "ELisp_Vectorlike", JSCLASS_HAS_PRIVATE,
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


static void
elisp_classes_init(JSContext *cx, JS::HandleObject glob)
{
  JS_InitClass(cx, glob, nullptr, &elisp_cons_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS::RootedObject elisp_vector_proto
    (cx,
     JS_InitClass(cx, glob, nullptr, &elisp_vector_class, nullptr, 0,
                  nullptr, nullptr, nullptr, nullptr));
  JS_InitClass(cx, glob, nullptr, &elisp_symbol_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_marker_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_overlay_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_buffer_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_module_function_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_string_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_overlay_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_bool_vector_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
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
  JS_InitClass(cx, glob, nullptr, &elisp_misc_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_misc_any_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
  JS_InitClass(cx, glob, nullptr, &elisp_vectorlike_class, nullptr, 0,
               nullptr, nullptr, nullptr, nullptr);
}

#if 0
void jsprint(Lisp_Object *xp)
{
  Lisp_Object x = *xp;
  JSContext* cx = jsg.cx;
  if (x.v.isObject()) {
    JSObject *obj = &x.v.toObject();
    printf("%p %s %p\n", obj, JS_GetClass(obj)->name,
           JS_GetPrivate(obj));
    if (strcmp(JS_GetClass(obj)->name, "ELisp_Cons") == 0) {
      printf("car ");
      jsprint(&XCONS(x)->car);
      printf("cdr ");
      jsprint(&XCONS(x)->u.cdr);
    } else if (strcmp(JS_GetClass(obj)->name, "ELisp_String") == 0) {
      printf("= %s\n", SDATA(x));
    } else if (strcmp(JS_GetClass(obj)->name, "ELisp_Symbol") == 0) {
      printf("= '\n");
      jsprint(&XSYMBOL(x)->name);
    }
  } else {
    JS::RootedString str(cx, JS::ToString(cx, JS::Handle<JS::Value>::fromMarkedLocation(&x.v)));
    if (!str)
      return;
    char* bytes = JS_EncodeStringToUTF8(cx, str);
    if (!bytes)
      return;
    fprintf(stdout, "%s\n", bytes);
    JS_free(cx, bytes);
  }
}
#endif

void
syms_of_js (void)
{
  defsubr(&Sjs);
}

void js::ReportOutOfMemory(JSContext* cx)
{
  while (1);
}

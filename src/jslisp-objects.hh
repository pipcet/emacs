extern EMACS_INT js_hash_object (JS::HandleObject obj);

#define FORWARDED_COMMON                                \
  operator JS::Value() const                            \
  {                                                     \
    return v;                                           \
  }                                                     \
                                                        \
  bool isNull() { return v.isNull(); }                  \
  bool isObject() { return v.isObject(); }              \
  bool isInt32() { return v.isInt32(); }                \
  bool isSymbol() { return v.isSymbol(); }              \
  bool isNumber() { return v.isNumber(); }              \
  bool isDouble() { return v.isDouble(); }              \
  bool isString() { return v.isString(); }              \
  bool isCallable() {                                   \
    return isObject() && JS::IsCallable(&toObject());   \
  }                                                     \
                                                        \
  JSObject& toObject() { return v.toObject(); }         \
  int32_t toInt32() { return v.toInt32(); }             \
  auto toSymbol() { return v.toSymbol(); }              \
  double toNumber() { return v.toNumber(); }            \
  double toDouble() { return v.toDouble(); }            \
  auto toString() { return v.toString(); }              \
  auto asRawBits() { return v.asRawBits(); }

#define FORWARDED                                       \
  FORWARDED_COMMON                                      \
  void setNull() { v.setNull(); }                       \
  void setObject(JSObject &obj) { v.setObject(obj); }   \
  void setInt32(int32_t x) { v.setInt32(x); }           \
  void setDouble(double x) { v.setDouble(x); }

#define FORWARDED_RO                                 \
  FORWARDED_COMMON                                   \
  void setNull() { emacs_abort(); }                  \
  void setObject(JSObject &obj) { emacs_abort(); }   \
  void setInt32(int32_t x) { emacs_abort(); }        \
  void setDouble(double x) { emacs_abort(); }        \
  void set(JSReturnValue v)                          \
  {                                                  \
    emacs_abort();                                   \
  }

#define ARRAY_PROPS                                        \
  ELisp_Return_Value get_element (int32_t index);          \
  void set_element (int32_t index, ELisp_Handle el);       \
  ELisp_Return_Value get_property (const char *prop);      \
  void set_property (const char *prop, ELisp_Handle el);   \
  ELisp_Return_Value operator[](int32_t index) {           \
    return get_element(index);                             \
  }

#define XCLASS(name, clas, c_class)                                     \
  inline void xset ## name (c_class x)                                  \
  {                                                                     \
    if (!x) {                                                           \
      V.setNull();                                                      \
      return;                                                           \
    }                                                                   \
    JS::RootedValue val(jsg.cx, *(JS::Heap<JS::Value> *)x);             \
    if (val.isObject() && JS_GetClass (&val.toObject()) == &clas)       \
      V.set(val);                                                       \
    else {                                                              \
      JS::RootedObject proto(jsg.cx, clas ## _proto);                   \
      JS::RootedObject obj(jsg.cx, JS_NewObjectWithGivenProto(jsg.cx, &clas, proto)); \
      js_hash_object (obj);                                             \
      JS_SetPrivate(obj, x);                                            \
      V.setObject(*obj); /* XXX check error */                          \
      JS::RootedValue val2(jsg.cx, JS::Value(V));                       \
      *(JS::Heap<JS::Value> *)x = val2;                                 \
    }                                                                   \
  }                                                                     \
                                                                        \
  inline bool name ## p ()                                              \
  {                                                                     \
    if (!V.isObject())                                                  \
      return false;                                                     \
                                                                        \
    return JS_GetClass (&V.toObject()) == &clas;                        \
  }                                                                     \
                                                                        \
  inline c_class x ## name ()                                           \
  {                                                                     \
    return (c_class)JS_GetPrivate(&V.toObject());                       \
  }

#define XALL                                                            \
  XCLASS(vector, elisp_vector_class, struct Lisp_Vector *);             \
                                                                        \
  inline bool                                                           \
  symbolp ()                                                            \
  {                                                                     \
    if (!V.isObject())                                                  \
      return false;                                                     \
                                                                        \
    return JS_GetClass (&V.toObject()) == &elisp_symbol_class;          \
  }                                                                     \
                                                                        \
  inline bool                                                           \
  consp ()                                                              \
  {                                                                     \
    if (!V.isObject())                                                  \
      return false;                                                     \
                                                                        \
    return JS_GetClass (&V.toObject()) == &elisp_cons_class;            \
  }                                                                     \
                                                                        \
  inline void                                                           \
  xsetcons ()                                                           \
  {                                                                     \
    V.set(elisp_cons());                                                \
  }                                                                     \
                                                                        \
  inline EMACS_INT                                                      \
  xint ()                                                               \
  {                                                                     \
    return V.toInt32();                                                 \
  }                                                                     \
                                                                        \
  inline EMACS_UINT                                                     \
  xuint ()                                                              \
  {                                                                     \
    return (uint32_t) V.toInt32();                                      \
  }                                                                     \
                                                                        \
  inline EMACS_INT                                                      \
  xfastint ()                                                           \
  {                                                                     \
    return V.toInt32();                                                 \
  }                                                                     \
                                                                        \
  inline EMACS_INT                                                      \
  xhash ()                                                              \
  {                                                                     \
    if (V.isObject())                                                   \
      {                                                                 \
        JS::RootedObject obj(jsg.cx, &V.toObject());                    \
        return js_hash_object(obj);                                     \
      }                                                                 \
    return 0;                                                           \
  }                                                                     \
                                                                        \
  inline void                                                           \
  xsetint (EMACS_INT x)                                                 \
  {                                                                     \
    V.setInt32(x);                                                      \
  }                                                                     \
                                                                        \
  inline void                                                           \
  xsetfastint (EMACS_INT x)                                             \
  {                                                                     \
    V.setInt32(x);                                                      \
  }                                                                     \
                                                                        \
  inline void xsetfloat (double f)                                      \
  {                                                                     \
    V.setDouble (f);                                                    \
  }                                                                     \
                                                                        \
  inline enum Lisp_Type xtype ()                                        \
  {                                                                     \
    if (V.isSymbol ())                                                  \
      return Lisp_Symbol;                                               \
    if (V.isObject ())                                                  \
      {                                                                 \
        const JSClass *clasp = JS_GetClass(&V.toObject());              \
                                                                        \
        if (clasp == &elisp_cons_class)                                 \
          return Lisp_Cons;                                             \
        else if (clasp == &elisp_vector_class)                          \
          return Lisp_Vectorlike;                                       \
        else if (clasp == &elisp_symbol_class)                          \
          return Lisp_Symbol;                                           \
        else if (clasp == &elisp_string_class)                          \
          return Lisp_String;                                           \
        return Lisp_JSValue;                                            \
      }                                                                 \
    if (V.isInt32 ())                                                   \
      return Lisp_Int0;                                                 \
    if (V.isString ())                                                  \
      return Lisp_JSValue;                                              \
    if (V.isDouble ())                                                  \
      return Lisp_Float;                                                \
                                                                        \
    return Lisp_JSValue;                                                \
  }                                                                     \
                                                                        \
  inline bool                                                           \
  integerp ()                                                           \
  {                                                                     \
    return V.isInt32();                                                 \
  }                                                                     \
                                                                        \
  inline bool floatp ()                                                 \
  {                                                                     \
    return V.isDouble();                                                \
  }                                                                     \
                                                                        \
  inline double xfloat ()                                               \
  {                                                                     \
    return V.toNumber();                                                \
  }                                                                     \
                                                                        \
  inline bool eq (const JSReturnValue &v2)                              \
  {                                                                     \
    return V.asRawBits() == v2.v.asRawBits();                           \
  }                                                                     \

class JSReturnValue;
class JSHeapValue;
class JSStackValue;
class JSHandleValue;

class Lisp_Value_Return;
class Lisp_Value_Heap;
class Lisp_Value_Stack;
class Lisp_Value_Handle;

typedef Lisp_Value_Handle ELisp_Handle;
typedef Lisp_Value_Heap ELisp_Heap_Value;
typedef Lisp_Value_Heap ELisp_Struct_Value;
typedef Lisp_Value_Stack ELisp_Value;
typedef JSReturnValue ELisp_Return_Value;

extern ELisp_Return_Value elisp_cons (void);
extern ELisp_Return_Value elisp_array (int32_t);
extern void elisp_array_resize (ELisp_Handle, int32_t);

class JSReturnValue {
public:
  JS::Value v;
  static const int nothandle = 1;

  JSReturnValue() {}
  JSReturnValue(const JS::Value &v) : v(v) {}
  JSReturnValue(const JS::RootedValue &v) : v(v) {}
  JSReturnValue(const JS::Heap<JS::Value> &v) : v(v) {}
  JSReturnValue(const JS::Handle<JS::Value> &v) : v(v) {}
  JSReturnValue(const JS::MutableHandle<JS::Value> &v) : v(v) {}
  JSReturnValue(const JSHeapValue &);
  JSReturnValue(const JSStackValue &);
  JSReturnValue(const JSHandleValue &);
  JSReturnValue(const Lisp_Value_Return &);
  JSReturnValue(const Lisp_Value_Heap &);
  JSReturnValue(const Lisp_Value_Stack &);
  JSReturnValue(const Lisp_Value_Handle &);

  FORWARDED
#define V (*this)
  XALL
#undef V

  void set(JS::Value v2)
  {
    v = v2;
  }

  JS::Value get() const
  {
    return v;
  }
};

class JSHeapValue {
public:
  JS::Heap<JS::Value> v;
  static const int nothandle = 1;

  JSHeapValue() {}
  JSHeapValue(JSReturnValue v2) : v(v2) {}

  operator JSReturnValue()
  {
    return v;
  }

  operator JS::Heap<JS::Value>&()
  {
    return v;
  }

  operator JS::Value()
  {
    return v;
  }

  JSHeapValue operator=(JSReturnValue v2)
  {
    v = v2;
    return *this;
  }

  FORWARDED

  void set(JS::Value v2)
  {
    v = v2;
  }
};

class JSStackValue {
public:
  JS::Rooted<JS::Value> v;

  JSStackValue() : v(jsg.cx) {
    //printf("[A] %p %p\n", &v, v.previous());
  }

  JSStackValue(const JSReturnValue &v) : v(jsg.cx) {
    this->v = v.v;
    //printf("[B] %p %p\n", &this->v, this->v.previous());
  }

  JSStackValue(const JSStackValue &) = delete;
  JSStackValue(JSStackValue &&) = delete;

  operator JSReturnValue() const {
    return v.get();
  }

  JSStackValue &operator=(const JSStackValue &other)
  {
    (v = other.v).get();
    return *this;
  }

  operator JS::Rooted<JS::Value>&()
  {
    return v;
  }

  JSReturnValue operator=(JSHeapValue v2)
  {
    return JSReturnValue(v = v2.v);
  }

  JSReturnValue operator=(JSReturnValue v2)
  {
    return JSReturnValue(v = v2);
  }

  FORWARDED

  void set(JS::Value v2)
  {
    v.set(v2);
  }

#define V v
  XALL
#undef V
};

class JSHandleValue {
public:
  JS::HandleValue v;

  JSHandleValue() : v(JS::HandleValue::fromMarkedLocation(nullptr)) {
  }

  JSHandleValue(const JSReturnValue* v) : v(JS::HandleValue::fromMarkedLocation((JS::Value*)v)) {}
  JSHandleValue(const JS::Heap<JS::Value> *v) : v(JS::HandleValue::fromMarkedLocation(v->address())) {}
  JSHandleValue(const JS::Rooted<JS::Value> *v) : v(JS::HandleValue::fromMarkedLocation(v->address())) {}
  JSHandleValue(JS::HandleValue const &v) : v(JS::HandleValue::fromMarkedLocation(const_cast<const JS::Value *>(v.address()))) {}

  operator JS::HandleValue&()
  {
    return v;
  }

  FORWARDED_RO
};

class Lisp_Value_Return {
public:
  JSReturnValue v;

  Lisp_Value_Return() {}
  Lisp_Value_Return(const JSReturnValue v) : v(v) {}
  explicit Lisp_Value_Return(JSHeapValue v) : v(v) {}

  Lisp_Value_Return(Lisp_Value_Return const&r) {
    v.set(JS::Value(JSReturnValue(r.v)));
  };
  inline Lisp_Value_Return(Lisp_Value_Stack const&);
  inline Lisp_Value_Return(Lisp_Value_Handle const);
  //Lisp_Value(Lisp_Value &&) = delete;

  operator JSReturnValue() {
    return v;
  }

  Lisp_Value_Return &operator=(JSReturnValue v2) {
    v = v2;
    return *this;
  }
  inline Lisp_Value_Return &operator=(Lisp_Value_Stack &v2);
  inline Lisp_Value_Return &operator=(Lisp_Value_Handle v2);

  FORWARDED
#define V v
  XALL
#undef V
};

class Lisp_Value_Heap {
public:
  JSHeapValue v;

  Lisp_Value_Heap() {}
  Lisp_Value_Heap(const JSReturnValue v) : v(v) {}
  explicit Lisp_Value_Heap(JSHeapValue v) : v(v) {}

  Lisp_Value_Heap(Lisp_Value_Heap const&r) {
    v.set(JS::Value(JSReturnValue(r.v)));
  };
  inline Lisp_Value_Heap(Lisp_Value_Stack const&);
  inline Lisp_Value_Heap(Lisp_Value_Handle const);
  //Lisp_Value(Lisp_Value &&) = delete;

  operator JSReturnValue() {
    return v;
  }

  Lisp_Value_Heap &operator=(JSReturnValue v2) {
    v = v2;
    return *this;
  }
  inline Lisp_Value_Heap &operator=(Lisp_Value_Stack &v2);
  inline Lisp_Value_Heap &operator=(Lisp_Value_Handle v2);

  FORWARDED
#define V v
  XALL
#undef V
};

class Lisp_Value_Stack {
public:
  JSStackValue v;

  Lisp_Value_Stack() {
    //printf("[9] %p %p\n", &this->v.v, this->v.v.previous());
  }
  Lisp_Value_Stack(const JSReturnValue v) {
    this->v = v;
  }

  Lisp_Value_Stack(const Lisp_Value_Heap & v) {
    this->v = JSReturnValue(v);
  }
  Lisp_Value_Stack(const Lisp_Value_Handle v);
  explicit Lisp_Value_Stack(JSStackValue v) {
    this->v = v;
  }

#if 0
  Lisp_Value_Stack(Lisp_Value_Stack & r) {
    v.v.set(r.v.v);
    v.v.prev = r.v.v.prev;
    *v.v.stack = reinterpret_cast<JS::Rooted<void*>*>(&r.v.v);
    r.v.v.prev = reinterpret_cast<JS::Rooted<void*>*>(&v.v);
  }
#else
  Lisp_Value_Stack(const Lisp_Value_Stack &) = delete;
#endif

  Lisp_Value_Stack(Lisp_Value_Stack && r) {
    v.v.set(r.v.v);
    v.v.prev = r.v.v.prev;
    *v.v.stack = reinterpret_cast<JS::Rooted<void*>*>(&r.v.v);
    r.v.v.prev = reinterpret_cast<JS::Rooted<void*>*>(&v.v);
  }

  ~Lisp_Value_Stack()
  {
    //printf("~%p %p %p %p\n", this, v.v.prev, *v.v.stack, v.v.prev ? v.v.prev->prev : 0);
  }

#if 0
  Lisp_Value_Stack(Lisp_Value_Stack &&r) {
    if (&r != this)
      {
        {
          auto tmp = r.v.v.stack;
          r.v.v.stack = v.v.stack;
          v.v.stack = tmp;
        }
        {
          auto tmp = r.v.v.prev;
          r.v.v.prev = v.v.prev;
          v.v.prev = tmp;
        }
        {
          auto tmp = r.v.v.ptr;
          r.v.v.ptr = v.v.ptr;
          v.v.ptr = tmp;
        }
      }
  }
#endif

  operator JSReturnValue() {
    return v;
  }

  Lisp_Value_Stack &operator=(JSReturnValue v2) {
    v.v = v2.v;
    return *this;
  }
  Lisp_Value_Stack &operator=(Lisp_Value_Heap v2) {
    v.v = v2.v;
    return *this;
  }
  Lisp_Value_Stack &operator=(const Lisp_Value_Stack & v2) {
    return *this = JSReturnValue(v2);
  }
  inline Lisp_Value_Stack &operator=(const Lisp_Value_Handle &v2);

  FORWARDED
#define V v.v
  XALL
  ARRAY_PROPS
#undef V
};

class Lisp_Value_Handle {
public:
  JSHandleValue v;

  Lisp_Value_Handle() {
  }

  Lisp_Value_Handle(const Lisp_Value_Stack &v) : v(v.v.v) {}
  Lisp_Value_Handle(const Lisp_Value_Heap &v) : v(JS::HandleValue::fromMarkedLocation(const_cast<JS::Value*>(v.v.v.address()))) {}
  explicit Lisp_Value_Handle(JSHandleValue &v) : v(v) {}

  operator JS::HandleValue() {
    return v;
  }

  operator JSHandleValue() {
    return v;
  }

  Lisp_Value_Handle &operator=(JSReturnValue v2);
  Lisp_Value_Handle &operator=(Lisp_Value_Heap v2);

  FORWARDED
#define V v
  XALL
  ARRAY_PROPS
#undef V
};

class ELisp_Pointer {
public:
  enum {
        UNSAFE, STACK, HEAP
  } type;
  union {
    JSReturnValue *unsafe;
    Lisp_Value_Stack *stack;
    Lisp_Value_Heap *heap;
  } u;
  ELisp_Pointer()
  {
    type = UNSAFE;
    u.unsafe = 0;
  }
  ELisp_Pointer(JSReturnValue* x)
  {
    type = UNSAFE;
    u.unsafe = x;
  }
  ELisp_Pointer(Lisp_Value_Stack* x)
  {
    type = STACK;
    u.stack = x;
  }
  ELisp_Pointer(Lisp_Value_Heap* x)
  {
    type = HEAP;
    u.heap = x;
  }
  ELisp_Pointer(Lisp_Value_Handle *x);
  JSReturnValue operator*();
  /*

  {
    switch (type) {
    case UNSAFE:
      return *u.unsafe;
    case HEAP:
      return u.heap->v;
    case STACK:
      return u.stack->v;
    }
    asm volatile("ud2");
    emacs_abort();
  }
  */
  ELisp_Pointer operator+(ptrdiff_t off)
  {
    switch (type) {
    case UNSAFE:
      return ELisp_Pointer(u.unsafe + off);
    case HEAP:
      return ELisp_Pointer(&u.heap[off]);
    case STACK:
      return ELisp_Pointer(&u.stack[off]);
    }
    __builtin_unreachable ();
  }
  ELisp_Pointer operator-(ptrdiff_t off)
  {
    switch (type) {
    case UNSAFE:
      return ELisp_Pointer(u.unsafe - off);
    case HEAP:
      return ELisp_Pointer(&u.heap[-off]);
    case STACK:
      return ELisp_Pointer(&u.stack[-off]);
    }
    __builtin_unreachable ();
  }
  ELisp_Pointer operator+=(ptrdiff_t off)
  {
    switch (type) {
    case UNSAFE:
      return *this = ELisp_Pointer(u.unsafe + off);
    case HEAP:
      return *this = ELisp_Pointer(&u.heap[off]);
    case STACK:
      return *this = ELisp_Pointer(&u.stack[off]);
    }
    __builtin_unreachable ();
  }
  ptrdiff_t operator-(ELisp_Pointer other)
  {
    if (other.type != type)
      asm volatile("ud2");
    switch (type) {
    case UNSAFE:
      return other.u.unsafe - u.unsafe;
    case HEAP:
      return other.u.heap - u.heap;
    case STACK:
      return other.u.stack - u.stack;
    }
    __builtin_unreachable ();
  }
  ELisp_Pointer operator++(int)
  {
    auto ret = *this;
    *this = *this + 1;
    return ret;
  }
  ELisp_Pointer& operator++()
  {
    return *this = *this + 1;
  }
  ELisp_Pointer operator--(int)
  {
    auto ret = *this;
    *this = *this + (-1);
    return ret;
  }
  ELisp_Pointer& operator--()
  {
    return *this = *this + (-1);
  }
  const JSReturnValue operator[](ptrdiff_t off) const;
  /*  {
    switch (type) {
    case UNSAFE:
      return u.unsafe[off];
    case HEAP:
      return u.heap[off];
    case STACK:
      return *(JSReturnValue *)(u.stack[off].v.v.address());
    }
    asm volatile("ud2");
    emacs_abort();
    } */
  void set(JSReturnValue x)
  {
    switch (type) {
    case UNSAFE:
      *u.unsafe = x.v;
      return;
    case HEAP:
      u.heap->v = x.v;
      return;
    case STACK:
      u.stack->v = x.v;
      return;
    }
    asm volatile("ud2");
    emacs_abort();
  }
  JSReturnValue sref(ptrdiff_t off, JSReturnValue x)
  {
    switch (type) {
    case UNSAFE:
      return JSReturnValue(u.unsafe[off] = x.v);
    case HEAP:
      return JSReturnValue(JS::Value(u.heap[off].v = x.v));
    case STACK:
      return JSReturnValue(u.stack[off].v = x.v);
    }
    asm volatile("ud2");
    emacs_abort();
  }
  JSReturnValue ref(ptrdiff_t off)
  {
    switch (type) {
    case UNSAFE:
      return u.unsafe[off].v;
    case HEAP:
      return u.heap[off].v;
    case STACK:
      return u.stack[off].v;
    }
    asm volatile("ud2");
    emacs_abort();
  }
  void set(Lisp_Value_Stack &x)
  {
    switch (type) {
    case UNSAFE:
      *u.unsafe = x.v;
      return;
    case HEAP:
      u.heap->v = x.v;
      return;
    case STACK:
      u.stack->v = x.v;
      return;
    }
    asm volatile("ud2");
    emacs_abort();
  }
  // void set(Lisp_Value_Heap x)
  // {
  //   switch (type) {
  //   case UNSAFE:
  //     *u.unsafe = x.v;
  //     return;
  //   case HEAP:
  //     u.heap->v = x.v;
  //     return;
  //   case STACK:
  //     u.stack->v = x.v;
  //     return;
  //   }
  //   asm volatile("ud2");
  //   emacs_abort();
  // }
  // void set(Lisp_Value_Handle x)
  // {
  //   switch (type) {
  //   case UNSAFE:
  //     *u.unsafe = x.v;
  //     return;
  //   case HEAP:
  //     u.heap->v = x.v;
  //     return;
  //   case STACK:
  //     u.stack->v = x.v;
  //     return;
  //   }
  //   asm volatile("ud2");
  //   emacs_abort();
  // }
  bool operator==(int other)
  {
    switch (type) {
    case UNSAFE:
      return u.unsafe == 0;
    case HEAP:
      return u.heap == 0;
    case STACK:
      return u.stack == 0;
    }
    asm volatile("ud2");
    emacs_abort();
  }
  ELisp_Pointer(char *ptr);
  ELisp_Pointer(void *ptr);
  ELisp_Pointer(ptrdiff_t null)
  {
    type = STACK;
    u.stack = 0;
  }
  ELisp_Pointer(int null)
  {
    type = STACK;
    u.stack = 0;
  }
  operator void*()
  {
    switch (type) {
    case UNSAFE:
      return u.unsafe;
    case HEAP:
      return u.heap;
    case STACK:
      return u.stack;
    }
    asm volatile("ud2");
    emacs_abort();
  }
};

/* All the conversion functions. */

inline JSReturnValue::JSReturnValue(const JSHeapValue &v2) : v(v2)
{
}

inline JSReturnValue::JSReturnValue(const JSStackValue &v2) : v(v2)
{
}

inline JSReturnValue::JSReturnValue(const JSHandleValue &v2) : v(v2)
{
}

inline JSReturnValue::JSReturnValue(const Lisp_Value_Return &v2) : v(v2)
{
}

inline JSReturnValue::JSReturnValue(const Lisp_Value_Heap &v2) : v(v2)
{
}

inline JSReturnValue::JSReturnValue(const Lisp_Value_Stack &v2) : v(v2)
{
}

inline JSReturnValue::JSReturnValue(const Lisp_Value_Handle &v2) : v(v2)
{
}

inline Lisp_Value_Heap &Lisp_Value_Heap::operator=(Lisp_Value_Stack &v2)
{
  v = JSReturnValue(v2.v);
  return *this;
}
inline Lisp_Value_Heap &Lisp_Value_Heap::operator=(Lisp_Value_Handle v2)
{
  v = JSReturnValue(v2.v);
  return *this;
}
inline Lisp_Value_Stack &Lisp_Value_Stack::operator=(const Lisp_Value_Handle &v2) {
  return *this = JSReturnValue(v2.v);
}

inline Lisp_Value_Stack::Lisp_Value_Stack(const Lisp_Value_Handle v) : v(v)
{
}

inline Lisp_Value_Heap::Lisp_Value_Heap(const Lisp_Value_Stack &v2) {
  v.set(v2.v.v);
}

inline Lisp_Value_Heap::Lisp_Value_Heap(const Lisp_Value_Handle v2) {
  v.set(v2.v.v);
}

///#poison Lisp_Object
//typedef JSReturnValue Lisp_Object;
typedef Lisp_Value_Handle ELisp_Handle;
typedef Lisp_Value_Heap ELisp_Heap_Value;
typedef Lisp_Value_Heap ELisp_Struct_Value;
typedef Lisp_Value_Stack ELisp_Value;
struct ELisp_Vector { ptrdiff_t n; ELisp_Pointer vec;};

typedef struct ELisp_Vector ELisp_Vector_Handle;

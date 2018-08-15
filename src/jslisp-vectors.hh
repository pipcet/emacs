//extern void jsprint(Lisp_Object *x);

class JSVector {
public:
  JS::AutoValueVector vec;

  JSVector(JSContext *cx) : vec(cx) {}

  void resize(size_t n2) {
    if (!vec.resize(n2))
      emacs_abort();
  }
};

class ELisp_Dynvector {
public:
  size_t n;
  JSVector vec;

  ELisp_Dynvector() : vec(jsg.cx) {}

  ELisp_Dynvector(size_t n2) : vec(jsg.cx) {
    resize(n2);
  }

  operator ELisp_Pointer()
  {
    return (JSReturnValue *)vec.vec.begin();
  }

  JSReturnValue sref(size_t i, JSReturnValue v)
  {
    vec.vec[i].set(v.v);
    return JSReturnValue(v.v);
  }

  JSReturnValue ref(size_t i)
  {
    return JSReturnValue(vec.vec[i]);
  }

  void resize(size_t n2) {
    vec.resize(n2);
    n = n2;
  }
};

template<size_t n0>
class JSArray {
public:
  static const size_t n = n0;
  JS::AutoValueArray<n0> vec;

  JSArray() : vec(jsg.cx) {
  }
};

class JSMutabledVectorHandle {
public:
  JS::HandleValueArray vec;

  JSMutabledVectorHandle(JS::HandleValueArray vec) : vec(vec) {}

  JS::MutableHandleValue operator[](size_t i)
  {
    return JS::MutableHandleValue::fromMarkedLocation(const_cast<JS::Value*>(vec.begin() + i));
  }
};

#define ELisp_Array(symbol, n) ELisp_Value symbol ## _arr[(n)] = { }; struct ELisp_Vector symbol = { (n), symbol ## _arr }
#define ELisp_Array_Imm(symbol, ...) ELisp_Struct_Value symbol ## _arr[] = { __VA_ARGS__ }; struct ELisp_Vector symbol = { ARRAYELTS(symbol ## _arr), symbol ## _arr }



//extern void jsprint(Lisp_Object *x);

#if 1
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

  JSReturnValue set_element(size_t i, JSReturnValue v)
  {
    vec.vec[i].set(v.v);
    return JSReturnValue(v.v);
  }

  JSReturnValue get_element(size_t i)
  {
    return JSReturnValue(vec.vec[i]);
  }

  void resize(size_t n2) {
    vec.resize(n2);
    n = n2;
  }
};
#else
class ELisp_Dynvector {
public:
  ELisp_Value val;

  ELisp_Dynvector() : val(elisp_array(0)) {}

  ELisp_Dynvector(size_t n2) : val(elisp_array(n2)) {}

  operator ELisp_Pointer()
  {
    return ELisp_Pointer(val);
  }

  JSReturnValue set_element(size_t i, JSReturnValue v)
  {
    val.set_element(i, v.v);
    return JSReturnValue(v.v);
  }

  JSReturnValue get_element(size_t i)
  {
    return JSReturnValue(val.get_element(i));
  }

  JSReturnValue resize(size_t n2)
  {
    elisp_array_resize(val, n2);
  }
};
#endif

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



template<typename... As>
ELisp_Return_Value
CALLN(ELisp_Return_Value (*f) (ELisp_Vector_Handle), As... args)
{
  ELisp_Value arr[] = { ELisp_Return_Value (args)... };
  ELisp_Dynvector d;
  d.resize(ARRAYELTS (arr));
  for (size_t i = 0; i < ARRAYELTS(arr); i++)
    d.set_element(i, arr[i]);
  ELisp_Vector v = LV (ARRAYELTS (arr), d);
  auto ret = f (v);

  return ret;
}

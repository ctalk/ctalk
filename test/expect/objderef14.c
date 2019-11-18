
int main () {

  String new myObject;
  String new myObjectValue;
  
  /* (Object *)myObjectValue = *(myObject -> instancevars); *//***/
  (Object *)myObjectValue = myObject -> instancevars;

  /* The first two printf formats test terminal_printf_arg with
     ptr_fmt_is_alt_int_fmt in rexpr.c. */
  printf ("%#x\n", myObject);
  printf ("%p\n", myObjectValue);
  /* The second two test the alt format in fmt_rt_return. */
  printf ("%p\n", myObject -> __o_p_obj);
  printf ("%#x == %#x\n", myObjectValue -> __o_p_obj, myObject);

}

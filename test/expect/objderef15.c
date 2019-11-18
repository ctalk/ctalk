/* See the comments in objderef14.c. */

int main () {

  String new myObject;
  String new myObjectValue;
  
  (Object *)myObjectValue = myObject value;

  printf ("%#x\n", myObject);
  printf ("%p\n", myObjectValue);
  printf ("%p\n", myObject -> __o_p_obj);
  printf ("%#x == %#x\n", myObjectValue -> __o_p_obj, myObject);

}

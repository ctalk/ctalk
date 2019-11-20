/* This tests the typecast in the return statement.  The
   compiler should still use a single fn to return the int. */

Integer instanceMethod myMethod (void) {
  int i = 36ll;
  return (long int)i;
}


int main () {
  Integer new rcvrInt;
  printf ("%d\n", rcvrInt myMethod);
}

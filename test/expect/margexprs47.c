/* this checks the symbol_rval_to_void_ptr_lval special case... 
   a little contrived. */

Symbol new mySymbol;

char *htoa (char *, unsigned int); /* normally prototyped in ctalk.h */

Symbol instanceMethod myMethod (void) {
  void *myPtr;

  myPtr = (char *)mySymbol;

  printf ("%s\n", (char *)myPtr);
}

Symbol instanceMethod myMethod2 (void) {
  void *myPtr;

  /* Symbol : getValue returns the receiver if its value doesn't
     refer to a valid object. Then, __ctalk_to_c_ptr makes the
     translation in a C context anyway. */
  myPtr = (char *)*mySymbol;

  printf ("%s\n", (char *)myPtr);
}

int main () {
  OBJECT *myObj;
  char buf[16]; /* , addrbuf[16]; *//***/

  xstrcpy (buf, "Success");
  /* htoa (addrbuf, (unsigned int)buf); */ /***/

  myObj = mySymbol;

#if 1 /***/
  /* equivalent alternatives */
  SYMVAL(myObj -> instancevars -> __o_value) = (unsigned long int)
    buf;
  SYMVAL(myObj -> __o_value) = (unsigned long int) buf;
#else
  *(unsigned long int *)myObj -> instancevars -> __o_value
    = (unsigned long int)buf;
  *(unsigned long int *)myObj -> __o_value = (unsigned long int)buf;
#endif  

  mySymbol myMethod;
  mySymbol myMethod2;
}

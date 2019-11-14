/* $Id: cassign1.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  It's okay for the third printf statement's "%#x" format to
 *  generate a warning.  Type casts in printf () arguments get
 *  checked by other test programs (i.e., like clsvars3.c and
 *  clsvars4.c).
 */

int main () {
 
  Integer new myInt;
  OBJECT *myInt_alias;

  myInt_alias = myInt;

  printf ("The following three addresses should be the same.\n");
  printf ("%p\n", myInt_alias);
  printf ("%p\n", myInt);
#ifdef __x86_64
  printf ("%#lx\n", myInt);
#else  
  printf ("%#x\n", myInt);
#endif  

  return 0;
}

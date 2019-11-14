/* $Id: clsvars3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/* This should not cause a warning with the typecast in the printf
   argument. */

Event class MyClass;
MyClass classVariable classSym Symbol NULL;

int main () {

  String new s;

  *MyClass classSym = "Hello, world!";

  s = *MyClass classSym;

  printf ("%s\n", s);
  printf ("%s\n", (char*) *MyClass classSym);

}


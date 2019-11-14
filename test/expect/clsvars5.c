/* $Id: clsvars5.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

char *my_fn (void) {
  return "ok!";
}

Event class MyClass;
MyClass classVariable classSym String 0;

int main () {

  classSym = my_fn ();

  printf ("%s\n", classSym);

}


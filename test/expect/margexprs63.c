/* $Id: margexprs63.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/*
 *  As many uncontrived examples of prefix ++ operators as I
 *  can think of.
 */

/* From margexprs15.c, but with int *'s. */

int main (int argc, char **argv) {
  int idx_int, *idx_int_p;
  Integer new integerObject;

  idx_int = 0;
  idx_int_p = &idx_int;

  integerObject = ++(*idx_int_p);
  printf ("%d\n", integerObject);
  integerObject = ++(*idx_int_p);
  printf ("%d\n", integerObject);
  integerObject = ++(*idx_int_p);
  printf ("%d\n", integerObject);
  integerObject = ++(*idx_int_p);
  printf ("%d\n", integerObject);

  idx_int = 0;
  idx_int_p = &idx_int;
  integerObject = ++*idx_int_p;
  printf ("%d\n", integerObject);
  integerObject = ++*idx_int_p;
  printf ("%d\n", integerObject);
  integerObject = ++*idx_int_p;
  printf ("%d\n", integerObject);
  integerObject = ++*idx_int_p;
  printf ("%d\n", integerObject);
}

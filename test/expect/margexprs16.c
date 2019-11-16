/* $Id: margexprs16.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/*
 *  Like margexprs12.c, except for postfix ops.
 */

int main (int argc, char **argv) {
  int idx_int;
  Integer new integerObject;

  idx_int = 0;
  integerObject = idx_int++;
  printf ("%d\n", integerObject);
  integerObject = idx_int++;
  printf ("%d\n", integerObject);
  integerObject = idx_int++;
  printf ("%d\n", integerObject);
  integerObject = idx_int++;
  printf ("%d\n", integerObject);
}

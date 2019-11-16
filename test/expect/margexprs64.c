/* $Id: margexprs64.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/*
 *  Like margexprs16.c, except it uses int *'s.
 */

int main (int argc, char **argv) {
  int idx_int, *idx_int_p;
  Integer new integerObject;

  /* not sure if this is correct C, but it seems to work well
     here */
  idx_int = 0;
  idx_int_p = &idx_int;
  integerObject = *idx_int_p++;
  printf ("%d\n", integerObject);
  integerObject = *idx_int_p++;
  printf ("%d\n", integerObject);
  integerObject = *idx_int_p++;
  printf ("%d\n", integerObject);
  integerObject = *idx_int_p++;
  printf ("%d\n", integerObject);

  idx_int = 0;
  integerObject = (*idx_int_p)++;
  printf ("%d\n", integerObject);
  integerObject = (*idx_int_p)++;
  printf ("%d\n", integerObject);
  integerObject = (*idx_int_p)++;
  printf ("%d\n", integerObject);
  integerObject = (*idx_int_p)++;
  printf ("%d\n", integerObject);
}

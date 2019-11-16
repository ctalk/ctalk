/* $Id: margexprs65.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/* Similar to margexprs6.c, but with C variables. */
/*
 *  Like margexprs17.c, except with int *'s.
 */

int main (int argc, char **argv) {

  int idx_int, *idx_int_p;
  Array new rcvrArray;

  WriteFileStream classInit;

  idx_int = 0;
  idx_int_p = &idx_int;
  rcvrArray atPut ++*idx_int_p, "Element0";
  rcvrArray atPut (++*idx_int_p), "Element1";
  idx_int = 0;
  stdoutStream writeStream rcvrArray at ++*idx_int_p;
  stdoutStream writeStream "\n";
  stdoutStream writeStream rcvrArray at (++*idx_int_p);
  stdoutStream writeStream "\n";

  idx_int = 0;
  rcvrArray atPut ++*idx_int_p, "Element0";
  rcvrArray atPut (++*idx_int_p), "Element1";
  idx_int = 0;
  stdoutStream writeStream (rcvrArray at ++*idx_int_p);
  stdoutStream writeStream "\n";
  stdoutStream writeStream (rcvrArray at (++*idx_int_p));
  stdoutStream writeStream "\n";
}

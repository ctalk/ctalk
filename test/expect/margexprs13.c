/* $Id: margexprs13.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/* Similar to margexprs6.c, but with C variables. */
/*
 *  Check that an argument receiver-message expression binds
 *  correctly for a Collection (at least Array) receiver.
 */

int main (int argc, char **argv) {

  int idx_int;
  Array new rcvrArray;

  WriteFileStream classInit;

  idx_int = 0;
  rcvrArray atPut idx_int, "Element0";
  rcvrArray atPut (++idx_int), "Element1";
  rcvrArray atPut (++idx_int), "Element2";
  stdoutStream writeStream "The next two lines should be identical.\n";
  stdoutStream writeStream rcvrArray at idx_int;
  stdoutStream writeStream "\n";
  stdoutStream writeStream (rcvrArray at idx_int);
  stdoutStream writeStream "\n";

  idx_int = 0;
  rcvrArray atPut idx_int, "Element0";
  rcvrArray atPut ++idx_int, "Element1";
  rcvrArray atPut ++idx_int, "Element2";
  stdoutStream writeStream "The next two lines should be identical.\n";
  stdoutStream writeStream rcvrArray at idx_int;
  stdoutStream writeStream "\n";
  stdoutStream writeStream (rcvrArray at idx_int);
  stdoutStream writeStream "\n";
}

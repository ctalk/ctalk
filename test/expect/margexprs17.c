/* $Id: margexprs17.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/* Similar to margexprs6.c, but with C variables. */
/*
 *  Like margexprs13.c, except with postfix ops.
 */

int main (int argc, char **argv) {

  int idx_int;
  Array new rcvrArray;

  WriteFileStream classInit;

  idx_int = 0;
  rcvrArray atPut ++idx_int, "Element0";
  rcvrArray atPut (++idx_int), "Element1";
  /*
   *  Expressions like ++(idx_int) as a method argument require a lot 
   *  of effort in order to work out a receiver.  At least the libraries
   *  provide intelligent warning messages, but the ugly details should 
   *  be worked out in a future release.
   */
/*   rcvrArray atPut ++(idx_int), "Element2"; */
  /*   stdoutStream writeStream "The next two lines should be identical.\n"; */
  idx_int = 0;
  stdoutStream writeStream rcvrArray at ++idx_int;
  stdoutStream writeStream "\n";
  stdoutStream writeStream rcvrArray at (++idx_int);
  stdoutStream writeStream "\n";
/*   stdoutStream writeStream rcvrArray at ++(idx_int); */
/*   stdoutStream writeStream "\n"; */

  idx_int = 0;
  rcvrArray atPut ++idx_int, "Element0";
  rcvrArray atPut (++idx_int), "Element1";
  idx_int = 0;
  stdoutStream writeStream (rcvrArray at ++idx_int);
  stdoutStream writeStream "\n";
  stdoutStream writeStream (rcvrArray at (++idx_int));
  stdoutStream writeStream "\n";
}

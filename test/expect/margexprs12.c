/* $Id: margexprs12.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/* Similar to margexprs6.c, but with C variables. */
/*
 *  Check that an argument receiver-message expression binds
 *  correctly for a Collection (at least Array) receiver.
 */

int main (int argc, char **argv) {

  int idx;
  Array new rcvrArray;
  Character new idxChar;
  String new elementString;

  WriteFileStream classInit;

  idx = 0;
  rcvrArray atPut idx, "Element0";
  idx = 1;
  rcvrArray atPut idx, "Element1";
  idx = 2;
  rcvrArray atPut idx, "Element2";

  idx = 1;
  stdoutStream writeStream "The next two lines should be identical.\n";
  stdoutStream writeStream rcvrArray at idx;
  stdoutStream writeStream "\n";
  stdoutStream writeStream (rcvrArray at idx);
  stdoutStream writeStream "\n";

  idx = 1;
  stdoutStream writeStream "The next two lines should be identical.\n";
  stdoutStream writeStream rcvrArray at idx;
  stdoutStream writeStream "\n";
  stdoutStream writeStream (rcvrArray at idx);
  stdoutStream writeStream "\n";
}

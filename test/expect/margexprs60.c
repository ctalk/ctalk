/* $Id: margexprs60.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/* Similar to margexprs6.c, but with C variables. */
/*
 *  Check that an argument receiver-message expression binds
 *  correctly for a Collection (at least Array) receiver.
 */

/* 
 *  From margexprs12.c, but with int *'s.
 */

int main (int argc, char **argv) {

  int idx, *idx_p;
  Array new rcvrArray;
  Character new idxChar;
  String new elementString;

  WriteFileStream classInit;

  idx = 0;
  idx_p = &idx;
  rcvrArray atPut *idx_p, "Element0";
  idx = 1;
  rcvrArray atPut *idx_p, "Element1";
  idx = 2;
  rcvrArray atPut *idx_p, "Element2";

  idx = 1;
  stdoutStream writeStream "The next two lines should be identical.\n";
  stdoutStream writeStream rcvrArray at *idx_p;
  stdoutStream writeStream "\n";
  stdoutStream writeStream (rcvrArray at *idx_p);
  stdoutStream writeStream "\n";

  idx = 1;
  stdoutStream writeStream "The next two lines should be identical.\n";
  stdoutStream writeStream rcvrArray at *idx_p;
  stdoutStream writeStream "\n";
  stdoutStream writeStream (rcvrArray at *idx_p);
  stdoutStream writeStream "\n";
}

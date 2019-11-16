/* $Id: margexprs6.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/*
 *  Check that an argument receiver-message expression binds
 *  correctly for a Collection (at least Array) receiver.
 */

int main (int argc, char **argv) {

  Array new rcvrArray;
  Character new idxChar;
  String new elementString;

  WriteFileStream classInit;

  rcvrArray atPut 0, "Element0";
  rcvrArray atPut 1, "Element1";
  rcvrArray atPut 2, "Element2";

  stdoutStream writeStream "The next two lines should be identical.\n";
  stdoutStream writeStream rcvrArray at 1;
  stdoutStream writeStream "\n";
  stdoutStream writeStream (rcvrArray at 1);
  stdoutStream writeStream "\n";
}

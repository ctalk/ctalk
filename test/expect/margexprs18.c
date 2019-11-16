/* $Id: margexprs18.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  See the comments in margexprs17.c.
 */

int main (int argc, char **argv) {

  int idx_int;
  char str[32], *s;
  Array new rcvrArray;

  WriteFileStream classInit;

  xstrcpy (str, "Element");
  s = str;
  idx_int = 0;
  rcvrArray atPut idx_int++, s++;
  rcvrArray atPut (idx_int++), (s++);
  rcvrArray atPut (idx_int), (s);
  idx_int = 0;
  stdoutStream writeStream rcvrArray at idx_int++;
  stdoutStream writeStream "\n";
  stdoutStream writeStream rcvrArray at (idx_int++);
  stdoutStream writeStream "\n";
  stdoutStream writeStream rcvrArray at (idx_int);
  stdoutStream writeStream "\n";

  idx_int = 0;
  s = str;
  rcvrArray atPut idx_int++, s++;
  rcvrArray atPut idx_int++, s++;
  rcvrArray atPut idx_int, s;
  idx_int = 0;
  stdoutStream writeStream (rcvrArray at idx_int++);
  stdoutStream writeStream "\n";
  stdoutStream writeStream (rcvrArray at idx_int++);
  stdoutStream writeStream "\n";
  stdoutStream writeStream (rcvrArray at idx_int);
  stdoutStream writeStream "\n";
}

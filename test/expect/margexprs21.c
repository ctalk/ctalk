/* $Id: margexprs21.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

int main (int argc, char **argv) {

  int idx_int[2], n;
  char str[32], *s;
  Array new rcvrArray;

  WriteFileStream classInit;

  xstrcpy (str, "Element");
  s = str;
  n = 1;
  idx_int[n] = 0;
  rcvrArray atPut ++idx_int[n], ++s;
  rcvrArray atPut (++idx_int[n]), (++s);
  rcvrArray atPut (idx_int[n]), (s);
  stdoutStream writeStream "The next two lines should be identical.\n";
  stdoutStream writeStream rcvrArray at idx_int[n];
  stdoutStream writeStream "\n";
  stdoutStream writeStream (rcvrArray at idx_int[n]);
  stdoutStream writeStream "\n";

  idx_int[n] = 0;
  s = str;
  rcvrArray atPut ++idx_int[n], ++s;
  rcvrArray atPut ++idx_int[n], ++s;
  rcvrArray atPut idx_int, s;
  stdoutStream writeStream "The next two lines should be identical.\n";
  stdoutStream writeStream rcvrArray at idx_int[n];
  stdoutStream writeStream "\n";
  stdoutStream writeStream (rcvrArray at idx_int[n]);
  stdoutStream writeStream "\n";
}

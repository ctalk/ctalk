/* $Id: margexprs23.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

int main (int argc, char **argv) {

  struct {
    int i;
  } s_struct;
  char str[32], *s;
  Array new rcvrArray;

  WriteFileStream classInit;

  xstrcpy (str, "Element");
  s = str;
  s_struct.i = 0;
  rcvrArray atPut s_struct.i, ++s;
  ++s_struct.i;
  rcvrArray atPut (s_struct.i), (++s);
  ++s_struct.i;
  rcvrArray atPut (s_struct.i), (s);
  s_struct.i = 0;
  stdoutStream writeStream rcvrArray at s_struct.i;
  stdoutStream writeStream "\n";
  ++s_struct.i;
  stdoutStream writeStream rcvrArray at s_struct.i;
  stdoutStream writeStream "\n";
  ++s_struct.i;
  stdoutStream writeStream rcvrArray at s_struct.i;
  stdoutStream writeStream "\n";

  s_struct.i = 0;
  s = str;
  rcvrArray atPut s_struct.i, ++s;
  ++s_struct.i;
  rcvrArray atPut s_struct.i, ++s;
  ++s_struct.i;
  rcvrArray atPut s_struct.i, s;
  s_struct.i = 0;
  stdoutStream writeStream (rcvrArray at s_struct.i);
  stdoutStream writeStream "\n";
  ++s_struct.i;
  stdoutStream writeStream (rcvrArray at s_struct.i);
  stdoutStream writeStream "\n";
  ++s_struct.i;
  stdoutStream writeStream (rcvrArray at s_struct.i);
  stdoutStream writeStream "\n";
}

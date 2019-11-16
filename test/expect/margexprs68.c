/* $Id: margexprs68.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/* like margexprs23.c, but with int *'s */

int main (int argc, char **argv) {

  struct {
    int i;
    int *i_p;
  } s_struct;
  char str[32], *s;
  Array new rcvrArray;

  WriteFileStream classInit;

  xstrcpy (str, "Element");
  s = str;
  s_struct.i = 0;
  s_struct.i_p = &s_struct.i;
  rcvrArray atPut *(s_struct.i_p), ++s;
  ++*(s_struct.i_p);
  rcvrArray atPut (*(s_struct.i_p)), (++s);
  ++*(s_struct.i_p);
  rcvrArray atPut (*(s_struct.i_p)), (s);
  *(s_struct.i_p) = 0;
  stdoutStream writeStream rcvrArray at *(s_struct.i_p);
  stdoutStream writeStream "\n";
  ++*(s_struct.i_p);
  stdoutStream writeStream rcvrArray at *(s_struct.i_p);
  stdoutStream writeStream "\n";
  ++*(s_struct.i_p);
  stdoutStream writeStream rcvrArray at *(s_struct.i_p);
  stdoutStream writeStream "\n";

  s_struct.i = 0;
  s = str;
  rcvrArray atPut *(s_struct.i_p), ++s;
  ++*(s_struct.i_p);
  rcvrArray atPut *(s_struct.i_p), ++s;
  ++*(s_struct.i_p);
  rcvrArray atPut *(s_struct.i_p), s;
  *(s_struct.i_p) = 0;
  stdoutStream writeStream (rcvrArray at *(s_struct.i_p));
  stdoutStream writeStream "\n";
  ++*(s_struct.i_p);
  stdoutStream writeStream (rcvrArray at *(s_struct.i_p));
  stdoutStream writeStream "\n";
  ++*(s_struct.i_p);
  stdoutStream writeStream (rcvrArray at *(s_struct.i_p));
  stdoutStream writeStream "\n";
}

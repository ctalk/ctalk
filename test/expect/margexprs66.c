/* $Id: margexprs66.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  See the comments in margexprs18.c, which this is similar to,
 *  except for the int *.  The convoluted parenthesization should
 *  be correct for the operator precedence.
 */

int main (int argc, char **argv) {

  int idx_int, *idx_int_p;
  char str[32], *s;
  Array new rcvrArray;

  WriteFileStream classInit;

  xstrcpy (str, "Element");
  s = str;
  idx_int = 0;
  idx_int_p = &idx_int;
  rcvrArray atPut (*idx_int_p)++, s++;
  rcvrArray atPut ((*idx_int_p)++), (s++);
  rcvrArray atPut (*idx_int_p), (s);
  *idx_int_p = 0;
  stdoutStream writeStream rcvrArray at (*idx_int_p)++;
  stdoutStream writeStream "\n";
  stdoutStream writeStream rcvrArray at ((*idx_int_p)++);
  stdoutStream writeStream "\n";
  stdoutStream writeStream rcvrArray at (*idx_int_p);
  stdoutStream writeStream "\n";

  *idx_int_p = 0;
  s = str;
  rcvrArray atPut (*idx_int_p)++, s++;
  rcvrArray atPut (*idx_int_p)++, s++;
  rcvrArray atPut *idx_int_p, s;
  *idx_int_p = 0;
  stdoutStream writeStream (rcvrArray at (*idx_int_p)++);
  stdoutStream writeStream "\n";
  stdoutStream writeStream (rcvrArray at (*idx_int_p)++);
  stdoutStream writeStream "\n";
  stdoutStream writeStream (rcvrArray at *idx_int_p);
  stdoutStream writeStream "\n";
}

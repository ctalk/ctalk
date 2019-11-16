/* $Id: margexprs8.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/* This is the same as margexprs2.c, except with C variables as terms. */

int main () {

  int i_term, i_term_2;

  String new myString;
  String new mySubString;
  Integer new stringLength;

  myString = "This is a test string.";

  stringLength = myString length;

  i_term = 1;
  i_term_2 = 1;

  mySubString = myString subString i_term, stringLength - i_term;
  printf ("%s\n", mySubString);

  mySubString = myString subString (i_term, stringLength - i_term);
  printf ("%s\n", mySubString);

  mySubString = myString subString (i_term, (stringLength - i_term));
  printf ("%s\n", mySubString);

  mySubString = myString subString i_term + 1, stringLength - i_term + 1;
  printf ("%s\n", mySubString);

  mySubString = myString subString 1 + i_term_2, stringLength - 1 + i_term_2;
  printf ("%s\n", mySubString);

  mySubString = myString subString i_term + i_term_2, stringLength - i_term + i_term_2;
  printf ("%s\n", mySubString);

  mySubString = myString subString i_term + 1, stringLength - (i_term + 1);
  printf ("%s\n", mySubString);

  mySubString = myString subString 1 + i_term_2, stringLength - (1 + i_term_2);
  printf ("%s\n", mySubString);

  mySubString = myString subString i_term + i_term_2, stringLength - (i_term + i_term_2);
  printf ("%s\n", mySubString);

  mySubString = myString subString i_term + 1, (stringLength - (i_term + 1));
  printf ("%s\n", mySubString);

  mySubString = myString subString 1 + i_term_2, (stringLength - (1 + i_term_2));
  printf ("%s\n", mySubString);

  mySubString = myString subString i_term + i_term_2, (stringLength - (i_term + i_term_2));
  printf ("%s\n", mySubString);

  mySubString = myString subString (i_term + 1, (stringLength - (i_term + 1)));
  printf ("%s\n", mySubString);

  mySubString = myString subString (1 + i_term_2, (stringLength - (1 + i_term_2)));
  printf ("%s\n", mySubString);

  mySubString = myString subString (i_term + i_term_2, (stringLength - (i_term + i_term_2)));
  printf ("%s\n", mySubString);

  exit(0);
}

/* $Id: margexprs2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

int main () {

  String new myString;
  String new mySubString;
  Integer new stringLength;

  myString = "This is a test string.";

  stringLength = myString length;

  mySubString = myString subString 1, stringLength - 1;
  printf ("%s\n", mySubString);

  mySubString = myString subString (1, stringLength - 1);
  printf ("%s\n", mySubString);

  mySubString = myString subString (1, (stringLength - 1));
  printf ("%s\n", mySubString);

  mySubString = myString subString 1 + 1, stringLength - 1 + 1;
  printf ("%s\n", mySubString);

  mySubString = myString subString 1 + 1, stringLength - (1 + 1);
  printf ("%s\n", mySubString);

  mySubString = myString subString 1 + 1, (stringLength - (1 + 1));
  printf ("%s\n", mySubString);

  mySubString = myString subString (1 + 1, (stringLength - (1 + 1)));
  printf ("%s\n", mySubString);



  exit(0);
}

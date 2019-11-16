/* $Id: margexprs.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

int main () {

  Array new myArray;

  myArray atPut 0, "Element at 0";

  myArray atPut (1, "Element at 1");

  myArray atPut ((0 + 2), ("Element at " + (0 + 2) asString));

  myArray atPut (0 + 3), "Element at " + (0 + 3) asString;

  myArray atPut (0 + 4), "Element at " + ((0 + 4) asString);

  myArray atPut (0 + 5), ("Element at " + (0 + 5) asString);

  myArray atPut (0 + 6), ("Element at " + ((0 + 6) asString));

  myArray atPut ((0 + 7), "Element at " + (0 + 7) asString);

  myArray atPut ((0 + 8), "Element at " + ((0 + 8) asString));

  myArray atPut ((0 + 9), ("Element at " + ((0 + 9) asString)));

  myArray atPut 0 + 10, "Element at " + (0 + 10) asString;

  myArray atPut 0 + 11, "Element at " + ((0 + 11) asString);

  myArray atPut 0 + 12, ("Element at " + (0 + 12) asString);

  myArray atPut 0 + 13, ("Element at " + ((0 + 13) asString));



  printf ("%s\n", myArray at 0);
  printf ("%s\n", myArray at 1);
  printf ("%s\n", myArray at 2);
  printf ("%s\n", myArray at 3);
  printf ("%s\n", myArray at 4);
  printf ("%s\n", myArray at 5);
  printf ("%s\n", myArray at 6);
  printf ("%s\n", myArray at 7);
  printf ("%s\n", myArray at 8);
  printf ("%s\n", myArray at 9);
  printf ("%s\n", myArray at 10);
  printf ("%s\n", myArray at 11);
  printf ("%s\n", myArray at 12);
  printf ("%s\n", myArray at 13);

  exit(0);
}

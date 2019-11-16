/* $Id: margexprs7.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/* This is the same as margexprs.c, except with C variables as terms. */


int main () {

  int i_term, i_term_2;

  Array new myArray;

  i_term = 0;
  myArray atPut i_term, "Element at 0";

  i_term = 1;
  myArray atPut (i_term, "Element at 1");

  i_term = 0;
  i_term_2 = 2;
  myArray atPut ((i_term + 2), ("Element at " + (i_term + 2) asString));

  i_term_2 = 3;
  myArray atPut (i_term + 3), "Element at " + (i_term + 3) asString;

  i_term_2 = 4;
  myArray atPut (i_term + 4), "Element at " + ((i_term + 4) asString);

  i_term_2 = 5;
  myArray atPut (i_term + 5), ("Element at " + (i_term + 5) asString);

  i_term_2 = 6;
  myArray atPut (i_term + 6), ("Element at " + ((i_term + 6) asString));

  i_term_2 = 7;
  myArray atPut ((i_term + 7), "Element at " + (i_term + 7) asString);

  i_term_2 = 8;
  myArray atPut ((i_term + 8), "Element at " + ((i_term + 8) asString));

  i_term_2 = 9;
  myArray atPut ((i_term + 9), ("Element at " + ((i_term + 9) asString)));

  i_term_2 = 10;
  myArray atPut i_term + 10, "Element at " + (i_term + 10) asString;

  i_term_2 = 11;
  myArray atPut i_term + 11, "Element at " + ((i_term + 11) asString);

  i_term_2 = 12;
  myArray atPut i_term + 12, ("Element at " + (i_term + 12) asString);

  i_term_2 = 13;
  myArray atPut i_term + 13, ("Element at " + ((i_term + 13) asString));

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

  i_term = 0;
  myArray atPut i_term, "Element at 0";

  i_term = 1;
  myArray atPut (i_term, "Element at 1");

  i_term = 0;
  i_term_2 = 2;
  myArray atPut ((0 + i_term_2), ("Element at " + (0 + i_term_2) asString));

  i_term_2 = 3;
  myArray atPut (0 + i_term_2), "Element at " + (0 + i_term_2) asString;

  i_term_2 = 4;
  myArray atPut (0 + i_term_2), "Element at " + ((0 + i_term_2) asString);

  i_term_2 = 5;
  myArray atPut (0 + i_term_2), ("Element at " + (0 + i_term_2) asString);

  i_term_2 = 6;
  myArray atPut (0 + i_term_2), ("Element at " + ((0 + i_term_2) asString));

  i_term_2 = 7;
  myArray atPut ((0 + i_term_2), "Element at " + (0 + i_term_2) asString);

  i_term_2 = 8;
  myArray atPut ((0 + i_term_2), "Element at " + ((0 + i_term_2) asString));

  i_term_2 = 9;
  myArray atPut ((0 + i_term_2), ("Element at " + ((0 + i_term_2) asString)));

  i_term_2 = 10;
  myArray atPut 0 + i_term_2, "Element at " + (0 + i_term_2) asString;

  i_term_2 = 11;
  myArray atPut 0 + i_term_2, "Element at " + ((0 + i_term_2) asString);

  i_term_2 = 12;
  myArray atPut 0 + i_term_2, ("Element at " + (0 + i_term_2) asString);

  i_term_2 = 13;
  myArray atPut 0 + i_term_2, ("Element at " + ((0 + i_term_2) asString));

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

  i_term = 0;
  myArray atPut i_term, "Element at 0";

  i_term = 1;
  myArray atPut (i_term, "Element at 1");

  i_term = 0;
  i_term_2 = 2;
  myArray atPut ((i_term + i_term_2), ("Element at " + (i_term + i_term_2) asString));

  i_term_2 = 3;
  myArray atPut (i_term + i_term_2), "Element at " + (i_term + i_term_2) asString;

  i_term_2 = 4;
  myArray atPut (i_term + i_term_2), "Element at " + ((i_term + i_term_2) asString);

  i_term_2 = 5;
  myArray atPut (i_term + i_term_2), ("Element at " + (i_term + i_term_2) asString);

  i_term_2 = 6;
  myArray atPut (i_term + i_term_2), ("Element at " + ((i_term + i_term_2) asString));

  i_term_2 = 7;
  myArray atPut ((i_term + i_term_2), "Element at " + (i_term + i_term_2) asString);

  i_term_2 = 8;
  myArray atPut ((i_term + i_term_2), "Element at " + ((i_term + i_term_2) asString));

  i_term_2 = 9;
  myArray atPut ((i_term + i_term_2), ("Element at " + ((i_term + i_term_2) asString)));

  i_term_2 = 10;
  myArray atPut i_term + i_term_2, "Element at " + (i_term + i_term_2) asString;

  i_term_2 = 11;
  myArray atPut i_term + i_term_2, "Element at " + ((i_term + i_term_2) asString);

  i_term_2 = 12;
  myArray atPut i_term + i_term_2, ("Element at " + (i_term + i_term_2) asString);

  i_term_2 = 13;
  myArray atPut i_term + i_term_2, ("Element at " + ((i_term + i_term_2) asString));

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

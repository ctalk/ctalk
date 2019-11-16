/* $Id: margexprs38.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/* This is the same as margexprs.c, except with C variables as terms. */


int main () {

  Integer new i_term;
  Integer new i_term_2;

  Array new myArray;

  i_term = 0;
  myArray atPut i_term, "Element at 0";

  i_term = 1;
  myArray atPut (i_term, "Element at 1");

  i_term = 0;
  i_term_2 = 2;

  myArray atPut ((i_term_2)++, ("Element at " + (i_term + 2) asString));

  myArray atPut (i_term_2)++, "Element at " + (i_term + 3) asString;

  myArray atPut (i_term_2)++, "Element at " + ((i_term + 4) asString);

  myArray atPut i_term_2++, ("Element at " + (i_term + 5) asString);

  myArray atPut (i_term_2++), ("Element at " + ((i_term + 6) asString));

  myArray atPut i_term_2++, "Element at " + (i_term + 7) asString;

  myArray atPut i_term_2++, "Element at " + ((i_term + 8) asString);

  myArray atPut (i_term_2++, ("Element at " + ((i_term + 9) asString)));

  myArray atPut i_term_2++, "Element at " + (i_term + 10) asString;

  myArray atPut i_term_2++, "Element at " + ((i_term + 11) asString);

  myArray atPut i_term_2++, ("Element at " + (i_term + 12) asString);

  myArray atPut i_term_2++, ("Element at " + ((i_term + 13) asString));

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


  printf ("----------------------\n");

  i_term = 0;
  i_term_2 = 2;

  myArray atPut (++(i_term_2), ("Element at " + (i_term + 2) asString));

  myArray atPut ++(i_term_2), "Element at " + (i_term + 3) asString;

  myArray atPut ++(i_term_2), "Element at " + ((i_term + 4) asString);

  myArray atPut ++i_term_2, ("Element at " + (i_term + 5) asString);

  myArray atPut (++i_term_2), ("Element at " + ((i_term + 6) asString));

  myArray atPut ++i_term_2, "Element at " + (i_term + 7) asString;

  myArray atPut ++i_term_2, "Element at " + ((i_term + 8) asString);

  myArray atPut (++i_term_2, ("Element at " + ((i_term + 9) asString)));

  myArray atPut ++i_term_2, "Element at " + (i_term + 10) asString;

  myArray atPut ++i_term_2, "Element at " + ((i_term + 11) asString);

  myArray atPut ++i_term_2, ("Element at " + (i_term + 12) asString);

  myArray atPut ++i_term_2, ("Element at " + ((i_term + 13) asString));

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

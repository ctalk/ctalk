/* $Id: margexprs24.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/* A sanity check for the appearance of, "eval," anywhere in an
   expression that we've already decided needs to be evaluated.
   Also a check for whether the vm can handle whitespace at the 
   beginning of an expression.

   These results should be identical to margexprs.c.
*/

int main () {

  Array new myArray;

  myArray atPut eval 0, eval "Element at 0";

  myArray atPut (eval 1, eval "Element at 1");

  myArray atPut (eval (0 + 2), eval ("Element at " + (0 + 2) asString));

  myArray atPut eval (0 + 3), eval "Element at " + (0 + 3) asString;

  myArray atPut eval (0 + 4), eval "Element at " + ((0 + 4) asString);

  myArray atPut eval (0 + 5), eval ("Element at " + (0 + 5) asString);

  myArray atPut eval (0 + 6), eval ("Element at " + ((0 + 6) asString));

  myArray atPut (eval (0 + 7), eval "Element at " + (0 + 7) asString);

  myArray atPut (eval (0 + 8), eval "Element at " + ((0 + 8) asString));

  myArray atPut (eval (0 + 9), eval ("Element at " + ((0 + 9) asString)));

  myArray atPut eval 0 + 10, eval "Element at " + (0 + 10) asString;

  myArray atPut eval 0 + 11, eval "Element at " + ((0 + 11) asString);

  myArray atPut eval 0 + 12, eval ("Element at " + (0 + 12) asString);

  myArray atPut eval 0 + 13, eval ("Element at " + ((0 + 13) asString));



  printf ("%s\n",  eval myArray at 0);
  printf ("%s\n",  myArray at 1);
  printf ("%s\n",  myArray at 2);
  printf ("%s\n",  myArray at 3);
  printf ("%s\n",  myArray at 4);
  printf ("%s\n",  myArray at 5);
  printf ("%s\n",  myArray at 6);
  printf ("%s\n",  myArray at 7);
  printf ("%s\n",  myArray at 8);
  printf ("%s\n",  myArray at 9);
  printf ("%s\n",  myArray at 10);
  printf ("%s\n",  myArray at 11);
  printf ("%s\n",  myArray at 12);
  printf ("%s\n",  myArray at 13);

  exit(0);
}

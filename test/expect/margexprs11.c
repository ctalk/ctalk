/* $Id: margexprs11.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/* This is similar to margexprs5.c, but with C variables. */

int main (int argc, char **argv) {

     int i_term;
  Array new rcvrArray;
/*   Character new idxChar; */
  char idxChar;
  String new elementString;

  rcvrArray atPut 0, "Element0";
  rcvrArray atPut 1, "Element1";
  rcvrArray atPut 2, "Element2";

  i_term = 0;
  elementString = rcvrArray at i_term;

  printf ("%s\n", elementString);

  printf ("%s\n", rcvrArray at i_term);


  idxChar = '\001';

  elementString = rcvrArray at idxChar asInteger;

  printf ("%s\n", elementString);

  printf ("%s\n", rcvrArray at idxChar asInteger);

  idxChar = '\002';

  elementString = rcvrArray at idxChar asInteger;

  printf ("%s\n", elementString);

  printf ("%s\n", rcvrArray at idxChar asInteger);

}

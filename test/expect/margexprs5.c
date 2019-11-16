/* $Id: margexprs5.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

int main (int argc, char **argv) {

  Array new rcvrArray;
  Character new idxChar;
  String new elementString;

  rcvrArray atPut 0, "Element0";
  rcvrArray atPut 1, "Element1";
  rcvrArray atPut 2, "Element2";

  elementString = rcvrArray at 0;

  printf ("%s\n", elementString);

  printf ("%s\n", rcvrArray at 0);


  idxChar = '\001';

  elementString = rcvrArray at idxChar asInteger;

  printf ("%s\n", elementString);

  printf ("%s\n", rcvrArray at idxChar asInteger);

  idxChar = '\002';

  elementString = rcvrArray at idxChar asInteger;

  printf ("%s\n", elementString);

  printf ("%s\n", rcvrArray at idxChar asInteger);

}

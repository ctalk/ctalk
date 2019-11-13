/* $Id: argblk6.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Argument block slightly complex expression.
 */

String class MyListOfStrings;
MyListOfStrings instanceVariable strings List NULL;

int main () {

  MyListOfStrings new stringList;
  String new sPrefix;
  String new s;

  sPrefix = "This element is ";

  stringList strings push "l1";
  stringList strings push "l2";

  stringList strings map {
    s = sPrefix + self;
    printf ("%s\n", s);
  }
  exit(0);
}

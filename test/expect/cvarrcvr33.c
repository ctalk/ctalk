/* $Id: cvarrcvr33.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

int main () {

  String new myString;
  OBJECT *string_alias;

  string_alias =  &myString;

  (string_alias -> instancevars) delete;

  printf ("Pass\n");
}

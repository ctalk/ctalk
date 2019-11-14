/* $Id: cvarrcvr30.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/* 
 * this should generate a warning at the second, "length," message 
 * (the expression will evaluate to a Character receiver).  The
 * result should be undefined, so the program can print any number
 * there.
 */

int main () {

  String new myString;
  Integer new i;
  Symbol new alias;
  OBJECT *symbol_alias;


  myString = "Hello, world!";

  alias = &myString;
  symbol_alias =  alias;

  i = ((*symbol_alias) -> instancevars -> __o_value) length;

  printf ("%d\n", i);

  i = (*(symbol_alias -> instancevars -> __o_value)) length;

  printf ("%d\n", i);
}

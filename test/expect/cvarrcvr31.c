/* $Id: cvarrcvr31.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  TO DO - 
 * 
 *  Be able to handle an expression like this:
 *
 *  i = ((string_alias -> (instancevars -> __o_value))) length;
 *
 *  where we try to evaluate the subexpression (instancevars -> __o_value)
 *  independently of string_alias -- maybe check for a right-grouped set 
 *  of parens, and then scan left-to-right.
 *
 *  The result of the first two expressions should be undefined, so
 *  the program can print any number there.
 *
 */



/*
 *  This should generate warnings for the first two, "length,"
 *  messages. (The expressions will evaluate to Character
 *  receivers.)
 */


int main () {

  String new myString;
  Integer new i;
  OBJECT *string_alias;


  myString = "Hello, world!";

  string_alias =  &myString;

  i = ((*string_alias -> instancevars -> __o_value)) length;

  printf ("%d\n", i);

  i = (((*string_alias -> instancevars -> __o_value))) length;

  printf ("%d\n", i);

  i = (((*string_alias) -> instancevars -> __o_value)) length;

  printf ("%d\n", i);

}

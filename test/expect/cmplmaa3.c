/* $Id: cmplmaa3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Expressions with class variables that use subclassed constructors.
 */

Event class MyClass;
MyClass classVariable classArray AssociativeArray 0;

int main () {

  MyClass classArray atPut "key1", "value_without_parens_around_args";
  printf ("%s\n", MyClass classArray at "key1");

  MyClass classArray atPut ("key2", "value_with_parens_around_arglists");
  printf ("%s\n", MyClass classArray at ("key2"));

  MyClass classArray atPut ("key3"), ("value_with_parens_around_args");
  printf ("%s\n", MyClass classArray at ("key3"));
}


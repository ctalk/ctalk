/* $Id: clsvars2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Class variables that use their own constructors.
 *  Test expressions in both receiver and C contexts.
 */

Event class MyClass;
MyClass classVariable classKeys AssociativeArray NULL;
MyClass classVariable classList List NULL;

int main () {

  String new listNode1;
  String new listNode2;
  String new listNode3;

  MyClass classKeys atPut "key1", "value1";
  printf ("%s\n", MyClass classKeys at "key1");

  MyClass classKeys atPut "key2", "value2";
  printf ("%s\n", MyClass classKeys at "key2");

  MyClass classKeys atPut "key3", "value3";
  printf ("%s\n", MyClass classKeys at "key3");

  classKeys atPut "key4", "value4";
  printf ("%s\n", MyClass classKeys at "key4");

  classKeys atPut "key5", "value5";
  printf ("%s\n", MyClass classKeys at "key5");

  classKeys atPut "key6", "value6";
  printf ("%s\n", MyClass classKeys at "key6");

  printf ("%s\n", classKeys at "key1");
  printf ("%s\n", classKeys at "key2");
  printf ("%s\n", classKeys at "key3");
  printf ("%s\n", classKeys at "key4");
  printf ("%s\n", classKeys at "key5");
  printf ("%s\n", classKeys at "key6");

  listNode1 = "node 1";
  listNode2 = "node 2";
  listNode3 = "node 3";

  MyClass classList push listNode1;
  MyClass classList push listNode2;
  MyClass classList push listNode3;

  printf ("%s\n", MyClass classList pop);
  printf ("%s\n", MyClass classList pop);
  printf ("%s\n", MyClass classList pop);

  classList push listNode1;
  classList push listNode2;
  classList push listNode3;

  printf ("%s\n", classList unshift);
  printf ("%s\n", classList unshift);
  printf ("%s\n", classList unshift);

}


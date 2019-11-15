/* $Id: instvars2.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

Object class MyClass;
MyClass instanceVariable myInstanceVariable0 Integer 0;
MyClass instanceVariable myInstanceVariable1 Integer 0;
MyClass instanceVariable myInstanceVariable2 Integer 0;
MyClass instanceVariable myInstanceVariable3 Integer 0;
MyClass instanceVariable myInstanceVariable4 Integer 0;

MyClass instanceMethod myMethod (MyClass myArg) {
  struct {
    int s_0,
      s_1,
      s_2,
      s_3,
      s_4;
  } s;
  OBJECT *myInstanceVariable0_alias;
  OBJECT *myInstanceVariable1_alias;
  OBJECT *myInstanceVariable2_alias;
  OBJECT *myInstanceVariable3_alias;
  OBJECT *myInstanceVariable4_alias;
  char int_buf[16];


  s.s_0 = 0;
  s.s_1 = 1;
  s.s_2 = 2;
  s.s_3 = 3;
  s.s_4 = 4;

  myInstanceVariable0_alias = myArg myInstanceVariable0;
  myInstanceVariable1_alias = myArg myInstanceVariable1;
  myInstanceVariable2_alias = myArg myInstanceVariable2;
  myInstanceVariable3_alias = myArg myInstanceVariable3;
  myInstanceVariable4_alias = myArg myInstanceVariable4;

  /* __ctalkDecimalIntegerToASCII (s.s_0, int_buf);
  __ctalkSetObjectValueVar (myInstanceVariable0_alias, int_buf);
  __ctalkDecimalIntegerToASCII (s.s_1, int_buf);
  __ctalkSetObjectValueVar (myInstanceVariable1_alias, int_buf);
  __ctalkDecimalIntegerToASCII (s.s_2, int_buf);
  __ctalkSetObjectValueVar (myInstanceVariable2_alias, int_buf);
  __ctalkDecimalIntegerToASCII (s.s_3, int_buf);
  __ctalkSetObjectValueVar (myInstanceVariable3_alias, int_buf);
  __ctalkDecimalIntegerToASCII (s.s_4, int_buf);
  __ctalkSetObjectValueVar (myInstanceVariable4_alias, int_buf); *//***/

  *(int *)myInstanceVariable0_alias -> instancevars -> __o_value = s.s_0;
  *(int *)myInstanceVariable1_alias -> instancevars -> __o_value = s.s_1;
  *(int *)myInstanceVariable2_alias -> instancevars -> __o_value = s.s_2;
  *(int *)myInstanceVariable3_alias -> instancevars -> __o_value = s.s_3;
  *(int *)myInstanceVariable4_alias -> instancevars -> __o_value = s.s_4;

  printf ("%d\t%d\t%d\t%d\t%d\n", 
	  myArg myInstanceVariable0,
	  myArg myInstanceVariable1,
	  myArg myInstanceVariable2,
	  myArg myInstanceVariable3,
	  myArg myInstanceVariable4);
	  
  s.s_0 += 10;
  s.s_1 += 10;
  s.s_2 += 10;
  s.s_3 += 10;
  s.s_4 += 10;
  /* __ctalkDecimalIntegerToASCII (s.s_0, int_buf);
  __ctalkSetObjectValueVar (myArg myInstanceVariable0, int_buf);
  __ctalkDecimalIntegerToASCII (s.s_1, int_buf);
  __ctalkSetObjectValueVar (myArg myInstanceVariable1, int_buf);
  __ctalkDecimalIntegerToASCII (s.s_2, int_buf);
  __ctalkSetObjectValueVar (myArg myInstanceVariable2, int_buf);
  __ctalkDecimalIntegerToASCII (s.s_3, int_buf);
  __ctalkSetObjectValueVar (myArg myInstanceVariable3, int_buf);
  __ctalkDecimalIntegerToASCII (s.s_4, int_buf);
  __ctalkSetObjectValueVar (myArg myInstanceVariable4, int_buf); *//***/

  *(int *)myInstanceVariable0_alias -> instancevars -> __o_value = s.s_0;
  *(int *)myInstanceVariable1_alias -> instancevars -> __o_value = s.s_1;
  *(int *)myInstanceVariable2_alias -> instancevars -> __o_value = s.s_2;
  *(int *)myInstanceVariable3_alias -> instancevars -> __o_value = s.s_3;
  *(int *)myInstanceVariable4_alias -> instancevars -> __o_value = s.s_4;

  printf ("%d\t%d\t%d\t%d\t%d\n", 
	  myArg myInstanceVariable0,
	  myArg myInstanceVariable1,
	  myArg myInstanceVariable2,
	  myArg myInstanceVariable3,
	  myArg myInstanceVariable4);

  s.s_0 += 10;
  s.s_1 += 10;
  s.s_2 += 10;
  s.s_3 += 10;
  s.s_4 += 10;
  myArg myInstanceVariable0 = s.s_0;
  myArg myInstanceVariable1 = s.s_1;
  myArg myInstanceVariable2 = s.s_2;
  myArg myInstanceVariable3 = s.s_3;
  myArg myInstanceVariable4 = s.s_4;

  printf ("%d\t%d\t%d\t%d\t%d\n", 
	  myArg myInstanceVariable0,
	  myArg myInstanceVariable1,
	  myArg myInstanceVariable2,
	  myArg myInstanceVariable3,
	  myArg myInstanceVariable4);
}

int main () {

  MyClass new myReceiver;
  MyClass new myArg;

  myReceiver myMethod myArg;
}


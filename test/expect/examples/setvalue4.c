
Integer class MyInteger;

MyInteger instanceMethod = set_value (int __intArg) {
  OBJECT *self_alias;
  char buf[MAXLABEL];
  int i;
  self_alias = self;
  sprintf (buf, "%d", __intArg);
  i = atoi (buf);
/***/ /* abbreviate or expand this as needed for the tutorial */
  /* __ctalkSetObjectValueVar (self_alias, buf); *//***/
  *(int *)self_alias -> __o_value = i;
  *(int *)self_alias -> instancevars -> __o_value = i;
  return self;
}

int main () {
  MyInteger new myInt;

  myInt = 2;
  printf ("%d\n", myInt);
}

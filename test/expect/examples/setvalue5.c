
Integer class MyInteger;

MyInteger instanceMethod = set_value (int __intArg) {
  OBJECT *self_alias;
  char buf[MAXLABEL];
  int i;
  Integer new localInt;
  if (self value is Integer) {
    /* self copy __intArg asInteger; *//***/

    /* __ctalkAddInstanceVariable (self, "value", localInt value); *//***/
    /* self_alias = self; */
    /* also expand this as needed for the tutorial */ /***/
    /* *(int *)self_alias -> instancevars -> __o_value = i; *//***/
    self = __intArg asInteger;
  } else {
    self_alias = self;
    /* here, too */ /***/
    *(int *)self_alias -> instancevars -> __o_value = __intArg asInteger;
    /* sprintf (buf, "%d", __intArg);
       __ctalkSetObjectValueVar (self_alias, buf); *//***/
  }
  return self;
}

int main () {
  MyInteger new myInt;

  myInt = 2;
  printf ("%d\n", myInt);
}


int get_Integer_value (OBJECT *o) {
  OBJECT *o_value;
  int i;
  if ((o_value = __ctalkGetInstanceVariable (o, "value", 1))
      != NULL) {
    /* sscanf (o_value -> __o_value, "%d", &i); *//***/
    i = *(int *)o_value -> __o_value;
  } else {
    /* sscanf (o -> __o_value, "%d", &i); *//***/
    i = *(int *)o -> __o_value;
  }
  return i;
}

int add_int_fn (int rcvr, int arg) {
  printf ("add_int_fn: %d: ", rcvr + arg);
  if ((rcvr + arg) == 2) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
}

Integer instanceMethod addSelfInt (int arg) {
  add_int_fn (get_Integer_value(self), arg);
}

int main () {
  Integer new myInt;
  myInt = 1;
  myInt addSelfInt 1;
}

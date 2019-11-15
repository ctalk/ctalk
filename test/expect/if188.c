/* this tests the is_OBJECT_ptr_array_deref message, which exits
   the compiler immediately */


Integer instanceMethod myStuff (Integer intArg) {
  Integer new x_2;
  List new l;
  OBJECT *objs[3];
  OBJECT *s;
  OBJECT *t;

  x_2 = 6;

  objs[0] = __ctalkCreateObjectInit ("s", "Integer", "Magnitude",
				     LOCAL_VAR, "1");
  objs[1] = __ctalkCreateObjectInit ("t", "Integer", "Magnitude",
				     LOCAL_VAR, "1");

  l = "one";
  l map {
    if (objs[0] -> __o_value) {
      printf ("Pass\n");
    } else {
      printf ("Fail\n");
    }
  }
  objs[0] delete;
  objs[1] delete;
}

int main () {
  Integer new myReceiver;
  int myInt = 5;
  myReceiver myStuff myInt;
}

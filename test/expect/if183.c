/* Note that this test will generate a lot of fails on PowerPC systems
   which are bigendian. That means when we try to take the length of
   an __o_value string, then __o_value[0] is likely to be '\0'. */


void myStuff (int intArg) {
  Integer new x_2;
  OBJECT *s;
  OBJECT *t;

  x_2 = 6;

  s = __ctalkCreateObjectInit ("s", "Integer", "Magnitude",
			       LOCAL_VAR, "1");
  t = __ctalkCreateObjectInit ("t", "Integer", "Magnitude",
			       LOCAL_VAR, "1");

  if (s -> __o_value length == 1) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s -> __o_value length + x_2 == 7) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (x_2 + s -> __o_value length == 7) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s -> __o_value length + x_2 + intArg == 12) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (x_2 + s -> __o_value length + intArg < 14) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (x_2 + intArg + s -> __o_value length == 12) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s -> __o_value length + x_2 + s -> __o_value length + intArg == 13) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s -> __o_value length + s -> __o_value length + x_2 + intArg == 13) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s -> __o_value length + intArg + s -> __o_value length + x_2 == 13) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s -> __o_value length + intArg + s -> __o_value length +
      x_2 + t -> __o_value length == 14) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s -> __o_value length + intArg + t -> __o_value length +
      x_2 + s -> __o_value length == 14) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  /* These test for memory leaks. */
  if (s -> __o_value length + intArg + s -> __o_value length +
      s -> __o_value length + x_2 == 14) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s -> __o_value length + intArg + s -> __o_value length +
      s -> __o_value length + x_2 + s -> __o_value length == 15) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  if (s -> __o_value length + intArg + s -> __o_value length +
      s -> __o_value length + x_2 + s -> __o_value length +
      s -> __o_value length == 16) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
  s delete;
  t delete;
}

int main () {
  int myInt = 5;
  myStuff (myInt);
}

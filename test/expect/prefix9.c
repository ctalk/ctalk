Integer instanceMethod myMethod (void) {
  Integer new i;
  Integer new i2;
  LongInteger new l;
  LongInteger new l2;
  Float new f;
  Float new f2;

  i = 0;
  i2 = !i;
  if (i2) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  l = 0L;
  l2 = !l;
  if (l2) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  f = 0.0;
  f2 = !f;
  if (f2) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }
}

int main () {
  Integer new i;
  i myMethod;
}

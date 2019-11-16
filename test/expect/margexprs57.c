int myTime (int test_arg) {
  return time (NULL);
}

CTime instanceMethod myUTCTime (void) {
  Integer new intArg;

  intArg = 2;
  self = myTime (intArg);
  return self;
}

int main () {
  CTime new c;

  c myUTCTime;

  printf ("%s", c cTime);
  exit (0);
}

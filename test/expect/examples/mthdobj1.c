String instanceMethod selfConcat (String __arg) {
  self = self + __arg;
  return NULL;
}

int main () {
  Method new m;
  String new s;
  Exception new e;
  Integer new i;

  for (i = 0 ; i <= 100; i = i + 1) {
    s = "Hello,";
    m definedInstanceMethod "String", "selfConcat";
    m withArg " world!";
    s methodObjectMessage m; /* methodObjectMessage is defined */
    /* in Object class.               */
    if (e pending) {
      e handle;
    } else {
      printf ("%d. %s\n", i, s);
    }
  }
}


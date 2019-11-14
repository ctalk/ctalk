Integer instanceMethod printVal (void) {

  char buf[0xff];

  xstrcpy (buf, self value asString);
  printf ("%s\n", self);
  printf ("%s\n", self value);
  printf ("%s\n", buf);

}

int main () {

  Integer new i;

  i = 34;

  i printVal;
}

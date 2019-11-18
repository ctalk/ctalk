
/* A positive result should be okay - we don't necessarily support
   negative characters, even though they're "signed" by default. */
Point instanceMethod myMethod (void) {
  Integer new charObj;
  Integer new intObj;
  charObj = -self y asCharacter;
  printf ("charObj = %c\n", charObj);
  charObj = eval -self y asCharacter;
  printf ("charObj = %c\n", charObj);
  intObj = eval -self y;
  printf ("intObj = %d\n", intObj);
}

int main (int argc, char **argv) {
  Point new pt;

  pt y = 80; /* 'P' */

  pt myMethod;
}

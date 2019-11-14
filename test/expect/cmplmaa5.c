/* pretend we're calculating  where to insert a line break. */
/* this tests how we handle argument expression "self" references in eval_expr
   and related functions. */
String instanceMethod lineBreak (void) {
  Integer new receiverLength;
  Integer new remaining;
  String new lineString;
  remaining = 10;
  receiverLength = self length;

  lineString = self subString ((self length) - remaining), remaining;
  printf ("%s\n", lineString);
  lineString = self subString (self length - remaining), receiverLength;
  printf ("%s\n", lineString);
  lineString = self subString self length - remaining, receiverLength;
  printf ("%s\n", lineString);
}

int main (int argc, char *argv[]) {
  String new str;

  str = "This is text, and this is yet more text.";
  str lineBreak;
}

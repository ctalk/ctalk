

/* Tests '\0' as an argument to __ctalk_arg (). */

int main () {

  String new s;

  s = "\"Hello, world!\"\"";

  s atPut (s length - 1), '\0';

  printf ("%s\n", s);
}


String instanceMethod assignSelf (void) {

  Symbol new localSym;

  *localSym = __inspect_get_receiver (512);

  printf ("%s\n", *localSym);
}

int main () {

  String  new str;

  str = "str value";

  str assignSelf;
}

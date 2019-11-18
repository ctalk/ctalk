

String instanceMethod assign (void) {

  Object new myObj;

  myObj = __inspect_get_receiver (512);

  printf ("%s\n", myObj);
}

int main () {

  String new str;

  str = "str value";

  str assign;
}

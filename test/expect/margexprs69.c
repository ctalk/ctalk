
String instanceMethod assignString (String arg) {
  if (arg is String) {
    self = arg;
    return SUCCESS;
  } else {
    return ERROR;
  }
}


int main () {
  String new str;
  if (str assignString "Hello, world!" < 0) {
    printf ("fail\n");
  } else {
    printf ("pass: %s\n", str);
  }
}

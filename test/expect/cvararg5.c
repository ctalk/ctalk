

String instanceMethod printEntry (String prefix, Integer param1, Integer param2) {
  self printOn "%s:%d:%d", prefix, param1, param2;
  printf ("%s\n", self);
}

int main () {
  List new myList;
  String new myString;
  int arg1 = 0, arg2 = 0;
  OBJECT *self_var;
  int i;

  myList = "one", "two", "three", "four", "five";

  myList map {
    self_var = self;
    for (i = 0; i < 500; ++i) {
      if (self_var) {
	myString printEntry self, arg1 + i, arg2 + i;
      }
    }
  }
}

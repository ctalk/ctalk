

String instanceMethod printEntry (Integer param1, Integer param2) {
  self printOn "%d:%d", param1, param2;
  printf ("%s\n", self);
}

List instanceMethod addSeries (String s) {
  int arg1 = 0, arg2 = 0, i;
  OBJECT *self_var;

  self map {
    self_var = self;
    if (self_var) {
      for (i = 0; i < 500; ++i) {
	arg1 += 1; arg2 += 1;
	s printEntry arg1, arg2;
      }
    }
  }
}

int main () {
  List new myList;
  String new myString;

  myList = "one", "two", "three", "four", "five";

  myList addSeries myString;
}

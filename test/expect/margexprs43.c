

typedef struct {
  char str_member_a[255];
  char str_member_b[255];
} MY_STRUCT;

String instanceMethod myMethod (void) {
  String new myString;
  MY_STRUCT my_struct;
  xstrcpy (my_struct.str_member_a, "Hello");
  myString = my_struct.str_member_a subString 0, 2;
  printf ("%s\n", myString);
}

int main () {
  String new myString;
  MY_STRUCT my_struct;
  xstrcpy (my_struct.str_member_a, "Hello, world!");
  myString = my_struct.str_member_a subString 0, 2;
  printf ("%s\n", myString);
  myString myMethod;
}

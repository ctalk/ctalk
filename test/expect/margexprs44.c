

typedef struct {
  char str_member_a[255];
  char str_member_b[255];
} MY_STRUCT;

String instanceMethod myMethod (void) {
  String new myString;
  MY_STRUCT my_struct;
  MY_STRUCT *my_struct_ptr;
  xstrcpy (my_struct.str_member_a, "Hello");
  my_struct_ptr = &my_struct;
  myString = my_struct_ptr -> str_member_a subString 0, 2;
  printf ("%s\n", myString);
}

int main () {
  String new myString;
  MY_STRUCT my_struct;
  MY_STRUCT *my_struct_ptr;
  xstrcpy (my_struct.str_member_a, "Hello, world!");
  my_struct_ptr = &my_struct;
  myString = my_struct_ptr -> str_member_a subString 0, 2;
  printf ("%s\n", myString);
  myString myMethod;
}

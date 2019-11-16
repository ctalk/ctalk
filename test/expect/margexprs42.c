

struct _my_struct {
  char str_member_a[255];
  char str_member_b[255];
} my_struct;

String instanceMethod myMethod (void) {
  String new myString;
  struct _my_struct *struct_ptr;
  struct_ptr = &my_struct;
  myString = struct_ptr -> str_member_a subString 0, 2;
  printf ("%s\n", myString);
}

int main () {
  String new myString;
  struct _my_struct *struct_ptr;
  struct_ptr = &my_struct;
  xstrcpy (struct_ptr -> str_member_a, "Hello, world!");
  myString = struct_ptr -> str_member_a subString 0, 2;
  printf ("%s\n", myString);
  myString myMethod;
}

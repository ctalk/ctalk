/* This tests the code in cvar_expr_needs_translation. */

struct _my_struct {
  int int_element;
} my_struct;

typedef struct _my_struct MY_STRUCT_TYPE;

MY_STRUCT_TYPE my_derived_struct;

int main () {
  int myInt;

  my_struct.int_element = 10;
  myInt = 10 + my_struct.int_element + 5;
  printf ("%d\n", myInt);

  my_derived_struct.int_element = 10;
  myInt = 10 + my_derived_struct.int_element + 5;
  printf ("%d\n", myInt);

}

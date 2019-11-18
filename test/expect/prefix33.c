/* tests the different contexts in prefix_method_expr_a () */

int main () {
  int i_int;
  Integer new iObj;

  i_int = 100;
  iObj = 100;

  if (TRUE)
    ++i_int;

  if (TRUE) {
    ++i_int;
  }

  if (TRUE)
    ++iObj;

  if (TRUE) {
    ++iObj;
  }

  printf ("%d\n", i_int);
  printf ("%d\n", iObj);
}

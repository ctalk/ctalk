
Integer instanceMethod becomeMethod (void) {
  Integer new j;
  j become self;
  printf ("%d\n", j);
}


int main () {
  Integer new i;
  i = 2;
  i becomeMethod;
  i = 3;
  i becomeMethod;
  i = 4;
  i becomeMethod;
}

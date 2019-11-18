
/* Checks for the correct translation from the printf arg expressions
   to a run-time expressions, handled in method_args (). */

int main () {
  Integer new int1;
  Integer new int2;
  Integer new int3;
  Integer new int4;
  Integer new int5;


  int1 = 1;
  int2 = 2;
  int3 = 3;
  int4 = 4;
  int5 = 4;

  printf ("%d %d %d %c %c\n", int1, int2, int3, int4 + '0', int5 + '1');
}

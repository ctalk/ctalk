
/* this should generate no warnings if the compiler inserts a
   __ctalk_exitFn call at the end of my_func */

void my_func (int my_int) {
  
  if (my_int < 0) {
    exit (1);
  }
}


int main () {
  int my_int = 0;
  String new myStr;
  my_func (my_int);
  myStr printOn "%d\n", my_int;
  printf ("done\n");
}

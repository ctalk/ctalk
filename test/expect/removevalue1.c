
int main () {

  Symbol new s1;
  Symbol new s2;
  String new str;
  
  str = "Hello, world!";

  *s1 = str;
  *s2 = str;

  printf ("%s\n", *s1);
  printf ("%s\n", *s2);

  printf ("--\n");

  s1 removeValue;

  printf ("%s\n", *s1);
  printf ("%s\n", *s2);

  printf ("--\n");

  *s1 = str;
  printf ("%s\n", *s1);
  printf ("%s\n", *s2);

}

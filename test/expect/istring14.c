
int main () {

  String new s;
  String new t;
  Integer  new i;

  s = "Hello, world!";

  for (i = 0; i < 8; ++i) 
    printf ("%s\n", s + i);

  t = s + i;

  for (; i >= 0; --i) 
    printf ("%s\n", t - i);

  for (i = 0; i < 8; ++i) 
    printf ("%s\n", t - i);

}

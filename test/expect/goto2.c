
int main () {

  int i = 0;

 loop_again:
  while (i < 10) {
    ++i;
    if (i > 5)
      goto loop_again;
    printf ("%d\n", i);
  }

  return (0);
}

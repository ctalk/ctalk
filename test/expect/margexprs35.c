

struct _n_tab {
  int arry[3];
} tab[3];

Integer new sInt1;
Integer new sInt2;

int main (int argc, char **argv) {

  Integer new n;

  tab[0].arry[0] = 11;
  tab[1].arry[1] = 22;
  tab[2].arry[2] = 33;

  sInt1 = 0;
  sInt2 = 0;
  n = tab[sInt1].arry[sInt2];
  printf ("%d\n", n);

  sInt1 = 1;
  sInt2 = 1;
  n = tab[sInt1].arry[sInt2];
  printf ("%d\n", n);

  sInt1 = 2;
  sInt2 = 2;
  n = tab[sInt1].arry[sInt2];
  printf ("%d\n", n);

}

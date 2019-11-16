

struct _n_tab {
  int mbr_a;
} tab[3][3] = {
  {11, 22, 33},
  {44, 55, 66},
  {77, 88, 99},
};

Integer new sInt1;
Integer new sInt2;

int main (int argc, char **argv) {

  Integer new n;

  sInt1 = 0;
  sInt2 = 0;
  n = tab[sInt1][sInt2].mbr_a;
  printf ("%d\n", n);

  sInt1 = 1;
  n = tab[sInt1][sInt2].mbr_a;
  printf ("%d\n", n);

  sInt1 = 2;
  n = tab[sInt1][sInt2].mbr_a;
  printf ("%d\n", n);

}

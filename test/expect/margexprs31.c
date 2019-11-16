

struct _n_tab {
   int mbr_a,
     mbr_b,
     mbr_c;
} tab[] = {
  {11, 22, 33},
  {44, 55, 66},
  {77, 88, 99},
};

Integer new sInt;

int main (int argc, char **argv) {

  Integer new n;

  sInt = 0;
  n = tab[sInt].mbr_a;
  printf ("%d\n", n);

  sInt = 1;
  n = tab[sInt].mbr_a;
  printf ("%d\n", n);

  sInt = 2;
  n = tab[sInt].mbr_a;
  printf ("%d\n", n);

}



struct _n_tab {
  int mbr_a,
    mbr_b,
    mbr_c;
} tab[] = {
  {11, 22, 33}
};

Integer instanceMethod printTab (void) {

  self = tab[0].mbr_a;
  printf ("%d\n", self);

  self = tab[0].mbr_b;
  printf ("%d\n", self);

  self = tab[0].mbr_c;
  printf ("%d\n", self);

}

int main (int argc, char **argv) {

  Integer new n;

  n = tab[0].mbr_a;
  printf ("%d\n", n);

  n = tab[0].mbr_b;
  printf ("%d\n", n);

  n = tab[0].mbr_c;
  printf ("%d\n", n);

  n printTab;
}

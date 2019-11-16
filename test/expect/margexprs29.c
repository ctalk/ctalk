
int tab_idx = 0;

struct _n_tab {
  int mbr_a;
} tab[3][3] = {
  {11, 22, 33},
  {44, 55, 66},
  {77, 88, 99}
};

Integer instanceMethod printTab (void) {


  tab_idx = 0;

  self = tab[tab_idx][0].mbr_a;
  printf ("%d\n", self);

  ++tab_idx;

  self = tab[tab_idx][1].mbr_a;
  printf ("%d\n", self);

  ++tab_idx;

  self = tab[tab_idx][2].mbr_a;
  printf ("%d\n", self);

}

int main (int argc, char **argv) {

  Integer new n;

  n = tab[tab_idx][0].mbr_a;
  printf ("%d\n", n);

  ++tab_idx;

  n = tab[tab_idx][1].mbr_a;
  printf ("%d\n", n);

  ++tab_idx;

  n = tab[tab_idx][2].mbr_a;
  printf ("%d\n", n);

  n printTab;
}



struct _sub_elem {
  int int_mbr;
};

struct _elem {
  struct _sub_elem subelem;
};

struct _n_tab {
  struct _elem elements[3];
} tab[3];

Integer new sInt1;
Integer new sInt2;

int main (int argc, char **argv) {

  Integer new n;

  tab[0].elements[0].subelem.int_mbr = 11;
  tab[1].elements[1].subelem.int_mbr = 22;
  tab[2].elements[2].subelem.int_mbr = 33;

  sInt1 = 0;
  sInt2 = 0;
  n = tab[sInt1].elements[sInt2].subelem.int_mbr;
  printf ("%d\n", n);

  sInt1 = 1;
  sInt2 = 1;
  n = tab[sInt1].elements[sInt2].subelem.int_mbr;
  printf ("%d\n", n);

  sInt1 = 2;
  sInt2 = 2;
  n = tab[sInt1].elements[sInt2].subelem.int_mbr;
  printf ("%d\n", n);

}

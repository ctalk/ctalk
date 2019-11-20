
struct STYPE1 {
  int stype1_a;
  int stype1_b;
  int stype1_c;
};

struct STYPE2 {
  int stype2_a;
  int stype2_b;
  int stype2_c;
};

struct STYPE3 {
  int stype3_a;
  int stype3_b;
  int stype3_c;
};

union U {
  struct STYPE1 s1;
  struct STYPE2 s2;
  struct STYPE3 s3;
};

int main (int argc, char **argv) {

  Integer new i;

  union U u_union;
  u_union.s1.stype1_a = 1;

  i = u_union.s1.stype1_a;
  printf ("The following int should be 1:\n");
  printf ("%d\n", i);
}



int main (int argc, char **argv) {

  Array new a;
  Key new k;
  Key new j;
  Integer new i;

  a atPut 0, "value0";
  a atPut 1, "value1";
  a atPut 2, "value2";
  a atPut 3, "value3";

  k = *a;
  i = 0;

  while (++k) {
    printf ("%s\n", *k);
    if (i == 1) {
	j = k;
	printf ("j = k: %s\n", *k);
    }
    ++i;
  }

  printf ("-------------------\n");
  printf ("%s\n", *j);

}

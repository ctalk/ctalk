

int main (int argc, char **argv) {

  List new a;
  Key new k;
  Key new j;
  Integer new i;

  a push "value1";
  a push "value2";
  a push "value3";
  a push "value4";

  k = *a;
  i = 0;

  while (++k) {
    printf ("%s\n",*k);
    if (i == 1) {
	j = k;
	printf ("j = k: %s\n", *k);
    }
    ++i;
  }

  printf ("-------------------\n");
  printf ("%s\n", *j);

}

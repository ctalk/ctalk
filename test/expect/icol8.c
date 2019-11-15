

int main (int argc, char **argv) {

  AssociativeArray new a;
  Key new k;
  Key new j;
  Integer new i;

  a atPut "key1", "value1";
  a atPut "key2", "value2";
  a atPut "key3", "value3";
  a atPut "key4", "value4";

  k = *a;
  i = 0;

  while (++k) {
    printf ("%s --> %s\n", k name, *k);
    if (i == 1) {
	j = k;
	printf ("j = k: %s\n", *k);
    }
    ++i;
  }

  printf ("-------------------\n");
  printf ("%s\n", *j);

}

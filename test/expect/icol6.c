

int main (int argc, char **argv) {

  AssociativeArray new a;
  Key new k;
  Key new j;

  a atPut "key1", "value1";
  a atPut "key2", "value2";
  a atPut "key3", "value3";

  k = *a;

  printf ("%s\n", k name);

  printf ("%s\n", (k++) name);
  j = k;

  printf ("%s\n", (k++) name);
  j = k;
#if 0
  printf ("-------------------\n");
  printf ("%s\n", j-- name);
  printf ("%s\n", j-- name);
#endif
}

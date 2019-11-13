
int main () {

  AssociativeArray new a1;
  AssociativeArray new a2;
  String new value1;
  String new value2;
  String new value3;
  Symbol new removed;

  value1 = "Value 1";
  value2 = "Value 2";
  value3 = "Value 3";

  a1 atPut "key1", value1;
  a1 atPut "key2", value2;
  a1 atPut "key3", value3;

  a2 atPut "key1", value1;
  a2 atPut "key2", value2;
  a2 atPut "key3", value3;

  a1 map {
    printf ("%s %p\n", self value, self addressOf);
  }
  printf ("------------------------\n");
  a2 map {
    printf ("%s %p\n", self value, self addressOf);
  }
  printf ("------------------------\n");

  removed = a1 removeAt "key1";

  a1 map {
    printf ("%s %p\n", self value, self addressOf);
  }
  printf ("------------------------\n");
  a2 map {
    printf ("%s %p\n", self value, self addressOf);
  }
  printf ("------------------------\n");

  a1 atPut "key1", value1;

  a1 map {
    printf ("%s %p\n", self value, self addressOf);
  }
  printf ("------------------------\n");
  a2 map {
    printf ("%s %p\n", self value, self addressOf);
  }
  printf ("------------------------\n");

}

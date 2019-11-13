
int main () {
  AssociativeArray new a;
  a atPut "key1", "value1";
  a atPut "key2", "value2";
  a atPut "key3", "value3";
  a atPut "key4", "value4";

  a mapKeys {
    if (self name == "key3")
      break;
    printf ("%s\n", self getValue value);
  }
}

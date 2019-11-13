
int main () {
  Array new a;
  a atPut 0, "0";
  a atPut 1, "1";
  a atPut 2, "2";
  a atPut 3, "3";

  a map {
    if (self == "2")
      break;
    printf ("%s\n", self);
  }
}

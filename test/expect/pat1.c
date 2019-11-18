
int main () {
  String new pattern, str;
  
  pattern = m/ap*/;
  str = "apple";
  if (str =~ pattern) {
    printf ("match\n");
  } else {
    printf ("no match\n");
  }

  str = "erc";
  if (str =~ pattern) {
    printf ("match\n");
  } else {
    printf ("no match\n");
  }
}

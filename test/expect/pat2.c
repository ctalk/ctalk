
int main () {
  String new str;
  
  str = "apple";
  if (str =~ m/ap*/) {
    printf ("match\n");
  } else {
    printf ("no match\n");
  }

  str = "ord";
  if (str =~ m/ap*/) {
    printf ("match\n");
  } else {
    printf ("no match\n");
  }
}

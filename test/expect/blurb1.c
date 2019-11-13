/* Tests the validity of a blurb about Ctalk on Usenet. */

int main () {

  List new aList;
  Key new aKey;

  aList push "item1";
  aList push "item2";
  aList push "item3";

  aKey = *aList;

  do {
    printf ("%s\n", *aKey);
  } while (++aKey);

}

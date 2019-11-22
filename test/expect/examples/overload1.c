

String instanceMethod myConcat (String s1) {

  self = self + s1;

}

String instanceMethod myConcat (String s1, String s2) {

  self = self + s1 + s2;
}

int main () {
  String new s1;
  String new s2;
  String new s3;

  s1 = "Hello, ";
  s2 = "world! ";
  s3 = "Again.";

  s1 myConcat s2;
  printf ("%s\n", s1);
  
  s1 myConcat s2, s3;
  printf ("%s\n", s1);
}

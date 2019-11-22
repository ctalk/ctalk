
int main () {

  List new bigList;
  String new str;
  Integer new idxInt;


  for (idxInt = 0; idxInt < 1000; ++idxInt) {

    str = String basicNew idxInt asString, idxInt asString;

    bigList push str;
  }

  bigList map {

    printf ("%s, %s\n", self name, self value);
  }
  
}

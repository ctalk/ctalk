
int main () {

  List new bigList;
  Symbol new sym;
  Integer new idxInt;


  for (idxInt = 0; idxInt < 10000; ++idxInt) {

    *sym = String basicNew idxInt asString, idxInt asString;

    bigList push *sym;
  }

  bigList map {

    printf ("%s, %s\n", self name, self value);
  }
  
}

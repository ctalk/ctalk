/* Similar to argblk39.c, except checks c variables in methods. */

Integer instanceMethod testMethod (void) {

  List new l;
  List new l_sub_1;
  List new l_sub_2;
  List new subList;
  unsigned int i;

  l_sub_1 push "l_sub_1-1";
  l_sub_1 push "l_sub_1-2";
  l_sub_2 push "l_sub_2-1";
  l_sub_2 push "l_sub_2-2";

  l push l_sub_1;
  l push l_sub_2;

  i = 0;

  l map { 

    subList become self;

    subList map {

      printf ("%u. %s\n", i, self);

      i -= 1;
    }

  }
}

int main () {
  Integer new myInt;

  myInt testMethod;
}

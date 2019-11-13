/* Like argblk43.c, but it checks C vars in methods. */

typedef unsigned char my_uchar;

Integer instanceMethod testMethod (void) {

  List new l;
  List new l_sub_1;
  List new l_sub_2;
  List new subList;
  my_uchar i;

  l_sub_1 push "l_sub_1-1";
  l_sub_1 push "l_sub_1-2";
  l_sub_2 push "l_sub_2-1";
  l_sub_2 push "l_sub_2-2";

  l push l_sub_1;
  l push l_sub_2;

  i = 01;

  l map { 

    subList become self;

    subList map {

      printf ("%hhu. %s\n", i, self);

      i -= 1;
    }

  }

}

int main () {
  Integer new myInt;
  myInt testMethod;
}

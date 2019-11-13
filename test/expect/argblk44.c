/* Like argblk39.c except with a typedef. */

typedef unsigned int my_uint;

int main () {

  List new l;
  List new l_sub_1;
  List new l_sub_2;
  List new subList;
  my_uint i;

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

  exit(0);
}

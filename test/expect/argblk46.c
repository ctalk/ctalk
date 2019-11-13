/* Like argblk41.c except with a typedef. */

typedef unsigned long long int my_ullint;

int main () {

  List new l;
  List new l_sub_1;
  List new l_sub_2;
  List new subList;
  my_ullint i;

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

      printf ("%llu. %s\n", i, self);

      i -= 1;
    }

  }

  exit(0);
}
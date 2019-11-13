/* $Id: argblk20.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/*
 *  Note that the items in the sublists are pushed in different order
 *  than in argblk[18-19].c
 */


int main () {

  List new l;
  List new l_sub_1;
  List new l_sub_2;
  List new subList;
  int i;

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

      printf ("%d. %s\n", i, self);

      i += 1;
    }

  }

  exit(0);
}

/* $Id: argblk12.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */
/* Test continue keyword, in a block in a function. */
int main () {

  List new l;

  l push 1;
  l push 2;
  l push 3;
  l push 4;
  l push 5;
  l push 6;
  l push 7;
  l push 8;
  l push 9;
  l push 10;

  l map {
    if ((self % 2) == 0)
      continue;

    printf ("%d\n", self);

  }
}

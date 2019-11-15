/* $Id: if7.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Test an if expression with a while (...) (...)
 *  loop as the if block.
 */

int main (char **argv, int argc) {

  int i;

  if (1)
    for (i = 0; i < 10; ++i)
      printf ("Pass\n");

  exit(0);
}

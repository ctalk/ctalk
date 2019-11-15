/* $Id: if6.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Test an if expression with a while (...) (...)
 *  loop as the if block.
 */

int main (char **argv, int argc) {

  if (1)
    while (0) (printf ("Fail\n"));

  printf ("Pass\n");

  exit(0);
}

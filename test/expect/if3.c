/* $Id: if3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Test an if expression with a do {...} while (...)
 *  loop as the if block.
 */

int main (char **argv, int argc) {

  if (1)
    do { printf ("Pass\n"); } while (0);

  exit(0);
}

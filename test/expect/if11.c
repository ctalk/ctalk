/* $Id: if11.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

/*
 *  Make sure we place braces correctly for each if (..) block,
 *  including an extra brace at the end.  See the comments in
 *  loop_block_end () (loop.c).
 */

int main (char **argv, int argc) {

  int i = 0;

  if (i == 0) 
    printf ("Pass\n");
  else if (i == 1)
    printf ("Fail\n");

  exit(0);
}

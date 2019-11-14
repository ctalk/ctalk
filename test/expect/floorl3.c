/* $Id: floorl3.c,v 1.1.1.1 2019/10/26 23:40:51 rkiesling Exp $ */

#include <math.h>

int main () {

  double c_floor, d_arg;
  Float new floorObj;

  d_arg = 3.2;
  c_floor = floorl (d_arg);

  if ((floorObj = floorl(d_arg)) == c_floor) {
    printf ("Pass\n");
  } else {
    printf ("Fail\n");
  }

  exit(0);
}
